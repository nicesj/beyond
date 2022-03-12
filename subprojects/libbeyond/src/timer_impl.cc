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
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/timer_private.h"

#include "timer_impl.h"

namespace beyond {

Timer::impl *Timer::impl::Create(void)
{
    Timer::impl *inst;

    int ret = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (ret < 0) {
        ErrPrintCode(errno, "timerfd_create");
        return nullptr;
    }

    try {
        inst = new Timer::impl(ret);
    } catch (std::exception &e) {
        ErrPrint("new timer: %s", e.what());
        if (close(ret) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    return inst;
}

void Timer::impl::Destroy(void)
{
    if (close(GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    delete this;
}

Timer::impl::impl(int fd)
    : Timer(fd)
    , registeredTime(0.0f)
{
}

int Timer::impl::SetTimer(double timer)
{
    struct itimerspec spec;

    spec.it_interval.tv_sec = (time_t)timer;
    spec.it_interval.tv_nsec = (timer - spec.it_interval.tv_sec) * 1000000000;

    if (clock_gettime(CLOCK_MONOTONIC, &spec.it_value) < 0) {
        int ret = -errno;
        ErrPrintCode(errno, "clock_gettime");
        return ret;
    }

    spec.it_value.tv_sec += spec.it_interval.tv_sec;
    spec.it_value.tv_nsec += spec.it_interval.tv_nsec;

    if (timerfd_settime(GetHandle(), TFD_TIMER_ABSTIME, &spec, nullptr) < 0) {
        int ret = -errno;
        ErrPrintCode(errno, "timerfd_settime");
        return ret;
    }

    registeredTime = timer;
    DbgPrint("Armed interval: %lu.%.9lu", spec.it_interval.tv_sec, spec.it_interval.tv_nsec);
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

    ret = read(GetHandle(), &eventData->sequenceId, sizeof(uint64_t));
    if (ret < 0 && errno != EAGAIN) {
        ret = -errno;
        ErrPrintCode(errno, "read");
        delete eventData;
        eventData = nullptr;
        return ret;
    }

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
