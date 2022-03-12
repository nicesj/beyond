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

#ifndef __BEYOND_MOCK_SOCKET_CTRL_H__
#define __BEYOND_MOCK_SOCKET_CTRL_H__

#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timerfd.h>

namespace mock {
class Ctrl {
public:
    // socket
    struct args_socket {
        int domain;
        int type;
        int protocol;
    };

    struct args_connect {
        int sockfd;
        const sockaddr *addr;
        socklen_t addrlen;
    };

    struct args_accept {
        int sockfd;
        sockaddr *addr;
        socklen_t *addrlen;
    };

    struct args_listen {
        int sockfd;
        int backlog;
    };

    struct args_bind {
        int sockfd;
        const sockaddr *addr;
        socklen_t addrlen;
    };

    struct args_setsockopt {
        int sockfd;
        int level;
        int option_name;
        const void *option_value;
        socklen_t option_len;
    };

    struct args_shutdown {
        int sockfd;
        int how;
    };

    // epoll
    struct args_epoll_create1 {
        int flags;
    };

    struct args_epoll_wait {
        int epfd;
        epoll_event *events;
        int maxevents;
        int timeout;
    };

    struct args_epoll_ctl {
        int epfd;
        int op;
        int fd;
        epoll_event *event;
    };

    // timerfd
    struct args_timerfd_create {
        int clockid;
        int flags;
    };

    struct args_timerfd_settime {
        int fd;
        int flags;
        const itimerspec *new_value;
        itimerspec *old_value;
    };

    struct args_timerfd_gettime {
        int fd;
        itimerspec *curr_value;
    };

    struct args_socketpair {
        int domain;
        int type;
        int protocol;
        int sv[2];
    };

    struct Expect {
        const char *name;
        union ReturnValue {
            void *val_voidptr;
            int val_int;
        } ret;
        void *args;
        int count;
        Expect *next;

        Expect();
        virtual ~Expect();

        void (*delete_args)(void *args);
        bool use_native;

        Expect &Return(void *ret);
        Expect &Return(int ret);
        Expect &Times(int count);
        Expect &UseNative(void);
    };

    static Ctrl *instance;

    Expect *head;
    Expect *tail;

    bool fallbackNative;

    explicit Ctrl(bool fallback = false);
    virtual ~Ctrl(void);

    // socket
    Expect &socket(int domain, int type, int protocol);
    Expect &connect(int sockfd, const sockaddr *addr, socklen_t addrlen);
    Expect &accept(int socket, sockaddr *restrict address,
                   socklen_t *restrict address_len);
    Expect &listen(int sockfd, int backlog);
    Expect &bind(int sockfd, const sockaddr *addr, socklen_t addrlen);
    Expect &setsockopt(int socket, int level, int option_name,
                       const void *option_value, socklen_t option_len);
    Expect &shutdown(int sockfd, int how);

    // timerfd
    Expect &timerfd_create(int clockid, int flags);
    Expect &timerfd_settime(int fd, int flags, const itimerspec *new_value,
                            itimerspec *old_value);
    Expect &timerfd_gettime(int fd, itimerspec *curr_value);

    // epoll
    Expect &epoll_create1(int flags);
    Expect &epoll_wait(int epfd, epoll_event *events, int maxevents,
                       int timeout);
    Expect &epoll_ctl(int epfd, int op, int fd, epoll_event *event);

    // socketpair
    Expect &socketpair(int domain, int type, int protocol, int sv[2]);
};
} // namespace mock

#endif // __BEYOND_MOCK_SOCKET_CTRL_H__
