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

#ifndef __BEYOND_PRIVATE_INFERENCE_H__
#define __BEYOND_PRIVATE_INFERENCE_H__

#include "beyond/common.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_runtime_interface_private.h"
#include "beyond/private/inference_peer_interface_private.h"

namespace beyond {

class API Inference : public InferenceInterface {
public:
    class Peer;
    class Runtime;

    static Inference *Create(const beyond_argument *arg);

    using InferenceInterface::LoadModel;
    virtual int LoadModel(const char **model, int count) = 0;

    virtual int AddRuntime(InferenceInterface::RuntimeInterface *runtime) = 0;
    virtual int RemoveRuntime(InferenceInterface::RuntimeInterface *runtime) = 0;

    virtual int AddPeer(InferenceInterface::PeerInterface *peer) = 0;
    virtual int RemovePeer(InferenceInterface::PeerInterface *peer) = 0;

    virtual void Destroy(void) = 0;

public: // utilities
    static const char *TensorTypeToString(beyond_tensor_type type);
    static beyond_tensor_type TensorTypeStringToType(const char *str);
    static int TensorTypeToSize(beyond_tensor_type type);

protected:
    Inference(void) = default;
    virtual ~Inference(void) = default;

private:
    class impl;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_INFERENCE_H__
