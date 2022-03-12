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

#include "peer_grpc_client.h"
#include "peer_nn.grpc.pb.h"
#include "peer_grpc_client_auth.h"
#include "peer_grpc_client_gst.h"
#include "peer_model.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <ctime>

#include <exception>
#include <memory>

#include <grpc++/grpc++.h>
#include <glib.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"

Peer::GrpcClient::GrpcClient(Peer *peer)
    : peer(peer)
    , gst(nullptr)
{
}

Peer::GrpcClient::~GrpcClient(void)
{
}

Peer::GrpcClient *Peer::GrpcClient::Create(Peer *peer, const char *address, const char *rootCA)
{
    GrpcClient *impls;

    try {
        impls = new GrpcClient(peer);
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    grpc::ChannelArguments args;
    std::shared_ptr<::grpc::ChannelInterface> channel;
    std::shared_ptr<grpc::ChannelCredentials> cred;

    if (rootCA != nullptr) {
        grpc::SslCredentialsOptions opts = {
            rootCA,
        };
        std::shared_ptr<grpc::CallCredentials> callCred;
        std::shared_ptr<grpc::ChannelCredentials> sslCred;

        sslCred = grpc::SslCredentials(opts);
        callCred = grpc::MetadataCredentialsFromPlugin(std::unique_ptr<grpc::MetadataCredentialsPlugin>(new Peer::GrpcClient::Auth(impls)));
        cred = grpc::CompositeChannelCredentials(sslCred, callCred);
        channel = grpc::CreateCustomChannel(grpc::string(address), cred, args);
        DbgPrint("Connect to the client through a secured channel %s", address);
    } else {
        cred = grpc::InsecureChannelCredentials();
        channel = grpc::CreateChannel(grpc::string(address), cred);
        DbgPrint("Connect to the client through a insecured channel %s", address);
    }

    impls->stub = ::peer_nn::RPC::NewStub(channel);
    if (impls->stub == nullptr) {
        ErrPrint("Unable to create the client stub");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    return impls;
}

void Peer::GrpcClient::Destroy(void)
{
    if (gst != nullptr) {
        gst->Destroy();
        gst = nullptr;
    }

    delete this;
}

int Peer::GrpcClient::Configure(const beyond_plugin_peer_nn_config::server_description *server)
{
    ::peer_nn::Configuration request;
    ::peer_nn::Response response;
    ::grpc::ClientContext context;

    context.AddMetadata("id", peerId);

    request.set_input_type(server->input_type);

    if (server->preprocessing != nullptr) {
        request.set_preprocessing(server->preprocessing);
    }

    if (server->postprocessing != nullptr) {
        request.set_postprocessing(server->postprocessing);
    }

    if (server->framework != nullptr) {
        request.set_framework(server->framework);
    }

    if (server->accel != nullptr) {
        request.set_accel(server->accel);
    }

    grpc::Status status = stub->Configure(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    return static_cast<int>(response.status());
}

uint64_t Peer::GrpcClient::GetRandom()
{
    FILE *fp;
    uint64_t seed;

    fp = fopen("/dev/urandom", "rb");
    do {
        size_t rc = 0;

        if (fp != nullptr) {
            rc = fread(&seed, 1, sizeof(seed), fp);
            fclose(fp);
        }

        if (rc < sizeof(seed))
            break;

        return seed;
    } while (false);

    srandom(static_cast<unsigned int>(time(nullptr)));
    return random();
}

int Peer::GrpcClient::ExchangeKey(void)
{
    ::peer_nn::ExchangeKeyRequest request;
    ::peer_nn::ExchangeKeyResponse response;
    ::grpc::ClientContext context;
    beyond::AuthenticatorInterface *auth = nullptr;

    unsigned long nonce = GetRandom();

    if (peer->caAuthenticator != nullptr) {
        DbgPrint("CA Authenticator is selected");
        auth = peer->caAuthenticator;
    } else if (peer->authenticator != nullptr) {
        DbgPrint("Authenticator is selected");
        auth = peer->authenticator;
    } else {
        DbgPrint("Insecured mode. Exchange peer Ids");
    }

    std::string _secretKey;
    if (auth != nullptr) {
        if (peer->info == nullptr || peer->info->uuid[0] == '\0') {
            ErrPrint("Insufficient credential");
            return -EINVAL;
        }

        void *secretKey = nullptr;
        int secretKeySize = 0;
        int ret = auth->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, secretKey, secretKeySize);
        if (ret < 0) {
            ErrPrint("Failed to generate the secret key");
            return ret;
        } else if (secretKey == nullptr || secretKeySize <= 0) {
            ErrPrint("Invalid key generated");
            return -EINVAL;
        }

        int credSize = sizeof(Peer::Credential) + secretKeySize;
        _secretKey = std::string(static_cast<char *>(secretKey), secretKeySize);

        // TODO: validate the credSize
        // The credSize must smaller than the asymmetric key size
        // (ex. asymmetric key size = 4096 bits. The credSize must smaller than 512 bytes)
        Peer::Credential *cred = static_cast<Peer::Credential *>(malloc(credSize));
        if (cred == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "malloc");
            free(secretKey);
            secretKey = nullptr;
            return ret;
        }

        memcpy(cred->uuid, peer->info->uuid, sizeof(cred->uuid));
        memcpy(cred->payload, secretKey, secretKeySize);
        cred->sessionKeyLength = secretKeySize;
        cred->nonce = nonce;
        free(secretKey);
        secretKey = nullptr;

        DbgPrint("Authenticator is prepared: Encrypt %d bytes", credSize);
        ret = auth->Encrypt(
            beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY,
            static_cast<const void *>(cred),
            credSize);

        if (ret < 0) {
            ErrPrint("Failed to encrypt the key string");
            free(cred);
            cred = nullptr;
            return ret;
        }

        void *output = nullptr;
        int outputSize = 0;
        ret = auth->GetResult(output, outputSize);
        free(cred);
        cred = nullptr;
        if (ret < 0) {
            ErrPrint("Failed to get encrypted key string");
            return ret;
        } else if (output == nullptr || outputSize <= 0) {
            ErrPrint("Invalid encrypted key string");
            return -EFAULT;
        }
        DbgPrint("Result size: %d", outputSize);

        request.set_key(output, outputSize);
        free(output);
        output = nullptr;
    } else {
        request.set_key(std::string("insecure"));
    }

    grpc::Status status = stub->ExchangeKey(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    if (response.status() == 0) {
        peerId = response.id();

        gst = Peer::GrpcClient::Gst::Create(peerId, this);
        if (gst == nullptr) {
            // TODO:
            // Let the server knows what happens on here
            return -EFAULT;
        }

        gst->SetSecret(_secretKey);
        gst->SetNonce(nonce);
    }

    return static_cast<int>(response.status());
}

int Peer::GrpcClient::LoadModel(const char *modelFilename)
{
    ::peer_nn::Model request;
    ::peer_nn::Response response;
    ::grpc::ClientContext context;

    context.AddMetadata("id", peerId);

    request.set_filename(modelFilename);
    grpc::Status status = stub->LoadModel(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    // NOTE:
    // Reset cached model information
    gst->GetModel()->SetInputTensorInfo(nullptr, 0);
    gst->GetModel()->SetOutputTensorInfo(nullptr, 0);

    return static_cast<int>(response.status());
}

int Peer::GrpcClient::UploadModel(const char *modelFilename)
{
    ::peer_nn::Response response;
    ::grpc::ClientContext context;

    context.AddMetadata("id", peerId);

    std::unique_ptr<::grpc::ClientWriter<::peer_nn::ModelFile>> writer(stub->UploadModel(&context, &response));
    ::peer_nn::ModelFile file;

    FILE *fp;
    char *buffer = static_cast<char *>(malloc(Peer::GrpcClient::CHUNK_SIZE));
    if (buffer == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc failed");
        return ret;
    }

    fp = fopen(modelFilename, "r");
    if (fp == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "fopen failed: %s", modelFilename);
        free(buffer);
        buffer = nullptr;
        return ret;
    }

    size_t sz;
    while ((sz = fread(buffer, 1, Peer::GrpcClient::CHUNK_SIZE, fp)) > 0) {
        file.set_content(buffer, sz);
        writer->Write(file);
    }

    if (ferror(fp) != 0) {
        ErrPrintCode(errno, "fread");
        clearerr(fp);
    }

    if (fclose(fp) < 0) {
        ErrPrintCode(errno, "fclose");
    }

    free(buffer);
    buffer = nullptr;
    writer->WritesDone();

    ::grpc::Status status = writer->Finish();
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    return 0;
}

int Peer::GrpcClient::GetTensorInfoFromResponse(::peer_nn::TensorInfos &tensorInfos, beyond_tensor_info *&info, int &size)
{
    beyond_tensor_info *_info = nullptr;
    int _size = tensorInfos.info_size();
    assert(_size > 0 && "Invalid size of tensorInfos");
    _info = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * _size));
    if (_info == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    for (int i = 0; i < _size; i++) {
        _info[i].type = static_cast<beyond_tensor_type>(tensorInfos.info(i).type());
        _info[i].size = tensorInfos.info(i).size();
        const char *name = tensorInfos.info(i).name().c_str();
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

        _info[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * tensorInfos.info(i).dims().data_size()));
        if (_info[i].dims == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "malloc");
            free(_info[i].name);
            _info[i].name = nullptr;
            Peer::Model::FreeTensorInfo(_info, i);
            return ret;
        }

        _info[i].dims->size = tensorInfos.info(i).dims().data_size();
        for (int j = 0; j < _info[i].dims->size; j++) {
            _info[i].dims->data[j] = tensorInfos.info(i).dims().data(j);
        }
    }

    size = _size;
    info = _info;
    return 0;
}

int Peer::GrpcClient::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    const beyond_tensor_info *_info;
    int _size;
    int ret = gst->GetModel()->GetInputTensorInfo(_info, _size);
    if (ret == 0 && _info != nullptr && _size > 0) {
        info = _info;
        size = _size;
        return 0;
    }

    ::grpc::ClientContext context;
    ::peer_nn::Empty request;
    ::peer_nn::TensorInfos tensorInfos;

    context.AddMetadata("id", peerId);

    ::grpc::Status status = stub->GetInputTensorInfo(&context, request, &tensorInfos);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    beyond_tensor_info *__info = nullptr;
    _size = 0;
    ret = GetTensorInfoFromResponse(tensorInfos, __info, _size);
    if (ret < 0) {
        return ret;
    }
    assert(__info != nullptr && _size > 0 && "Invalid tensor info and size are extracted");

    ret = gst->GetModel()->SetInputTensorInfo(__info, _size);
    if (ret < 0) {
        Peer::Model::FreeTensorInfo(__info, _size);
        return ret;
    }

    info = __info;
    size = _size;
    return 0;
}

int Peer::GrpcClient::SetRequest(::peer_nn::TensorInfos &request, const beyond_tensor_info *info, int size)
{
    for (int i = 0; i < size; i++) {
        ::peer_nn::TensorInfo *_info = request.add_info();
        _info->set_type(static_cast<::peer_nn::TensorType>(info[i].type));
        _info->set_size(info[i].size);

        if (info[i].name != nullptr) {
            _info->set_name(info[i].name);
        }

        if (info[i].dims->size <= 0) {
            return -EINVAL;
        }

        for (int j = 0; j < info[i].dims->size; j++) {
            _info->mutable_dims()->add_data(info[i].dims->data[j]);
        }
    }

    return 0;
}

int Peer::GrpcClient::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    ::grpc::ClientContext context;
    ::peer_nn::TensorInfos request;
    ::peer_nn::Response response;

    context.AddMetadata("id", peerId);

    int ret = SetRequest(request, info, size);
    if (ret < 0) {
        return ret;
    }

    ::grpc::Status status = stub->SetInputTensorInfo(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    beyond_tensor_info *_info;
    ret = Peer::Model::DupTensorInfo(_info, info, size);
    if (ret < 0) {
        return ret;
    }

    ret = gst->GetModel()->SetInputTensorInfo(_info, size);
    if (ret < 0) {
        Peer::Model::FreeTensorInfo(_info, size);
        return ret;
    }

    return response.status();
}

int Peer::GrpcClient::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    const beyond_tensor_info *_info;
    int _size;
    int ret = gst->GetModel()->GetOutputTensorInfo(_info, _size);
    if (ret == 0 && _info != nullptr && _size > 0) {
        info = _info;
        size = _size;
        return 0;
    }

    ::grpc::ClientContext context;
    ::peer_nn::Empty request;
    ::peer_nn::TensorInfos tensorInfos;

    context.AddMetadata("id", peerId);

    ::grpc::Status status = stub->GetOutputTensorInfo(&context, request, &tensorInfos);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    beyond_tensor_info *__info = nullptr;
    _size = 0;
    ret = GetTensorInfoFromResponse(tensorInfos, __info, _size);
    if (ret < 0) {
        return ret;
    }
    assert(__info != nullptr && _size > 0 && "Invalid tensor info and size are extracted");

    ret = gst->GetModel()->SetOutputTensorInfo(__info, _size);
    if (ret < 0) {
        Peer::Model::FreeTensorInfo(__info, _size);
        return ret;
    }

    info = __info;
    size = _size;
    return 0;
}

int Peer::GrpcClient::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    ::grpc::ClientContext context;
    ::peer_nn::TensorInfos request;
    ::peer_nn::Response response;

    context.AddMetadata("id", peerId);

    int ret = SetRequest(request, info, size);
    if (ret < 0) {
        return ret;
    }

    ::grpc::Status status = stub->SetOutputTensorInfo(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    beyond_tensor_info *_info;
    ret = Peer::Model::DupTensorInfo(_info, info, size);
    if (ret < 0) {
        return ret;
    }

    ret = gst->GetModel()->SetOutputTensorInfo(_info, size);
    if (ret < 0) {
        Peer::Model::FreeTensorInfo(_info, size);
        return ret;
    }

    return response.status();
}

int Peer::GrpcClient::Prepare(int &requestPort, int &responsePort)
{
    ::grpc::ClientContext context;
    ::peer_nn::Empty request;
    ::peer_nn::PreparedResponse response;

    context.AddMetadata("id", peerId);

    ::grpc::Status status = stub->Prepare(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    int ret = response.status();
    if (ret < 0) {
        ErrPrint("Response status: %d", ret);
        return ret;
    }

    requestPort = response.request_port();
    responsePort = response.response_port();

    return ret;
}

int Peer::GrpcClient::Stop(void)
{
    ::grpc::ClientContext context;
    ::peer_nn::Empty request;
    ::peer_nn::Response response;

    context.AddMetadata("id", peerId);

    ::grpc::Status status = stub->Stop(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    return response.status();
}

int Peer::GrpcClient::GetInfo(beyond_peer_info *info)
{
    ::grpc::ClientContext context;
    ::peer_nn::Empty request;
    ::peer_nn::Info response;

    context.AddMetadata("id", peerId);

    ::grpc::Status status = stub->GetInfo(&context, request, &response);
    if (status.ok() == false) {
        ErrPrint("Error: %d, message: %s", static_cast<int>(status.error_code()), status.error_message().c_str());
        return -EFAULT;
    }

    beyond_peer_info_runtime *runtimes = nullptr;
    int count_of_runtimes = response.runtimes_size();
    if (count_of_runtimes > 0) {
        runtimes = static_cast<beyond_peer_info_runtime *>(calloc(count_of_runtimes, sizeof(beyond_peer_info_runtime)));
        if (runtimes == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "calloc");
            return ret;
        }

        for (int i = 0; i < count_of_runtimes; i++) {
            runtimes[i].count_of_devices = response.runtimes(i).devices_size();
            runtimes[i].name = strdup(response.runtimes(i).name().c_str());
            if (runtimes[i].name == nullptr) {
                int ret = -errno;
                ErrPrintCode(errno, "strdup");
                Peer::ResetRuntime(runtimes, i);
                return ret;
            }

            runtimes[i].devices = nullptr;
            if (runtimes[i].count_of_devices > 0) {
                runtimes[i].devices = static_cast<beyond_peer_info_device *>(calloc(runtimes->count_of_devices, sizeof(beyond_peer_info_device)));
                if (runtimes[i].devices == nullptr) {
                    int ret = -errno;
                    ErrPrintCode(errno, "calloc");
                    free(runtimes[i].name);
                    runtimes[i].name = nullptr;
                    Peer::ResetRuntime(runtimes, i);
                    return ret;
                }

                for (int j = 0; j < runtimes[i].count_of_devices; j++) {
                    runtimes[i].devices[j].name = strdup(response.runtimes(i).devices(j).name().c_str());
                    if (runtimes[i].devices[j].name == nullptr) {
                        int ret = -errno;
                        ErrPrintCode(errno, "strdup");
                        while (--j >= 0) {
                            free(runtimes[i].devices[j].name);
                            runtimes[i].devices[j].name = nullptr;
                        }
                        free(runtimes[i].devices);
                        runtimes[i].devices = nullptr;
                        free(runtimes[i].name);
                        runtimes[i].name = nullptr;
                        Peer::ResetRuntime(runtimes, i);
                        return ret;
                    }
                }
            }
        }
    }

    Peer::ResetRuntime(info->runtimes, info->count_of_runtimes);
    info->runtimes = runtimes;
    info->count_of_runtimes = count_of_runtimes;
    info->free_memory = response.free_memory();
    info->free_storage = response.free_storage();
    return 0;
}

Peer::GrpcClient::Gst *Peer::GrpcClient::GetGst(void)
{
    return gst;
}
