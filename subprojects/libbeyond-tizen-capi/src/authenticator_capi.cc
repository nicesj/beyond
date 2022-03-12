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
#include "beyond/private/authenticator_private.h"
#include "beyond/common.h"
#include "beyond/authenticator.h"

#include "authenticator_internal.h"
#include "beyond_tizen_internal.h"

struct beyond_authenticator {
    beyond_tizen_handle _tizen;
    GPollFD eventFD;

    int (*eventHandler)(beyond_authenticator_h auth, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::AuthenticatorInterface *beyond_authenticator_get_authenticator(beyond_authenticator_h handle)
{
    beyond::AuthenticatorInterface *_handle = nullptr;

    if (beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(handle, _handle) < 0) {
        return nullptr;
    }

    return _handle;
}

static gboolean glib_authenticator_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

static gboolean glib_authenticator_event_check(GSource *source)
{
    beyond_authenticator *handle = reinterpret_cast<beyond_authenticator *>(source);

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

static gboolean glib_authenticator_event_handler(GSource *source, GSourceFunc callback, gpointer user_data)
{
    beyond_authenticator *handle = reinterpret_cast<beyond_authenticator *>(source);
    beyond::AuthenticatorInterface::EventData *authenticatorEventData = nullptr;
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    beyond::AuthenticatorInterface *authenticator = nullptr;

    int ret = beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(handle, authenticator);
    if (ret < 0) {
        return FALSE;
    }

    ret = authenticator->FetchEventData(evtData);
    if (ret < 0) {
        ErrPrintCode(-ret, "Fetch event data failed");
    } else {
        authenticatorEventData = static_cast<beyond::AuthenticatorInterface::EventData *>(evtData);
    }

    if (handle->eventHandler != nullptr) {
        beyond_event_info event;

        if (ret < 0 || authenticatorEventData == nullptr || (authenticatorEventData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
            event.type = beyond_event_type::BEYOND_EVENT_TYPE_AUTHENTICATOR_ERROR;
            event.data = nullptr;
        } else {
            event.type = authenticatorEventData->type;
            event.data = authenticatorEventData->data;
        }

        handle->eventHandler(
            static_cast<beyond_authenticator_h>(handle),
            &event,
            handle->data);
    }

    authenticator->DestroyEventData(evtData);
    return TRUE;
}

beyond_authenticator_h beyond_authenticator_create(struct beyond_argument *arg)
{
    beyond::Authenticator *authenticator;
    static GSourceFuncs g_handlers = {
        glib_authenticator_event_prepare,
        glib_authenticator_event_check,
        glib_authenticator_event_handler,
        nullptr,
    };

    GSource *source = g_source_new(&g_handlers, sizeof(beyond_authenticator));
    if (source == nullptr) {
        ErrPrint("Unable to allocate g_source");
        return nullptr;
    }

    beyond_authenticator *handle = reinterpret_cast<beyond_authenticator *>(source);
    handle->eventHandler = nullptr;
    handle->data = nullptr;

    authenticator = beyond::Authenticator::Create(arg);
    if (authenticator == nullptr) {
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    if (authenticator->GetHandle() >= 0) {
        GMainContext *ctx = g_main_context_ref_thread_default();
        if (ctx == nullptr) {
            ErrPrint("Failed to get defalt thread context");
            authenticator->Destroy();
            authenticator = nullptr;
            g_source_unref(source);
            source = nullptr;
            return nullptr;
        }

        handle->eventFD.fd = authenticator->GetHandle();
        handle->eventFD.events = G_IO_IN | G_IO_ERR;
        g_source_add_poll(source, &handle->eventFD);
        if (g_source_attach(source, ctx) == 0) {
            ErrPrint("Unable to attach source to the context");
            authenticator->Destroy();
            authenticator = nullptr;
            g_source_unref(source);
            source = nullptr;
            return nullptr;
        }
        g_source_unref(source);
    } else {
        DbgPrint("Async mode is not supported");
    }

    beyond_tizen_handle_init(handle);
    beyond_tizen_handle_set_handle(handle, authenticator);
    return static_cast<beyond_authenticator_h>(handle);
}

int beyond_authenticator_set_event_callback(beyond_authenticator_h auth, int (*event)(beyond_authenticator_h auth, struct beyond_event_info *event, void *data), void *data)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond_authenticator *_handle = static_cast<beyond_authenticator *>(auth);

    _handle->eventHandler = event;
    _handle->data = data;
    return 0;
}

int beyond_authenticator_configure(beyond_authenticator_h auth, struct beyond_config *config)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
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

    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        return -EINVAL;
    }

    return authenticator->Configure(config);
}

int beyond_authenticator_activate(beyond_authenticator_h auth)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        return -EINVAL;
    }

    return authenticator->Activate();
}

int beyond_authenticator_deactivate(beyond_authenticator_h auth)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        return -EINVAL;
    }

    return authenticator->Deactivate();
}

int beyond_authenticator_prepare(beyond_authenticator_h auth)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        return -EINVAL;
    }

    return authenticator->Prepare();
}

void beyond_authenticator_destroy(beyond_authenticator_h auth)
{
    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond::Authenticator *authenticator = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Authenticator>(auth, authenticator) < 0 || authenticator == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    authenticator->Destroy();

    GSource *source = reinterpret_cast<GSource *>(auth);
    GMainContext *ctx = g_source_get_context(source);

    beyond_tizen_handle_set_handle(auth, nullptr);
    beyond_tizen_handle_deinit(auth);
    g_source_destroy(source);
    g_source_unref(source);
    source = nullptr;

    if (ctx == nullptr) {
        DbgPrint("Async mode was not supported");
    } else {
        g_main_context_unref(ctx);
        ctx = nullptr;
    }
}
