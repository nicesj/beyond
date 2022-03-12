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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_CLIENT_GST_SOURCE_H__
#define __BEYOND_PEER_NN_PEER_GRPC_CLIENT_GST_SOURCE_H__

#include "peer_grpc_client_gst.h"

#include <glib.h>
#include <gst/gst.h>

class Peer::GrpcClient::Gst::Source final {
public:
    static Source *Create(Peer::GrpcClient::Gst *gstClient, const gchar *pipelineDesc);
    void Destroy(void);

    int Stop(void);

    int Invoke(Peer::GrpcClient::Gst::InvokeData *invokeData);

    static void BusHandler(GstBus *bus, GstMessage *message, gpointer user_data);
    static void StartFeedHandler(GstElement *pipeline, guint size, gpointer user_data);
    static void StopFeedHandler(GstElement *pipeline, gpointer user_data);

private:
    Source(void);
    ~Source(void);

    Peer::GrpcClient::Gst *gstClient;
    GstElement *pipeline;
    GstElement *element;
    GstBus *bus;
};

#endif // __BEYOND_PEER_NN_PEER_GST_CLIENT_SOURCE_H__
