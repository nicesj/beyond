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

#ifndef __BEYOND_SESSION_H__
#define __BEYOND_SESSION_H__

#include <beyond/common.h>
#include <beyond/session.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void *beyond_session_h;

API beyond_session_h beyond_session_create(int thread, int signal);

API void beyond_session_destroy(beyond_session_h session);

API int beyond_session_get_descriptor(beyond_session_h handle);

API beyond_event_handler_h beyond_session_add_handler(beyond_session_h handle, beyond_object_h object, int type, enum beyond_handler_return (*eventHandler)(beyond_object_h, int, void *), void (*cancelHandler)(beyond_object_h object, void *callbackData), void *callbackData);

API int beyond_session_remove_handler(beyond_session_h handle, beyond_event_handler_h handler);

API beyond_event_handler_h beyond_session_add_fd_handler(beyond_session_h handle, int fd, int type, enum beyond_handler_return (*eventHandler)(beyond_object_h, int, void *), void (*cancelHandler)(beyond_object_h, void *), void *callbackData);

API int beyond_session_remove_fd_handler(beyond_session_h handle, beyond_event_handler_h handler);

API int beyond_session_run(beyond_session_h handle, int eventQueueSize, int loop_count, int timeout_in_ms);

API int beyond_session_stop(beyond_session_h handle);

API int beyond_session_set_stop_handler(beyond_session_h handle, void (*handler)(beyond_session_h, void *), void *callbackData);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_SESSION_H__
