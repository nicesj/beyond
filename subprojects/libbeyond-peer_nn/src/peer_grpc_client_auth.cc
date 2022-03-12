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

#include "peer_grpc_client_auth.h"
#include "peer_grpc_client_gst.h"
#include <map>

Peer::GrpcClient::Auth::Auth(Peer::GrpcClient *client)
    : grpcClient(client)
{
}

Peer::GrpcClient::Auth::~Auth(void)
{
}

grpc::Status Peer::GrpcClient::Auth::GetMetadata(
    grpc::string_ref service_url, grpc::string_ref method_name,
    const grpc::AuthContext &channel_auth_context,
    std::multimap<grpc::string, grpc::string> *metadata)
{
    // NOTE:
    // If there is a way to get the method name from the server's metadata processor,
    // we don't need to set the method name to the metadata.
    metadata->insert(std::make_pair(grpc::string("method_name"), grpc::string(method_name.data())));
    if (strncmp("ExchangeKey", method_name.data(), method_name.length()) == 0) {
        return grpc::Status::OK;
    }

    unsigned long nonce = grpcClient->GetGst()->GetNonce();

    metadata->insert(std::make_pair(grpc::string("nonce"), std::to_string(nonce)));
    metadata->insert(std::make_pair(grpc::string("id"), grpcClient->peerId));

    grpcClient->GetGst()->SetNonce(++nonce);
    DbgPrint("Let's update the metadata - %s %s", service_url.data(), method_name.data());
    return grpc::Status::OK;
}
