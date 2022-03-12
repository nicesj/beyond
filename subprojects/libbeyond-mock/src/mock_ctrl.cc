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

#include <cstdio>
#include <new>
#include <cassert>

#include <dlfcn.h>
#include <sys/socket.h>

#include "beyond/mock/mock.h"
#include "beyond/mock/mock_ctrl.h"

namespace mock {

Ctrl *Ctrl::instance = nullptr;

Ctrl::Expect::Expect()
    : name(nullptr)
    , ret{ nullptr }
    , args(nullptr)
    , count(0)
    , next(nullptr)
    , delete_args(nullptr)
    , use_native(false)
{
}

Ctrl::Expect::~Expect()
{
    if (this->delete_args != nullptr) {
        this->delete_args(this->args);
    }
}

Ctrl::Ctrl(bool fallback)
    : head(nullptr)
    , tail(nullptr)
    , fallbackNative(fallback)
{
    if (instance != nullptr) {
        throw "Instance is in use";
    }

    instance = this;
}

Ctrl::~Ctrl()
{
    Ctrl::Expect *i;
    Ctrl::Expect *n;

    instance = nullptr;

    i = head;
    do {
        n = i->next;

        fprintf(stderr, "NAME: %s, COUNT: %d\n", i->name, i->count);
        assert(i->count == 0);

        delete i;

        i = n;
    } while (i != nullptr);
}

Ctrl::Expect &Ctrl::Expect::Return(void *value)
{
    this->ret.val_voidptr = value;
    return *this;
}

Ctrl::Expect &Ctrl::Expect::Return(int value)
{
    this->ret.val_int = value;
    return *this;
}

Ctrl::Expect &Ctrl::Expect::Times(int count)
{
    this->count = count;
    return *this;
}

Ctrl::Expect &Ctrl::Expect::UseNative(void)
{
    this->use_native = true;
    return *this;
}

Ctrl::Expect &Ctrl::socket(int domain, int type, int protocol)
{
    Ctrl::Expect *node;
    Ctrl::args_socket *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_socket();
    args->domain = domain;
    args->type = type;
    args->protocol = protocol;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_socket *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::connect(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    Ctrl::Expect *node;
    Ctrl::args_connect *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_connect();
    args->sockfd = sockfd;
    args->addr = addr;
    args->addrlen = addrlen;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_connect *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::accept(int sockfd, sockaddr *restrict address,
                           socklen_t *restrict address_len)
{
    Ctrl::Expect *node;
    Ctrl::args_accept *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_accept();
    args->sockfd = sockfd;
    args->addr = address;
    args->addrlen = address_len;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_accept *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::listen(int sockfd, int backlog)
{
    Ctrl::Expect *node;
    Ctrl::args_listen *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_listen();
    args->sockfd = sockfd;
    args->backlog = backlog;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_listen *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::bind(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    Ctrl::Expect *node;
    Ctrl::args_bind *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_bind();
    args->sockfd = sockfd;
    args->addr = addr;
    args->addrlen = addrlen;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_bind *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::setsockopt(int sockfd, int level, int option_name,
                               const void *option_value, socklen_t option_len)
{
    Ctrl::Expect *node;
    Ctrl::args_setsockopt *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_setsockopt();
    args->sockfd = sockfd;
    args->level = level;
    args->option_name = option_name;
    args->option_value = option_value;
    args->option_len = option_len;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_setsockopt *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::shutdown(int sockfd, int how)
{
    Ctrl::Expect *node;
    Ctrl::args_shutdown *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_shutdown();
    args->sockfd = sockfd;
    args->how = how;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_shutdown *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::timerfd_create(int clockid, int flags)
{
    Ctrl::Expect *node;
    Ctrl::args_timerfd_create *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_timerfd_create();
    args->clockid = clockid;
    args->flags = flags;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_timerfd_create *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::timerfd_settime(int fd, int flags,
                                    const itimerspec *new_value,
                                    itimerspec *old_value)
{
    Ctrl::Expect *node;
    Ctrl::args_timerfd_settime *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_timerfd_settime();
    args->fd = fd;
    args->flags = flags;
    args->new_value = new_value;
    args->old_value = old_value;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_timerfd_settime *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::timerfd_gettime(int fd, itimerspec *curr_value)
{
    Ctrl::Expect *node;
    Ctrl::args_timerfd_gettime *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_timerfd_gettime();
    args->fd = fd;
    args->curr_value = curr_value;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_timerfd_gettime *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::epoll_create1(int flags)
{
    Ctrl::Expect *node;
    Ctrl::args_epoll_create1 *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_epoll_create1();
    args->flags = flags;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_epoll_create1 *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::epoll_wait(int epfd, epoll_event *events, int maxevents,
                               int timeout)
{
    Ctrl::Expect *node;
    Ctrl::args_epoll_wait *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_epoll_wait();
    args->epfd = epfd;
    args->events = events;
    args->maxevents = maxevents;
    args->timeout = timeout;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_epoll_wait *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::epoll_ctl(int epfd, int op, int fd, epoll_event *event)
{
    Ctrl::Expect *node;
    Ctrl::args_epoll_ctl *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_epoll_ctl();
    args->epfd = epfd;
    args->op = op;
    args->fd = fd;
    args->event = event;
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_epoll_ctl *>(args);
    };

    return *node;
}

Ctrl::Expect &Ctrl::socketpair(int domain, int type, int protocol, int sv[2])
{
    Ctrl::Expect *node;
    Ctrl::args_socketpair *args;

    try {
        node = new Expect();
    } catch (std::bad_alloc &) {
        throw;
    }

    node->next = nullptr;
    node->name = __func__;

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    args = new args_socketpair();
    args->domain = domain;
    args->type = type;
    args->protocol = protocol;
    args->sv[0] = sv[0];
    args->sv[1] = sv[1];
    node->args = static_cast<void *>(args);

    node->delete_args = [](void *args) {
        delete static_cast<args_socketpair *>(args);
    };

    return *node;
}

} // namespace mock
