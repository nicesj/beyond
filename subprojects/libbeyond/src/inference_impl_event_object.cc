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
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/event_object_private.h"

#include "inference_impl.h"
#include "inference_impl_remote.h"
#include "inference_impl_event_object.h"

namespace beyond {

Inference::impl::remote::EventObject::EventObject(int fd)
    : beyond::EventObject(fd)
    , publishHandle(-1)
{
}

Inference::impl::remote::EventObject *Inference::impl::remote::EventObject::Create(void)
{
    Inference::impl::remote::EventObject *impl;
    int pfd[2];

    // pfd is going to be used for event system of co-inference remote type
    if (pipe(pfd) < 0) {
        ErrPrintCode(errno, "pipe");
        return nullptr;
    }

    if (fcntl(pfd[0], F_SETFD, FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    if (fcntl(pfd[1], F_SETFD, FD_CLOEXEC) < 0) {
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
        impl = new Inference::impl::remote::EventObject(pfd[0]);
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

int Inference::impl::remote::EventObject::FetchEventData(EventObjectInterface::EventData *&evtData)
{
    return beyond::EventObject::FetchEventData(evtData, [](EventObjectInterface *evtObj, int type, EventObjectInterface::EventData *&evtData) -> beyond_handler_return {
        EventObjectInterface::EventData *inferenceEventData = nullptr;
        int ret;

        if (evtData != nullptr) {
            assert(!"evtData must be nullptr");
            ErrPrint("evtData must be nullptr");
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        }

        ret = read(evtObj->GetHandle(), &inferenceEventData, sizeof(inferenceEventData));
        if (ret < 0 || inferenceEventData == nullptr) {
            ErrPrintCode(errno, "read");
            try {
                evtData = new EventObjectInterface::EventData();
            } catch (std::exception &e) {
                ErrPrint("new: %s", e.what());
                return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
            }

            evtData->type = beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
        } else {
            evtData = inferenceEventData;
            evtData->type |= type;
        }

        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    });
}

int Inference::impl::remote::EventObject::PublishEventData(EventObjectInterface::EventData *evtData)
{
    if (write(publishHandle, &evtData, sizeof(EventObjectInterface::EventData *)) < 0) {
        int ret = -errno;
        ErrPrintCode(errno, "write");
        return ret;
    }

    return 0;
}

void Inference::impl::remote::EventObject::Destroy(void)
{
    if (close(GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(publishHandle) < 0) {
        ErrPrintCode(errno, "close");
    }

    delete this;
}

} // namespace beyond
