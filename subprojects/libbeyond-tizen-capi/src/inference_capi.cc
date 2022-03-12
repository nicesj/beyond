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
#include "beyond/inference.h"
#include "beyond/private/event_object_interface_private.h"
#include "beyond/private/inference_peer_private.h"
#include "beyond/private/inference_runtime_private.h"
#include "beyond/private/inference_private.h"

#include "inference_internal.h"
#include "inference_peer_internal.h"
#include "inference_runtime_internal.h"

#include "beyond_tizen_internal.h"

struct beyond_inference {
    beyond_tizen_handle _tizen;
    GPollFD eventFD;

    void (*output)(beyond_inference_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

static beyond_tensor_container *tensor_container_ref(beyond_tensor_container *container)
{
    ++container->refcnt;
    return container;
}

static beyond_tensor_container *tensor_container_unref(beyond_tensor_container *container)
{
    --container->refcnt;

    if (container->refcnt == 0) {
        beyond::InferenceInterface *inference = nullptr;
        if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(container->handle, inference) < 0 || inference == nullptr) {
            assert("Handle is not valid");
        } else {
            inference->FreeTensor(container->tensor, container->size);
            container->size = 0;
            container->tensor = nullptr; // tensor ptr will be reset to nullptr by FreeTensor() but for the readability
            free(container);
            container = nullptr;
        }
    }

    return container;
}

beyond::InferenceInterface *beyond_inference_get_inference(beyond_inference_h handle)
{
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0) {
        return nullptr;
    }
    return inference;
}

static gboolean glib_inference_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

beyond_tensor_h beyond_inference_allocate_tensor(beyond_inference_h handle, const struct beyond_tensor_info *info, int size)
{
    if (handle == nullptr || info == nullptr || size <= 0) {
        ErrPrint("Invalid argument (%p, %p, %d)", handle, info, size);
        return nullptr;
    }

    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return nullptr;
    }

    beyond_tensor_container *container;
    container = static_cast<beyond_tensor_container *>(malloc(sizeof(beyond_tensor_container)));
    if (container == nullptr) {
        ErrPrintCode(errno, "malloc");
        return nullptr;
    }

    container->refcnt = 1;
    container->size = size;
    container->handle = handle;

    int ret = inference->AllocateTensor(info, container->size, container->tensor);
    if (ret < 0) {
        ErrPrint("Failed to allocate tensor");
        free(container);
        container = nullptr;
        return nullptr;
    }

    return reinterpret_cast<beyond_tensor_h>(container);
}

beyond_tensor_h beyond_inference_ref_tensor(beyond_tensor_h ptr)
{
    beyond_tensor_container *container = reinterpret_cast<beyond_tensor_container *>(ptr);
    (void)tensor_container_ref(container);
    return ptr;
}

beyond_tensor_h beyond_inference_unref_tensor(beyond_tensor_h ptr)
{
    if (ptr == nullptr) {
        return nullptr;
    }

    beyond_tensor_container *container = reinterpret_cast<beyond_tensor_container *>(ptr);
    return reinterpret_cast<beyond_tensor_h>(tensor_container_unref(container));
}

static gboolean glib_inference_event_check(GSource *source)
{
    beyond_inference *handle = reinterpret_cast<beyond_inference *>(source);

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
static gboolean glib_inference_event_handler(GSource *source, GSourceFunc callback, gpointer user_data)
{
    beyond_inference *handle = reinterpret_cast<beyond_inference *>(source);
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    beyond_event_info event = {
        .type = beyond_event_type::BEYOND_EVENT_TYPE_NONE,
        .data = nullptr,
    };
    beyond::InferenceInterface *inference = nullptr;

    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return FALSE;
    }

    int ret = inference->FetchEventData(evtData);

    if (ret < 0 || evtData == nullptr || (evtData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        if (ret < 0) {
            ErrPrintCode(-ret, "Fetch event data failed");
        }
        event.type = beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    } else if ((evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_MASK) != 0) {
        event.type = evtData->type;
        beyond_inference_context *context = static_cast<beyond_inference_context *>(evtData->data);
        if (context != nullptr) {
            context->input_tensor = tensor_container_unref(context->input_tensor);
            event.data = const_cast<void *>(context->user_context);
            free(context);
            context = nullptr;
        }
    } else {
        event.type = evtData->type;
        event.data = evtData->data;
    }

    if (handle->output != nullptr) {
        handle->output(static_cast<beyond_inference_h>(handle), &event, handle->data);
    }

    inference->DestroyEventData(evtData);
    return TRUE;
}

// Manage the Inference operation
beyond_inference_h beyond_inference_create(struct beyond_argument *option)
{
    static GSourceFuncs g_handlers = {
        glib_inference_event_prepare,
        glib_inference_event_check,
        glib_inference_event_handler,
        nullptr,
    };

    GSource *source = g_source_new(&g_handlers, sizeof(beyond_inference));
    if (source == nullptr) {
        ErrPrint("unable to allocate g_source");
        return nullptr;
    }

    beyond_inference *handle = reinterpret_cast<beyond_inference *>(source);
    handle->output = nullptr;
    handle->data = nullptr;

    beyond::Inference *inference = beyond::Inference::Create(option);
    if (inference == nullptr) {
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    GMainContext *ctx = g_main_context_ref_thread_default();
    if (ctx == nullptr) {
        ErrPrint("failed to get default thread context");
        inference->Destroy();
        inference = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }

    handle->eventFD.fd = inference->GetHandle();
    handle->eventFD.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(source, &handle->eventFD);
    if (g_source_attach(source, ctx) == 0) {
        ErrPrint("Unable to attach the source to the context");
        inference->Destroy();
        inference = nullptr;
        g_source_unref(source);
        source = nullptr;
        return nullptr;
    }
    g_source_unref(source);

    beyond_tizen_handle_init(handle);
    beyond_tizen_handle_set_handle(handle, inference);
    return static_cast<beyond_inference_h>(handle);
}

int beyond_inference_set_output_callback(beyond_inference_h handle, void (*output)(beyond_inference_h handle, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond_inference *_handle = static_cast<beyond_inference *>(handle);
    _handle->output = output;
    _handle->data = data;
    return 0;
}

int beyond_inference_configure(beyond_inference_h handle, struct beyond_config *config)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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

    return inference->Configure(config);
}

int beyond_inference_load_model(beyond_inference_h handle, const char **models, int count)
{
    if (handle == nullptr || models == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, models);
        return -EINVAL;
    }

    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    return inference->LoadModel(models, count);
}

int beyond_inference_set_input_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info *info, int size)
{
    if (handle == nullptr || info == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, info);
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    return inference->SetInputTensorInfo(info, size);
}

int beyond_inference_set_output_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info *info, int size)
{
    if (handle == nullptr || info == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, info);
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->SetOutputTensorInfo(info, size);
}

int beyond_inference_get_input_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info **info, int *size)
{
    if (handle == nullptr || info == nullptr || size == nullptr) {
        ErrPrint("Invalid argument (%p, %p, %p)", handle, info, size);
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->GetInputTensorInfo(*info, *size);
}

int beyond_inference_get_output_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info **info, int *size)
{
    if (handle == nullptr || info == nullptr || size == nullptr) {
        ErrPrint("Invalid argument (%p, %p, %p)", handle, info, size);
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->GetOutputTensorInfo(*info, *size);
}

int beyond_inference_prepare(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->Prepare();
}

int beyond_inference_do(beyond_inference_h handle, const beyond_tensor_h ptr, const void *user_context)
{
    if (handle == nullptr || ptr == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, ptr);
        return -EINVAL;
    }

    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    int ret;

    const beyond_tensor_container *container = reinterpret_cast<beyond_tensor_container *>(ptr);
    if (container->handle != handle) {
        ErrPrint("Tensor buffer is not proper to the given handle");
        return -EINVAL;
    }

    beyond_inference_context *context = static_cast<beyond_inference_context *>(malloc(sizeof(beyond_inference_context)));
    if (context == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    context->user_context = user_context;
    context->input_tensor = tensor_container_ref(const_cast<beyond_tensor_container *>(container));

    ret = inference->Invoke(container->tensor, container->size, context);
    if (ret < 0) {
        (void)tensor_container_unref(context->input_tensor);
        free(context);
    }

    return ret;
}

int beyond_inference_get_output(beyond_inference_h handle, beyond_tensor_h *ptr, int *size)
{
    if (handle == nullptr || ptr == nullptr || size == nullptr) {
        ErrPrint("Invalid argument (%p, %p, %p)", handle, ptr, size);
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    beyond_tensor_container *container = static_cast<beyond_tensor_container *>(malloc(sizeof(beyond_tensor_container)));
    if (container == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }
    container->handle = handle;
    container->refcnt = 1;

    int ret = inference->GetOutput(container->tensor, container->size);
    if (ret < 0) {
        free(container);
        container = nullptr;
        return ret;
    }

    *ptr = reinterpret_cast<beyond_tensor_h>(container);
    *size = container->size;
    return 0;
}

// TODO:
// Will be removed in the next PR
int beyond_inference_get_input(beyond_inference_h handle, beyond_tensor_h *ptr)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    if (ptr == nullptr) {
        ErrPrint("Invalid tensor ptr");
        return -EINVAL;
    }

    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    DbgPrint("inference: %p", inference);

    // TODO:
    // Do we need keep this method?
    return -ENOSYS;
}

int beyond_inference_stop(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->Stop();
}

int beyond_inference_is_stopped(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    // TODO:
    // Get the state of the inference instance.
    // How?
    DbgPrint("inference: %p", inference);

    return -ENOSYS;
}

int beyond_inference_add_peer(beyond_inference_h handle, beyond_peer_h peer)
{
    if (handle == nullptr || peer == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, peer);
        return -EINVAL;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    beyond::InferenceInterface::PeerInterface *_peer = beyond_peer_get_peer(peer);
    return inference->AddPeer(_peer);
}

int beyond_inference_remove_peer(beyond_inference_h handle, beyond_peer_h peer)
{
    if (handle == nullptr || peer == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, peer);
        return -EINVAL;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    beyond::InferenceInterface::PeerInterface *_peer = beyond_peer_get_peer(peer);
    return inference->RemovePeer(_peer);
}

int beyond_inference_add_runtime(beyond_inference_h handle, beyond_runtime_h runtime)
{
    if (handle == nullptr || runtime == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, runtime);
        return -EINVAL;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    beyond::InferenceInterface::RuntimeInterface *_runtime = beyond_runtime_get_runtime(runtime);
    return inference->AddRuntime(_runtime);
}

int beyond_inference_remove_runtime(beyond_inference_h handle, beyond_runtime_h runtime)
{
    if (handle == nullptr || runtime == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, runtime);
        return -EINVAL;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    beyond::InferenceInterface::RuntimeInterface *_runtime = beyond_runtime_get_runtime(runtime);
    return inference->RemoveRuntime(_runtime);
}

void beyond_inference_destroy(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_tizen_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    inference->Destroy();
    inference = nullptr;

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
