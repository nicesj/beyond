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

#include "peer_grpc_client_gst_sink.h"
#include "peer_event_object.h"
#include "peer_model.h"

#include <cstdio>
#include <cerrno>

#include <pthread.h>

#include <glib.h>
#include <gst/gst.h>

void Peer::GrpcClient::Gst::Sink::BusHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
    Peer::GrpcClient::Gst::Sink *impls = static_cast<Peer::GrpcClient::Gst::Sink *>(user_data);
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
    Peer *peer = impls->gstClient->grpcClient->peer;

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

void Peer::GrpcClient::Gst::Sink::NewDataHandler(GstElement *element, GstBuffer *buffer, gpointer user_data)
{
    Peer::GrpcClient::Gst::Sink *impls = static_cast<Peer::GrpcClient::Gst::Sink *>(user_data);
    if (impls == nullptr) {
        assert(!"impls is nullptr");
        return;
    }
    Peer::GrpcClient::Gst *gstClient = impls->gstClient;
    Peer *peer = gstClient->grpcClient->peer;

    GstPad *sink_pad = gst_element_get_static_pad(element, SINKX_NAME);
    if (sink_pad != nullptr) {
        // negotiated
        GstCaps *caps = gst_pad_get_current_caps(sink_pad);
        if (caps != nullptr) {
            Peer::GrpcClient::Gst::Sink::ParseCaps(caps);
            gst_caps_unref(caps);
        }

        caps = gst_pad_get_pad_template_caps(sink_pad);
        if (caps != nullptr) {
            Peer::GrpcClient::Gst::Sink::ParseCaps(caps);
            gst_caps_unref(caps);
        }

        gst_object_unref(sink_pad);
    }

    const beyond_tensor_info *tensorInfo = nullptr;
    int size = 0;
    int ret = gstClient->grpcClient->GetOutputTensorInfo(tensorInfo, size);
    if (ret < 0) {
        ErrPrint("Unable to get the model output shape");
        (void)peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR);
        return;
    } else if (tensorInfo == nullptr || size == 0) {
        ErrPrint("Output tensor info is not valid");
    }

    Peer::GrpcClient::Gst::InvokeData *inferenceData = nullptr;
    guint num_mems = gst_buffer_n_memory(buffer);
    if (num_mems != static_cast<guint>(size)) {
        ErrPrint("Output size mismatch %u <> %d", num_mems, size);
        (void)peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR);
        return;
    }

    ret = pthread_mutex_lock(&gstClient->requestQueueMutex);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_mutex_lock");
    }

    if (gstClient->requestQueue.empty() == false) {
        inferenceData = gstClient->requestQueue.front();
        gstClient->requestQueue.pop();
    }

    ret = pthread_mutex_unlock(&gstClient->requestQueueMutex);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_mutex_unlock");
    }

    if (inferenceData == nullptr) {
        ErrPrint("Unable to get the inferenceData from the queue");
        (void)peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR);
        return;
    }

    // Update the tensor and its count
    inferenceData->size = num_mems;

    beyond_tensor *tensor = static_cast<beyond_tensor *>(calloc(num_mems, sizeof(beyond_tensor)));
    inferenceData->tensor = tensor;
    if (inferenceData->tensor == nullptr) {
        ErrPrintCode(errno, "calloc");
        if (peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR, const_cast<void *>(inferenceData->context)) < 0) {
            ErrPrint("Unable to publish an event");
        }
        delete inferenceData;
        inferenceData = nullptr;
        return;
    }

    guint i;
    for (i = 0; i < num_mems; i++) {
        GstMapInfo info;
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (mem == nullptr) {
            ErrPrint("Failed to peek a memory");
            break;
        }

        if (gst_memory_map(mem, &info, GST_MAP_READ)) {
            tensor[i].size = info.size;
            tensor[i].data = malloc(tensor[i].size);
            if (tensor[i].data == nullptr) {
                ErrPrintCode(errno, "malloc");
                break;
            }
            memcpy(tensor[i].data, info.data, info.size);

            if (tensorInfo != nullptr) {
                tensor[i].type = tensorInfo[i].type;
            } else {
                DbgPrint("Warning: Tensor type is not determined");
                tensor[i].type = BEYOND_TENSOR_TYPE_UNSUPPORTED;
            }

            gst_memory_unmap(mem, &info);
        }
    }

    if (i != num_mems) {
        free(tensor[i].data);
        tensor[i].data = nullptr;
        while (--i > 0) {
            free(tensor[i].data);
            tensor[i].data = nullptr;
        }

        free(tensor);
        tensor = nullptr;
        inferenceData->tensor = nullptr;

        if (peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR, const_cast<void *>(inferenceData->context)) < 0) {
            ErrPrint("Unable to publish an event");
        }

        delete inferenceData;
        inferenceData = nullptr;
        return;
    }

    // NOTE:
    // Separate userContext from the inferenceData.
    void *context = const_cast<void *>(inferenceData->context);
    inferenceData->context = nullptr;

    ret = gstClient->threadCtx.output->Send(Peer::GrpcClient::Gst::Command::IdInvoke, static_cast<void *>(inferenceData));
    if (ret < 0) {
        if (peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR, context) < 0) {
            ErrPrint("Unable to publish the event");
            // Go ahead, there is nothing to do for this anymore.
        }

        for (i = 0; i < num_mems; i++) {
            free(tensor[i].data);
            tensor[i].data = nullptr;
        }
        free(tensor);
        tensor = nullptr;
        inferenceData->tensor = nullptr;
        delete inferenceData;
        inferenceData = nullptr;
    } else {
        // After sending the inferenceData,
        // Do not access the inferenceData afterwards.
        if (peer->eventObject->PublishEventData(beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_SUCCESS, context) < 0) {
            ErrPrint("Unable to publish the event");
            // Go ahead, there is nothing to do for this anymore.
        }
    }
}

void Peer::GrpcClient::Gst::Sink::ParseCaps(GstCaps *caps)
{
    if (caps == nullptr) {
        ErrPrint("Caps is null");
        return;
    }

    guint caps_size, i;

    caps_size = gst_caps_get_size(caps);

    for (i = 0; i < caps_size; i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        gchar *str = gst_structure_to_string(structure);
        g_free(str);
    }
}

Peer::GrpcClient::Gst::Sink *Peer::GrpcClient::Gst::Sink::Create(Peer::GrpcClient::Gst *gstClient, const gchar *pipelineDesc)
{
    Peer::GrpcClient::Gst::Sink *impls;
    try {
        impls = new Peer::GrpcClient::Gst::Sink();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    GError *error = nullptr;
    DbgPrint("Pipeline [%s]", pipelineDesc);
    impls->pipeline = gst_parse_launch(pipelineDesc, &error);
    if (error != nullptr) {
        ErrPrint("%s", error->message);
        g_error_free(error);

        delete impls;
        impls = nullptr;
        return nullptr;
    }

    impls->bus = gst_element_get_bus(impls->pipeline);
    if (impls->bus == nullptr) {
        ErrPrint("Failed to get a bus");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    impls->element = gst_bin_get_by_name(GST_BIN(impls->pipeline), SINKX_NAME);
    if (impls->element == nullptr) {
        ErrPrint("Failed to get the " SINKX_NAME " element");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    GSource *source = gst_bus_create_watch(impls->bus);
    if (source == nullptr) {
        ErrPrint("Failed to create a watch for bus");
        delete impls;
        impls = nullptr;
        return nullptr;
    }

    // G_SOURCE_FUNC = since 2.58
    g_source_set_callback(source,
                          (GSourceFunc)Peer::GrpcClient::Gst::Sink::BusHandler,
                          static_cast<gpointer>(impls), nullptr);
    g_source_attach(source, g_main_context_get_thread_default());
    g_source_unref(source);
    source = nullptr;

    g_object_set(impls->element, "emit-signal", static_cast<gboolean>(TRUE), nullptr);
    g_signal_connect(impls->element,
                     "new-data",
                     G_CALLBACK(Peer::GrpcClient::Gst::Sink::NewDataHandler),
                     static_cast<gpointer>(impls));

    impls->gstClient = gstClient;

    GstStateChangeReturn scret = gst_element_set_state(impls->pipeline, GST_STATE_PLAYING);
    if (scret == GST_STATE_CHANGE_FAILURE) {
        ErrPrint("set PLAYING failed");
    }

    return impls;
}

void Peer::GrpcClient::Gst::Sink::Destroy(void)
{
    delete this;
}

int Peer::GrpcClient::Gst::Sink::Stop(void)
{
    GstStateChangeReturn scret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (scret == GST_STATE_CHANGE_FAILURE) {
        ErrPrint("set PAUSED failed");
        return -EFAULT;
    }
    return 0;
}

Peer::GrpcClient::Gst::Sink::Sink(void)
    : gstClient(nullptr)
    , pipeline(nullptr)
    , element(nullptr)
    , bus(nullptr)
{
}

Peer::GrpcClient::Gst::Sink::~Sink(void)
{
    if (element != nullptr) {
        gst_object_unref(element);
        element = nullptr;
    }

    if (bus != nullptr) {
        gst_object_unref(bus);
        bus = nullptr;
    }

    if (pipeline != nullptr) {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}
