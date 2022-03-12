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
// In case of the mock,
// the assert() must not be filtered
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>

#include <dlfcn.h>
#include <sys/epoll.h>

#include "beyond/mock/mock.h"
#include "beyond/mock/mock_ctrl.h"

extern "C" {
int epoll_create1(int flags);
int epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout);
int epoll_ctl(int epfd, int op, int fd, epoll_event *event);
}

int epoll_create1(int flags)
{
    CTRL_CLIENT(epoll_create1, arg,
                ((arg->flags == flags || arg->flags == mock::Type::AnyInt)),
                val_int, (flags));

    assert(!"No matched function found - epoll_create1");
    return -1;
}

int epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout)
{
    CTRL_CLIENT(
        epoll_wait, arg,
        ((arg->epfd == epfd || arg->epfd == mock::Type::AnyInt) &&
         (arg->events == events ||
          arg->events ==
              reinterpret_cast<const epoll_event *>(mock::Type::AnyPtr)) &&
         (arg->maxevents == maxevents ||
          arg->maxevents == mock::Type::AnyInt) &&
         (arg->timeout == timeout || arg->timeout == mock::Type::AnyInt)),
        val_int, (epfd, events, maxevents, timeout));

    assert(!"No matched function found - epoll_wait");
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, epoll_event *event)
{
    CTRL_CLIENT(epoll_ctl, arg,
                ((arg->epfd == epfd || arg->epfd == mock::Type::AnyInt) &&
                 (arg->op == op || arg->op == mock::Type::AnyInt) &&
                 (arg->fd == fd || arg->fd == mock::Type::AnyInt) &&
                 (arg->event == event ||
                  arg->event == reinterpret_cast<const epoll_event *>(
                                    mock::Type::AnyPtr))),
                val_int, (epfd, op, fd, event));
    assert(!"No matched function found - epoll_ctl");
    return -1;
}
