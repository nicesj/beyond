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

#ifndef __BEYOND_INFERENCE_H__
#define __BEYOND_INFERENCE_H__

#include <beyond/common.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void **beyond_tensor_h;
#define BEYOND_TENSOR(handle) ((struct beyond_tensor *)((handle)[0]))

// Manage the Inference operation
API beyond_inference_h beyond_inference_create(struct beyond_argument *option);

API int beyond_inference_set_output_callback(beyond_inference_h handle, void (*output)(beyond_inference_h handle, struct beyond_event_info *event, void *data), void *data);

API int beyond_inference_configure(beyond_inference_h handle, struct beyond_config *conf);

API int beyond_inference_load_model(beyond_inference_h handle, const char **models, int count);

API int beyond_inference_set_input_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info *info, int size);
API int beyond_inference_set_output_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info *info, int size);

API int beyond_inference_get_input_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info **info, int *size);
API int beyond_inference_get_output_tensor_info(beyond_inference_h handle, const struct beyond_tensor_info **info, int *size);

API int beyond_inference_prepare(beyond_inference_h handle);

API beyond_tensor_h beyond_inference_allocate_tensor(beyond_inference_h handle, const struct beyond_tensor_info *info, int size);
API beyond_tensor_h beyond_inference_ref_tensor(beyond_tensor_h ptr);
API beyond_tensor_h beyond_inference_unref_tensor(beyond_tensor_h ptr);

API int beyond_inference_do(beyond_inference_h handle, const beyond_tensor_h tensor, const void *context);
API int beyond_inference_get_output(beyond_inference_h handle, beyond_tensor_h *tensor, int *size);
// TODO: will be removed in the next PR
API int beyond_inference_get_input(beyond_inference_h handle, struct beyond_tensor **tensor, int *size);

API int beyond_inference_stop(beyond_inference_h handle);
API int beyond_inference_is_stopped(beyond_inference_h handle);

API int beyond_inference_add_peer(beyond_inference_h handle, beyond_peer_h peer);
API int beyond_inference_remove_peer(beyond_inference_h handle, beyond_peer_h peer);

API int beyond_inference_add_runtime(beyond_inference_h handle, beyond_runtime_h runtime);
API int beyond_inference_remove_runtime(beyond_inference_h handle, beyond_runtime_h runtime);

API void beyond_inference_destroy(beyond_inference_h handle);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_INFERENCE_H__
