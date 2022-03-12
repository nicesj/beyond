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
#include "nsdeventloop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <stdexcept>
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

NsdEventLoop::NsdEventLoop()
    : looprun_fd(-1)
{
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        ErrPrintCode(errno, "epoll_create1");
        throw std::runtime_error("epoll_create1");
    }
}

NsdEventLoop::~NsdEventLoop()
{
    quit();
    close(epoll_fd);
}

void NsdEventLoop::addWatch(int fd, eventCallback cb)
{
    auto result = callbackMap.insert(std::make_pair(fd, cb));
    if (result.second == false) {
        ErrPrint("Invalid fd(%d), already exists", fd);
        throw NsdLoopException(-EEXIST);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    ev.data.fd = fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        ErrPrintCode(errno, "epoll_fd() Fail");
        callbackMap.erase(result.first);
        throw NsdLoopException(-errno);
    }
}

void NsdEventLoop::delWatch(int fd)
{
    auto result = callbackMap.find(fd);
    if (result == callbackMap.end()) {
        ErrPrint("Invalid fd(%d), not found", fd);
        throw NsdLoopException(-ENOENT);
    }
    callbackMap.erase(result);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void NsdEventLoop::run()
{
    looprun_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (looprun_fd < 0) {
        ErrPrintCode(errno, "eventfd() Fail");
        throw NsdLoopException(-EFAULT);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    ev.data.fd = looprun_fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, looprun_fd, &ev);
    if (ret < 0) {
        ErrPrintCode(errno, "epoll_ctl() Fail");
        close(looprun_fd);
        looprun_fd = -1;
        throw NsdLoopException(-errno);
    }

    loop = std::thread([this]() {
        bool isLoopRun = true;
        while (isLoopRun) {
            struct epoll_event events[2];
            int event_count = epoll_pwait(epoll_fd, events, 1, -1, nullptr);
            if (event_count > 0) {
                for (int i = 0; i < event_count; i++) {
                    if (events[i].data.fd == looprun_fd) {
                        isLoopRun = false;
                        break;
                    } else {
                        auto result = callbackMap.find(events[i].data.fd);
                        if (result == callbackMap.end()) {
                            ErrPrint("Invalid fd(%d), not found", events[i].data.fd);
                        }
                        result->second();
                    }
                }
            }
        }
    });
}

void NsdEventLoop::quit()
{
    if (looprun_fd == -1) {
        ErrPrint("No Event Loop");
        return;
    }

    uint64_t u = 1;
    int ret = write(looprun_fd, &u, sizeof(u));
    if (ret < 0) {
        ErrPrintCode(errno, "write() Fail");
        throw NsdLoopException(-errno);
    }
    loop.join();

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, looprun_fd, NULL);
    close(looprun_fd);
    looprun_fd = -1;
}

NsdLoopException::NsdLoopException(int ret)
    : runtime_error("Fail in NsdEventLoop")
    , returnValue(ret)
{
}
