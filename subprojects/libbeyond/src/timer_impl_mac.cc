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

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cassert>
#include <exception>

#include <libgen.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/timer_private.h"

#include "event_loop_impl_mac.h" // EVENT_ID_TIMER
#include "timer_impl_mac.h"

namespace beyond {

Timer::impl *Timer::impl::Create(void)
{
    Timer::impl *inst;

    try {
        inst = new Timer::impl(EVENT_ID_TIMER);
    } catch (std::exception &e) {
        ErrPrint("new timer: %s", e.what());
        return nullptr;
    }

    return inst;
}

void Timer::impl::Destroy(void)
{
    delete this;
}

Timer::impl::impl(int fd)
    : Timer(fd)
    , registeredTime(0.0f)
    , sequenceId(0)
{
}

int Timer::impl::SetTimer(double timer)
{
    DbgPrint("Armed interval: %lf", timer);
    registeredTime = timer;
    return 0;
}

double Timer::impl::GetTimer(void) const
{
    return registeredTime;
}

int Timer::impl::GetHandle(void) const
{
    return EventObject::GetHandle();
}

int Timer::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    // TODO: Implement this
    return EventObject::AddHandler(handler, type, data);
}

int Timer::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    // TODO: Implement this
    return EventObject::RemoveHandler(handler, type, data);
}

int Timer::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    Timer::EventData *eventData;

    try {
        eventData = new Timer::EventData();
    } catch (std::exception &e) {
        ErrPrint("new eventData: %s", e.what());
        return -ENOMEM;
    }

    int ret;

    EventObjectInterface::EventData *iEventData = static_cast<EventObjectInterface::EventData *>(eventData);
    ret = EventObject::FetchEventData(iEventData);
    if (ret < 0) {
        delete eventData;
        eventData = nullptr;
        return ret;
    }

    eventData->sequenceId = sequenceId++;

    // errno == EAGAIN means that there are no expired timers
    data = eventData;
    return 0;
}

int Timer::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    delete data;
    data = nullptr;
    return 0;
}

} // namespace beyond
