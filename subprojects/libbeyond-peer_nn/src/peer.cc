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

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "peer.h"
#include "peer_event_object.h"
#include "peer_grpc_client_gst.h"
#include "peer_grpc_server_gst.h"

#include <exception>
#include <sstream>

#include <cstdio>
#include <cerrno>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include <unistd.h>

#define DEFAULT_FRAMEWORK "tensorflow-lite"

Peer *Peer::Create(bool isServer, const char *framework, const char *accel, const char *storagePath)
{
    Peer *peer;

    try {
        peer = new Peer();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    peer->eventObject = Peer::EventObject::Create();
    if (peer->eventObject == nullptr) {
        delete peer;
        peer = nullptr;
        return nullptr;
    }

    if (isServer == true) {
        peer->serverCtx = std::make_unique<Peer::ServerContext>();

        if (storagePath != nullptr) {
            peer->serverCtx->storagePath = std::string(storagePath);
        }
    } else {
        peer->clientCtx = std::make_unique<Peer::ClientContext>();

        if (framework != nullptr) {
            peer->clientCtx->framework = std::string(framework);
            if (accel != nullptr) {
                peer->clientCtx->accel = std::string(accel);
            }

            // NOTE:
            // Prepare the reserved configuration.
            // Even if the application does not call configure(),
            // the peer has to be configured
            peer->reservedConfiguration = static_cast<beyond_plugin_peer_nn_config *>(calloc(1, sizeof(beyond_plugin_peer_nn_config)));
            if (peer->reservedConfiguration == nullptr) {
                ErrPrintCode(errno, "calloc");
                peer->eventObject->Destroy();
                peer->eventObject = nullptr;

                delete peer;
                peer = nullptr;
                return nullptr;
            }

            peer->reservedConfiguration->server.framework = strdup(framework);
            if (accel != nullptr) {
                peer->reservedConfiguration->server.accel = strdup(accel);
            }
            if (peer->reservedConfiguration->server.framework == nullptr) {
                ErrPrintCode(errno, "strdup");
                FreeConfig(peer->reservedConfiguration);
                peer->eventObject->Destroy();
                peer->eventObject = nullptr;

                delete peer;
                peer = nullptr;
                return nullptr;
            }
        }
    }

    return peer;
}

// Invoked from the ApplicationContext
const char *Peer::GetModuleName(void) const
{
    return Peer::NAME;
}

// Invoked from the ApplicationContext
const char *Peer::GetModuleType(void) const
{
    return beyond::ModuleInterface::TYPE_PEER;
}

void Peer::Destroy(void)
{
    eventObject->Destroy();
    eventObject = nullptr;

    ResetInfo(info);

    delete this;
}

int Peer::GetHandle(void) const
{
    return eventObject->GetHandle();
}

int Peer::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject->AddHandler(handler, type, data);
}

int Peer::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject->RemoveHandler(handler, type, data);
}

int Peer::FetchEventData(EventObjectInterface::EventData *&data)
{
    return eventObject->FetchEventData(data);
}

int Peer::DestroyEventData(EventObjectInterface::EventData *&data)
{
    delete data;
    data = nullptr;
    return 0;
}

void Peer::ConfigureImageInput(beyond_input_config *config, std::ostringstream &client_format, std::ostringstream &server_format)
{
    client_format << "video/x-raw,format=" << config->config.image.format;
    client_format << ",width=" << config->config.image.width << ",height=" << config->config.image.height << ",framerate=0/1";
    if (strstr(config->config.image.format, "I420")) {
        client_format << " ! jpegenc";
    } else {
        // Note: Specific subset of JPEG is supported(such as I420, YUY2) for sink caps of RTP payload
        client_format << " ! videoconvert ! video/x-raw,format=I420 ! jpegenc";
    }

    server_format << "jpegdec ! videoconvert";
    if (config->config.image.width != config->config.image.convert_width || config->config.image.height != config->config.image.convert_height) {
        server_format << " ! videoscale ";
    }

    server_format << " ! video/x-raw,format=" << config->config.image.convert_format;
    server_format << ",width=" << config->config.image.convert_width;
    server_format << ",height=" << config->config.image.convert_height;
    server_format << " ! tensor_converter";
    if (config->config.image.transform_mode != nullptr && config->config.image.transform_option != nullptr) {
        server_format << " ! tensor_transform mode=" << config->config.image.transform_mode;
        server_format << " option=" << config->config.image.transform_option;
    }
}

void Peer::ConfigureVideoInput(beyond_input_config *config, std::ostringstream &client_format, std::ostringstream &server_format)
{
    client_format << "video/x-raw,format=" << config->config.video.frame.format;
    client_format << ",width=" << config->config.video.frame.width << ",height=" << config->config.video.frame.height << ",framerate=" << config->config.video.fps << "/1";
    if (strstr(config->config.video.frame.format, "I420")) {
        client_format << " ! vp8enc";
    } else {
        // Note: VP8 only supports I420 format
        client_format << " ! videoconvert ! video/x-raw,format=I420 ! vp8enc";
    }

    server_format << "vp8dec ! videoconvert";
    if (config->config.video.frame.width != config->config.video.frame.convert_width || config->config.video.frame.height != config->config.video.frame.convert_height) {
        server_format << " ! videoscale ";
    }

    server_format << " ! video/x-raw,format=" << config->config.video.frame.convert_format;
    server_format << ",width=" << config->config.video.frame.convert_width;
    server_format << ",height=" << config->config.video.frame.convert_height;
    server_format << " ! tensor_converter";
    if (config->config.video.frame.transform_mode != nullptr && config->config.video.frame.transform_option != nullptr) {
        server_format << " ! tensor_transform mode=" << config->config.video.frame.transform_mode;
        server_format << " option=" << config->config.video.frame.transform_option;
    }
}

int Peer::ConfigureInput(const beyond_config *options)
{
    if (serverCtx != nullptr) {
        // NOTE:
        // There are multiple GST could be created.
        // so the gst configuration is not supported from the edge side
        return 0;
    }

    if (clientCtx == nullptr) {
        return 0;
    }

    beyond_plugin_peer_nn_config _config;
    beyond_plugin_peer_nn_config *_options;
    std::ostringstream client_format;
    std::ostringstream server_format;
    char *client_desc = nullptr;
    char *server_desc = nullptr;

    if (options->type == BEYOND_CONFIG_TYPE_INPUT) {
        beyond_input_config *config = static_cast<beyond_input_config *>(options->object);
        if (config->input_type != BEYOND_INPUT_TYPE_IMAGE && config->input_type != BEYOND_INPUT_TYPE_VIDEO) {
            ErrPrint("Not yet supported");
            return -ENOTSUP;
        }

        _options = &_config;

        _options->client.postprocessing = nullptr;
        _options->server.postprocessing = nullptr;

        // TODO:
        // In case of the old version
        // there is no way to set the accelerators
        // The options structure should be updated.
        if (clientCtx != nullptr && clientCtx->framework.empty() == false) {
            _options->server.framework = const_cast<char *>(clientCtx->framework.c_str());
        } else {
            _options->server.framework = const_cast<char *>(DEFAULT_FRAMEWORK);
        }

        if (clientCtx != nullptr && clientCtx->accel.empty() == false) {
            _options->server.accel = const_cast<char *>(clientCtx->accel.c_str());
        } else {
            _options->server.accel = const_cast<char *>("");
        }
        DbgPrint("Selected framework: [%s]  accel: [%s]", _options->server.framework, _options->server.accel);

        _options->client.input_type = _options->server.input_type = config->input_type;
        if (config->input_type == BEYOND_INPUT_TYPE_IMAGE) {
            ConfigureImageInput(config, client_format, server_format);
        } else if (config->input_type == BEYOND_INPUT_TYPE_VIDEO) {
            ConfigureVideoInput(config, client_format, server_format);
        }

        client_desc = strdup(client_format.str().c_str());
        if (client_desc == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "strdup");
            return ret;
        }

        server_format << " !";

        server_desc = strdup(server_format.str().c_str());
        if (server_desc == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "strdup");
            free(client_desc);
            client_desc = nullptr;
            return ret;
        }

        _options->client.preprocessing = client_desc;
        _options->server.preprocessing = server_desc;
    } else {
        _options = static_cast<beyond_plugin_peer_nn_config *>(options->object);
    }

    int ret;

    do {
        beyond_plugin_peer_nn_config *_reservedConfiguration = nullptr;
        if (clientCtx->grpc == nullptr) {
            // NOTE:
            // Reserve to send the Configuration request after activation
            _reservedConfiguration = DuplicateConfig(_options);
            ret = 0;
        } else {
            ret = clientCtx->grpc->Configure(&_options->server);
            if (ret < 0) {
                break;
            }

            ret = clientCtx->grpc->GetGst()->Configure(&_options->client);
            if (ret < 0) {
                FreeConfig(_reservedConfiguration);
            }
        }

        if (_reservedConfiguration != nullptr) { // Update reservedConfiguration
            FreeConfig(reservedConfiguration);
            reservedConfiguration = _reservedConfiguration;
        }
    } while (0);

    free(server_desc);
    server_desc = nullptr;
    free(client_desc);
    client_desc = nullptr;

    return ret;
}

int Peer::ConfigureAuthenticator(const beyond_config *options)
{
    beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(options->object);
    authenticator = auth;
    return 0;
}

int Peer::ConfigureCAAuthenticator(const beyond_config *options)
{
    beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(options->object);
    caAuthenticator = auth;
    return 0;
}

int Peer::Configure(const beyond_config *options)
{
    if (options == nullptr) {
        return -EINVAL;
    }

    if (options->object == nullptr) {
        return -EINVAL;
    }

    if (options->type == BEYOND_CONFIG_TYPE_AUTHENTICATOR) {
        return ConfigureAuthenticator(options);
    } else if (options->type == BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR) {
        return ConfigureCAAuthenticator(options);
    }

    return ConfigureInput(options);
}

int Peer::LoadModel(const char *model)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (model == nullptr) {
        return -EINVAL;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    int ret = clientCtx->grpc->LoadModel(model);
    if (ret == -ENOENT) {
        ret = clientCtx->grpc->UploadModel(model);
    }

    return ret;
}

int Peer::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    const beyond_tensor_info *_info = nullptr;
    int _size = 0;

    int ret = clientCtx->grpc->GetInputTensorInfo(_info, _size);
    if (ret < 0) {
        return ret;
    }

    info = _info;
    size = _size;
    return 0;
}

int Peer::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    const beyond_tensor_info *_info;
    int _size;

    int ret = clientCtx->grpc->GetOutputTensorInfo(_info, _size);
    if (ret < 0) {
        return ret;
    }

    info = _info;
    size = _size;
    return 0;
}

int Peer::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (info == nullptr || size <= 0 || info->size == 0 || info->dims == nullptr) {
        return -EINVAL;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    return clientCtx->grpc->SetInputTensorInfo(info, size);
}

int Peer::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (info == nullptr || size <= 0 || info->size == 0 || info->dims == nullptr) {
        return -EINVAL;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    return clientCtx->grpc->SetOutputTensorInfo(info, size);
}

int Peer::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (info == nullptr || size <= 0) {
        return -EINVAL;
    }

    beyond_tensor *_tensor = static_cast<beyond_tensor *>(malloc(sizeof(beyond_tensor) * size));
    if (_tensor == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    int ret = 0;
    int i;

    for (i = 0; i < size; i++) {
        _tensor[i].size = info[i].size;
        if (_tensor[i].size <= 0) {
            ErrPrint("[%d] Invalid size of tensor: %d", i, _tensor[i].size);
            ret = -EINVAL;
            break;
        }

        _tensor[i].type = info[i].type;
        _tensor[i].data = malloc(info[i].size);
        if (_tensor[i].data == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "malloc for tensor.data");
            break;
        }
    }

    if (ret < 0) {
        while (--i >= 0) {
            free(_tensor[i].data);
            _tensor[i].data = nullptr;
        }
        free(_tensor);
        _tensor = nullptr;
        return ret;
    }

    tensor = _tensor;
    return 0;
}

void Peer::FreeTensor(beyond_tensor *&tensor, int size)
{
    if (clientCtx == nullptr) {
        return;
    }

    if (tensor == nullptr || size <= 0) {
        return;
    }

    for (int i = 0; i < size; i++) {
        free(tensor[i].data);
        tensor[i].data = nullptr;
    }
    free(tensor);
    tensor = nullptr;
    return;
}

int Peer::Prepare(void)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    int reqPort = 0;
    int resPort = 0;
    int ret = clientCtx->grpc->Prepare(reqPort, resPort);
    if (ret < 0) {
        ErrPrint("GRPC prepare: %d", ret);
        return ret;
    }

    ret = clientCtx->grpc->GetGst()->Prepare(info->host, reqPort, resPort);
    if (ret < 0) {
        // TODO:
        // Handling the prepared grpc
        ErrPrint("GST prepare: %d", ret);
    }

    return ret;
}

int Peer::Invoke(const beyond_tensor *input, int size, const void *context)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    if (clientCtx->grpc->GetGst() == nullptr) {
        return -EILSEQ;
    }

    if (input == nullptr || size <= 0) {
        return -EINVAL;
    }

    for (int i = 0; i < size; i++) {
        if (input[i].size <= 0 || input[i].data == nullptr) {
            ErrPrint("%d size %d data %p", i, input[i].size, input[i].data);
            return -EINVAL;
        }
    }

    return clientCtx->grpc->GetGst()->Invoke(input, size, context);
}

int Peer::GetOutput(beyond_tensor *&tensor, int &size)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    if (clientCtx->grpc->GetGst() == nullptr) {
        return -EILSEQ;
    }

    return clientCtx->grpc->GetGst()->GetOutput(tensor, size);
}

int Peer::Stop(void)
{
    if (clientCtx == nullptr) {
        return -ENOTSUP;
    }

    if (clientCtx->grpc == nullptr) {
        return -EILSEQ;
    }

    int ret = clientCtx->grpc->GetGst()->Stop();
    if (ret < 0) {
        return ret;
    }

    ret = clientCtx->grpc->Stop();
    if (ret < 0) {
        ErrPrint("grpc Stop: %d", ret);
    }

    return ret;
}

int Peer::Activate(void)
{
    int ret = 0;

    if (info == nullptr) {
        ErrPrint("Peer information is not prepared");
        return -EINVAL;
    }

    std::string address = info->host;
    address = address + ":" + std::to_string(info->port[0]);

    void *privateKey = nullptr;
    void *certificate = nullptr;
    void *rootCert = nullptr;

    do {
        if (authenticator != nullptr) {
            DbgPrint("Authenticator is configured. get the private key and certificate");
            int keySize = 0;
            ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, privateKey, keySize);
            if (ret < 0) {
                ErrPrint("Failed to get the private key");
                break;
            }
            if (privateKey == nullptr || keySize <= 0) {
                ErrPrint("Invalid privateKey");
                ret = -EINVAL;
                break;
            }

            keySize = 0;
            ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, certificate, keySize);
            if (ret < 0) {
                ErrPrint("Failed to get a certificate");
                break;
            }
            if (certificate == nullptr || keySize <= 0) {
                ErrPrint("Invalid certificate");
                ret = -EINVAL;
                break;
            }
        }

        if (caAuthenticator != nullptr) {
            int keySize = 0;
            ret = caAuthenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, rootCert, keySize);
            if (ret < 0) {
                ErrPrint("Failed to get a root certificate");
                ret = -EINVAL;
                break;
            }
            if (rootCert == nullptr || keySize <= 0) {
                ErrPrint("Invalid root certificate");
                ret = -EINVAL;
                break;
            }
        }

        if (serverCtx != nullptr) {
            if (serverCtx->grpc != nullptr) {
                ret = -EALREADY;
                break;
            }

            serverCtx->grpc = Peer::GrpcServer::Create(this, address.c_str(), static_cast<const char *>(certificate), static_cast<const char *>(privateKey), static_cast<const char *>(rootCert));
            if (serverCtx->grpc == nullptr) {
                ret = -EFAULT;
                break;
            }
        } else if (clientCtx != nullptr) {
            if (clientCtx->grpc != nullptr) {
                ret = -EALREADY;
                break;
            }

            const char *cert = static_cast<const char *>(rootCert);
            if (caAuthenticator == nullptr) {
                // NOTE:
                // If there is no CA Auth, try to use the Peer's Auth
                DbgPrint("Try to use the Certificate");
                cert = static_cast<const char *>(certificate);
            }

            clientCtx->grpc = Peer::GrpcClient::Create(this, address.c_str(), cert);
            if (clientCtx->grpc == nullptr) {
                ret = -EFAULT;
                break;
            }

            if (authenticator != nullptr || caAuthenticator != nullptr) {
                if (info->uuid[0] == '\0') {
                    ErrPrint("Peer INFO is not sufficient to get authenticated");
                    clientCtx->grpc->Destroy();
                    clientCtx->grpc = nullptr;
                    ret = -EINVAL;
                    break;
                }
            }

            // NOTE:
            // ExchangeKey must be comes first than the others
            // otherwise, the grpc call will be failed if the authenticator is enabled.
            //
            // In order to service multiple peers,
            // it is necessary to exchange peer Id even there is no credentials
            //
            // After this call, the Gst will be available
            ret = clientCtx->grpc->ExchangeKey();
            if (ret < 0) {
                clientCtx->grpc->Destroy();
                clientCtx->grpc = nullptr;
                break;
            }

            if (reservedConfiguration) {
                ret = clientCtx->grpc->Configure(&reservedConfiguration->server);
                if (ret < 0) {
                    clientCtx->grpc->Destroy();
                    clientCtx->grpc = nullptr;
                    break;
                }

                ret = clientCtx->grpc->GetGst()->Configure(&reservedConfiguration->client);
                if (ret < 0) {
                    clientCtx->grpc->Destroy();
                    clientCtx->grpc = nullptr;
                    break;
                }
            }

            // NOTE:
            // Free the reserved configuration only if we succeed to activate
            FreeConfig(reservedConfiguration);
        } else {
            ErrPrint("The client or the server context was not initialized");
            ret = -EFAULT;
            break;
        }
    } while (0);

    free(privateKey);
    privateKey = nullptr;
    free(certificate);
    certificate = nullptr;
    free(rootCert);
    rootCert = nullptr;

    return ret;
}

int Peer::Deactivate(void)
{
    if (serverCtx != nullptr) {
        if (serverCtx->grpc == nullptr) {
            return -EILSEQ;
        }

        serverCtx->grpc->Destroy();
        serverCtx->grpc = nullptr;
    } else if (clientCtx != nullptr) {
        if (clientCtx->grpc == nullptr) {
            return -EILSEQ;
        }

        clientCtx->grpc->Destroy();
        clientCtx->grpc = nullptr;
    } else {
        ErrPrint("The client or the server context was not initialized");
        return -EFAULT;
    }

    return 0;
}

int Peer::GetInfo(const beyond_peer_info *&info)
{
    if (clientCtx != nullptr && clientCtx->grpc != nullptr) {
        clientCtx->grpc->GetInfo(this->info);
    }

    info = const_cast<const beyond_peer_info *>(this->info);
    return 0;
}

beyond_plugin_peer_nn_config *Peer::DuplicateConfig(const beyond_plugin_peer_nn_config *config)
{
    beyond_plugin_peer_nn_config *_config;

    if (config == nullptr) {
        return nullptr;
    }

    _config = static_cast<beyond_plugin_peer_nn_config *>(calloc(1, sizeof(beyond_plugin_peer_nn_config)));
    if (_config == nullptr) {
        ErrPrintCode(errno, "calloc");
        return nullptr;
    }

    _config->client.input_type = _config->server.input_type = config->client.input_type;

    if (config->client.preprocessing != nullptr) {
        _config->client.preprocessing = strdup(config->client.preprocessing);
        if (_config->client.preprocessing == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    if (config->client.postprocessing != nullptr) {
        _config->client.postprocessing = strdup(config->client.postprocessing);
        if (_config->client.postprocessing == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    if (config->server.preprocessing != nullptr) {
        _config->server.preprocessing = strdup(config->server.preprocessing);
        if (_config->server.preprocessing == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    if (config->server.postprocessing != nullptr) {
        _config->server.postprocessing = strdup(config->server.postprocessing);
        if (_config->server.postprocessing == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    if (config->server.framework != nullptr) {
        _config->server.framework = strdup(config->server.framework);
        if (_config->server.framework == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    if (config->server.accel != nullptr) {
        _config->server.accel = strdup(config->server.accel);
        if (_config->server.accel == nullptr) {
            ErrPrintCode(errno, "strdup");
            FreeConfig(_config);
            return nullptr;
        }
    }

    return _config;
}

void Peer::FreeConfig(beyond_plugin_peer_nn_config *&config)
{
    if (config == nullptr) {
        return;
    }

    free(config->server.preprocessing);
    config->server.preprocessing = nullptr;
    free(config->server.postprocessing);
    config->server.postprocessing = nullptr;
    free(config->client.preprocessing);
    config->client.preprocessing = nullptr;
    free(config->client.postprocessing);
    config->client.postprocessing = nullptr;
    free(config->server.framework);
    config->server.framework = nullptr;
    free(config->server.accel);
    config->server.accel = nullptr;
    free(config);
    config = nullptr;
}

void Peer::ResetRuntime(beyond_peer_info_runtime *&runtimes, int count_of_runtimes)
{
    for (int i = 0; i < count_of_runtimes; i++) {
        free(runtimes[i].name);
        runtimes[i].name = nullptr;

        for (int j = 0; j < runtimes[i].count_of_devices; j++) {
            free(runtimes[i].devices[j].name);
            runtimes[i].devices[j].name = nullptr;
        }

        free(runtimes[i].devices);
        runtimes[i].devices = nullptr;
    }

    free(runtimes);
    runtimes = nullptr;
    count_of_runtimes = 0;
}

void Peer::ResetInfo(beyond_peer_info *&info)
{
    if (info == nullptr) {
        return;
    }

    ResetRuntime(info->runtimes, info->count_of_runtimes);
    info->count_of_runtimes = 0;

    free(info->host);
    info->host = nullptr;

    info->uuid[0] = '\0';

    // TODO: change the type of port
    info->port[0] = 0;
    info->free_memory = 0llu;
    info->free_storage = 0llu;

    free(info);
    info = nullptr;
}

int Peer::SetInfo(beyond_peer_info *info)
{
    // Discovery module should set peer information first
    if (info == nullptr) {
        return -EINVAL;
    }

    beyond_peer_info *newInfo;

    // TODO:
    // If the peer information is updated after the peer is prepared,
    // we have to rebuild the pipeline.

    if (info->host == nullptr) {
        return -EINVAL;
    }

    // TODO: change the type of port
    if (info->port[0] == 0) {
        if (serverCtx != nullptr) {
            DbgPrint("The port is going to be selected by the grpc");
        } else if (clientCtx != nullptr) {
            DbgPrint("Invalid port");
            return -EINVAL;
        } else {
            ErrPrint("There is no valid context");
            assert(!"There is no valid context");
            return -EFAULT;
        }
    }

    newInfo = static_cast<beyond_peer_info *>(calloc(1, sizeof(beyond_peer_info)));
    if (newInfo == nullptr) {
        return -ENOMEM;
    }

    newInfo->host = strdup(info->host);
    if (newInfo->host == nullptr) {
        free(newInfo);
        newInfo = nullptr;
        return -ENOMEM;
    }

    snprintf(newInfo->uuid, BEYOND_UUID_LEN, "%s", info->uuid);

    // TODO: change the type of port
    newInfo->port[0] = info->port[0];

    if (info->count_of_runtimes > 0) {
        newInfo->runtimes = static_cast<beyond_peer_info_runtime *>(calloc(info->count_of_runtimes, sizeof(beyond_peer_info_runtime)));
        if (newInfo->runtimes == nullptr) {
            ResetInfo(newInfo);
            return -ENOMEM;
        }

        newInfo->count_of_runtimes = info->count_of_runtimes;

        for (int i = 0; i < info->count_of_runtimes; i++) {
            newInfo->runtimes[i].name = strdup(info->runtimes[i].name);
            if (newInfo->runtimes[i].name == nullptr) {
                ResetInfo(newInfo);
                return -ENOMEM;
            }

            newInfo->runtimes[i].devices = static_cast<beyond_peer_info_device *>(calloc(info->runtimes[i].count_of_devices, sizeof(beyond_peer_info_device)));
            if (newInfo->runtimes[i].devices == nullptr) {
                ResetInfo(newInfo);
                return -ENOMEM;
            }

            newInfo->runtimes[i].count_of_devices = info->runtimes[i].count_of_devices;

            for (int j = 0; j < info->runtimes[i].count_of_devices; j++) {
                newInfo->runtimes[i].devices[j].name = strdup(info->runtimes[i].devices[j].name);
                if (newInfo->runtimes[i].devices[j].name == nullptr) {
                    ResetInfo(newInfo);
                    return -ENOMEM;
                }
            }
        }
    }

    newInfo->free_memory = info->free_memory;
    newInfo->free_storage = info->free_storage;

    ResetInfo(this->info);

    this->info = newInfo;

    int ret = eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_PEER_INFO_UPDATED);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

Peer::Peer(void)
    : eventObject(nullptr)
    , info(nullptr)
    , reservedConfiguration(nullptr)
    , authenticator(nullptr)
    , caAuthenticator(nullptr)
{
}

Peer::~Peer(void)
{
}
