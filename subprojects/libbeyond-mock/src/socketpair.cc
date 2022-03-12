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
#include <sys/socket.h>
#include <sys/types.h>

#include "beyond/mock/mock.h"
#include "beyond/mock/mock_ctrl.h"

extern "C" {
int socketpair(int domain, int type, int protocol, int sv[2]);
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
    CTRL_CLIENT(
        socketpair, arg,
        ((arg->domain == domain || arg->domain == mock::Type::AnyInt) &&
         (arg->type == type || arg->type == mock::Type::AnyInt) &&
         (arg->protocol == protocol || arg->protocol == mock::Type::AnyInt) &&
         (arg->sv[0] == sv[0] || arg->sv[0] == mock::Type::AnyInt) &&
         (arg->sv[1] == sv[1] || arg->sv[1] == mock::Type::AnyInt)),
        val_int, (domain, type, protocol, sv));

    assert(!"No matched function found - socketapri");
    return -1;
}
