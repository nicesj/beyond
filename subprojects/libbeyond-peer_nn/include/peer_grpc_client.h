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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_CLIENT_H__
#define __BEYOND_PEER_NN_PEER_GRPC_CLIENT_H__

#include "peer.h"
#include "peer_nn.grpc.pb.h"

#include "beyond/plugin/peer_nn_plugin.h"

#include <memory>
#include <string>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

class Peer::GrpcClient {
public:
    constexpr static const int CHUNK_SIZE = 4096;

public:
    class Gst;

public:
    static GrpcClient *Create(Peer *peer, const char *address, const char *rootCA = nullptr);
    virtual void Destroy(void);

    int Configure(const beyond_plugin_peer_nn_config::server_description *server);
    int ExchangeKey(void);

    int LoadModel(const char *model);
    int UploadModel(const char *model);

    int GetInfo(beyond_peer_info *info);

    int GetInputTensorInfo(const beyond_tensor_info *&info, int &size);
    int SetInputTensorInfo(const beyond_tensor_info *info, int size);
    int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size);
    int SetOutputTensorInfo(const beyond_tensor_info *info, int size);

    int Prepare(int &request_port, int &response_port);

    int Stop(void);

    Peer::GrpcClient::Gst *GetGst(void);

private:
    class Auth;

private:
    explicit GrpcClient(Peer *peer);
    virtual ~GrpcClient(void);

    uint64_t GetRandom();
    void ResetTensorInfo(beyond_tensor_info *&info, int &size);
    int GetTensorInfoFromResponse(::peer_nn::TensorInfos &tensorInfos, beyond_tensor_info *&info, int &size);
    int SetRequest(::peer_nn::TensorInfos &request, const beyond_tensor_info *info, int size);

    std::unique_ptr<peer_nn::RPC::Stub> stub;
    Peer *peer;
    Peer::GrpcClient::Gst *gst;
    std::string peerId;
};

#endif // __BEYOND_PEER_NN_PEER_GRPC_CLIENT_H__
