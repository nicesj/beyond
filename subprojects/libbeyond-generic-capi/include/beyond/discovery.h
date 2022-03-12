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

#ifndef __BEYOND_DISCOVERY_H__
#define __BEYOND_DISCOVERY_H__

#include <beyond/common.h>
#include <beyond/session.h>

#if defined(__cplusplus)
extern "C" {
#endif

API beyond_discovery_h beyond_discovery_create(beyond_session_h session, struct beyond_argument *arg);

// What could we do in this event callback?
// maybe,...
//  * Create a peer instance
//  * Update information of a peer instance
// The event structure is able to contain the configuration parameter for creating a peer instance
// such as a connection session instance to make a channel of control each other
API int beyond_discovery_set_event_callback(beyond_discovery_h handle, int (*event)(beyond_discovery_h handle, struct beyond_event_info *event, void *data), void *data);

API int beyond_discovery_configure(beyond_discovery_h handle, struct beyond_config *config);

// Activate discovery module
API int beyond_discovery_activate(beyond_discovery_h handle);

// Deactivate discovery module
API int beyond_discovery_deactivate(beyond_discovery_h handle);

API void beyond_discovery_destroy(beyond_discovery_h handle);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_DISCOVERY_H__
