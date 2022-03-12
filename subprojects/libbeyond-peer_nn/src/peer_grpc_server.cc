/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "peer_grpc_server.h"
#include "peer_nn.grpc.pb.h"
#include "peer_grpc_server_auth.h"
#include "peer_grpc_server_gst.h"
#include "peer_model.h"

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <exception>
#include <memory>
#include <vector>

#include <sys/time.h>
#include <pthread.h>
#include <grpc++/grpc++.h>
#include <glib.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"

Peer::GrpcServer::GrpcServer(void)
    : server(nullptr)
    , peer(nullptr)
    , nextPeerId(0)
{
}

Peer::GrpcServer::~GrpcServer(void)
{
}

Peer::GrpcServer *Peer::GrpcServer::Create(Peer *peer, const char *address, const char *certificate, const char *privateKey, const char *rootCert)
{
    GrpcServer *impls;

    if (address == nullptr) {
        ErrPrint("Invalid arguments");
        return nullptr;
    }

    if (peer == nullptr) {
        ErrPrint("Invalid arguments");
        return nullptr;
    }

    try {
        impls = new GrpcServer();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    impls->peer = peer;

    int boundPort = 0;
    ::grpc::ServerBuilder builder;

    if (certificate != nullptr && privateKey != nullptr) {
        grpc::SslServerCredentialsOptions sslOps;
        grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = {
            privateKey,
            certificate,
        };
        sslOps.pem_key_cert_pairs.push_back(keycert);
        if (rootCert != nullptr) {
            sslOps.pem_root_certs = rootCert;
        }
        sslOps.client_certificate_request = GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;

        std::shared_ptr<grpc::ServerCredentials> cred = grpc::SslServerCredentials(sslOps);
        cred->SetAuthMetadataProcessor(std::shared_ptr<grpc::AuthMetadataProcessor>(new Peer::GrpcServer::Auth(impls)));
        builder.AddListeningPort(address, cred, &boundPort);
        DbgPrint("Secured channel is created (%s)", address);
    } else {
        builder.AddListeningPort(address, ::grpc::InsecureServerCredentials(), &boundPort);
        DbgPrint("Insecure channel is created port(%s)", address);
    }

    builder.RegisterService(impls);

    impls->server = builder.BuildAndStart();
    if (impls->server == nullptr) {
        ErrPrint("Failed to start a server");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    if (impls->peer->info != nullptr) {
        impls->peer->info->port[0] = boundPort;
    }
    DbgPrint("Bounded port is %d", boundPort);

    int status = pthread_create(&impls->threadId, nullptr, Peer::GrpcServer::Main, impls);
    if (status != 0) {
        ErrPrintCode(status, "pthread_create");
        impls->server->Shutdown();

        delete impls;
        impls = nullptr;
        return nullptr;
    }

    return impls;
}

void Peer::GrpcServer::Destroy(void)
{
    void *ret;
    int status;

    server->Shutdown();

    status = pthread_join(threadId, &ret);
    if (status != 0) {
        ErrPrintCode(status, "ptherad_join");
    }

    std::map<std::string, Peer::GrpcServer::Gst *>::iterator it;
    for (it = clientMap.begin(); it != clientMap.end(); ++it) {
        it->second->Destroy();
    }

    delete this;
}

int Peer::GrpcServer::GetGst(::grpc::ServerContext *context, Peer::GrpcServer::Gst *&gst)
{
    std::multimap<grpc::string_ref, grpc::string_ref>::const_iterator it;

    it = context->client_metadata().find("id");
    if (it == context->client_metadata().end()) {
        return -EINVAL;
    }

    const grpc::string_ref _peerId = it->second;
    std::string peerId = std::string(_peerId.data(), _peerId.length());

    if (peerId.empty() == true) {
        return -ENOENT;
    }

    gst = clientMap[peerId];
    return 0;
}

::grpc::Status Peer::GrpcServer::Configure(::grpc::ServerContext *context, const ::peer_nn::Configuration *request, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    beyond_plugin_peer_nn_config::server_description server = {
        .input_type = request->input_type(),
        .preprocessing = const_cast<char *>(request->preprocessing().c_str()),
        .postprocessing = const_cast<char *>(request->postprocessing().c_str()),
        .framework = const_cast<char *>(request->framework().c_str()),
        .accel = const_cast<char *>(request->accel().c_str())
    };

    int ret = gst->Configure(&server);
    if (ret < 0) {
        ErrPrint("Configure: %d", ret);
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::ExchangeKey(::grpc::ServerContext *context, const ::peer_nn::ExchangeKeyRequest *request, ::peer_nn::ExchangeKeyResponse *response)
{
    int ret;
    beyond::AuthenticatorInterface *auth = nullptr;

    DbgPrint("Key received: %zu bytes", request->key().length());

    if (peer->caAuthenticator != nullptr) {
        DbgPrint("CA Authenticator is selected");
        auth = peer->caAuthenticator;
    } else if (peer->authenticator != nullptr) {
        DbgPrint("Authenticator is selected");
        auth = peer->authenticator;
    }

    if (auth != nullptr) {
        DbgPrint("Authenticator is prepared");
        ret = auth->Decrypt(
            beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY,
            request->key().c_str(),
            request->key().length());
        if (ret < 0) {
            ErrPrint("Failed to decrypt the key string");
            response->set_status(ret);
            return ::grpc::Status(::grpc::StatusCode::OK, "OK");
        }

        void *key = nullptr;
        int keyLength = 0;

        ret = auth->GetResult(key, keyLength);
        if (ret < 0) {
            ErrPrint("Failed to get a decrypted string");
        } else if (key == nullptr || keyLength <= 0) {
            ErrPrint("Invalid decrypted key string");
            ret = -EFAULT;
        } else {
            Peer::Credential *cred = static_cast<Peer::Credential *>(key);

            DbgPrint("Successfully decrypted: uuid: %s", cred->uuid);
            DbgPrint("Session key length: %d", cred->sessionKeyLength);

            if (strncmp(peer->info->uuid, cred->uuid, strlen(peer->info->uuid)) != 0) {
                ErrPrint("Invalid UUID");
                ret = -EINVAL;
            } else {
                std::string id = std::to_string(nextPeerId);

                Peer::GrpcServer::Gst *gst = Peer::GrpcServer::Gst::Create(id, this);
                if (gst == nullptr) {
                    ErrPrint("Failed to create a GST instance");
                    ret = -EFAULT;
                } else {
                    std::string secretKey = std::string(cred->payload, cred->sessionKeyLength);
                    gst->SetSecret(secretKey);
                    gst->SetNonce(cred->nonce);

                    clientMap[id] = gst;
                    response->set_id(id);
                    ret = 0;
                    ++nextPeerId;
                }
            }

            free(cred);
            cred = nullptr;
        }
    } else {
        ErrPrint("Run under the insecured mode");
        std::string id = std::to_string(nextPeerId);
        Peer::GrpcServer::Gst *gst = Peer::GrpcServer::Gst::Create(id, this);
        if (gst == nullptr) {
            ErrPrint("Failed to create a GST instance");
            ret = -EFAULT;
        } else {
            std::string secretKey;
            gst->SetSecret(secretKey); // empty string

            clientMap[id] = gst;
            response->set_id(id);
            ret = 0;
            ++nextPeerId;
        }
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::LoadModel(::grpc::ServerContext *context, const ::peer_nn::Model *request, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    std::string filename = request->filename();
    std::string base_name = filename.substr(filename.find_last_of("/\\") + 1);

    if (peer->serverCtx->storagePath.empty() == false) {
        filename = peer->serverCtx->storagePath;
        if (filename[filename.size() - 1] != '/') {
            filename += "/";
        }
        filename += base_name;
    } else {
        filename = base_name;
    }

    int ret = gst->GetModel()->SetModelPath(filename.c_str());
    if (ret == 0) {
        if (access(filename.c_str(), R_OK) < 0) {
            ErrPrintCode(errno, "access: %s", filename.c_str());
            ret = -ENOENT;
            // NOTE:
            // The client is going to invoke the UploadModel
        }
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::UploadModel(::grpc::ServerContext *context, ::grpc::ServerReader<::peer_nn::ModelFile> *reader, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    // NOTE:
    // Stream... filename must be gotten from LoadModel
    // If there is no invocation of the LoadModel, this function must get failed
    if (gst->GetModel()->GetModelPath() == nullptr) {
        ErrPrint("LoadModel was not invoked properly");
        return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "Failed precondition");
    }

    int ret = 0;
    FILE *fp = fopen(gst->GetModel()->GetModelPath(), "w+");
    if (fp != nullptr) {
        ::peer_nn::ModelFile fileContents;

        while (reader->Read(&fileContents) == true) {
            int sz = fwrite(fileContents.content().c_str(), 1, fileContents.content().size(), fp);
            if (sz <= 0 || static_cast<unsigned int>(sz) != fileContents.content().size()) {
                ret = ferror(fp);
                clearerr(fp);
                ErrPrintCode(ret, "fwrite: returns %d", sz);
                ret = -ret;
                break;
            }
        }

        if (fclose(fp) < 0) {
            ErrPrintCode(errno, "fclose");
        }
    } else {
        ret = -errno;
        ErrPrintCode(errno, "fopen, %s", gst->GetModel()->GetModelPath());
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::GetInputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::TensorInfos *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    const beyond_tensor_info *info = nullptr;
    int size = 0;

    int ret = gst->GetModel()->GetInputTensorInfo(info, size);
    if (ret == 0) {
        assert(info != nullptr && size > 0);
        ret = InfoToResponse(info, size, response);
        if (ret < 0) {
            response->set_status(ret);
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
        }
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

int Peer::GrpcServer::InfoToResponse(const beyond_tensor_info *info, int size, ::peer_nn::TensorInfos *response)
{
    for (int i = 0; i < size; i++) {
        ::peer_nn::TensorInfo *_info = response->add_info();
        if (_info == nullptr) {
            ErrPrint("Failed to add a new tensor info");
            return -EFAULT;
        }
        _info->set_type(static_cast<::peer_nn::TensorType>(info[i].type));
        _info->set_name(!!info[i].name ? info[i].name : "");
        _info->set_size(info[i].size);

        ::peer_nn::Dimensions *dims = _info->mutable_dims();
        if (dims == nullptr) {
            ErrPrint("Dims is nullptr");
            return -EFAULT;
        }

        for (int j = 0; j < info[i].dims->size; j++) {
            dims->add_data(info[i].dims->data[j]);
        }
    }

    return 0;
}

int Peer::GrpcServer::RequestToInfo(const ::peer_nn::TensorInfos *request, beyond_tensor_info *&info)
{
    beyond_tensor_info *_info;
    _info = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * request->info_size()));
    if (_info == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    for (int i = 0; i < request->info_size(); i++) {
        _info[i].type = static_cast<beyond_tensor_type>(request->info(i).type());
        _info[i].size = request->info(i).size();
        const char *name = request->info(i).name().c_str();
        if (name != nullptr) {
            _info[i].name = strdup(name);
            if (_info[i].name == nullptr) {
                int ret = -errno;
                ErrPrintCode(errno, "strdup");
                Peer::Model::FreeTensorInfo(_info, i);
                return ret;
            }
        } else {
            _info[i].name = nullptr;
        }

        _info[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * request->info(i).dims().data_size()));
        if (_info[i].dims == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "malloc");

            free(_info[i].name);
            _info[i].name = nullptr;

            Peer::Model::FreeTensorInfo(_info, i);
            return ret;
        }

        _info[i].dims->size = request->info(i).dims().data_size();
        for (int j = 0; j < _info[i].dims->size; j++) {
            _info[i].dims->data[j] = request->info(i).dims().data(j);
        }
    }

    info = _info;
    return 0;
}

::grpc::Status Peer::GrpcServer::SetInputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::TensorInfos *request, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    beyond_tensor_info *_info = nullptr;
    int ret = RequestToInfo(request, _info);
    if (ret < 0) {
        response->set_status(ret);
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
    }

    ret = gst->GetModel()->SetInputTensorInfo(_info, request->info_size());
    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::GetOutputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::TensorInfos *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    const beyond_tensor_info *info = nullptr;
    int size = 0;

    int ret = gst->GetModel()->GetOutputTensorInfo(info, size);
    if (ret == 0) {
        assert(info != nullptr && size > 0);
        ret = InfoToResponse(info, size, response);
        if (ret < 0) {
            response->set_status(ret);
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
        }
    }

    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::SetOutputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::TensorInfos *request, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    beyond_tensor_info *_info = nullptr;
    int ret = RequestToInfo(request, _info);
    if (ret < 0) {
        response->set_status(ret);
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
    }

    ret = gst->GetModel()->SetOutputTensorInfo(_info, request->info_size());
    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::Prepare(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::PreparedResponse *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    // NOTE:
    // Prepare the gst pipeline in order to get the tensor shape from the model
    int requestPort = 0;
    int responsePort = 0;
    int ret = gst->Prepare(requestPort, responsePort);
    response->set_status(ret);
    response->set_request_port(requestPort);
    response->set_response_port(responsePort);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::Stop(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::Response *response)
{
    Peer::GrpcServer::Gst *gst = nullptr;

    if (GetGst(context, gst) < 0) {
        response->set_status(-ENOENT);
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    int ret = gst->Stop();
    response->set_status(ret);
    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

::grpc::Status Peer::GrpcServer::GetInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::Info *response)
{
    if (peer->info == nullptr) {
        ErrPrint("PeerInfo does not set");
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not Found");
    }

    for (int i = 0; i < peer->info->count_of_runtimes; i++) {
        ::peer_nn::RuntimeInfo *_runtime = response->add_runtimes();
        if (_runtime == nullptr) {
            ErrPrint("Failed to add a new runtime");
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
        }

        _runtime->set_name(peer->info->runtimes[i].name);
        for (int j = 0; j < peer->info->runtimes[i].count_of_devices; j++) {
            ::peer_nn::DeviceInfo *_device = _runtime->add_devices();
            if (_device == nullptr) {
                ErrPrint("Failed to add a new device");
                return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
            }
            _device->set_name(peer->info->runtimes[i].devices[j].name);
        }
    }

    beyond::ResourceInfoCollector collector(peer->serverCtx->storagePath);
    collector.collectResourceInfo(peer->info);
    response->set_free_memory(peer->info->free_memory);
    response->set_free_storage(peer->info->free_storage);

    return ::grpc::Status(::grpc::StatusCode::OK, "OK");
}

void *Peer::GrpcServer::Main(void *ptr)
{
    GrpcServer *service = static_cast<GrpcServer *>(ptr);

    service->server->Wait();

    return nullptr;
}
