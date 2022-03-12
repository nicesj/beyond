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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_CLIENT_GST_H__
#define __BEYOND_PEER_NN_PEER_GRPC_CLIENT_GST_H__

#include "peer_grpc_client.h"
#include "peer_model.h"

#include <string>
#include <memory>
#include <queue>

#include <glib.h>
#include <pthread.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include "beyond/plugin/peer_nn_plugin.h"

#define SRCX_NAME "srcx"
#define SINKX_NAME "sinkx"

class Peer::GrpcClient::Gst final {
public:
    static Gst *Create(std::string &peerId, Peer::GrpcClient *grpcClient);
    void Destroy(void);

    int Configure(const beyond_plugin_peer_nn_config::client_description *options);
    int Prepare(const char *host, int requestPort, int responsePort);
    int Invoke(const beyond_tensor *input, int size, const void *context);
    int GetOutput(beyond_tensor *&tensor, int &size);
    int Stop(void);
    void SetSecret(std::string &secret);
    std::shared_ptr<Peer::Model> GetModel(void) const;
    unsigned long GetNonce(void) const;
    void SetNonce(unsigned long nonce = 0);

private:
    class Source;
    class Sink;
    // There is a new thread for integrating the nnstreamer (gst_X) to the glib main loop.
    // The glib main loop is created on a newly created thread.
    // In order to control the nnstreamer thread, this command structure would be used.
    enum Command : int {
        IdReady = 0x00,
        IdPrepare = 0x01,
        IdInvoke = 0x02,
        IdStop = 0x03,
        IdExit = 0x04,
        IdLast = 0x05,
    };

    struct Thread {
        typedef int (*CommandHandler)(Peer::GrpcClient::Gst *impls, void *data);

        struct EventUserData {
            GSource _source;
            Peer::GrpcClient::Gst *impls;
            GPollFD fd;
        };

        GMainLoop *loop;
        Peer::GrpcClient::Gst::Source *gstSource;
        Peer::GrpcClient::Gst::Sink *gstSink;
        const CommandHandler cmdTable[Command::IdLast];
        std::unique_ptr<beyond::CommandObject> command;
        std::unique_ptr<beyond::CommandObject> output;

        static int CommandHandlerReady(Peer::GrpcClient::Gst *impls, void *data);
        static int CommandHandlerPrepare(Peer::GrpcClient::Gst *impls, void *data);
        static int CommandHandlerInvoke(Peer::GrpcClient::Gst *impls, void *data);
        static int CommandHandlerStop(Peer::GrpcClient::Gst *impls, void *data);
        static int CommandHandlerExit(Peer::GrpcClient::Gst *impls, void *data);

        static gboolean CommandPrepare(GSource *source, gint *timeout);
        static gboolean CommandCheck(GSource *source);
        static gboolean CommandHandle(GSource *source, GSourceFunc callback, gpointer user_data);
        static void *Main(void *arg);
    };

    struct PrepareData {
        gchar *request;
        gchar *response;
    };

    struct InvokeData {
        const beyond_tensor *tensor;
        int size;
        const void *context;
    };

    struct RtpConfig {
        std::string payloader;
        std::string encodingName;
        int payload;
    };

private:
    Gst(void);
    virtual ~Gst(void);

    static int TensorInfoToDesc(const beyond_tensor_info *info, int size, char *&strbuf, int &len);

private:
    Peer::GrpcClient *grpcClient;
    unsigned long nonce;
    pthread_mutex_t requestQueueMutex;
    std::unique_ptr<beyond::CommandObject> command;
    std::unique_ptr<beyond::CommandObject> output;
    Thread threadCtx;
    pthread_t threadId;
    std::shared_ptr<Peer::Model> model;

    RtpConfig rtpConfig;
    std::string preprocessing;
    std::string postprocessing;
    std::string secretKey;
    std::queue<InvokeData *> requestQueue;
    std::string peerId;
};

#include "peer_grpc_client_gst_sink.h"
#include "peer_grpc_client_gst_source.h"

#endif // __BEYOND_PEER_NN_PEER_GRPC_CLIENT_GST_H__
