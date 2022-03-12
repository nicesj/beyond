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

#include "peer_grpc_server_gst.h"
#include "peer_event_object.h"
#include "peer_model.h"

#include <cstdio>
#include <cerrno>

#include <exception>
#include <memory>
#include <string>

#include <pthread.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>

#include <glib.h>
#include <gst/gst.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"

#define RANK_MAX_SIZE 3
#define DIMS_PARSER(str, size, values) (Peer::GrpcServer::Gst::dimsParsers[((size)-1)](str, values))

Peer::GrpcServer::Gst::DimsParser_t Peer::GrpcServer::Gst::dimsParsers[] = {
    Peer::GrpcServer::Gst::DimsParser1,
    Peer::GrpcServer::Gst::DimsParser2,
    Peer::GrpcServer::Gst::DimsParser3,
    Peer::GrpcServer::Gst::DimsParser4
};

// NOTE:
// NHWC (tflite)
// nnstreamer reverses the dimension for the CAPS negotiation its internal uses.
int Peer::GrpcServer::Gst::DimsParser1(const char *str, int *values)
{
    if (sscanf(str, "%d", values + 1) != 1) {
        return -EINVAL;
    }

    values[0] = 1;
    return values[1];
}

int Peer::GrpcServer::Gst::DimsParser2(const char *str, int *values)
{
    if (sscanf(str, "%d:%d", values + 2, values + 1) != 2) {
        return -EINVAL;
    }

    values[0] = 1;
    return values[1] * values[2];
}

int Peer::GrpcServer::Gst::DimsParser3(const char *str, int *values)
{
    if (sscanf(str, "%d:%d:%d", values + 3, values + 2, values + 1) != 3) {
        return -EINVAL;
    }

    values[0] = 1;
    return values[1] * values[2] * values[3];
}

int Peer::GrpcServer::Gst::DimsParser4(const char *str, int *values)
{
    if (sscanf(str, "%d:%d:%d:%d", values + 3, values + 2, values + 1, values + 0) != 4) {
        return -EINVAL;
    }

    return values[0] * values[1] * values[2] * values[3];
}

std::string Peer::GrpcServer::Gst::TensorInfoToProperties(const char *type, const beyond_tensor_info *info, int size)
{
    std::string props;

    if (info == nullptr) {
        return props;
    }

    props = std::string(" ") + std::string(type) + std::string("type=");
    for (int i = 0; i < size; i++) {
        if (i > 0) {
            props += ",";
        }
        props += beyond::Inference::TensorTypeToString(info[i].type);
    }

    props += std::string(" ") + std::string(type) + std::string("=");
    for (int i = 0; i < size; i++) {
        if (i > 0) {
            props += ",";
        }

        switch (info[i].dims->size) {
        case 4:
            props += std::to_string(info[i].dims->data[3]) + ":";
            FALLTHROUGH;
        case 3:
            props += std::to_string(info[i].dims->data[2]) + ":";
            FALLTHROUGH;
        case 2:
            props += std::to_string(info[i].dims->data[1]) + ":";
            FALLTHROUGH;
        case 1:
            props += std::to_string(info[i].dims->data[0]);
            FALLTHROUGH;
        default:
            switch (info[i].dims->size) {
            case 1:
                props += ":1";
                FALLTHROUGH;
            case 2:
                props += ":1";
                FALLTHROUGH;
            case 3:
                props += ":1";
                FALLTHROUGH;
            case 4:
                FALLTHROUGH;
            default:
                break;
            }
            break;
        }
    }

    return props;
}

int Peer::GrpcServer::Gst::ExtractTensorInfo(GstElement *element, const char *type, beyond_tensor_info *&info, int &size)
{
    gchar *prop = nullptr;

    int i = 0;
    bool done = false;
    std::vector<int> rankVector;
    char attribute[24];
    int ret;

    if (snprintf(attribute, sizeof(attribute), "%sranks", type) < 0) {
        ret = -errno;
        ErrPrintCode(errno, "snprintf");
        return ret;
    }

    g_object_get(G_OBJECT(element), attribute, &prop, nullptr);
    if (prop == nullptr || strlen(prop) == 0) {
        ErrPrint("Unable to get the property %s, fallback to use maximum size", attribute);
        // NOTE:
        // Fallback to use maximum value.
        // Old version of tensor-filter does not provide inputrank attribute.
        // In order to get it over, fallback to use the maximum rank size if it does not exist.
    } else {
        int rank = 0;
        int mult = 1;
        for (i = 0; done == false; i++) {
            switch (prop[i]) {
            case '0' ... '9':
                rank += mult * (prop[i] - '0');
                mult *= 10;
                break;
            case '\0':
                done = true;
                FALLTHROUGH;
            case ',':
                rankVector.push_back(rank);
                rank = 0;
                mult = 1;
                break;
            default:
                done = true;
                break;
            }
        }
    }
    g_free(prop);
    prop = nullptr;

    done = false;
    int offset = 0;
    snprintf(attribute, sizeof(attribute), "%stype", type);
    prop = nullptr;
    g_object_get(G_OBJECT(element), attribute, &prop, nullptr);
    if (prop == nullptr || strlen(prop) == 0) {
        ErrPrint("Unable to get the property %s", attribute);
        return -EFAULT;
    }
    std::string typeString(prop);
    std::vector<beyond_tensor_type> typeVector;
    for (i = 0; done == false; i++) {
        switch (prop[i]) {
        case '\0':
            done = true;
            FALLTHROUGH;
        case ',':
            std::string tmp = typeString.substr(offset, i - offset);
            typeVector.push_back(beyond::Inference::TensorTypeStringToType(tmp.c_str()));
            offset = i + 1;
            break;
        }
    }
    g_free(prop);
    prop = nullptr;

    done = false;
    offset = 0;
    prop = nullptr;
    g_object_get(G_OBJECT(element), type, &prop, nullptr);
    if (prop == nullptr || strlen(prop) == 0) {
        ErrPrint("Unable to get the property %s", type);
        return -EFAULT;
    }
    std::string dimsString(prop);
    std::vector<std::string> dimsVector;
    for (i = 0; done == false; i++) {
        switch (prop[i]) {
        case '\0':
            done = true;
            FALLTHROUGH;
        case ',':
            std::string tmp = dimsString.substr(offset, i - offset);
            dimsVector.push_back(tmp);
            offset = i + 1;
            break;
        }
    }
    g_free(prop);
    prop = nullptr;

    beyond_tensor_info *tensorInfo = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * typeVector.size()));
    if (tensorInfo == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    for (i = 0; i < static_cast<int>(typeVector.size()); i++) {
        // NOTE:
        // For the old version of tensor-filter, use the maximum rank value if it does not provided
        int rank = 3;
        if (rankVector.size() > 0) {
            rank = rankVector[i];
        }

        // NOTE:
        // NNStreamer always allocate 4 dimensions (its maximum size)
        // and fills unused part with 1
        beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * (rank + 1)));
        if (dims == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "malloc");
            Peer::Model::FreeTensorInfo(tensorInfo, i);
            return ret;
        }

        dims->size = rank + 1;

        ret = DIMS_PARSER(dimsVector[i].c_str(), rank, dims->data);
        if (ret < 0) {
            ErrPrint("Failed to parse the dims");
            free(dims);
            dims = nullptr;
            Peer::Model::FreeTensorInfo(tensorInfo, i);
            return ret;
        }

        tensorInfo[i].size = beyond::Inference::TensorTypeToSize(typeVector[i]) * ret;
        tensorInfo[i].type = typeVector[i];
        tensorInfo[i].name = nullptr;
        tensorInfo[i].dims = dims;
    }

    info = tensorInfo;
    size = typeVector.size();
    return 0;
}

void Peer::GrpcServer::Gst::Thread::BusHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
    Peer::GrpcServer::Gst *impls = static_cast<Peer::GrpcServer::Gst *>(user_data);
    if (impls == nullptr) {
        assert(!"Peer cannot be nullptr");
        ErrPrint("Peer cannot be nullptr");
        return;
    }

    if (message == nullptr) {
        assert(!"message cannot be nullptr");
        ErrPrint("message cannot be nullptr");
        return;
    }

    gchar *debug = nullptr;
    GError *error = nullptr;
    Peer *peer = impls->grpc->peer;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error, &debug);
        ErrPrint("%s: %s", gst_message_type_get_name(GST_MESSAGE_TYPE(message)), error->message);
        g_error_free(error);
        error = nullptr;

        g_free(static_cast<gpointer>(debug));
        debug = nullptr;

        // TODO:
        // Need to get the current inference data
        if (peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR) < 0) {
            ErrPrint("Unable to publish event");
        }
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &error, &debug);
        DbgPrint("%s: %s", gst_message_type_get_name(GST_MESSAGE_TYPE(message)), error->message);
        g_error_free(error);
        error = nullptr;

        g_free(static_cast<gpointer>(debug));
        debug = nullptr;

        // TODO:
        // What should we do?
        break;
    case GST_MESSAGE_STATE_CHANGED: {
        GstState newstate, oldstate;
        gst_message_parse_state_changed(message, &oldstate, &newstate, nullptr);

        if (oldstate == GST_STATE_PLAYING && newstate == GST_STATE_PAUSED) {
            int ret = peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_STOPPED);
            if (ret < 0) {
                DbgPrint("Unable to publish stopped event: %d", ret);
            }
        }
        break;
    }
    default:
        DbgPrint("%s", gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
        break;
    }
}

int Peer::GrpcServer::Gst::Thread::CommandHandlerReady(Peer::GrpcServer::Gst *impls, void *data)
{
    return 0;
}

int Peer::GrpcServer::Gst::Thread::CommandHandlerPrepare(Peer::GrpcServer::Gst *impls, void *data)
{
    PrepareData *prepareData = static_cast<PrepareData *>(data);
    assert(prepareData != nullptr);
    GstElement *serverSource = nullptr;
    GstElement *serverSink = nullptr;
    GstElement *tensorFilter = nullptr;

    int ret = 0;

    do {
        GError *error = nullptr;

        DbgPrint("Pipeline[%s]", prepareData->pipelineDescription);
        impls->threadCtx.pipeline = gst_parse_launch(prepareData->pipelineDescription, &error);
        if (error != nullptr) {
            ErrPrint("%s", error->message);
            g_error_free(error);
            ret = -EFAULT;
            break;
        }

        if (impls->threadCtx.pipeline == nullptr) {
            ErrPrint("Failed to construct the gst pipeline: %s", prepareData->pipelineDescription);
            ret = -EFAULT;
            break;
        }

        impls->threadCtx.bus = gst_element_get_bus(impls->threadCtx.pipeline);
        if (impls->threadCtx.bus == nullptr) {
            ErrPrint("Failed to get a bus");
            g_object_unref(impls->threadCtx.pipeline);
            impls->threadCtx.pipeline = nullptr;
            ret = -EFAULT;
            break;
        }

        GSource *source = gst_bus_create_watch(impls->threadCtx.bus);
        if (source == nullptr) {
            ErrPrint("Failed to create a watch for the bus");
            g_object_unref(impls->threadCtx.pipeline);
            impls->threadCtx.pipeline = nullptr;
            g_object_unref(impls->threadCtx.bus);
            impls->threadCtx.bus = nullptr;
            ret = -EFAULT;
            break;
        }

        serverSource = gst_bin_get_by_name(GST_BIN(impls->threadCtx.pipeline), "serverSource");
        if (serverSource == nullptr) {
            ErrPrint("Failed to get serverSource element");
            g_object_unref(source);
            source = nullptr;
            g_object_unref(impls->threadCtx.pipeline);
            impls->threadCtx.pipeline = nullptr;
            g_object_unref(impls->threadCtx.bus);
            impls->threadCtx.bus = nullptr;
            ret = -EFAULT;
            break;
        }

        serverSink = gst_bin_get_by_name(GST_BIN(impls->threadCtx.pipeline), "serverSink");
        if (serverSink == nullptr) {
            ErrPrint("Failed to get serverSink element");
            g_object_unref(source);
            source = nullptr;
            g_object_unref(impls->threadCtx.pipeline);
            impls->threadCtx.pipeline = nullptr;
            g_object_unref(impls->threadCtx.bus);
            impls->threadCtx.bus = nullptr;
            ret = -EFAULT;
            break;
        }

        tensorFilter = gst_bin_get_by_name(GST_BIN(impls->threadCtx.pipeline), "tensorFilter");
        if (tensorFilter == nullptr) {
            ErrPrint("Failed to get serverSink element");
            g_object_unref(source);
            source = nullptr;
            g_object_unref(impls->threadCtx.pipeline);
            impls->threadCtx.pipeline = nullptr;
            g_object_unref(impls->threadCtx.bus);
            impls->threadCtx.bus = nullptr;
            ret = -EFAULT;
            break;
        }

        g_source_set_callback(source,
                              (GSourceFunc)Peer::GrpcServer::Gst::Thread::BusHandler,
                              static_cast<gpointer>(impls), nullptr);
        g_source_attach(source, g_main_context_get_thread_default());
        g_source_unref(source);
        source = nullptr;
    } while (0);

    g_free(prepareData->pipelineDescription);
    prepareData->pipelineDescription = nullptr;

    if (ret == 0) {
        GstStateChangeReturn scret = gst_element_set_state(impls->threadCtx.pipeline, GST_STATE_PLAYING);
        if (scret == GST_STATE_CHANGE_FAILURE) {
            ErrPrint("set PLAYING failed");
            ret = -EFAULT;
        } else {
            gint port = -1;

            if (impls->preprocessing.empty() == true) {
                // NOTE: we are using TCP when the input is not configured
                // tcpserversrc property name
                g_object_get(G_OBJECT(serverSource), "current-port", &port, nullptr);
                prepareData->port.request = port;
            } else {
                // udpsrc property name
                g_object_get(G_OBJECT(serverSource), "port", &port, nullptr);
                prepareData->port.request = port;
            }

            port = -1;
            // tcpservrsink property name
            g_object_get(G_OBJECT(serverSink), "current-port", &port, nullptr);
            prepareData->port.response = port;

            const beyond_tensor_info *info = nullptr;
            int size = 0;
            int tensorinfo_status;

            tensorinfo_status = impls->model->GetInputTensorInfo(info, size);
            if (tensorinfo_status != 0 || info == nullptr || size <= 0) {
                beyond_tensor_info *_info;
                if (Peer::GrpcServer::Gst::ExtractTensorInfo(tensorFilter, "input", _info, size) == 0) {
                    impls->model->SetInputTensorInfo(_info, size);
                }
            }

            info = nullptr;
            size = 0;
            tensorinfo_status = impls->model->GetOutputTensorInfo(info, size);
            if (tensorinfo_status != 0 || info == nullptr || size <= 0) {
                beyond_tensor_info *_info;
                if (Peer::GrpcServer::Gst::ExtractTensorInfo(tensorFilter, "output", _info, size) == 0) {
                    impls->model->SetOutputTensorInfo(_info, size);
                }
            }
        }
    }

    prepareData->port.status = ret;

    ret = impls->threadCtx.command->Send(Command::IdPrepare, static_cast<void *>(prepareData));
    if (ret < 0) {
        ErrPrint("Failed to send a command");
    }

    return ret;
}

int Peer::GrpcServer::Gst::Thread::CommandHandlerStop(Peer::GrpcServer::Gst *impls, void *data)
{
    // TODO:
    // Stop the pipeline
    return 0;
}

int Peer::GrpcServer::Gst::Thread::CommandHandlerExit(Peer::GrpcServer::Gst *impls, void *data)
{
    g_main_loop_quit(impls->threadCtx.loop);
    return 0;
}

gboolean Peer::GrpcServer::Gst::Thread::CommandPrepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

gboolean Peer::GrpcServer::Gst::Thread::CommandCheck(GSource *source)
{
    Peer::GrpcServer::Gst::Thread::EventUserData *ud = reinterpret_cast<Peer::GrpcServer::Gst::Thread::EventUserData *>(source);

    if ((ud->fd.revents & G_IO_IN) == G_IO_IN) {
        return TRUE;
    }

    if ((ud->fd.revents & G_IO_ERR) == G_IO_ERR) {
        ErrPrint("Error event!");
        return TRUE;
    }

    return FALSE;
}

gboolean Peer::GrpcServer::Gst::Thread::CommandHandle(GSource *source, GSourceFunc callback, gpointer user_data)
{
    Peer::GrpcServer::Gst::Thread::EventUserData *ud = reinterpret_cast<Peer::GrpcServer::Gst::Thread::EventUserData *>(source);
    Peer::GrpcServer::Gst *impls = ud->impls;

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

std::shared_ptr<Peer::Model> Peer::GrpcServer::Gst::GetModel(void) const
{
    return model;
}

Peer::GrpcServer::Gst *Peer::GrpcServer::Gst::Create(std::string &peerId, Peer::GrpcServer *grpcServer)
{
    GrpcServer::Gst *impls;
    int spfd[2];

    try {
        impls = new GrpcServer::Gst();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    impls->grpc = grpcServer;
    impls->peerId = peerId;
    impls->model = std::make_shared<Peer::Model>();

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, spfd) < 0) {
        ErrPrintCode(errno, "socketpair");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    impls->command = std::make_unique<beyond::CommandObject>(spfd[0]);
    impls->threadCtx.command = std::make_unique<beyond::CommandObject>(spfd[1]);

    int status = pthread_create(&impls->threadId, nullptr, Peer::GrpcServer::Gst::Thread::Main, static_cast<void *>(impls));
    if (status != 0) {
        ErrPrintCode(status, "pthread_create");
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

void Peer::GrpcServer::Gst::Destroy(void)
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

    delete this;
}

int Peer::GrpcServer::Gst::Configure(const beyond_plugin_peer_nn_config::server_description *server)
{
    beyond_input_type input_type = static_cast<beyond_input_type>(server->input_type);
    if (input_type == BEYOND_INPUT_TYPE_IMAGE) {
        rtpConfig = {
            .depayloader = "rtpjpegdepay",
            .encodingName = "JPEG",
            .payload = 26,
        };
    } else if (input_type == BEYOND_INPUT_TYPE_VIDEO) {
        rtpConfig = {
            .depayloader = "rtpvp8depay",
            .encodingName = "VP8",
            .payload = 96,
        };
    }

    if (server->preprocessing != nullptr) {
        preprocessing = std::string(server->preprocessing);
    }

    if (server->postprocessing != nullptr) {
        postprocessing = std::string(server->postprocessing);
    }

    if (server->framework != nullptr && strlen(server->framework) > 0) {
        framework = std::string(server->framework);
    } else {
        framework = std::string("tensorflow-lite");
    }

    if (server->accel != nullptr && strlen(server->accel) > 0) {
        accel = std::string("accelerator=true:") + std::string(server->accel);
    } else {
        accel = std::string("accelerator=true:gpu");
    }

    return 0;
}

int Peer::GrpcServer::Gst::Prepare(int &reqPort, int &resPort)
{
    const char *modelPath = model->GetModelPath();
    if (modelPath == nullptr) {
        ErrPrint("Invalid model, it seems that there is no model loaded");
        return -EILSEQ;
    }

    PrepareData *prepareData;

    try {
        prepareData = new PrepareData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    std::string inputProps;
    std::string outputProps;
    const beyond_tensor_info *info;
    int size;
    int ret = model->GetInputTensorInfo(info, size);
    if (ret == 0 && info != nullptr && size > 0) {
        inputProps = Peer::GrpcServer::Gst::TensorInfoToProperties("input", info, size);
    }

    ret = model->GetOutputTensorInfo(info, size);
    if (ret == 0 && info != nullptr && size > 0) {
        outputProps = Peer::GrpcServer::Gst::TensorInfoToProperties("output", info, size);
    }

    gchar *prePipeline = nullptr;
    if (preprocessing.empty() == true) {
        // NOTE
        // When input_config is not set, use tcp + gdp payloder
        // TODO: need to handle secured channel maybe by using RTSP over tcp + tls

        // Otherwise, need to find general format of rtp payloder for raw tensor
        prePipeline = g_strdup_printf("tcpserversrc name=serverSource host=0.0.0.0 port=0 ! gdpdepay");
    } else {
        // TODO: we may get media type from the Configure and than,
        // need to break down Rx decoder and its payload
        if (secretKey.empty() == false) {
            // Now, we have to build the srtp pipeline using the secretKey
            DbgPrint("GST SecretKey found: %s", secretKey.c_str());
            std::string _secretKey;
            const char *ptr = secretKey.c_str();
            for (int i = 0; i < MASTER_KEY_SIZE && ptr[i] != '\0'; ++i) {
                char hex[3];
                snprintf(hex, sizeof(hex), "%.2X", ptr[i]);
                _secretKey += std::string(hex);
            }
            prePipeline = g_strdup_printf(
                "udpsrc name=serverSource address=0.0.0.0 port=0 "
                "caps=\"application/x-srtp, encoding-name=%s, payload=%d, ssrc=(uint)%s, roc=(uint)0, "
                "srtp-key=(buffer)%s, srtp-cipher=(string)aes-128-icm, srtp-auth=(string)hmac-sha1-80, "
                "srtcp-cipher=(string)aes-128-icm, srtcp-auth=(string)hmac-sha1-80\" ! srtpdec ! %s",
                rtpConfig.encodingName.c_str(),
                rtpConfig.payload,
                peerId.c_str(), _secretKey.c_str(),
                rtpConfig.depayloader.c_str());
        } else {
            prePipeline = g_strdup_printf(
                "udpsrc name=serverSource address=0.0.0.0 port=0 "
                "caps=\"application/x-rtp, encoding-name=%s, payload=%d\" ! "
                "%s",
                rtpConfig.encodingName.c_str(),
                rtpConfig.payload,
                rtpConfig.depayloader.c_str());
        }
    }

    if (prePipeline == nullptr) {
        ErrPrint("Failed to build a pipeline description");
        delete prepareData;
        prepareData = nullptr;
        return -ENOMEM;
    }

    prepareData->pipelineDescription = g_strdup_printf(
        "%s ! %s "
        "tensor_filter name=tensorFilter framework=%s model=%s %s %s %s ! tensor_decoder mode=flatbuf ! gdppay ! "
        "tcpserversink name=serverSink host=0.0.0.0 port=0",
        prePipeline,
        preprocessing.c_str(), framework.c_str(), modelPath, accel.c_str(), inputProps.c_str(), outputProps.c_str());
    g_free(prePipeline);
    prePipeline = nullptr;
    if (prepareData->pipelineDescription == nullptr) {
        ErrPrint("Failed to build a pipeline description");
        delete prepareData;
        prepareData = nullptr;
        return -ENOMEM;
    }

    ret = command->Send(Command::IdPrepare, static_cast<void *>(prepareData));
    if (ret < 0) {
        g_free(prepareData->pipelineDescription);
        prepareData->pipelineDescription = nullptr;
        delete prepareData;
        prepareData = nullptr;
        return ret;
    }

    int cmdId = Command::IdLast;
    void *data = nullptr;
    ret = command->Recv(cmdId, data);
    if (ret < 0 || cmdId != Command::IdPrepare) {
        delete prepareData;
        prepareData = nullptr;
        return ret;
    }

    prepareData = static_cast<PrepareData *>(data);
    reqPort = prepareData->port.request;
    resPort = prepareData->port.response;
    ret = prepareData->port.status;

    delete prepareData;
    prepareData = nullptr;

    return ret;
}

int Peer::GrpcServer::Gst::Stop(void)
{
    return command->Send(Command::IdStop);
}

void Peer::GrpcServer::Gst::SetSecret(std::string &secret)
{
    secretKey = secret;
}

Peer::GrpcServer::Gst::Gst(void)
    : grpc(nullptr)
    , framework("tensorflow-lite")
    , threadCtx{
        .loop = nullptr,
        .pipeline = nullptr,
        .bus = nullptr,
        .cmdTable = {
            Peer::GrpcServer::Gst::Thread::CommandHandlerReady,
            Peer::GrpcServer::Gst::Thread::CommandHandlerPrepare,
            Peer::GrpcServer::Gst::Thread::CommandHandlerStop,
            Peer::GrpcServer::Gst::Thread::CommandHandlerExit,
        },
    }
{
}

Peer::GrpcServer::Gst::~Gst(void)
{
}

void *Peer::GrpcServer::Gst::Thread::Main(void *arg)
{
    Peer::GrpcServer::Gst *impls = static_cast<Peer::GrpcServer::Gst *>(arg);

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
        Peer::GrpcServer::Gst::Thread::CommandPrepare,
        Peer::GrpcServer::Gst::Thread::CommandCheck,
        Peer::GrpcServer::Gst::Thread::CommandHandle,
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

void Peer::GrpcServer::Gst::SetNonce(unsigned long _nonce)
{
    nonce = _nonce;
}

unsigned long Peer::GrpcServer::Gst::GetNonce(void) const
{
    return nonce;
}
