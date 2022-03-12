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
#include "beyond/private/discovery_private.h"
#include "beyond/common.h"
#include "beyond/discovery.h"

#include "discovery_internal.h"
#include "authenticator_internal.h"

#include "beyond_tizen_internal.h"

struct beyond_discovery {
    beyond_tizen_handle _tizen;
    GPollFD eventFD;

    int (*eventHandler)(beyond_discovery_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::DiscoveryInterface *beyond_discovery_get_discovery(beyond_discovery_h handle)
{
    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0) {
        return nullptr;
    }
    return discovery;
}

static gboolean glib_discovery_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

static gboolean glib_discovery_event_check(GSource *source)
{
    beyond_discovery *handle = reinterpret_cast<beyond_discovery *>(source);

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

static gboolean glib_discovery_event_handler(GSource *source, GSourceFunc callback, gpointer user_data)
{
    beyond_discovery *handle = reinterpret_cast<beyond_discovery *>(source);
    beyond::DiscoveryInterface::EventData *discoveryEventData = nullptr;
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    beyond::DiscoveryInterface *discovery = nullptr;

    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return FALSE;
    }

    int ret = discovery->FetchEventData(evtData);
    if (ret < 0) {
        ErrPrintCode(-ret, "Fetch event data failed");
    } else {
        discoveryEventData = static_cast<beyond::DiscoveryInterface::EventData *>(evtData);
    }

    if (handle->eventHandler != nullptr) {
        beyond_event_info event;

        if (ret < 0 || discoveryEventData == nullptr || (discoveryEventData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
            event.type = beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_ERROR;
            event.data = nullptr;
        } else {
            event.type = discoveryEventData->type;
            event.data = discoveryEventData->data;
        }

        handle->eventHandler(
            static_cast<beyond_discovery_h>(handle),
            &event,
            handle->data);
    }

    discovery->DestroyEventData(evtData);
    return TRUE;
}

beyond_discovery_h beyond_discovery_create(struct beyond_argument *arg)
{
    static GSourceFuncs g_handlers = {
        glib_discovery_event_prepare,
        glib_discovery_event_check,
        glib_discovery_event_handler,
        nullptr,
    };

    GSource *source = g_source_new(&g_handlers, sizeof(beyond_discovery));
    if (source == nullptr) {
        ErrPrint("Unable to allocate g_source");
        return nullptr;
    }

    beyond_discovery *handle = reinterpret_cast<beyond_discovery *>(source);
    handle->eventHandler = nullptr;
    handle->data = nullptr;

    beyond::Discovery *discovery = beyond::Discovery::Create(arg);
    if (discovery == nullptr) {
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    if (discovery->GetHandle() >= 0) {
        GMainContext *ctx = g_main_context_ref_thread_default();
        if (ctx == nullptr) {
            ErrPrint("Failed to get defalt thread context");
            discovery->Destroy();
            discovery = nullptr;
            g_source_unref(source);
            source = nullptr;
            return nullptr;
        }

        handle->eventFD.fd = discovery->GetHandle();
        handle->eventFD.events = G_IO_IN | G_IO_ERR;
        g_source_add_poll(source, &handle->eventFD);
        if (g_source_attach(source, ctx) == 0) {
            ErrPrint("Unable to attach source to the context");
            discovery->Destroy();
            discovery = nullptr;
            g_source_unref(source);
            source = nullptr;
            return nullptr;
        }
        g_source_unref(source);
    } else {
        DbgPrint("Async mode is not supported");
    }

    beyond_tizen_handle_init(handle);
    beyond_tizen_handle_set_handle(handle, discovery);
    return static_cast<beyond_discovery_h>(handle);
}

int beyond_discovery_set_event_callback(beyond_discovery_h handle, int (*event)(beyond_discovery_h handle, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond_discovery *_handle = static_cast<beyond_discovery *>(handle);

    _handle->eventHandler = event;
    _handle->data = data;
    return 0;
}

int beyond_discovery_configure(beyond_discovery_h handle, struct beyond_config *config)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
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

    return discovery->Configure(config);
}

// Activate discovery module
int beyond_discovery_activate(beyond_discovery_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return -EINVAL;
    }

    return discovery->Activate();
}

// Deactivate discovery module
int beyond_discovery_deactivate(beyond_discovery_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return -EINVAL;
    }

    return discovery->Deactivate();
}

void beyond_discovery_destroy(beyond_discovery_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond::Discovery *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Discovery>(handle, discovery) < 0 || discovery == nullptr) {
        return;
    }

    discovery->Destroy();

    GSource *source = reinterpret_cast<GSource *>(handle);

    GMainContext *ctx = g_source_get_context(source);
    if (ctx == nullptr) {
        ErrPrint("Context cannot be nullptr");
        assert(!"Context cannot be nullptr");
    }

    beyond_tizen_handle_deinit(handle);
    g_source_destroy(source);
    g_source_unref(source);
    g_main_context_unref(ctx);
}

int beyond_discovery_set_item(beyond_discovery_h handle, const char *key, const void *value, uint8_t valueSize)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return -EINVAL;
    }

    return discovery->SetItem(key, value, valueSize);
}

int beyond_discovery_remove_item(beyond_discovery_h handle, const char *key)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return -EINVAL;
    }

    return discovery->RemoveItem(key);
}
