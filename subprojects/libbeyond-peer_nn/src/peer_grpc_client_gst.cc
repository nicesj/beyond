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

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "peer_grpc_client_gst.h"
#include "peer_model.h"

#include <cstdio>
#include <cerrno>

#include <exception>
#include <memory>

#include <pthread.h>

#include <glib.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"

int Peer::GrpcClient::Gst::Thread::CommandHandlerReady(Peer::GrpcClient::Gst *impls, void *data)
{
    return 0;
}

int Peer::GrpcClient::Gst::Thread::CommandHandlerPrepare(Peer::GrpcClient::Gst *impls, void *data)
{
    PrepareData *prepareData = static_cast<PrepareData *>(data);
    int ret = 0;

    do {
        impls->threadCtx.gstSource = Peer::GrpcClient::Gst::Source::Create(impls, prepareData->request);
        if (impls->threadCtx.gstSource == nullptr) {
            ErrPrint("Failed to build the gst pipeline");
            ret = -EFAULT;
            break;
        }

        impls->threadCtx.gstSink = Peer::GrpcClient::Gst::Sink::Create(impls, prepareData->response);
        if (impls->threadCtx.gstSink == nullptr) {
            ErrPrint("Failed to build the gst pipeline");
            impls->threadCtx.gstSource->Destroy();
            impls->threadCtx.gstSource = nullptr;
            ret = -EFAULT;
            break;
        }
    } while (0);

    g_free(prepareData->request);
    g_free(prepareData->response);

    delete prepareData;
    prepareData = nullptr;

    ret = impls->threadCtx.command->Send(Command::IdPrepare, reinterpret_cast<void *>(static_cast<long>(ret)));
    if (ret < 0) {
        ErrPrint("Failed to send preparae done notification");
        return ret;
    }

    return 0;
}

int Peer::GrpcClient::Gst::Thread::CommandHandlerInvoke(Peer::GrpcClient::Gst *impls, void *data)
{
    InvokeData *invokeData = static_cast<InvokeData *>(data);

    int ret = impls->threadCtx.gstSource->Invoke(invokeData);
    if (ret < 0) {
        ErrPrint("Failed to run invoke");
    }

    ret = impls->threadCtx.command->Send(Command::IdInvoke, reinterpret_cast<void *>(static_cast<long>(ret)));
    if (ret < 0) {
        ErrPrint("Failed to send invoke request done notification");
        return ret;
    }

    return 0;
}

int Peer::GrpcClient::Gst::Thread::CommandHandlerStop(Peer::GrpcClient::Gst *impls, void *data)
{
    impls->threadCtx.gstSource->Stop();
    impls->threadCtx.gstSink->Stop();
    return 0;
}

int Peer::GrpcClient::Gst::Thread::CommandHandlerExit(Peer::GrpcClient::Gst *impls, void *data)
{
    g_main_loop_quit(impls->threadCtx.loop);
    return 0;
}

gboolean Peer::GrpcClient::Gst::Thread::CommandPrepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

gboolean Peer::GrpcClient::Gst::Thread::CommandCheck(GSource *source)
{
    Peer::GrpcClient::Gst::Thread::EventUserData *ud = reinterpret_cast<Peer::GrpcClient::Gst::Thread::EventUserData *>(source);

    if ((ud->fd.revents & G_IO_IN) == G_IO_IN) {
        return TRUE;
    }

    if ((ud->fd.revents & G_IO_ERR) == G_IO_ERR) {
        ErrPrint("Error event!");
        return TRUE;
    }

    return FALSE;
}

gboolean Peer::GrpcClient::Gst::Thread::CommandHandle(GSource *source, GSourceFunc callback, gpointer user_data)
{
    Peer::GrpcClient::Gst::Thread::EventUserData *ud = reinterpret_cast<Peer::GrpcClient::Gst::Thread::EventUserData *>(source);
    Peer::GrpcClient::Gst *impls = ud->impls;

    if ((ud->fd.revents & G_IO_ERR) == G_IO_ERR) {
        ErrPrint("Error!");
        return FALSE;
    }

    int id;
    void *data;
    if (impls->threadCtx.command->Recv(id, data) < 0) {
        ErrPrint("Failed to get a command");
        return FALSE;
    }

    if (id >= Command::IdReady && id < Command::IdLast && impls->threadCtx.cmdTable[id] != nullptr) {
        int ret = impls->threadCtx.cmdTable[id](impls, data);
        if (ret < 0) {
            ErrPrint("command handler returns %d", ret);

            // NOTE:
            // Return true in order to handle the next command properly
            return TRUE;
        }
    } else {
        assert(id >= Command::IdReady && id < Command::IdLast);
        assert(impls->threadCtx.cmdTable[id] != nullptr);
        ErrPrint("Handler was not mapped or invalid id");
    }

    return TRUE;
}

std::shared_ptr<Peer::Model> Peer::GrpcClient::Gst::GetModel(void) const
{
    return model;
}

Peer::GrpcClient::Gst *Peer::GrpcClient::Gst::Create(std::string &peerId, Peer::GrpcClient *grpcClient)
{
    GrpcClient::Gst *impls;
    int spfd[2];
    int pfd[2];

    try {
        impls = new GrpcClient::Gst();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    impls->grpcClient = grpcClient;
    impls->model = std::make_shared<Peer::Model>();
    impls->peerId = peerId;

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, spfd) < 0) {
        ErrPrintCode(errno, "socketpair");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    if (pipe2(pfd, O_CLOEXEC) < 0) {
        ErrPrintCode(errno, "pipe2");
        if (close(spfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    impls->command = std::make_unique<beyond::CommandObject>(spfd[0]);
    impls->threadCtx.command = std::make_unique<beyond::CommandObject>(spfd[1]);
    impls->output = std::make_unique<beyond::CommandObject>(pfd[0]);
    impls->threadCtx.output = std::make_unique<beyond::CommandObject>(pfd[1]);

    int status = pthread_create(&impls->threadId, nullptr, Peer::GrpcClient::Gst::Thread::Main, static_cast<void *>(impls));
    if (status != 0) {
        ErrPrintCode(status, "pthread_create");
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    // Wait for the g_main_loop ready
    int cmdId = Command::IdLast;
    if (impls->command->Recv(cmdId) < 0 || cmdId != Command::IdReady) {
        ErrPrint("CommandId: 0x%.8X", cmdId);
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    return impls;
}

void Peer::GrpcClient::Gst::Destroy(void)
{
    void *retval;

    if (command->Send(Command::IdExit) < 0) {
        ErrPrint("Failed to send Exit command");
    }

    int status = pthread_join(threadId, &retval);
    if (status != 0) {
        ErrPrintCode(status, "pthread_join");
    }

    if (close(command->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(threadCtx.command->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(output->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(threadCtx.output->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (threadCtx.gstSource != nullptr) {
        threadCtx.gstSource->Destroy();
        threadCtx.gstSource = nullptr;
    }

    if (threadCtx.gstSink != nullptr) {
        threadCtx.gstSink->Destroy();
        threadCtx.gstSink = nullptr;
    }

    delete this;
}

int Peer::GrpcClient::Gst::Configure(const beyond_plugin_peer_nn_config::client_description *client)
{
    beyond_input_type input_type = static_cast<beyond_input_type>(client->input_type);
    if (input_type == BEYOND_INPUT_TYPE_IMAGE) {
        rtpConfig = {
            .payloader = "rtpjpegpay",
            .encodingName = "JPEG",
            .payload = 26,
        };
    } else if (input_type == BEYOND_INPUT_TYPE_VIDEO) {
        rtpConfig = {
            .payloader = "rtpvp8pay",
            .encodingName = "VP8",
            .payload = 96,
        };
    }

    if (client->preprocessing != nullptr) {
        preprocessing = std::string(client->preprocessing);
    }

    if (client->postprocessing != nullptr) {
        postprocessing = std::string(client->postprocessing);
    }
    return 0;
}

int Peer::GrpcClient::Gst::TensorInfoToDesc(const beyond_tensor_info *info, int size, char *&strbuf, int &len)
{
    std::string desc;

    desc = std::string("other/tensors,num_tensors=") + std::to_string(size) + ",framerate=(fraction)0/1";

    for (int i = 0; i < size; i++) {
        if (i > 0) {
            desc += std::string(",");
        }

        // NOTE:
        // NHWC (tflite)
        // nnstreamer reverses the dimension for the CAPS negotiation its internal uses.
        desc += std::string(",types=(string)") + std::string(beyond::Inference::TensorTypeToString(info[i].type)) + std::string(",dimensions=(string)");
        switch (info[i].dims->size) {
        case 4:
            desc += std::to_string(info[i].dims->data[3]) + std::string(":");
            FALLTHROUGH;
        case 3:
            desc += std::to_string(info[i].dims->data[2]) + std::string(":");
            FALLTHROUGH;
        case 2:
            desc += std::to_string(info[i].dims->data[1]) + std::string(":");
            FALLTHROUGH;
        case 1:
            desc += std::to_string(info[i].dims->data[0]);
            FALLTHROUGH;
        default:
            switch (info[i].dims->size) {
            case 1:
                desc += std::string(":1");
                FALLTHROUGH;
            case 2:
                desc += std::string(":1");
                FALLTHROUGH;
            case 3:
                desc += std::string(":1");
                FALLTHROUGH;
            default:
                break;
            }
            break;
        }
    }

    char *_strbuf = strdup(desc.c_str());
    if (_strbuf == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "strdup");
        return ret;
    }

    strbuf = _strbuf;
    len = desc.size();
    return 0;
}

int Peer::GrpcClient::Gst::Prepare(const char *host, int requestPort, int responsePort)
{
    char *infoString;

    if (preprocessing.empty() == true) {
        const beyond_tensor_info *info;
        int size;

        int len;

        int ret = grpcClient->GetInputTensorInfo(info, size);
        if (ret < 0) {
            ErrPrint("Unable to get the input tensorInfo");
            return ret;
        }

        ret = TensorInfoToDesc(info, size, infoString, len);
        if (ret < 0) {
            ErrPrint("Failed to build the pipeline description for the tensor information");
            return ret;
        }
    } else {
        infoString = strdup(preprocessing.c_str());
        if (infoString == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "strdup");
            return ret;
        }
    }

    PrepareData *desc;

    try {
        desc = new PrepareData();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        free(infoString);
        infoString = nullptr;
        return -ENOMEM;
    }

    gchar *postPipeline = nullptr;
    if (preprocessing.empty() == true) {
        // NOTE
        // When input_config is not set, use tcp + gdp payloder
        // TODO: need to handle secured channel maybe by using RTSP over tcp + tls

        // Otherwise, need to find general format of rtp payloder for raw tensor
        postPipeline = g_strdup_printf("gdppay ! tcpclientsink host=%s port=%d", host, requestPort);
    } else {
        if (secretKey.empty() == false) {
            // NOTE
            // In this case, the input config must build a jpegenc caps
            // something like..
            //
            // "videoconvert ! video/x-raw,format=I420 ! jpegenc"
            //
            // otherwise the pipeline will not be built
            std::string _secretKey;
            const char *ptr = secretKey.c_str();
            for (int i = 0; i < MASTER_KEY_SIZE && ptr[i] != '\0'; ++i) {
                char hex[3];
                snprintf(hex, sizeof(hex), "%.2X", ptr[i]);
                _secretKey += std::string(hex);
            }

            // Now, we have to build the srtp pipeline using the secretKey
            // the secretKey will be used in the gst instance in order to set the property "key" of the srtpenc element
            // as type of the buffer.
            DbgPrint("GST SecretKey found: %s", secretKey.c_str());
            postPipeline = g_strdup_printf(
                "%s ! application/x-rtp,encoding-name=%s,payload=%d,ssrc=(uint)%s ! "
                "srtpenc key=%s ! "
                "udpsink host=%s port=%d",
                rtpConfig.payloader.c_str(),
                rtpConfig.encodingName.c_str(),
                rtpConfig.payload,
                peerId.c_str(),
                _secretKey.c_str(),
                host, requestPort);
        } else {
            postPipeline = g_strdup_printf(
                "%s ! application/x-rtp,encoding-name=%s,payload=%d ! "
                "udpsink host=%s port=%d",
                rtpConfig.payloader.c_str(),
                rtpConfig.encodingName.c_str(),
                rtpConfig.payload,
                host, requestPort);
        }
    }

    if (postPipeline == nullptr) {
        ErrPrint("g_strdup_printf failed");
        free(infoString);
        infoString = nullptr;
        delete desc;
        desc = nullptr;
        return -EFAULT;
    }

    desc->request = g_strdup_printf("appsrc name=" SRCX_NAME " ! %s ! queue leaky=2 max-size-buffers=1 ! %s", infoString, postPipeline);
    g_free(postPipeline);
    postPipeline = nullptr;
    free(infoString);
    infoString = nullptr;
    if (desc->request == nullptr) {
        ErrPrint("g_strdup_printf failed");
        delete desc;
        desc = nullptr;
        return -EFAULT;
    }

    desc->response = g_strdup_printf("tcpclientsrc host=%s port=%d ! gdpdepay ! other/flatbuf-tensor,framerate=0/1 ! tensor_converter ! tensor_sink name=" SINKX_NAME, host, responsePort);
    if (desc->response == nullptr) {
        ErrPrint("g_strdup_printf failed");
        g_free(desc->request);
        desc->request = nullptr;

        delete desc;
        desc = nullptr;
        return -EFAULT;
    }

    int ret = command->Send(Command::IdPrepare, static_cast<void *>(desc));
    if (ret < 0) {
        g_free(desc->request);
        desc->request = nullptr;

        g_free(desc->response);
        desc->response = nullptr;

        delete desc;
        desc = nullptr;
        return ret;
    }

    int cmdId = Command::IdLast;
    void *data = nullptr;
    ret = command->Recv(cmdId, data);
    if (ret < 0 || cmdId != Command::IdPrepare) {
        ErrPrint("Command error: %d %.8X", ret, cmdId);
        return ret < 0 ? ret : -EINVAL;
    }

    return static_cast<int>(reinterpret_cast<long>(data));
}

int Peer::GrpcClient::Gst::Invoke(const beyond_tensor *input, int size, const void *context)
{
    InvokeData *invokeData;

    try {
        invokeData = new InvokeData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    invokeData->tensor = input;
    invokeData->size = size;
    invokeData->context = context;

    int ret = command->Send(Command::IdInvoke, static_cast<void *>(invokeData));
    if (ret < 0) {
        delete invokeData;
        invokeData = nullptr;
        return ret;
    }

    int cmdId = Command::IdLast;
    void *data = nullptr;

    ret = command->Recv(cmdId, data);
    if (ret < 0 || cmdId != Command::IdInvoke) {
        return ret < 0 ? ret : -EINVAL;
    }

    return static_cast<int>(reinterpret_cast<long>(data));
}

int Peer::GrpcClient::Gst::GetOutput(beyond_tensor *&tensor, int &size)
{
    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    int ret;

    ret = output->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdInvoke || cmdData == nullptr) {
        ErrPrint("Output command is wrong: %.8X data(%p)", cmdId, cmdData);
        return ret;
    }

    InvokeData *inferenceData = static_cast<InvokeData *>(cmdData);
    tensor = const_cast<beyond_tensor *>(inferenceData->tensor);
    size = inferenceData->size;

    delete inferenceData;
    inferenceData = nullptr;
    return 0;
}

int Peer::GrpcClient::Gst::Stop(void)
{
    return command->Send(Command::IdStop);
}

Peer::GrpcClient::Gst::Gst(void)
    : grpcClient(nullptr)
    , nonce(0)
    , requestQueueMutex(PTHREAD_MUTEX_INITIALIZER)
    , command(nullptr)
    , output(nullptr)
    , threadCtx{
        .loop = nullptr,
        .gstSource = nullptr,
        .gstSink = nullptr,
        .cmdTable = {
            Peer::GrpcClient::Gst::Thread::CommandHandlerReady,
            Peer::GrpcClient::Gst::Thread::CommandHandlerPrepare,
            Peer::GrpcClient::Gst::Thread::CommandHandlerInvoke,
            Peer::GrpcClient::Gst::Thread::CommandHandlerStop,
            Peer::GrpcClient::Gst::Thread::CommandHandlerExit,
        },
    }
{
}

Peer::GrpcClient::Gst::~Gst(void)
{
    int ret = pthread_mutex_destroy(&requestQueueMutex);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_mutex_destroy");
    }
}

void *Peer::GrpcClient::Gst::Thread::Main(void *arg)
{
    Peer::GrpcClient::Gst *impls = static_cast<Peer::GrpcClient::Gst *>(arg);

    GMainContext *ctx = g_main_context_new();
    if (ctx == nullptr) {
        ErrPrint("Failed to create a new context");
        return nullptr;
    }
    g_main_context_push_thread_default(ctx);

    impls->threadCtx.loop = g_main_loop_new(ctx, FALSE);
    if (impls->threadCtx.loop == nullptr) {
        ErrPrint("Failed to create a new main loop");
        g_main_context_pop_thread_default(ctx);
        g_main_context_unref(ctx);
        ctx = nullptr;
        return nullptr;
    }

    GSourceFuncs srcs = {
        Peer::GrpcClient::Gst::Thread::CommandPrepare,
        Peer::GrpcClient::Gst::Thread::CommandCheck,
        Peer::GrpcClient::Gst::Thread::CommandHandle,
        nullptr,
    };
    GSource *source = g_source_new(&srcs, sizeof(Thread::EventUserData));
    Thread::EventUserData *ud = reinterpret_cast<Thread::EventUserData *>(source);
    ud->impls = impls;
    ud->fd.fd = impls->threadCtx.command->GetHandle();
    ud->fd.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(source, &ud->fd);
    guint id = g_source_attach(source, ctx);
    g_source_unref(source);
    if (id == 0) {
        g_main_context_pop_thread_default(ctx);
        g_main_context_unref(ctx);
        ctx = nullptr;
        return nullptr;
    }

    if (impls->threadCtx.command->Send(Command::IdReady) < 0) {
        g_source_destroy(source);
        source = nullptr;

        g_main_context_pop_thread_default(ctx);
        g_main_context_unref(ctx);
        ctx = nullptr;
        return nullptr;
    }

    g_main_loop_run(impls->threadCtx.loop);

    g_source_destroy(source);
    source = nullptr;

    g_main_loop_unref(impls->threadCtx.loop);
    impls->threadCtx.loop = nullptr;

    g_main_context_pop_thread_default(ctx);
    g_main_context_unref(ctx);
    ctx = nullptr;
    return nullptr;
}

void Peer::GrpcClient::Gst::SetSecret(std::string &secret)
{
    secretKey = secret;
}

void Peer::GrpcClient::Gst::SetNonce(unsigned long _nonce)
{
    nonce = _nonce;
}

unsigned long Peer::GrpcClient::Gst::GetNonce(void) const
{
    return nonce;
}
