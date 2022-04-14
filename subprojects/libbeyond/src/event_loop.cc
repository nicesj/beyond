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

#include <libgen.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/event_loop_private.h"

#if defined(__APPLE__)
#include "event_loop_impl_mac.h"
#else
#include "event_loop_impl.h"
#endif

namespace beyond {

EventLoop *EventLoop::Create(bool thread, bool signal)
{
    return EventLoop::impl::Create(thread, signal);
}

beyond::EventLoop::HandlerObject::HandlerObject()
    : eventObject(nullptr)
    , type(0)
    , data(nullptr)
    , eventHandler(nullptr)
    , cancelHandler(nullptr)
{
}
} // namespace beyond
