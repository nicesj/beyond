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

#ifndef __BEYOND_PEER_NN_PEER_GRPC_SERVER_GST_H__
#define __BEYOND_PEER_NN_PEER_GRPC_SERVER_GST_H__

#include "peer_grpc_server.h"
#include "peer_model.h"

#include <string>

#include <glib.h>
#include <gst/gst.h>
#include <pthread.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

class Peer::GrpcServer::Gst final {
public:
    static Gst *Create(std::string &peerId, Peer::GrpcServer *grpcServer);
    void Destroy(void);

    int Configure(const beyond_plugin_peer_nn_config::server_description *config);
    int Prepare(int &reqPort, int &resPort);
    int Stop(void);
    void SetSecret(std::string &secret);
    std::shared_ptr<Peer::Model> GetModel(void) const;
    unsigned long GetNonce(void) const;
    void SetNonce(unsigned long nonce = 0);

private:
    // There is a new thread for integrating the nnstreamer (gst_X) to the glib main loop.
    // The glib main loop is created on a newly created thread.
    // In order to control the nnstreamer thread, this command structure would be used.
    enum Command : int {
        IdReady = 0x00,
        IdPrepare = 0x01,
        IdStop = 0x02,
        IdExit = 0x03,
        IdLast = 0x04,
    };

    struct Thread {
        typedef int (*CommandHandler)(Peer::GrpcServer::Gst *impls, void *data);

        struct EventUserData {
            GSource _source;
            Peer::GrpcServer::Gst *impls;
            GPollFD fd;
        };

        GMainLoop *loop;
        GstElement *pipeline;
        GstBus *bus;
        const CommandHandler cmdTable[Command::IdLast];
        std::unique_ptr<beyond::CommandObject> command;

        static void BusHandler(GstBus *bus, GstMessage *message, gpointer user_data);

        static int CommandHandlerReady(Peer::GrpcServer::Gst *impls, void *data);
        static int CommandHandlerPrepare(Peer::GrpcServer::Gst *impls, void *data);
        static int CommandHandlerStop(Peer::GrpcServer::Gst *impls, void *data);
        static int CommandHandlerExit(Peer::GrpcServer::Gst *impls, void *data);

        static gboolean CommandPrepare(GSource *source, gint *timeout);
        static gboolean CommandCheck(GSource *source);
        static gboolean CommandHandle(GSource *source, GSourceFunc callback, gpointer user_data);
        static void *Main(void *arg);
    };

    union PrepareData {
        gchar *pipelineDescription;
        struct Port {
            gint request;
            gint response;
            int status;
        } port;
    };

    typedef int (*DimsParser_t)(const char *, int *);

    struct RtpConfig {
        std::string depayloader;
        std::string encodingName;
        int payload;
    };

private:
    Gst(void);
    virtual ~Gst(void);

    static int DimsParser1(const char *str, int *values);
    static int DimsParser2(const char *str, int *values);
    static int DimsParser3(const char *str, int *values);
    static int DimsParser4(const char *str, int *values);
    static int ExtractTensorInfo(GstElement *element, const char *type, beyond_tensor_info *&info, int &size);
    static std::string TensorInfoToProperties(const char *type, const beyond_tensor_info *info, int size);

private:
    Peer::GrpcServer *grpc;
    std::string framework;
    std::string accel;
    std::unique_ptr<beyond::CommandObject> command;
    Thread threadCtx;
    pthread_t threadId;
    unsigned long nonce;

    RtpConfig rtpConfig;
    std::string preprocessing;
    std::string postprocessing;
    std::string secretKey;
    std::shared_ptr<Peer::Model> model;
    std::string peerId;

    static DimsParser_t dimsParsers[4];
};

#endif // __BEYOND_PEER_NN_PEER_GRPC_SERVER_GST_H__
