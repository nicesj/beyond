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

#if !defined(__BEYOND_RUNTIME_H__)
#define __BEYOND_RUNTIME_H__

#include <beyond/common.h>

#if defined(__cplusplus)
extern "C" {
#endif

API beyond_runtime_h beyond_runtime_create(struct beyond_argument *arg);

API int beyond_runtime_set_event_callback(beyond_runtime_h runtime, int (*event)(beyond_runtime_h runtime, struct beyond_event_info *event, void *data), void *data);

API int beyond_runtime_configure(beyond_runtime_h runtime, struct beyond_config *config);

API void beyond_runtime_destroy(beyond_runtime_h runtime);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_RUNTIME_H__
