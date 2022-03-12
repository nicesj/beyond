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

#ifndef __BEYOND_PEER_H__
#define __BEYOND_PEER_H__

#include <beyond/common.h>
#include <beyond/session.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Manage the other devices (e.g Android Edge device)
API beyond_peer_h beyond_peer_create(beyond_session_h session, struct beyond_argument *arg);

API int beyond_peer_set_event_callback(beyond_peer_h peer, int (*event)(beyond_peer_h peer, struct beyond_event_info *event, void *data), void *data);

// If there are required object for initiating the peer object, use this method.
API int beyond_peer_configure(beyond_peer_h peer, struct beyond_config *config);

// this function is going to return the latest status information of the peer after the peer instance is created
// and had at least one or several times of connections before.
API int beyond_peer_get_info(beyond_peer_h peer, const struct beyond_peer_info *info);

// this function provide a way to update the peer information using the other discovery module.
API int beyond_peer_set_info(beyond_peer_h peer, struct beyond_peer_info *info);

// Create a connection and prepare communication channel
API int beyond_peer_activate(beyond_peer_h peer);

// Destroy the connection
API int beyond_peer_deactivate(beyond_peer_h peer);

API void beyond_peer_destroy(beyond_peer_h peer);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_PEER_H__
