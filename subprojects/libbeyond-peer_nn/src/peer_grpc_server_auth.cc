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

#include "peer_grpc_server_auth.h"
#include "peer_grpc_server_gst.h"

#include <cassert>
#include <map>

Peer::GrpcServer::Auth::Auth(Peer::GrpcServer *server)
    : grpcServer(server)
{
    DbgPrint("Constructed");
}

Peer::GrpcServer::Auth::~Auth(void)
{
}

bool Peer::GrpcServer::Auth::IsBlocking() const
{
    return true;
}

grpc::Status Peer::GrpcServer::Auth::Process(const grpc::AuthMetadataProcessor::InputMetadata &auth_metadata,
                                             grpc::AuthContext *context,
                                             grpc::AuthMetadataProcessor::OutputMetadata *consumed_auth_metadata,
                                             grpc::AuthMetadataProcessor::OutputMetadata *response_metadata)
{
    assert(consumed_auth_metadata != nullptr);
    assert(context != nullptr);
    assert(response_metadata != nullptr);

    // :path
    std::multimap<grpc::string_ref, grpc::string_ref>::const_iterator methodName = auth_metadata.find(grpc::string_ref("method_name"));
    if (methodName == auth_metadata.end()) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, std::string("Invalid method_name"));
    }

    if (methodName->second.length() <= 0) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, std::string("Invalid method_name"));
    }

    if (strncmp(methodName->second.data(), "ExchangeKey", methodName->second.length()) == 0) {
        return grpc::Status::OK;
    }

    std::multimap<grpc::string_ref, grpc::string_ref>::const_iterator peerId = auth_metadata.find(grpc::string_ref("id"));
    if (peerId == auth_metadata.end()) {
        ErrPrint("Peer Id not found");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, std::string("Invalid argument"));
    }

    if (peerId->second.length() <= 0) {
        ErrPrint("Invalid Peer Id");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, std::string("Invalid argument"));
    }

    Peer::GrpcServer::Gst *gst = grpcServer->clientMap[std::string(peerId->second.data(), peerId->second.length())];
    if (gst == nullptr) {
        ErrPrint("GST is not initialized for the given Peer Id");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, std::string("Not Found"));
    }

    unsigned long nonce = gst->GetNonce();

    std::multimap<grpc::string_ref, grpc::string_ref>::const_iterator nonceIt = auth_metadata.find(grpc::string_ref("nonce"));
    if (nonceIt == auth_metadata.end()) {
        ErrPrint("Invalid nonce");
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, std::string("Unauthenticated"));
    }

    if (nonceIt->second.size() <= 0) {
        ErrPrint("Invalid nonce");
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, std::string("Unauthenticated"));
    }

    DbgPrint("%lu %s", nonce, std::string(nonceIt->second.data(), nonceIt->second.size()).c_str());

    if (std::to_string(nonce).compare(std::string(nonceIt->second.data(), nonceIt->second.size())) != 0) {
        ErrPrint("Invalid nonce");
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, std::string("Unauthenticated"));
    }

    gst->SetNonce(++nonce);

    return grpc::Status::OK;
}
