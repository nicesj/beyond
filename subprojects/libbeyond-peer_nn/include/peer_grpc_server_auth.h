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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_SERVER_AUTH_H__
#define __BEYOND_PEER_NN_PEER_GRPC_SERVER_AUTH_H__

#include "peer_grpc_server.h"
#include <grpc++/grpc++.h>

class Peer::GrpcServer::Auth final : public grpc::AuthMetadataProcessor {
public:
    explicit Auth(Peer::GrpcServer *server);
    virtual ~Auth(void);

    bool IsBlocking() const override;
    grpc::Status Process(const grpc::AuthMetadataProcessor::InputMetadata &auth_metadata,
                         grpc::AuthContext *context,
                         grpc::AuthMetadataProcessor::OutputMetadata *consumed_auth_metadata,
                         grpc::AuthMetadataProcessor::OutputMetadata *response_metadata) override;

private:
    Peer::GrpcServer *grpcServer;
};

#endif // __BEYOND_PEER_NN_PEER_GRPC_SERVER_AUTH_H__
