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
#define _GNU_SOURCE // See feature_test_macros(7)
#endif

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <memory>
#include <cassert>
#include <exception>
#include <functional>

#include <fcntl.h> // Obtain O_* constant definitions
#include <libgen.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/event_loop_private.h"

#include "event_loop_impl_mac.h"
#include "timer_impl_mac.h"

#define SP_CTRL 0
#define SP_CTRL_EVT_TERMINATE 'x'
#define SP_CTRL_EVT_EVENT 'e'
#define SP_NOTI 1
#define SP_NOTI_EVT_DONE 'd'
#define MAX_NR_OF_EPOLL_EVENTS 10

#define THREAD_SCOPE_MAIN
#define THREAD_SCOPE_SVC

#define MUTEX_LOCK(v)                                \
    do {                                             \
        int ret = pthread_mutex_lock(v);             \
        if (ret != 0) {                              \
            ErrPrintCode(ret, "pthread_mutex_lock"); \
        }                                            \
    } while (0)

#define MUTEX_UNLOCK(v)                                \
    do {                                               \
        int ret = pthread_mutex_unlock(v);             \
        if (ret != 0) {                                \
            ErrPrintCode(ret, "pthread_mutex_unlock"); \
        }                                              \
    } while (0)

namespace beyond {

std::atomic_flag EventLoop::impl::signalLoop = ATOMIC_FLAG_INIT;

THREAD_SCOPE_SVC
beyond_handler_return EventLoop::impl::CtrlEventHandler(EventObjectBaseInterface *eventObject, int type, void *data)
{
    EventLoop::impl *pimpl = static_cast<EventLoop::impl *>(data);

    if ((type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) == beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
        DbgPrint("CtrlEventHandler returns with %X", type);
        return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
    }

    Event *evt;
    int ret;

    ret = read(eventObject->GetHandle(), &evt, sizeof(evt));
    if (ret != sizeof(evt)) {
        ErrPrintCode(errno, "read: %d", ret);
        return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
    }

    // NOTE:
    // Only the Stop() can send null event object, if there is no registered
    // StopHandler.
    if (evt == nullptr || evt->type == SP_CTRL_EVT_TERMINATE) {
        // Terminate loop
        InfoPrint("Let's stop the loop: %p (0x%X)", static_cast<void *>(evt), pimpl->eventLoopState);
    } else if (evt->type == SP_CTRL_EVT_EVENT) {
        // Do something..
        DbgPrint("CtrlEventHandler reads %p event", static_cast<void *>(evt));
    } else {
        DbgPrint("CtrlEventHandler reads %p %X", static_cast<void *>(evt), evt->type);
    }

    return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
}

THREAD_SCOPE_MAIN
EventLoop::impl::impl(void)
    : handle(-1)
    , spHandlerObject(nullptr)
    , spfd{ -1, -1 }
    , eventLoopStateMutex(PTHREAD_MUTEX_INITIALIZER)
    , eventLoopState(EventLoopState::STOPPED)
    , enable_thread(false)
    , main_thid(pthread_self())
    , svc_thid(pthread_self())
    , epollWaitSize(-1)
    , loopCount(-1)
    , timeoutInMS(-1)
    , enable_signal(false)
{
    stopEventObject.store(nullptr);
    destructed.clear();
}

THREAD_SCOPE_SVC
EventLoop::impl::~impl(void)
{
    // NOTE:
    // We are able to be sure that the thread is stopped already
    // (in case of the enable_thread is true)

    if (spHandlerObject != nullptr) {
        EventObjectBaseInterface *eventObject = spHandlerObject->eventObject;

        RemoveEventHandler(spHandlerObject);
        spHandlerObject = nullptr;

        // NOTE:
        // The 'handle' of the eventObject will be closed by the following close() function call.
        delete eventObject;
        eventObject = nullptr;
    }

    if (spfd[SP_CTRL] >= 0 && close(spfd[SP_CTRL]) < 0) {
        ErrPrintCode(errno, "close");
    }
    spfd[SP_CTRL] = -1;

    if (spfd[SP_NOTI] >= 0 && close(spfd[SP_NOTI]) < 0) {
        ErrPrintCode(errno, "close");
    }
    spfd[SP_NOTI] = -1;

    if (handle >= 0 && close(handle) < 0) {
        ErrPrintCode(errno, "close");
    }
    handle = -1;

    if (enable_signal == true) {
        signalLoop.clear();
    }

    int ret = pthread_mutex_destroy(&eventLoopStateMutex);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_mutex_destroy");
    }
}

THREAD_SCOPE_MAIN
EventLoop::impl *EventLoop::impl::Create(bool thread, bool signal)
{
    EventLoop::impl *pimpl;

    try {
        pimpl = new EventLoop::impl();
    } catch (std::exception &e) {
        ErrPrint("new eventloop: %s", e.what());
        return nullptr;
    }

    if (signal == true) {
        if (pimpl->signalLoop.test_and_set() == true) {
            ErrPrint("Signal Loop already exists: %d", static_cast<int>(signal));
            delete pimpl;
            pimpl = nullptr;
            return nullptr;
        }
    }

    pimpl->enable_signal = true;
    pimpl->enable_thread = thread;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pimpl->spfd) < 0) {
        ErrPrintCode(errno, "socketpair");

        // NOTE:
        // The man-page said:
        // > On Linux (and other systems), socketpair() does not modify sv on
        // failure.  A requirement standardizing this behavior was added in
        // POSIX.1-2016.
        //
        // However, the spfd[2] variables were changed. And it prints errors
        // when I try to close them. Therefore, just reset them to -1 for
        // preventing from close invalid file descriptors. Only in this case.
        pimpl->spfd[SP_CTRL] = -1;
        pimpl->spfd[SP_NOTI] = -1;

        // The destructor is going to be invoked.
        delete pimpl;
        pimpl = nullptr;
        return nullptr;
    }

    int ret;
    ret = kqueue();
    if (ret < 0) {
        ErrPrintCode(errno, "kqueue");

        delete pimpl;
        pimpl = nullptr;
        return nullptr;
    }

    pimpl->handle = ret;

    EventObject *generic;

    try {
        generic = new EventObject(pimpl->spfd[SP_NOTI]);
    } catch (std::exception &e) {
        ErrPrint("new generic: %s", e.what());

        delete pimpl;
        pimpl = nullptr;
        return nullptr;
    }

    pimpl->spHandlerObject = pimpl->AddEventHandler(
        generic,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        EventLoop::impl::CtrlEventHandler,
        static_cast<void *>(pimpl));
    if (pimpl->spHandlerObject == nullptr) {
        delete generic;
        generic = nullptr;

        // NOTE:
        // Rest of cleanup would be done in the destructor of the pimpl.
        delete pimpl;
        pimpl = nullptr;
        return nullptr;
    }

    return pimpl;
}

/**
 * @NOTE
 * This method has different behaviour when the BeyonD is run on the Mac PC.
 * in case of the Mac, the WaitEvent will returns catched signal count if the signal mode is enabled
 * otherwise, like the other platforms, the WaitEvent will returns number of occurred event or the errors
 */
THREAD_SCOPE_SVC
long EventLoop::impl::WaitEvent(struct kevent *events, int count, int timeout)
{
    int type = 0;
    int ret;
    int i;
    struct timespec _timeout;
    struct timespec *ptr;
    int signalCatched = 0;

    if (timeout < 0) {
        ptr = nullptr;
    } else {
        _timeout.tv_sec = timeout / 1000;
        _timeout.tv_nsec = (timeout - (_timeout.tv_sec * 1000)) * 1000;
        ptr = &_timeout;
    }

    ret = kevent(handle, nullptr, 0, events, count, ptr);
    if (ret < 0) {
        ret = -errno;
        if (errno == EINTR) {
            ErrPrintCode(errno, "kevent is interrupted: %d", ret);
        } else {
            ErrPrintCode(errno, "kevent");
        }
        goto out;
    } else if (ret == 0) {
        ErrPrintCode(errno, "Timed out");
        ret = -ETIMEDOUT;
        goto out;
    }

    for (i = 0; i < ret; i++) {
        if (events[i].udata == nullptr) {
            signalCatched++;
            continue;
        }

        HandlerObjectImpl *item = static_cast<HandlerObjectImpl *>(events[i].udata);

        type = 0;

        if (events[i].filter == EVFILT_READ) {
            if (item->type & beyond_event_type::BEYOND_EVENT_TYPE_READ) {
                type |= beyond_event_type::BEYOND_EVENT_TYPE_READ;
            }
        }

        if (events[i].filter == EVFILT_WRITE) {
            if (item->type & beyond_event_type::BEYOND_EVENT_TYPE_WRITE) {
                type |= beyond_event_type::BEYOND_EVENT_TYPE_WRITE;
            }
        }

        if (events[i].flags & EV_ERROR) {
            if (item->type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
                type |= beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
            }
        }

        if (events[i].filter == EVFILT_TIMER) {
            type |= beyond_event_type::BEYOND_EVENT_TYPE_READ;
        }

        if (type == 0) {
            continue;
        }

        if (item->in_use.exchange(true) == true) {
            // This means that the item is in-use by RemoveHandler method.
            // Just skip the current event processing
            DbgPrint("EventObject is in-use by RemoveHandler method");
            continue;
        }

        beyond_handler_return opt;
        assert(item->eventHandler != nullptr && "EventHandler must not be nullptr");
        opt = item->eventHandler(item->eventObject, type, item->data);

        // NOTE:
        // item->type can be changed to DELETE if a caller tried to remove
        // the handlerObject while processing the event loop.
        if (opt == beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL || item->state.load() == HandlerObjectImpl::State::DELETE) {
            // NOTE:
            // Set the delete state again for make sure that the handler cannot be deleted
            // by the cancel callback
            item->state.store(HandlerObjectImpl::State::DELETE);

            EventObjectBaseInterface *eventObject = item->eventObject;
            std::function<void(EventObjectBaseInterface *, void *)> cancelHandler = item->cancelHandler;
            void *callbackData = item->data;
            item->in_use.store(false);

            // NOTE:
            // It is safe to call the *REAL* remover.
            RemoveEventHandler(item);

            if (cancelHandler != nullptr) {
                cancelHandler(eventObject, callbackData);
            }
        } else {
            item->in_use.store(false);
        }
    }

    ret = signalCatched;

out:
    return ret;
}

int EventLoop::impl::Stop(void)
{
    return StopLoop();
}

THREAD_SCOPE_MAIN
THREAD_SCOPE_SVC
int EventLoop::impl::StopLoop(void)
{
    bool pushEvent;

    MUTEX_LOCK(&eventLoopStateMutex);
    if (!!(eventLoopState & (EventLoopState::STOP_REQUESTED | EventLoopState::STOPPED))) {
        ErrPrint("Already stopped: %p", static_cast<void *>(stopEventObject.load()));
        MUTEX_UNLOCK(&eventLoopStateMutex);
        return -EALREADY;
    }

    // NOTE:
    // Push an event if the poll waiting it
    pushEvent = eventLoopState == EventLoopState::STARTED;
    eventLoopState = EventLoopState::STOP_REQUESTED;

    MUTEX_UNLOCK(&eventLoopStateMutex);

    if (pushEvent == true) {
        void *ptr = nullptr;
        int ret = write(spfd[SP_CTRL], &ptr, sizeof(ptr));
        if (ret != sizeof(ptr)) {
            ret = -errno;
            ErrPrintCode(errno, "write");
            return ret;
        }
    }
    return 0;
}

THREAD_SCOPE_MAIN
void EventLoop::impl::Destroy(void)
{
    if (destructed.test_and_set() == true) {
        // 1. There is a registered stopHandler.
        //    The stopHandler will call the "Destroy()"
        // 2. User invoke the Destory() without Stop() call.
        // 3. Destroy() method try to Stop() the loop
        // 4. Destroy() triggers the stopHandler() and then the stopHandler() call the Destroy() again
        // 5. The second Destroy() call will see the "destructed" flag and do nothing
        // 6. comeback to the caller, the Destroy() will do its rest of works.
        return;
    }

    // NOTE:
    // Call the StopLoop() even if the loop is stopped,
    // we have to call it to guarantee the loop is stopped or requested to stop.
    int ret = StopLoop();
    if (ret == 0) {
        // It is recommended to stop the loop by the user explicitily.
        // So we will remain an error message to get the user knows what (s)he did.
        ErrPrint("Stop the loop from the Destroy()");
        // However, we will stop the loop implicitly and do the rest of the destruction work.
    }

    if (enable_thread == true) {
        int status;

        if (pthread_equal(pthread_self(), main_thid)) {
            void *ret;

            status = pthread_join(svc_thid, &ret);

            if (status != 0) {
                ErrPrintCode(status, "pthread_join: %ld", reinterpret_cast<long>(ret));
            } else if (ret != nullptr) {
                DbgPrint("ret is not nullptr: %p", ret);
            }
        } else if (pthread_equal(pthread_self(), svc_thid)) {
            status = pthread_detach(svc_thid);
            if (status != 0) {
                ErrPrintCode(status, "pthread_detach");
            }
        }
    }

    delete this;
}

THREAD_SCOPE_MAIN
THREAD_SCOPE_SVC
EventLoop::HandlerObject *EventLoop::impl::AddEventHandler(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, void *callbackData)
{
    return AddEventHandlerInternal(eventObject, type, eventHandler, callbackData);
}

THREAD_SCOPE_MAIN
THREAD_SCOPE_SVC
EventLoop::HandlerObject *EventLoop::impl::AddEventHandler(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, const std::function<void(EventObjectBaseInterface *, void *)> &cancelHandler, void *callbackData)
{
    EventLoop::HandlerObject *handlerObject = AddEventHandlerInternal(eventObject, type, eventHandler, callbackData);
    if (handlerObject != nullptr) {
        handlerObject->cancelHandler = cancelHandler;
    }

    return handlerObject;
}

EventLoop::HandlerObject *EventLoop::impl::AddEventHandlerInternal(EventObjectBaseInterface *eventObject, int type, const std::function<beyond_handler_return(EventObjectBaseInterface *, int, void *)> &eventHandler, void *callbackData)
{
    if (eventObject == nullptr || eventHandler == nullptr) {
        ErrPrint("Invalid argument, eventHandler: %cnull, eventObject: %p",
                 (eventHandler == nullptr ? ' ' : '!'),
                 static_cast<void *>(eventObject));
        return nullptr;
    }

    if (eventObject->GetHandle() < 0) {
        ErrPrint("Event object is not valid (or not yet initialized)");
        return nullptr;
    }

    HandlerObjectImpl *handlerObject;

    try {
        handlerObject = new HandlerObjectImpl();
    } catch (std::exception &e) {
        ErrPrint("Unable to allocate heap for handlerObject::internal: %s", e.what());
        return nullptr;
    }

    handlerObject->eventObject = eventObject;
    handlerObject->type = type;
    handlerObject->eventHandler = eventHandler;
    handlerObject->data = callbackData;
    handlerObject->cancelHandler = nullptr;
    handlerObject->in_use.store(false);

    struct kevent ev;
    if (handlerObject->eventObject->GetHandle() == EVENT_ID_TIMER) {
        beyond::Timer *timer = dynamic_cast<beyond::Timer *>(handlerObject->eventObject);
        if (timer == nullptr) {
            ErrPrint("Invalid event object is used with timer id");
            delete handlerObject;
            handlerObject = nullptr;
            return nullptr;
        }

        double dtv = timer->GetTimer();
        unsigned int ms = static_cast<unsigned int>(dtv * 1000);

        EV_SET(&ev, handlerObject->eventObject->GetHandle(), EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, ms, static_cast<void *>(handlerObject));
        if (kevent(handle, &ev, 1, nullptr, 0, nullptr) < 0) {
            ErrPrintCode(errno, "kevent");
            delete handlerObject;
            handlerObject = nullptr;
            return nullptr;
        }
    } else {
        EV_SET(&ev, handlerObject->eventObject->GetHandle(), 0, EV_ADD | EV_ENABLE, 0, 0, static_cast<void *>(handlerObject));
        if (handlerObject->type & beyond_event_type::BEYOND_EVENT_TYPE_READ) {
            ev.filter = EVFILT_READ;
            if (kevent(handle, &ev, 1, nullptr, 0, nullptr) < 0) {
                ErrPrintCode(errno, "kevent");
                delete handlerObject;
                handlerObject = nullptr;
                return nullptr;
            }
        }

        if (handlerObject->type & beyond_event_type::BEYOND_EVENT_TYPE_WRITE) {
            ev.filter = EVFILT_WRITE;
            if (kevent(handle, &ev, 1, nullptr, 0, nullptr) < 0) {
                ErrPrintCode(errno, "kevent");
                delete handlerObject;
                handlerObject = nullptr;
                return nullptr;
            }
        }
    }

    if (ev.filter == 0) {
        ErrPrint("Invalid filter");

        delete handlerObject;
        handlerObject = nullptr;
        return nullptr;
    }

    handlerObject->state.store(HandlerObjectImpl::State::INIT);
    return static_cast<EventLoop::HandlerObject *>(handlerObject);
}

THREAD_SCOPE_MAIN
THREAD_SCOPE_SVC
int EventLoop::impl::RemoveEventHandler(HandlerObjectImpl *handlerObject)
{
    if (handlerObject->in_use.exchange(true) == true) {
        // NOTE:
        // Change the handlerObject state to DELETE.
        // The event loop is going to manage this handlerObject automatically.
        handlerObject->state.store(HandlerObjectImpl::State::DELETE);
        return 0;
    }

    struct kevent ev;
    EV_SET(&ev, handlerObject->eventObject->GetHandle(), 0, EV_DELETE | EV_DISABLE, 0, 0, static_cast<void *>(handlerObject));

    if (handlerObject->type & beyond_event_type::BEYOND_EVENT_TYPE_READ) {
        ev.filter = EVFILT_WRITE;
        if (kevent(handle, &ev, 1, nullptr, 0, nullptr) < 0) {
            ErrPrintCode(errno, "kevent");
            // NOTE:
            // Do nothing even though there is an error for controlling an kevent
            // object.
        }
    }

    if (handlerObject->type & beyond_event_type::BEYOND_EVENT_TYPE_WRITE) {
        ev.filter = EVFILT_WRITE;
        if (kevent(handle, &ev, 1, nullptr, 0, nullptr) < 0) {
            ErrPrintCode(errno, "kevent");
            // NOTE:
            // Do nothing even though there is an error for controlling an epoll
            // object.
        }
    }

    handlerObject->state.store(HandlerObjectImpl::State::DELETED);

    delete handlerObject;
    handlerObject = nullptr;

    return 0;
}

THREAD_SCOPE_MAIN
THREAD_SCOPE_SVC
int EventLoop::impl::RemoveEventHandler(EventLoop::HandlerObject *_handlerObject)
{
    if (_handlerObject == nullptr || _handlerObject->eventObject == nullptr) {
        ErrPrint("Invalid argument: %p", static_cast<void *>(_handlerObject));
        return -EINVAL;
    }

    HandlerObjectImpl *handlerObject = static_cast<HandlerObjectImpl *>(_handlerObject);

    DbgPrint("Clear %p", static_cast<void *>(handlerObject));

    if ((handlerObject->state.load() & HandlerObjectImpl::State::DELETE_MASK) != HandlerObjectImpl::State::INIT) {
        DbgPrint("Already cleared: %p", static_cast<void *>(handlerObject));
        return -EALREADY;
    }

    return RemoveEventHandler(handlerObject);
}

void *EventLoop::impl::ThreadMain(void *callbackData)
{
    EventLoop::impl *pimpl;
    struct kevent *events;
    sigset_t oldset;
    sigset_t mask;
    int ret;

    pimpl = static_cast<EventLoop::impl *>(callbackData);

    if (pimpl->epollWaitSize <= 0 || pimpl->loopCount == 0) {
        ErrPrint("Invalid parameters: %d, %d", pimpl->epollWaitSize, pimpl->loopCount);
        MUTEX_LOCK(&pimpl->eventLoopStateMutex);
        pimpl->eventLoopState = EventLoopState::STOPPED;
        MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);
        return reinterpret_cast<void *>(-EINVAL);
    }

    events = static_cast<struct kevent *>(malloc(sizeof(*events) * pimpl->epollWaitSize));
    if (!events) {
        ret = -errno;
        ErrPrintCode(errno, "malloc");
        MUTEX_LOCK(&pimpl->eventLoopStateMutex);
        pimpl->eventLoopState = EventLoopState::STOPPED;
        MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);
        return reinterpret_cast<void *>(ret);
    }

    sigfillset(&mask);

    ret = pthread_sigmask(SIG_SETMASK, &mask, &oldset);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_sigmask");
        free(events);
        MUTEX_LOCK(&pimpl->eventLoopStateMutex);
        pimpl->eventLoopState = EventLoopState::STOPPED;
        MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);
        return reinterpret_cast<void *>(ret);
    }

    if (pimpl->enable_signal == true) {
        struct kevent ev;
        int signals[] = {
            SIGHUP, SIGUSR1, SIGUSR2, SIGTERM, SIGALRM, SIGURG, SIGSTOP, SIGTSTP, SIGCONT, SIGCHLD, SIGTTIN, SIGTTOU, SIGIO, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH, SIGINFO
        };
        for (int i = sizeof(signals) / sizeof(*signals) - 1; i >= 0; i--) {
            EV_SET(&ev, SIGINFO, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, nullptr);
            if (kevent(pimpl->handle, &ev, 1, nullptr, 0, nullptr) < 0) {
                ret = -errno;
                ErrPrintCode(errno, "epoll_ctl");
                free(events);
                (void)pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
                MUTEX_LOCK(&pimpl->eventLoopStateMutex);
                pimpl->eventLoopState = EventLoopState::STOPPED;
                MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);
                return reinterpret_cast<void *>(ret);
            }
        }
    }

    MUTEX_LOCK(&pimpl->eventLoopStateMutex);

    if (pimpl->eventLoopState == EventLoopState::START_REQUESTED) {
        pimpl->eventLoopState = EventLoopState::STARTED;
    }

    MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);

    while (pimpl->eventLoopState == EventLoopState::STARTED && pimpl->loopCount != 0) {
        if (pimpl->loopCount > 0) {
            pimpl->loopCount--;
        }

        // timeout: milli-seconds
        // -1 means infinite
        ret = pimpl->WaitEvent(events, pimpl->epollWaitSize, pimpl->timeoutInMS);
        if (ret == -ETIMEDOUT) {
            break;
        } else if (pimpl->enable_signal == true && ret > 0) {
            for (int i = 0; i < pimpl->epollWaitSize; i++) {
                if (events[i].udata == nullptr) {
                    InfoPrint("Signal catched (%lu)", events[i].ident);
                }
            }
        }
        ret = 0;
    }

    MUTEX_LOCK(&pimpl->eventLoopStateMutex);
    pimpl->eventLoopState = EventLoopState::STOPPED;
    MUTEX_UNLOCK(&pimpl->eventLoopStateMutex);

    if (pimpl->enable_signal == true) {
        struct kevent ev;
        int signals[] = {
            SIGHUP, SIGUSR1, SIGUSR2, SIGTERM, SIGALRM, SIGURG, SIGSTOP, SIGTSTP, SIGCONT, SIGCHLD, SIGTTIN, SIGTTOU, SIGIO, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH, SIGINFO
        };
        for (int i = sizeof(signals) / sizeof(*signals) - 1; i >= 0; i--) {
            EV_SET(&ev, SIGINFO, EVFILT_READ, EV_DELETE | EV_DISABLE, 0, 0, nullptr);
            if (kevent(pimpl->handle, &ev, 1, nullptr, 0, nullptr) < 0) {
                ErrPrintCode(errno, "kevent");
            }
        }
    }

    (void)pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
    free(events);

    // NOTE:
    // Just invoke the stopped callback
    // It doesn't matter that the stopping loop is requested or not
    //
    // This stopEventObject has to be called at the end of a function.
    // In order to delete EventLoop object safely.
    Event *stopEventObject = pimpl->stopEventObject.exchange(nullptr);
    if (stopEventObject != nullptr) {
        if (stopEventObject->handler != nullptr) {
            // In this stop callback, the event loop object can be deleted.
            // Therefore, we do not access the pimpl after this stopEvent
            // callback.

            stopEventObject->handler(stopEventObject->loop, stopEventObject->callbackData);

            // WARN WARN WARN:
            // Do not access the pimpl and the loop object after this function
            // callback. There is no guarantees that the pimpl and the loop
            // object still valid. Because the caller is able to invoke the
            // Destroy() method to delete the loop and the pimpl object.
            // Therefore, DO NOT ACCESS the pimpl and the loop object anymore.
        }

        delete stopEventObject;
        stopEventObject = nullptr;
    }

    return reinterpret_cast<void *>(ret);
}

THREAD_SCOPE_MAIN
int EventLoop::impl::Run(int epollWaitSize, int loop_count, int timeout_in_ms)
{
    int ret;
    int status;
    pthread_attr_t thattr;

    MUTEX_LOCK(&eventLoopStateMutex);
    if (eventLoopState != EventLoopState::STOPPED) {
        MUTEX_UNLOCK(&eventLoopStateMutex);
        ErrPrint("Thread is not proper to start: 0x%X", eventLoopState);
        return -EINVAL;
    }
    eventLoopState = EventLoopState::START_REQUESTED;
    MUTEX_UNLOCK(&eventLoopStateMutex);

    this->epollWaitSize = epollWaitSize;
    this->loopCount = loop_count;
    this->timeoutInMS = timeout_in_ms;

    if (enable_thread == false) {
        long tmp = reinterpret_cast<long>(ThreadMain(this));
        return static_cast<int>(tmp);
    }

    ret = pthread_attr_init(&thattr);
    if (ret != 0) {
        ErrPrintCode(ret, "pthread_attr_init");
        return -ret;
    }

    status = pthread_create(&svc_thid, &thattr, ThreadMain, this);
    ret = pthread_attr_destroy(&thattr);

    if (status != 0) {
        ErrPrintCode(status, "pthread_create");
        return -status;
    }

    if (ret != 0) {
        ErrPrintCode(ret, "pthread_attr_destroy");
        return -ret;
    }

    return 0;
}

int EventLoop::impl::SetStopHandler(const std::function<void(EventLoop *, void *)> &stopHandler, void *callbackData)
{
    Event *_stopEventObject = nullptr;

    if (stopHandler != nullptr) {
        try {
            _stopEventObject = new Event();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            return -ENOMEM;
        }

        _stopEventObject->loop = static_cast<EventLoop *>(this);
        _stopEventObject->type = SP_CTRL_EVT_TERMINATE;
        _stopEventObject->handler = stopHandler;
        _stopEventObject->callbackData = callbackData;
    }

    _stopEventObject = stopEventObject.exchange(_stopEventObject);
    if (_stopEventObject != nullptr) {
        delete _stopEventObject;
        _stopEventObject = nullptr;
    }

    return 0;
}

int EventLoop::impl::GetHandle(void) const
{
    return handle;
}

} // namespace beyond

