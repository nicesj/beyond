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

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cerrno>

#include <vector>

#include <libgen.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_private.h"

namespace beyond {

EventObject::EventObject(int _handle)
    : handle(_handle)
{
}

int EventObject::GetHandle(void) const
{
    return handle;
}

int EventObject::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    HandlerData *info;

    std::vector<HandlerData *>::iterator it;

    for (it = handlers.begin(); it != handlers.end(); ++it) {
        info = *it;
        assert(info != nullptr && "info must not be nullptr");
        if (info->handler == handler && ((info->type & type) != 0) && info->data == data) {
            ErrPrint("Handler is already added");
            return -EALREADY;
        }
    }

    try {
        info = new HandlerData();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return -ENOMEM;
    }

    info->handler = handler;
    info->type = type;
    info->data = data;
    info->in_use = false;
    info->is_deleted = false;

    handlers.push_back(info);
    return 0;
}

int EventObject::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    std::vector<HandlerData *>::iterator it;

    for (it = handlers.begin(); it != handlers.end(); ++it) {
        HandlerData *info = *it;
        if (info->handler == handler && info->type == type && info->data == data) {
            if (info->is_deleted == true) {
                ErrPrint("Already removed");
                return -EALREADY;
            }

            if (info->in_use == true) {
                info->is_deleted = true;
                return 0;
            }

            handlers.erase(it);
            delete info;
            info = nullptr;
            return 0;
        }
    }

    return -ENOENT;
}

int EventObject::FetchEventData(EventObjectInterface::EventData *&data)
{
    return FetchEventData(data, [](EventObjectInterface *evtObj, int type, EventObjectInterface::EventData *&data) -> beyond_handler_return {
        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    });
}

int EventObject::FetchEventData(EventObjectInterface::EventData *&data, const std::function<beyond_handler_return(EventObjectInterface *, int type, EventObjectInterface::EventData *&)> &eventFetcher)
{
    if (eventFetcher == nullptr) {
        ErrPrint("Invalid argument, eventFetcher is nullptr");
        return -EINVAL;
    }

    int type = beyond_event_type::BEYOND_EVENT_TYPE_NONE;

    if (handle >= 0) {
        int ret;
        fd_set rset;
        fd_set eset;
        fd_set wset;
        timeval tv;

        FD_ZERO(&rset);
        FD_ZERO(&eset);
        FD_ZERO(&wset);

        FD_SET(handle, &rset);
        FD_SET(handle, &eset);
        FD_SET(handle, &wset);

        timerclear(&tv);

        ret = select(handle + 1, &rset, &wset, &eset, &tv);
        if (ret == 0) {
            ErrPrint("No events");
            return -EAGAIN;
        } else if (ret < 0) {
            ret = -errno;
            ErrPrintCode(errno, "select");
            return ret;
        } else {
            if (FD_ISSET(handle, &eset)) {
                ErrPrint("Exception occurred: %d, handle(%d)", ret, handle);
                type |= BEYOND_EVENT_TYPE_ERROR;
            }

            if (FD_ISSET(handle, &rset)) {
                type |= BEYOND_EVENT_TYPE_READ;
            }

            if (FD_ISSET(handle, &wset)) {
                type |= BEYOND_EVENT_TYPE_WRITE;
            }
        }
    } else {
        DbgPrint("Select is disabled");
    }

    if (eventFetcher(this, type, data) == beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL) {
        DbgPrint("EventFetcher returns CANCEL, Skip callback calls");
    } else {
        std::vector<HandlerData *>::iterator it = handlers.begin();
        while (it != handlers.end()) {
            HandlerData *info = *it;

            if ((info->type & data->type) != 0) {
                beyond_handler_return ret;
                beyond_event_info eventInfo;

                eventInfo.type = data->type;
                eventInfo.data = data->data;

                DbgPrint("Invoke added event handler");
                info->in_use = true;
                ret = info->handler(static_cast<beyond_object_h>(this), (data->type & info->type), &eventInfo, info->data);
                info->in_use = false;

                if (ret == BEYOND_HANDLER_RETURN_CANCEL || info->is_deleted == true) {
                    it = handlers.erase(it);
                    delete info;
                    info = nullptr;
                    continue;
                }
            }

            ++it;
        }
    }

    return 0;
}

int EventObject::DestroyEventData(EventObjectInterface::EventData *&data)
{
    delete data;
    data = nullptr;
    return 0;
}

} // namespace beyond
