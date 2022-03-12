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

#ifndef __BEYOND_AUTHENTICATOR_INTERFACE_H__
#define __BEYOND_AUTHENTICATOR_INTERFACE_H__

#include <beyond/common.h>

#if defined(__cplusplus)
extern "C" {
#endif

API beyond_authenticator_h beyond_authenticator_create(struct beyond_argument *arg);

API int beyond_authenticator_configure(beyond_authenticator_h auth, struct beyond_config *config);

API int beyond_authenticator_set_event_callback(beyond_authenticator_h auth, int (*event)(beyond_authenticator_h auth, struct beyond_event_info *event, void *data), void *data);

API int beyond_authenticator_deactivate(beyond_authenticator_h auth);

API int beyond_authenticator_activate(beyond_authenticator_h auth);

API int beyond_authenticator_prepare(beyond_authenticator_h auth);

API void beyond_authenticator_destroy(beyond_authenticator_h auth);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_AUTHENTICATOR_INTERFACE_H__
