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

#include <cstdio>
#include <cerrno>

#include <glib.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/peer.h"
#include "beyond/private/module_interface_private.h"
#include "beyond/private/event_object_interface_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_peer_interface_private.h"
#include "beyond/private/inference_peer_private.h"

#include "inference_peer_internal.h"
#include "authenticator_internal.h"
#include "beyond_tizen_internal.h"

struct beyond_peer {
    beyond_tizen_handle _tizen;
    GPollFD eventFD;

    int (*eventHandler)(beyond_peer_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::InferenceInterface::PeerInterface *beyond_peer_get_peer(beyond_peer_h handle)
{
    beyond::InferenceInterface::PeerInterface *peer = nullptr;

    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return nullptr;
    }

    return peer;
}

static gboolean glib_peer_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

static gboolean glib_peer_event_check(GSource *source)
{
    beyond_peer *handle = reinterpret_cast<beyond_peer *>(source);

    if ((handle->eventFD.revents & G_IO_IN) == G_IO_IN) {
        return TRUE;
    }

    if ((handle->eventFD.revents & G_IO_ERR) == G_IO_ERR) {
        // TODO:
        // Handling the error case
        ErrPrint("Error event!");
        return TRUE;
    }

    return FALSE;
}

// Invoked from the DeviceContext
static gboolean glib_peer_event_handler(GSource *source, GSourceFunc callback, gpointer user_data)
{
    beyond_peer *handle = reinterpret_cast<beyond_peer *>(source);

    beyond::InferenceInterface::PeerInterface::EventData *peerEventData = nullptr;
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    beyond::InferenceInterface::PeerInterface *peer = nullptr;

    if (beyond_tizen_handle_get_handle(handle, peer) < 0 || peer == nullptr) {
        return FALSE;
    }

    int ret = peer->FetchEventData(evtData);
    if (ret < 0) {
        ErrPrintCode(-ret, "Fetch event data failed");
    } else {
        peerEventData = static_cast<beyond::InferenceInterface::PeerInterface::EventData *>(evtData);
    }

    if (handle->eventHandler != nullptr) {
        beyond_event_info event;

        if (ret < 0 || peerEventData == nullptr || ((peerEventData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR)) {
            event.type = beyond_event_type::BEYOND_EVENT_TYPE_PEER_ERROR;
            event.data = nullptr;
        } else {
            event.type = peerEventData->type;
            event.data = peerEventData->data;
        }

        handle->eventHandler(
            static_cast<beyond_peer_h>(handle),
            &event,
            handle->data);
    }

    peer->DestroyEventData(evtData);
    return TRUE;
}

// Manage the other devices (e.g Android Edge device)
beyond_peer_h beyond_peer_create(struct beyond_argument *arg)
{
    static GSourceFuncs g_handlers = {
        glib_peer_event_prepare,
        glib_peer_event_check,
        glib_peer_event_handler,
        nullptr,
    };

    GSource *source = g_source_new(&g_handlers, sizeof(beyond_peer));
    if (source == nullptr) {
        ErrPrint("unable to allocate g_source");
        return nullptr;
    }

    beyond_peer *handle = reinterpret_cast<beyond_peer *>(source);
    handle->eventHandler = nullptr;
    handle->data = nullptr;

    beyond::Inference::Peer *peer = beyond::Inference::Peer::Create(arg);
    if (peer == nullptr) {
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    GMainContext *ctx = g_main_context_ref_thread_default();
    if (ctx == nullptr) {
        ErrPrint("failed to get default thread context");
        peer->Destroy();
        peer = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    handle->eventFD.fd = peer->GetHandle();
    handle->eventFD.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(source, &handle->eventFD);
    if (g_source_attach(source, ctx) == 0) {
        ErrPrint("Unable to attach the source to the context");
        peer->Destroy();
        peer = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }
    g_source_unref(source);

    beyond_tizen_handle_init(handle);
    beyond_tizen_handle_set_handle(handle, peer);
    return static_cast<beyond_peer_h>(handle);
}

int beyond_peer_set_event_callback(beyond_peer_h handle, int (*event)(beyond_peer_h handle, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond_peer *_handle = static_cast<beyond_peer *>(handle);

    _handle->eventHandler = event;
    _handle->data = data;
    return 0;
}

// If there are required object for initiating the peer object, use this method.
int beyond_peer_configure(beyond_peer_h handle, struct beyond_config *config)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }

    struct beyond_config _config;
    if (config != nullptr) {
        _config.type = config->type;
        if (beyond_tizen_handle_get_handle<void>(config->object, _config.object) < 0 || _config.object == nullptr) {
            _config.object = config->object;
        }
        config = &_config;
    }

    return peer->Configure(config);
}

// this function is going to return the latest status information of the peer after the peer instance is created
// and had at least one or several times of connections before.
int beyond_peer_get_info(beyond_peer_h handle, const struct beyond_peer_info *info)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }
    return peer->GetInfo(info);
}

// this function provide a way to update the peer information using the other discovery module.
int beyond_peer_set_info(beyond_peer_h handle, struct beyond_peer_info *info)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }
    return peer->SetInfo(info);
}

// Create a connection and prepare communication channel
int beyond_peer_activate(beyond_peer_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }
    return peer->Activate();
}

// Destroy the connection
int beyond_peer_deactivate(beyond_peer_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }
    return peer->Deactivate();
}

void beyond_peer_destroy(beyond_peer_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond::Inference::Peer *peer = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference::Peer>(handle, peer) < 0 || peer == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    peer->Destroy();
    peer = nullptr;

    GSource *source = reinterpret_cast<GSource *>(handle);

    GMainContext *ctx = g_source_get_context(source);
    if (ctx == nullptr) {
        assert(!"Context cannot be nullptr");
        ErrPrint("Context cannot be nullptr");
    }
    beyond_tizen_handle_set_handle(handle, nullptr);
    beyond_tizen_handle_deinit(handle);
    g_source_destroy(source);
    g_source_unref(source);
    source = nullptr;

    g_main_context_unref(ctx);
    ctx = nullptr;
}
