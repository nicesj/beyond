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
#include <cstring>
#include <cerrno>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/session.h"
#include "beyond/peer.h"
#include "beyond/private/module_interface_private.h"
#include "beyond/private/event_loop_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_peer_interface_private.h"
#include "beyond/private/inference_peer_private.h"

#include "session_internal.h"
#include "inference_peer_internal.h"
#include "authenticator_internal.h"
#include "beyond_generic_internal.h"

struct beyond_inference_peer {
    beyond_generic_handle _generic;
    beyond::EventLoop *eventLoop;
    beyond::EventLoop::HandlerObject *handlerObject;

    int (*eventHandler)(beyond_peer_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::InferenceInterface::PeerInterface *beyond_peer_get_peer(beyond_peer_h handle)
{
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return nullptr;
    }
    return peer;
}

// Manage the other devices (e.g Android Edge device)
beyond_peer_h beyond_peer_create(beyond_session_h session, struct beyond_argument *arg)
{
    beyond_inference_peer *handle;

    if (session == nullptr) {
        ErrPrint("invalid aragument: session is nullptr");
        return nullptr;
    }

    try {
        handle = new beyond_inference_peer();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    handle->eventHandler = nullptr;
    handle->data = nullptr;

    handle->eventLoop = static_cast<beyond::EventLoop *>(session);
    beyond::Inference::Peer *peer = beyond::Inference::Peer::Create(arg);
    if (peer == nullptr) {
        ErrPrint("Failed to create a peer instance");
        delete handle;
        handle = nullptr;
        return nullptr;
    }

    beyond_generic_handle_init(handle);
    beyond_generic_handle_set_handle(handle, peer);

    handle->handlerObject = handle->eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            beyond_inference_peer *handle = static_cast<beyond_inference_peer *>(data);
            beyond::EventObjectInterface::EventData *evtData = nullptr;
            beyond::InferenceInterface::PeerInterface *peer = nullptr;
            beyond_event_info event;
            int ret;

            if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
                return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
            }

            // TODO:
            // beyond peer event must be fetched using FetchEventData()
            // not from here.
            ret = peer->FetchEventData(evtData);
            if (ret < 0 || evtData == nullptr || (evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) == beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
                event.type = beyond_event_type::BEYOND_EVENT_TYPE_PEER_ERROR;
                event.data = nullptr;
            } else {
                event.type = evtData->type;
                event.data = evtData->data;
            }

            if (handle->eventHandler != nullptr) {
                handle->eventHandler(
                    static_cast<beyond_peer_h>(handle),
                    &event,
                    handle->data);
            }

            peer->DestroyEventData(evtData);

            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        static_cast<void *>(handle));
    if (handle->handlerObject == nullptr) {
        ErrPrint("Failed to create a peer instance");
        peer->Destroy();
        peer = nullptr;
        beyond_generic_handle_set_handle(handle, nullptr);
        beyond_generic_handle_deinit(handle);
        delete handle;
        handle = nullptr;
        return nullptr;
    }

    return static_cast<beyond_peer_h>(handle);
}

int beyond_peer_set_event_callback(beyond_peer_h handle, int (*event)(beyond_peer_h handle, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr || event == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond_inference_peer *_handle = static_cast<beyond_inference_peer *>(handle);
    _handle->eventHandler = event;
    _handle->data = data;
    return 0;
}

// If there are required object for initiating the peer object, use this method.
int beyond_peer_configure(beyond_peer_h handle, struct beyond_config *config)
{
    struct beyond_config _config;

    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        return -EINVAL;
    }

    if (config != nullptr) {
        // NOTE:
        // the config data should be updated in this case
        // BeyonD Private API does not know about the handle of tizen CAPI
        // therefore, the handle must be converted to beyond instance
        _config.type = config->type;
        if (beyond_generic_handle_get_handle<void>(config->object, _config.object) < 0 || _config.object == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
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
    beyond::InferenceInterface::PeerInterface *peer = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::PeerInterface>(handle, peer) < 0 || peer == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond_inference_peer *_handle = static_cast<beyond_inference_peer *>(handle);
    _handle->eventLoop->RemoveEventHandler(_handle->handlerObject);
    _handle->handlerObject = nullptr;

    peer->Destroy();
    peer = nullptr;

    beyond_generic_handle_set_handle(handle, nullptr);
    beyond_generic_handle_deinit(handle);

    delete _handle;
    _handle = nullptr;
}
