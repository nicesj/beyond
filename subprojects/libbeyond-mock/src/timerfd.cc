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

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstring>

#include <dlfcn.h>
#include <sys/timerfd.h>

#include "beyond/mock/mock.h"
#include "beyond/mock/mock_ctrl.h"

extern "C" {
int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags, const itimerspec *new_value,
                    itimerspec *old_value);
int timerfd_gettime(int fd, itimerspec *curr_value);
}

int timerfd_create(int clockid, int flags)
{
    CTRL_CLIENT(
        timerfd_create, arg,
        ((arg->clockid == clockid || arg->clockid == mock::Type::AnyInt) &&
         (arg->flags == flags || arg->flags == mock::Type::AnyInt)),
        val_int, (clockid, flags));

    assert(!"No matched function found - timerfd_create");
    return -1;
}

int timerfd_settime(int fd, int flags, const itimerspec *new_value,
                    itimerspec *old_value)
{
    CTRL_CLIENT(timerfd_settime, arg,
                ((arg->fd == fd || arg->fd == mock::Type::AnyInt) &&
                 (arg->flags == flags || arg->flags == mock::Type::AnyInt) &&
                 (arg->new_value == new_value ||
                  arg->new_value == reinterpret_cast<const itimerspec *>(
                                        mock::Type::AnyPtr)) &&
                 (arg->old_value == old_value ||
                  arg->old_value == reinterpret_cast<const itimerspec *>(
                                        mock::Type::AnyPtr))),
                val_int, (fd, flags, new_value, old_value));

    assert(!"No matched function found - timerfd_settime");
    return -1;
}

int timerfd_gettime(int fd, itimerspec *curr_value)
{
    CTRL_CLIENT(timerfd_gettime, arg,
                ((arg->fd == fd || arg->fd == mock::Type::AnyInt) &&
                 (arg->curr_value == curr_value ||
                  arg->curr_value == reinterpret_cast<const itimerspec *>(
                                         mock::Type::AnyPtr))),
                val_int, (fd, curr_value));

    assert(!"No matched function found - timerfd_gettime");
    return -1;
}
