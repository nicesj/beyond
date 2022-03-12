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

#include <exception>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/session.h"
#include "beyond/authenticator.h"

#include "session_internal.h"
#include "authenticator_internal.h"
#include "beyond_generic_internal.h"

struct beyond_authenticator {
    beyond_generic_handle _generic;
    beyond::EventLoop *eventLoop;
    beyond::EventLoop::HandlerObject *handlerObject;

    int (*eventHandler)(beyond_authenticator_h auth, struct beyond_event_info *event, void *data);
    void *data;
};

beyond::AuthenticatorInterface *beyond_authenticator_get_authenticator(beyond_authenticator_h handle)
{
    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(handle, authenticator) < 0) {
        return nullptr;
    }
    return authenticator;
}

beyond_authenticator_h beyond_authenticator_create(beyond_session_h session, struct beyond_argument *arg)
{
    beyond_authenticator *handle;

    if (session == nullptr) {
        ErrPrint("Invalid argument: session is nullptr");
        return nullptr;
    }

    try {
        handle = new beyond_authenticator();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    handle->handlerObject = nullptr;
    handle->eventHandler = nullptr;
    handle->data = nullptr;
    handle->eventLoop = static_cast<beyond::EventLoop *>(session);

    beyond::Authenticator *authenticator = beyond::Authenticator::Create(arg);
    if (authenticator == nullptr) {
        ErrPrint("Failed to create a authenticator instance");
        delete handle;
        return nullptr;
    }

    beyond_generic_handle_init(handle);
    beyond_generic_handle_set_handle(handle, authenticator);

    if (authenticator->GetHandle() >= 0) {
        handle->handlerObject = handle->eventLoop->AddEventHandler(
            static_cast<beyond::EventObjectBaseInterface *>(authenticator),
            beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
            [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
                beyond_authenticator *handle = static_cast<beyond_authenticator *>(data);
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                beyond_event_info event;
                beyond::AuthenticatorInterface *authenticator = nullptr;
                int ret;

                if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(handle, authenticator) < 0 || authenticator == nullptr) {
                    return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
                }

                ret = authenticator->FetchEventData(evtData);
                if (ret < 0 || evtData == nullptr || (evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) == beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
                    event.type = beyond_event_type::BEYOND_EVENT_TYPE_AUTHENTICATOR_ERROR;
                    event.data = nullptr;
                } else {
                    event.type = evtData->type;
                    event.data = evtData->data;
                }

                if (handle->eventHandler != nullptr) {
                    handle->eventHandler(
                        static_cast<beyond_authenticator_h>(handle),
                        &event,
                        handle->data);
                }

                authenticator->DestroyEventData(evtData);
                return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
            },
            static_cast<void *>(handle));
        if (handle->handlerObject == nullptr) {
            ErrPrint("Failed to add an event handler");
            authenticator->Destroy();
            authenticator = nullptr;
            beyond_generic_handle_set_handle(handle, nullptr);
            beyond_generic_handle_deinit(handle);
            delete handle;
            return nullptr;
        }
    } else {
        InfoPrint("Auth module does not support Async mode");
    }

    return static_cast<beyond_authenticator_h>(handle);
}

int beyond_authenticator_set_event_callback(beyond_authenticator_h auth, int (*event)(beyond_authenticator_h auth, struct beyond_event_info *event, void *data), void *data)
{
    if (auth == nullptr || event == nullptr) {
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
    struct beyond_config _config;

    if (auth == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        return -EINVAL;
    }

    if (config != nullptr) {
        // NOTE:
        // the config data should be updated in this case
        // BeyonD Private API does not know about the handle of generic CAPI
        // therefore, the handle must be converted to beyond instance
        _config.type = config->type;
        if (beyond_generic_handle_get_handle<void>(config->object, _config.object) < 0 || _config.object == nullptr) {
            _config.object = config->object;
        }
        config = &_config;
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
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
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
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
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

    beyond::AuthenticatorInterface *authenticator = nullptr;
    if (beyond_generic_handle_get_handle<beyond::AuthenticatorInterface>(auth, authenticator) < 0 || authenticator == nullptr) {
        ErrPrint("Invalid handle");
        return;
    }

    beyond_authenticator *_handle = static_cast<beyond_authenticator *>(auth);
    if (_handle->handlerObject != nullptr) {
        _handle->eventLoop->RemoveEventHandler(_handle->handlerObject);
        _handle->handlerObject = nullptr;
    }

    authenticator->Destroy();
    authenticator = nullptr;

    beyond_generic_handle_set_handle(auth, nullptr);
    beyond_generic_handle_deinit(auth);

    delete _handle;
    _handle = nullptr;
}
