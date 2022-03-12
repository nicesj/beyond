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
#include "beyond/inference.h"
#include "beyond/private/event_loop_private.h"
#include "beyond/private/inference_peer_private.h"
#include "beyond/private/inference_runtime_private.h"
#include "beyond/private/inference_private.h"

#include "session_internal.h"
#include "inference_internal.h"
#include "inference_peer_internal.h"
#include "inference_runtime_internal.h"
#include "beyond_generic_internal.h"

struct beyond_inference {
    beyond_generic_handle _generic;
    beyond::EventLoop *eventLoop;
    beyond::EventLoop::HandlerObject *handlerObject;

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
        beyond_inference *_handle = static_cast<beyond_inference *>(container->handle);
        beyond::InferenceInterface *inference = nullptr;
        if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(_handle, inference) < 0 || inference == nullptr) {
            assert("inference handle is not valid");
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
    if (beyond_generic_handle_get_handle(handle, inference) < 0 || inference == nullptr) {
        return nullptr;
    }
    return inference;
}

// Manage the Inference operation
beyond_inference_h beyond_inference_create(beyond_session_h session, struct beyond_argument *option)
{
    beyond_inference *handle;

    if (session == nullptr) {
        ErrPrint("Invalid argument: session is nullptr");
        return nullptr;
    }

    try {
        handle = new beyond_inference();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    handle->eventLoop = static_cast<beyond::EventLoop *>(session);

    beyond::Inference *inference = beyond::Inference::Create(option);
    if (inference == nullptr) {
        delete handle;
        handle = nullptr;
        return nullptr;
    }

    handle->output = nullptr;
    handle->data = nullptr;

    beyond_generic_handle_init(handle);
    beyond_generic_handle_set_handle(handle, inference);

    handle->handlerObject = handle->eventLoop->AddEventHandler(
        inference,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            beyond_inference *handle = static_cast<beyond_inference *>(data);
            beyond_event_info event = {
                .type = beyond_event_type::BEYOND_EVENT_TYPE_NONE,
                .data = nullptr,
            };
            beyond::EventObjectInterface::EventData *evtData = nullptr;
            beyond::InferenceInterface *inference = nullptr;
            if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
                return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
            }

            DbgPrint("Fetch the event data for inference instance:%p", handle->output);
            int ret = inference->FetchEventData(evtData);

            if (ret < 0 || evtData == nullptr || (evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR)) {
                DbgPrint("There is some errors on event data");
                event.type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
                event.data = nullptr;
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
                handle->output(handle, &event, handle->data);
            }

            inference->DestroyEventData(evtData);
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        handle);

    if (handle->handlerObject == nullptr) {
        ErrPrint("Unable to add a new event handler");

        inference->Destroy();
        inference = nullptr;

        beyond_generic_handle_set_handle(handle, nullptr);
        beyond_generic_handle_deinit(handle);

        delete handle;
        handle = nullptr;
        return nullptr;
    }

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

int beyond_inference_configure(beyond_inference_h handle, struct beyond_config *conf)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    return inference->Configure(conf);
}

int beyond_inference_load_model(beyond_inference_h handle, const char **models, int count)
{
    if (handle == nullptr || models == nullptr) {
        ErrPrint("Invalid argument (%p, %p)", handle, models);
        return -EINVAL;
    }
    beyond::Inference *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    return inference->GetOutputTensorInfo(*info, *size);
}

beyond_tensor_h beyond_inference_allocate_tensor(beyond_inference_h handle, const struct beyond_tensor_info *info, int size)
{
    if (handle == nullptr || info == nullptr || size <= 0) {
        ErrPrint("Invalid argument (%p, %p, %d)", handle, info, size);
        return nullptr;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return nullptr;
    }

    beyond_tensor_container *container = static_cast<beyond_tensor_container *>(malloc(sizeof(beyond_tensor_container)));
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

int beyond_inference_prepare(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    int ret;

    const beyond_tensor_container *container = reinterpret_cast<beyond_tensor_container *>(ptr);
    if (container->handle != handle) {
        ErrPrint("Invalid tensor: %p %p\n", container->handle, handle);
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }
    beyond_tensor_container *container;

    container = static_cast<beyond_tensor_container *>(malloc(sizeof(beyond_tensor_container)));
    if (container == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    container->refcnt = 1;
    container->handle = handle;

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

// TODO: will be removed in the next PR
int beyond_inference_get_input(beyond_inference_h handle, struct beyond_tensor **tensor, int *size)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
        return -EINVAL;
    }

    // TODO:
    // Is this really necessary API?
    DbgPrint("inference: %p", inference);

    return -ENOSYS;
}

int beyond_inference_stop(beyond_inference_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface *inference = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::Inference>(handle, inference) < 0 || inference == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond_inference *_handle = static_cast<beyond_inference *>(handle);

    _handle->eventLoop->RemoveEventHandler(_handle->handlerObject);
    _handle->handlerObject = nullptr;

    inference->Destroy();
    inference = nullptr;

    beyond_generic_handle_set_handle(handle, nullptr);
    beyond_generic_handle_deinit(handle);

    delete _handle;
    _handle = nullptr;
}
