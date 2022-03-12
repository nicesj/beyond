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
#include <cstdint>
#include <cassert>
#include <cstring>

#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "beyond/mock/mock.h"
#include "beyond/mock/mock_ctrl.h"

extern "C" {
int socket(int domain, int type, int protocol);
int connect(int sockfd, const sockaddr *addr, socklen_t addrlen);
int accept(int sockfd, sockaddr *restrict address,
           socklen_t *restrict address_len);
int listen(int sockfd, int backlog);
int bind(int sockfd, const sockaddr *addr, socklen_t addrlen);
int setsockopt(int sockfd, int level, int option_name, const void *option_value,
               socklen_t option_len);
int shutdown(int sockfd, int how);
}

int socket(int domain, int type, int protocol)
{
    CTRL_CLIENT(
        socket, arg,
        ((arg->domain == domain || arg->domain == mock::Type::AnyInt) &&
         (arg->type == type || arg->type == mock::Type::AnyInt) &&
         (arg->protocol == protocol || arg->protocol == mock::Type::AnyInt)),
        val_int, (domain, type, protocol));

    assert(!"No matched function found - socket");
    return -1;
}

int connect(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    CTRL_CLIENT(connect, arg,
                ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
                 (arg->addr == addr ||
                  arg->addr ==
                      reinterpret_cast<const sockaddr *>(mock::Type::AnyPtr)) &&
                 (arg->addrlen == addrlen ||
                  arg->addrlen == static_cast<socklen_t>(mock::Type::AnyInt))),
                val_int, (sockfd, addr, addrlen));

    assert(!"No matched function found - connect");
    return -1;
}

int accept(int sockfd, sockaddr *restrict addr, socklen_t *restrict addrlen)
{
    CTRL_CLIENT(
        accept, arg,
        ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
         (arg->addr == addr || arg->addr == reinterpret_cast<const sockaddr *>(
                                                mock::Type::AnyPtr)) &&
         (arg->addrlen == addrlen ||
          arg->addrlen == static_cast<const socklen_t *>(mock::Type::AnyPtr))),
        val_int, (sockfd, addr, addrlen));

    assert(!"No matched function found - accept");
    return -1;
}

int listen(int sockfd, int backlog)
{
    CTRL_CLIENT(
        listen, arg,
        ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
         (arg->backlog == backlog || arg->backlog == mock::Type::AnyInt)),
        val_int, (sockfd, backlog));

    assert(!"No matched function found - listen");
    return -1;
}

int bind(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    CTRL_CLIENT(bind, arg,
                ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
                 (arg->addr == addr ||
                  arg->addr ==
                      reinterpret_cast<const sockaddr *>(mock::Type::AnyPtr)) &&
                 (arg->addrlen == addrlen ||
                  arg->addrlen == static_cast<socklen_t>(mock::Type::AnyInt))),
                val_int, (sockfd, addr, addrlen));

    assert(!"No matched function found - bind");
    return -1;
}

int setsockopt(int sockfd, int level, int option_name, const void *option_value,
               socklen_t option_len)
{
    CTRL_CLIENT(
        setsockopt, arg,
        ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
         (arg->level == level || arg->level == mock::Type::AnyInt) &&
         (arg->option_name == option_name ||
          arg->option_name == mock::Type::AnyInt) &&
         (arg->option_value == option_value ||
          arg->option_value == mock::Type::AnyPtr) &&
         (arg->option_len == option_len ||
          arg->option_len == static_cast<socklen_t>(mock::Type::AnyInt))),
        val_int, (sockfd, level, option_name, option_value, option_len));

    assert(!"No matched function found - setsockopt");
    return -1;
}

int shutdown(int sockfd, int how)
{
    CTRL_CLIENT(shutdown, arg,
                ((arg->sockfd == sockfd || arg->sockfd == mock::Type::AnyInt) &&
                 (arg->how == how || arg->how == mock::Type::AnyInt)),
                val_int, (sockfd, how));

    assert(!"No matched function found - shutdown");
    return -1;
}
