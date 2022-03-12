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

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/session.h"
#include "beyond/runtime.h"
#include "beyond/private/event_loop_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_runtime_interface_private.h"
#include "beyond/private/inference_runtime_private.h"

#include "session_internal.h"
#include "inference_runtime_internal.h"
#include "beyond_generic_internal.h"

struct beyond_inference_runtime {
    beyond_generic_handle _generic;
    beyond::EventLoop *eventLoop;
    beyond::EventLoop::HandlerObject *handlerObject;

    int (*eventHandler)(beyond_runtime_h handle, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::InferenceInterface::RuntimeInterface *beyond_runtime_get_runtime(beyond_runtime_h handle)
{
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;

    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0) {
        return nullptr;
    }

    return runtime;
}

beyond_runtime_h beyond_runtime_create(beyond_session_h session, struct beyond_argument *arg)
{
    beyond_inference_runtime *handle;

    if (session == nullptr) {
        ErrPrint("invalid argument: session is nullptr");
        return nullptr;
    }

    try {
        handle = new beyond_inference_runtime();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    handle->eventHandler = nullptr;
    handle->data = nullptr;
    handle->eventLoop = static_cast<beyond::EventLoop *>(session);

    beyond::Inference::Runtime *runtime = beyond::Inference::Runtime::Create(arg);
    if (runtime == nullptr) {
        ErrPrint("Failed to create a runtime instance");
        delete handle;
        handle = nullptr;
        return nullptr;
    }

    beyond_generic_handle_init(handle);
    beyond_generic_handle_set_handle(handle, runtime);

    if (runtime->GetHandle() < 0) {
        DbgPrint("created runtime does not support asynchronous mode");
        handle->handlerObject = nullptr;
    } else {
        handle->handlerObject = handle->eventLoop->AddEventHandler(
            static_cast<beyond::EventObjectBaseInterface *>(runtime),
            beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
            [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
                beyond_inference_runtime *handle = static_cast<beyond_inference_runtime *>(data);
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                beyond_event_info event;
                beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;
                int ret;

                if (beyond_generic_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
                    return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
                }

                ret = runtime->FetchEventData(evtData);
                if (ret < 0 || evtData == nullptr || (evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) == beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
                    event.type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
                    event.data = nullptr;
                } else {
                    event.type = evtData->type;
                    event.data = evtData->data;
                }

                if (handle->eventHandler != nullptr) {
                    handle->eventHandler(
                        static_cast<beyond_runtime_h>(handle),
                        &event,
                        handle->data);
                }

                runtime->DestroyEventData(evtData);

                return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
            },
            static_cast<void *>(handle));
        if (handle->handlerObject == nullptr) {
            ErrPrint("Failed to create a runtime instance");
            runtime->Destroy();
            runtime = nullptr;
            beyond_generic_handle_set_handle(handle, nullptr);
            beyond_generic_handle_deinit(handle);
            delete handle;
            handle = nullptr;
            return nullptr;
        }
    }

    return static_cast<beyond_runtime_h>(handle);
}

int beyond_runtime_set_event_callback(beyond_runtime_h handle, int (*event)(beyond_runtime_h handle, struct beyond_event_info *event, void *data), void *data)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
        return -EINVAL;
    }

    if (runtime->GetHandle() < 0) {
        DbgPrint("Runtime does not support asynchronous mode");
        return -ENOTSUP;
    }

    beyond_inference_runtime *_handle = static_cast<beyond_inference_runtime *>(handle);
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
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
        return -EINVAL;
    }

    return runtime->Configure(config);
}

void beyond_runtime_destroy(beyond_runtime_h handle)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }
    beyond::InferenceInterface::RuntimeInterface *runtime = nullptr;
    if (beyond_generic_handle_get_handle<beyond::InferenceInterface::RuntimeInterface>(handle, runtime) < 0 || runtime == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond_inference_runtime *_handle = static_cast<beyond_inference_runtime *>(handle);
    if (_handle->handlerObject != nullptr) {
        _handle->eventLoop->RemoveEventHandler(_handle->handlerObject);
        _handle->handlerObject = nullptr;
    }

    runtime->Destroy();
    runtime = nullptr;

    beyond_generic_handle_set_handle(handle, nullptr);
    beyond_generic_handle_deinit(handle);

    delete _handle;
    _handle = nullptr;
}
