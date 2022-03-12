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

#ifndef __BEYOND_INTERNAL_EVENT_LOOP_IMPL_H__
#define __BEYOND_INTERNAL_EVENT_LOOP_IMPL_H__

#include <atomic>
#include <functional>
#include <vector>

#include <pthread.h>

#include "beyond/common.h"
#include "beyond/private/event_loop_private.h"

#define EVENT_ID_TIMER static_cast<unsigned int>(0x80000000)

namespace beyond {

class EventLoop::impl final : public EventLoop {
public:
    static impl *Create(bool thread, bool signal);
    void Destroy(void) override;

    HandlerObject *AddEventHandler(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, void *callbackData = nullptr) override;
    HandlerObject *AddEventHandler(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, const std::function<void(EventObjectBaseInterface *, void *)> &cancelHandler, void *callbackData = nullptr) override;
    int RemoveEventHandler(HandlerObject *handlerObject) override;

    int Run(int eventQueueSize = 10, int loop_count = -1, int timeout_in_ms = -1) override;
    int Stop(void) override;

    int SetStopHandler(const std::function<void(EventLoop *, void *)> &stopHandler, void *callbackData = nullptr) override;

public: // EventObjectBaseInterface interface
    int GetHandle(void) const override;

private:
    impl(void);
    ~impl(void);

    struct HandlerObjectImpl final : public EventLoop::HandlerObject {
        enum State : unsigned char {
            INIT = 0x00,
            DELETE = 0x01,
            DELETED = 0x02,
            DELETE_MASK = 0x03,
        };
        std::atomic<State> state;

        std::atomic<bool> in_use;
    };

    int RemoveEventHandler(HandlerObjectImpl *handlerObject);
    int StopLoop(void);

    int handle;

    HandlerObject *spHandlerObject;
    int spfd[2];

    struct Event {
        EventLoop *loop;
        int type;
        std::function<void(EventLoop *, void *)> handler;
        void *callbackData;
    };

    enum EventLoopState : unsigned int {
        STOPPED = 0x00,
        START_REQUESTED = 0x01,
        STARTED = 0x02,
        STOP_REQUESTED = 0x04,
    };

    pthread_mutex_t eventLoopStateMutex;
    EventLoopState eventLoopState;
    bool enable_thread;

    pthread_t main_thid;
    pthread_t svc_thid;

    std::atomic<Event *> stopEventObject;

    long WaitEvent(struct kevent *events, int count, int timeout);
    static beyond_handler_return CtrlEventHandler(EventObjectBaseInterface *eventObject, int type, void *data);
    EventLoop::HandlerObject *AddEventHandlerInternal(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, void *callbackData);

    int epollWaitSize;
    int loopCount;
    int timeoutInMS;
    static void *ThreadMain(void *callbackData);

    bool enable_signal;
    // Only one event loop can handle the signal
    static std::atomic_flag signalLoop;

    std::atomic_flag destructed;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_EVENT_LOOP_IMPL_H__
