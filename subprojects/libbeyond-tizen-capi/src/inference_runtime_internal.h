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

#if !defined(__BEYOND_INFERENCE_RUNTIME_INTERNAL_H__)
#define __BEYOND_INFERENCE_RUNTIME_INTERNAL_H__

#if defined(__cplusplus)
extern "C" {
#endif

extern beyond::InferenceInterface::RuntimeInterface *beyond_runtime_get_runtime(beyond_runtime_h handle);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_INFERENCE_RUNTIME_INTERNAL_H__
