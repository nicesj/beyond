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

#include "peer_grpc_client_gst_source.h"
#include "peer_event_object.h"

#include <cstdio>
#include <cerrno>

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

void Peer::GrpcClient::Gst::Source::BusHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
    Peer::GrpcClient::Gst::Source *impls = static_cast<Peer::GrpcClient::Gst::Source *>(user_data);
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
        gst_message_parse_state_changed(message, &oldstate, &newstate, NULL);
        break;
    }
    default:
        DbgPrint("%s", gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
        break;
    }
}

void Peer::GrpcClient::Gst::Source::StartFeedHandler(GstElement *pipeline, guint size, gpointer user_data)
{
    Peer::GrpcClient::Gst::Source *impls = static_cast<Peer::GrpcClient::Gst::Source *>(user_data);
    if (impls == nullptr) {
        assert(impls != nullptr && "source cannot be nullptr");
        ErrPrint("source cannot be nllptr");
    }
}

void Peer::GrpcClient::Gst::Source::StopFeedHandler(GstElement *pipeline, gpointer user_data)
{
    Peer::GrpcClient::Gst::Source *impls = static_cast<Peer::GrpcClient::Gst::Source *>(user_data);
    if (impls == nullptr) {
        assert(impls != nullptr && "source cannot be nullptr");
        ErrPrint("source cannot be nullptr");
    }
}

Peer::GrpcClient::Gst::Source *Peer::GrpcClient::Gst::Source::Create(Peer::GrpcClient::Gst *gstClient, const gchar *pipelineDesc)
{
    Peer::GrpcClient::Gst::Source *impls;

    try {
        impls = new Peer::GrpcClient::Gst::Source();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    GError *error = nullptr;
    DbgPrint("Pipeline [%s]", pipelineDesc);
    impls->pipeline = gst_parse_launch(pipelineDesc, &error);
    if (error) {
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

    impls->element = gst_bin_get_by_name(GST_BIN(impls->pipeline), SRCX_NAME);
    if (impls->element == nullptr) {
        ErrPrint("Failed to get " SRCX_NAME " element");
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
                          (GSourceFunc)Peer::GrpcClient::Gst::Source::BusHandler,
                          static_cast<gpointer>(impls), nullptr);
    g_source_attach(source, g_main_context_get_thread_default());
    g_source_unref(source);
    source = nullptr;

    g_signal_connect(impls->element,
                     "need-data",
                     G_CALLBACK(Peer::GrpcClient::Gst::Source::StartFeedHandler),
                     static_cast<gpointer>(impls));

    g_signal_connect(impls->element,
                     "enough-data",
                     G_CALLBACK(Peer::GrpcClient::Gst::Source::StopFeedHandler),
                     static_cast<gpointer>(impls));

    impls->gstClient = gstClient;

    GstStateChangeReturn scret = gst_element_set_state(impls->pipeline, GST_STATE_PLAYING);
    if (scret == GST_STATE_CHANGE_FAILURE) {
        ErrPrint("set PLAYING failed");
    }

    return impls;
}

int Peer::GrpcClient::Gst::Source::Invoke(Peer::GrpcClient::Gst::InvokeData *invokeData)
{
    int ret = 0;

    do {
        // CHECKME:
        // Pipeline preroll must be done first
        // Otherwise the first input tensor is going to be used for prerolling the pipeline!
        GstBuffer *buffer = gst_buffer_new();
        if (buffer == nullptr) {
            ErrPrint("Unable to create a gst buffer");
            ret = -ENOMEM;
            break;
        }

        for (int i = 0; i < invokeData->size; i++) {
            gsize mem_size = invokeData->tensor[i].size;
            gpointer mem_data = invokeData->tensor[i].data;

            GstMemory *mem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY, mem_data, mem_size, 0, mem_size, mem_data, nullptr);
            if (mem == nullptr) {
                ErrPrint("Failed to wrap the gst memory");
                gst_buffer_unref(buffer);
                ret = -EFAULT;
                break;
            }

            gst_buffer_append_memory(buffer, mem);
        }

        gst_app_src_push_buffer(GST_APP_SRC(element), buffer);
    } while (0);

    if (ret == 0) {
        int status = pthread_mutex_lock(&gstClient->requestQueueMutex);
        if (status != 0) {
            ErrPrintCode(status, "pthread_mutex_lock");
        }

        // TODO:
        // This requestQueue must be sync'd to the lossy queue element of the pipeline
        // If there is a dropped tensor, the requestQueue also drops the related invokeData.
        gstClient->requestQueue.push(invokeData);

        status = pthread_mutex_unlock(&gstClient->requestQueueMutex);
        if (status != 0) {
            ErrPrintCode(status, "pthread_mutex_unlock");
        }
    } else if (ret < 0) {
        delete invokeData;
        invokeData = nullptr;
    }

    return ret;
}

void Peer::GrpcClient::Gst::Source::Destroy(void)
{
    delete this;
}

int Peer::GrpcClient::Gst::Source::Stop(void)
{
    GstStateChangeReturn scret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (scret == GST_STATE_CHANGE_FAILURE) {
        ErrPrint("set PAUSED failed");
        return -EFAULT;
    }
    return 0;
}

Peer::GrpcClient::Gst::Source::Source(void)
    : pipeline(nullptr)
    , element(nullptr)
    , bus(nullptr)
{
}

Peer::GrpcClient::Gst::Source::~Source(void)
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
