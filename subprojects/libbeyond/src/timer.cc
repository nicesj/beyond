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
#include <cstring>
#include <ctime>
#include <cassert>
#include <exception>

#include <libgen.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/timer_private.h"

#include "timer_impl.h"

namespace beyond {

// Caution: Create() is not a method of TimerImpls
Timer *Timer::Create(void)
{
    return static_cast<Timer *>(Timer::impl::Create());
}

Timer::Timer(int fd)
    : EventObject(fd)
{
}

} // namespace beyond
