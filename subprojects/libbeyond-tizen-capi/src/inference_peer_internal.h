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

#ifndef __BEYOND_INTERNAL_INFERENCE_PEER_H__
#define __BEYOND_INTERNAL_INFERENCE_PEER_H__

#if defined(__cplusplus)
extern "C" {
#endif

extern beyond::InferenceInterface::PeerInterface *beyond_peer_get_peer(beyond_peer_h handle);

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_INTERNAL_PEER_INFERENCE_H__
