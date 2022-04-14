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

#include "authenticator.h"
#include "authenticator_event_object.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <memory>

#include <fcntl.h>
#include <unistd.h>

Authenticator::EventObject::EventObject(int fd)
    : beyond::EventObject(fd)
    , publishHandle(-1)
{
}

Authenticator::EventObject *Authenticator::EventObject::Create(void)
{
    Authenticator::EventObject *impl;
    int pfd[2];

    // pfd is going to be used for event system of co-inference remote type
    if (pipe(pfd) < 0) {
        ErrPrintCode(errno, "pipe");
        return nullptr;
    }

    if (fcntl(F_SETFD, pfd[0], FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    if (fcntl(F_SETFD, pfd[1], FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    try {
        impl = new Authenticator::EventObject(pfd[0]);
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    impl->publishHandle = pfd[1];
    return impl;
}

int Authenticator::EventObject::FetchEventData(EventObjectInterface::EventData *&evtData)
{
    return beyond::EventObject::FetchEventData(evtData, [](EventObjectInterface *evtObj, int type, EventObjectInterface::EventData *&evtData) -> beyond_handler_return {
        Authenticator::EventObject *peer = static_cast<Authenticator::EventObject *>(evtObj);
        EventObjectInterface::EventData *fetchEventData = nullptr;
        int ret;

        ret = read(peer->GetHandle(), &fetchEventData, sizeof(fetchEventData));
        if (ret < 0 || fetchEventData == nullptr) {
            try {
                fetchEventData = new EventObjectInterface::EventData();
            } catch (std::exception &e) {
                ErrPrint("new: %s", e.what());
                return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
            }

            fetchEventData->type = beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
        } else {
            fetchEventData->type |= type;
        }

        evtData = fetchEventData;

        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    });
}

int Authenticator::EventObject::PublishEventData(int type, void *data)
{
    EventObjectInterface::EventData *evtData;

    try {
        evtData = new EventObjectInterface::EventData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    evtData->type = type;
    evtData->data = data;

    if (write(publishHandle, &evtData, sizeof(evtData)) != sizeof(evtData)) {
        int ret = -errno;
        ErrPrintCode(errno, "write");
        delete evtData;
        evtData = nullptr;
        return ret;
    }

    return 0;
}

void Authenticator::EventObject::Destroy(void)
{
    if (close(GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(publishHandle) < 0) {
        ErrPrintCode(errno, "close");
    }

    delete this;
}
