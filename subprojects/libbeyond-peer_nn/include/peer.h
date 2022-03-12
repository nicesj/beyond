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

#ifndef __BEYOND_PEER_NN_PEER_H__
#define __BEYOND_PEER_NN_PEER_H__

#include <memory>
#include <string>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"

#define MASTER_KEY_SIZE 30

class Peer final : public beyond::InferenceInterface::PeerInterface {
public:
    static constexpr const char *NAME = BEYOND_PLUGIN_PEER_NN_NAME;
    static Peer *Create(bool isServer = false, const char *framework = nullptr, const char *accel = nullptr, const char *storagePath = nullptr);

public: // module interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;
    void Destroy(void) override;

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

public: // inference interface
    int Configure(const beyond_config *options = nullptr) override;
    int LoadModel(const char *model) override;
    int GetInputTensorInfo(const beyond_tensor_info *&info, int &size) override;
    int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size) override;
    int SetInputTensorInfo(const beyond_tensor_info *info, int size) override;
    int SetOutputTensorInfo(const beyond_tensor_info *info, int size) override;
    int AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor) override;
    void FreeTensor(beyond_tensor *&tensor, int size) override;
    int Prepare(void) override;

    int Invoke(const beyond_tensor *input, int size, const void *context = nullptr) override;
    int GetOutput(beyond_tensor *&tensor, int &size) override;

    int Stop(void) override;

public: // peer interface
    int Activate(void) override;
    int Deactivate(void) override;

    int GetInfo(const beyond_peer_info *&info) override;

    // NOTE:
    // SetInfo() method must invoke the event handler with BEYOND_PEER_EVENT_INFO_UPDATED event type.
    int SetInfo(beyond_peer_info *info) override;

private:
    class EventObject;
    class GrpcClient;
    class GrpcServer;
    class Model;

    struct ServerContext {
        Peer::GrpcServer *grpc;
        std::string storagePath;
    };

    struct ClientContext {
        Peer::GrpcClient *grpc;
        std::string framework;
        std::string accel;
    };

    struct Credential {
        uint64_t nonce;
        int32_t sessionKeyLength;
        char uuid[37]; // including the null byte
        char payload[1];
    };

private:
    Peer(void);
    virtual ~Peer(void);

    int ConfigureInput(const beyond_config *options);
    int ConfigureAuthenticator(const beyond_config *options);
    int ConfigureCAAuthenticator(const beyond_config *options);

    static void ConfigureImageInput(beyond_input_config *config, std::ostringstream &client_format, std::ostringstream &server_format);
    static void ConfigureVideoInput(beyond_input_config *config, std::ostringstream &client_format, std::ostringstream &server_format);

    static void ResetInfo(beyond_peer_info *&info);
    static void ResetRuntime(beyond_peer_info_runtime *&runtimes, int count_of_runtimes);
    static beyond_plugin_peer_nn_config *DuplicateConfig(const beyond_plugin_peer_nn_config *config);
    static void FreeConfig(beyond_plugin_peer_nn_config *&config);

private:
    std::unique_ptr<ServerContext> serverCtx;
    std::unique_ptr<ClientContext> clientCtx;

    Peer::EventObject *eventObject;
    beyond_peer_info *info;

    beyond_plugin_peer_nn_config *reservedConfiguration;
    beyond::AuthenticatorInterface *authenticator;
    beyond::AuthenticatorInterface *caAuthenticator;
};

#endif // __BEYOND_PEER_NN_PEER_H__
