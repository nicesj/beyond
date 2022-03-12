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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_CLIENT_AUTH_H__
#define __BEYOND_PEER_NN_PEER_GRPC_CLIENT_AUTH_H__

#include "peer_grpc_client.h"
#include <grpc++/grpc++.h>

class Peer::GrpcClient::Auth final : public grpc::MetadataCredentialsPlugin {
public:
    explicit Auth(Peer::GrpcClient *client);
    virtual ~Auth(void);

    grpc::Status GetMetadata(
        grpc::string_ref service_url, grpc::string_ref method_name,
        const grpc::AuthContext &channel_auth_context,
        std::multimap<grpc::string, grpc::string> *metadata) override;

private:
    Peer::GrpcClient *grpcClient;
};

#endif // __BEYOND_PEER_NN_PEER_GRPC_CLIENT_AUTH_H__
