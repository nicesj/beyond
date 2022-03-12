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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_SERVER_H__
#define __BEYOND_PEER_NN_PEER_GRPC_SERVER_H__

#include "peer.h"
#include "peer_nn.grpc.pb.h"

#include <memory>
#include <map>

#include <pthread.h>

class Peer::GrpcServer final : public ::peer_nn::RPC::Service {
public:
    static GrpcServer *Create(Peer *peer, const char *address, const char *certificate = nullptr, const char *privateKey = nullptr, const char *rootCert = nullptr);
    virtual void Destroy(void);

    ::grpc::Status Configure(::grpc::ServerContext *context, const ::peer_nn::Configuration *request, ::peer_nn::Response *response) override;
    ::grpc::Status ExchangeKey(::grpc::ServerContext *context, const ::peer_nn::ExchangeKeyRequest *request, ::peer_nn::ExchangeKeyResponse *response) override;
    ::grpc::Status LoadModel(::grpc::ServerContext *context, const ::peer_nn::Model *request, ::peer_nn::Response *response) override;
    ::grpc::Status UploadModel(::grpc::ServerContext *context, ::grpc::ServerReader<::peer_nn::ModelFile> *reader, ::peer_nn::Response *response) override;
    ::grpc::Status GetInputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::TensorInfos *response) override;
    ::grpc::Status SetInputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::TensorInfos *request, ::peer_nn::Response *response) override;
    ::grpc::Status GetOutputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::TensorInfos *response) override;
    ::grpc::Status SetOutputTensorInfo(::grpc::ServerContext *context, const ::peer_nn::TensorInfos *request, ::peer_nn::Response *response) override;
    ::grpc::Status Prepare(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::PreparedResponse *response) override;
    ::grpc::Status Stop(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::Response *response) override;
    ::grpc::Status GetInfo(::grpc::ServerContext *context, const ::peer_nn::Empty *request, ::peer_nn::Info *response) override;

private:
    class Auth;
    class Gst;

private:
    GrpcServer(void);
    virtual ~GrpcServer(void);
    int GetGst(::grpc::ServerContext *context, Peer::GrpcServer::Gst *&gst);
    int InfoToResponse(const beyond_tensor_info *info, int size, ::peer_nn::TensorInfos *response);
    int RequestToInfo(const ::peer_nn::TensorInfos *request, beyond_tensor_info *&info);

    static void *Main(void *ptr);

    pthread_t threadId;
    std::unique_ptr<::grpc::Server> server;
    Peer *peer;
    unsigned long nextPeerId;
    std::map<std::string, Peer::GrpcServer::Gst *> clientMap;
};

#endif // __BEYOND_PEER_NN_PEER_GRPC_SERVER_H__
