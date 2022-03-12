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

#include "beyond/private/event_object_private.h"
#include "beyond/private/event_loop_private.h"

#include "session_internal.h"

beyond_session_h beyond_session_create(int thread, int signal)
{
    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(static_cast<bool>(thread), static_cast<bool>(signal));

    return static_cast<beyond_session_h>(eventLoop);
}

void beyond_session_destroy(beyond_session_h handle)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop != nullptr) {
        eventLoop->Destroy();
    }
}

int beyond_session_get_descriptor(beyond_session_h handle)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        return -EINVAL;
    }

    return eventLoop->GetHandle();
}

beyond_event_handler_h beyond_session_add_handler(beyond_session_h handle, beyond_object_h object, int type, beyond_handler_return (*eventHandler)(beyond_object_h, int, void *), void (*cancelHandler)(beyond_object_h object, void *callbackData), void *callbackData)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument, eventLoop is nullptr");
        return nullptr;
    }

    if (eventHandler == nullptr) {
        ErrPrint("Invalid argument, eventHandler is nullptr");
        return nullptr;
    }

    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(object),
        type,
        [eventHandler](beyond::EventObjectBaseInterface *object, int type, void *callbackData) -> beyond_handler_return {
            return eventHandler(static_cast<beyond_object_h>(object), type, callbackData);
        },
        [cancelHandler](beyond::EventObjectBaseInterface *eventObject, void *callbackData) -> void {
            if (cancelHandler != nullptr) {
                cancelHandler(static_cast<beyond_object_h>(eventObject), callbackData);
            }
            return;
        },
        callbackData);

    return static_cast<beyond_event_handler_h>(handlerObject);
}

beyond_event_handler_h beyond_session_add_fd_handler(beyond_session_h handle, int fd, int type, enum beyond_handler_return (*eventHandler)(beyond_object_h, int, void *), void (*cancelHandler)(beyond_object_h object, void *callbackData), void *callbackData)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument, eventLoop is nullptr");
        return nullptr;
    }

    if (fd < 0) {
        ErrPrint("Invalid argument, negative fd");
        return nullptr;
    }

    if (eventHandler == nullptr) {
        ErrPrint("Invalid argument, eventHandler is nullptr");
        return nullptr;
    }

    beyond::EventObject *eventObject;

    try {
        eventObject = new beyond::EventObject(fd);
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(eventObject),
        type,
        [eventHandler, handle](beyond::EventObjectBaseInterface *object, int type, void *callbackData) -> beyond_handler_return {
            return eventHandler(static_cast<beyond_object_h>(handle), type, callbackData);
        },
        [cancelHandler, handle](beyond::EventObjectBaseInterface *eventObject, void *callbackData) -> void {
            if (cancelHandler != nullptr) {
                cancelHandler(static_cast<beyond_object_h>(handle), callbackData);
            }
            return;
        },
        callbackData);

    if (handlerObject == nullptr) {
        ErrPrint("Unable to add event handler");
        delete eventObject;
    }

    return static_cast<beyond_event_handler_h>(handlerObject);
}

int beyond_session_remove_fd_handler(beyond_session_h handle, beyond_event_handler_h handler)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }

    beyond::EventLoop::HandlerObject *handlerObject = static_cast<beyond::EventLoop::HandlerObject *>(handler);
    beyond::EventObject *eventObject = dynamic_cast<beyond::EventObject *>(handlerObject->eventObject);
    if (eventObject == nullptr) {
        ErrPrint("Wrong object handler is used");
        return -EINVAL;
    }

    int ret = eventLoop->RemoveEventHandler(static_cast<beyond::EventLoop::HandlerObject *>(handler));
    delete eventObject;
    eventObject = nullptr;
    return ret;
}

int beyond_session_remove_handler(beyond_session_h handle, beyond_event_handler_h handler)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }

    return eventLoop->RemoveEventHandler(static_cast<beyond::EventLoop::HandlerObject *>(handler));
}

int beyond_session_run(beyond_session_h handle, int eventQueueSize, int loop_count, int timeout_in_ms)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }

    return eventLoop->Run(eventQueueSize, loop_count, timeout_in_ms);
}

int beyond_session_stop(beyond_session_h handle)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }

    return eventLoop->Stop();
}

int beyond_session_set_stop_handler(beyond_session_h handle, void (*handler)(beyond_session_h, void *), void *callbackData)
{
    beyond::EventLoop *eventLoop = static_cast<beyond::EventLoop *>(handle);
    if (eventLoop == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }

    return eventLoop->SetStopHandler([handler](beyond::EventLoop *eventLoop, void *callbackData) -> void {
        return handler(static_cast<beyond_session_h>(eventLoop), callbackData);
    },
                                     callbackData);
}
