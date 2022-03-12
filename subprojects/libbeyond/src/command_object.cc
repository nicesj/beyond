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
#include <cerrno>
#include <cstring>

#include <exception>

#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/event_object_interface_private.h"
#include "beyond/private/command_object_interface_private.h"
#include "beyond/private/command_object_private.h"

namespace beyond {

CommandObject::CommandObject(int _handle)
    : handle(_handle)
{
}

int CommandObject::GetHandle(void) const
{
    return handle;
}

CommandObject::CommandData *CommandObject::NewCommandData(int id, void *data)
{
    CommandObject::CommandData *cmd;

    try {
        cmd = new CommandObject::CommandData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    cmd->id = id;
    cmd->data = data;
    return cmd;
}

void CommandObject::DeleteCommandData(CommandObject::CommandData *&cmd)
{
    delete cmd;
    cmd = nullptr;
}

int CommandObject::Send(int id, void *data)
{
    if (handle < 0) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    CommandObject::CommandData *cmd = NewCommandData(id, data);
    if (cmd == nullptr) {
        ErrPrint("NewCommandData");
        return -ENOMEM;
    }

    int ret = write(handle, &cmd, sizeof(cmd));
    if (ret < 0) {
        ret = -errno;
        ErrPrintCode(errno, "write");
        DeleteCommandData(cmd);
        return ret;
    } else if (ret != sizeof(cmd)) {
        DbgPrint("[TryAgain] Failed to write data an address size: %d", ret);
        assert(!"Failed to write data an address size");
        int again = write(handle, reinterpret_cast<char *>(&cmd) + ret, sizeof(cmd) - ret);
        if (again != (static_cast<int>(sizeof(cmd)) - ret)) {
            ErrPrint("Failed to write data an address size: %d", again);
            DeleteCommandData(cmd);
            return -EFAULT;
        }
    }

    return 0;
}

int CommandObject::Recv(int &id)
{
    void *data;
    return Recv(id, data);
}

int CommandObject::Recv(int &id, void *&data)
{
    if (handle < 0) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    CommandObject::CommandData *cmd = nullptr;
    int ret = read(handle, &cmd, sizeof(cmd));
    if (ret < 0) {
        ret = -errno;
        ErrPrintCode(errno, "read");
        return ret;
    } else if (ret != sizeof(cmd)) {
        DbgPrint("[TryAgain] Failed to read data an address size: %d", ret);
        assert(!"Failed to read data an address size");
        int again = read(handle, reinterpret_cast<char *>(&cmd) + ret, sizeof(cmd) - ret);
        if (again != (static_cast<int>(sizeof(cmd)) - ret)) {
            ErrPrint("Failed to read data an address size: %d", again);
            return -EFAULT;
        }
    }

    if (cmd == nullptr) {
        ErrPrint("Invalid command received");
        assert(!"Invalid command received");
        return -EFAULT;
    }

    id = cmd->id;
    data = cmd->data;

    delete cmd;
    cmd = nullptr;
    return 0;
}

} // namespace beyond
