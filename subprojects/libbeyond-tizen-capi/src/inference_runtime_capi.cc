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
#include "beyond/private/inference_runtime_private.h"
#include "beyond/common.h"
#include "beyond/runtime.h"

#include "inference_runtime_internal.h"
#include "beyond_tizen_internal.h"

struct beyond_runtime {
    beyond_tizen_handle _tizen;
    GPollFD eventFD;

    int (*eventHandler)(beyond_runtime_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::InferenceInterface::RuntimeInterface *beyond_runtime_get_runtime(beyond_runtime_h handle)
{
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0) {
        return nullptr;
    }
    return runtime;
}

static gboolean glib_runtime_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

static gboolean glib_runtime_event_check(GSource *source)
{
    beyond_runtime *handle = reinterpret_cast<beyond_runtime *>(source);

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

static gboolean glib_runtime_event_handler(GSource *source, GSourceFunc callback, gpointer user_data)
{
    beyond_runtime *handle = reinterpret_cast<beyond_runtime *>(source);

    beyond::InferenceInterface::RuntimeInterface::EventData *runtimeEventData = nullptr;
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;

    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
        return FALSE;
    }

    int ret = runtime->FetchEventData(evtData);
    if (ret < 0) {
        ErrPrintCode(-ret, "Fetch event data failed");
    } else {
        runtimeEventData = static_cast<beyond::InferenceInterface::RuntimeInterface::EventData *>(evtData);
    }

    if (handle->eventHandler != nullptr) {
        beyond_event_info event;

        if (ret < 0 || runtimeEventData == nullptr || ((runtimeEventData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR)) {
            event.type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
            event.data = nullptr;
        } else {
            event.type = runtimeEventData->type;
            event.data = runtimeEventData->data;
        }

        handle->eventHandler(
            static_cast<beyond_runtime_h>(handle),
            &event,
            handle->data);
    }

    runtime->DestroyEventData(evtData);
    return TRUE;
}

beyond_runtime_h beyond_runtime_create(struct beyond_argument *arg)
{
    static GSourceFuncs g_handlers = {
        glib_runtime_event_prepare,
        glib_runtime_event_check,
        glib_runtime_event_handler,
        nullptr,
    };

    GSource *source = g_source_new(&g_handlers, sizeof(beyond_runtime));
    if (source == nullptr) {
        ErrPrint("unable to allocate g_source");
        return nullptr;
    }

    beyond_runtime *handle = reinterpret_cast<beyond_runtime *>(source);
    handle->eventHandler = nullptr;
    handle->data = nullptr;
    beyond::Inference::Runtime *runtime = beyond::Inference::Runtime::Create(arg);
    if (runtime == nullptr) {
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    GMainContext *ctx = g_main_context_ref_thread_default();
    if (ctx == nullptr) {
        ErrPrint("failed to get default thread context");
        runtime->Destroy();
        runtime = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    handle->eventFD.fd = runtime->GetHandle();
    handle->eventFD.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(source, &handle->eventFD);
    if (g_source_attach(source, ctx) == 0) {
        ErrPrint("Unable to attach the source to the context");
        runtime->Destroy();
        runtime = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }
    g_source_unref(source);

    beyond_tizen_handle_init(handle);
    beyond_tizen_handle_set_handle(handle, runtime);
    return static_cast<beyond_runtime_h>(handle);
}

int beyond_runtime_set_event_callback(beyond_runtime_h handle, int (*event)(beyond_runtime_h runtime, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond_runtime *_handle = static_cast<beyond_runtime *>(handle);

    _handle->eventHandler = event;
    _handle->data = data;
    return 0;
}

int beyond_runtime_configure(beyond_runtime_h handle, struct beyond_config *config)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
        return -EINVAL;
    }

    beyond_config _config;
    if (config != nullptr) {
        _config.type = config->type;
        if (beyond_tizen_handle_get_handle<void>(config->object, _config.object) < 0 || _config.object == nullptr) {
            _config.object = config->object;
        }
        config = &_config;
    }

    return runtime->Configure(config);
}

void beyond_runtime_destroy(beyond_runtime_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }
    beyond::Inference::Runtime *runtime = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference::Runtime>(handle, runtime) < 0 || runtime == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    runtime->Destroy();
    runtime = nullptr;

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
