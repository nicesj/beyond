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

#ifndef __BEYOND_PRIVATE_INFERENCE_INTERFACE_H__
#define __BEYOND_PRIVATE_INFERENCE_INTERFACE_H__

#include <functional>

#include <beyond/common.h>
#include <beyond/private/event_object_interface_private.h>

namespace beyond {

class InferenceInterface : public EventObjectInterface {
public:
    class RuntimeInterface;
    class PeerInterface;

    struct EventData : public EventObjectInterface::EventData {
    };

public:
    virtual ~InferenceInterface(void) = default;

    virtual int Configure(const beyond_config *options = nullptr) = 0;

    virtual int LoadModel(const char *model) = 0;

    virtual int GetInputTensorInfo(const beyond_tensor_info *&info, int &size) = 0;
    virtual int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size) = 0;

    virtual int SetInputTensorInfo(const beyond_tensor_info *info, int size) = 0;
    virtual int SetOutputTensorInfo(const beyond_tensor_info *info, int size) = 0;

    virtual int AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor) = 0;
    virtual void FreeTensor(beyond_tensor *&tensor, int size) = 0;

    virtual int Prepare(void) = 0;

    virtual int Invoke(const beyond_tensor *input, int size, const void *context = nullptr) = 0;

    virtual int GetOutput(beyond_tensor *&tensor, int &size) = 0;

    virtual int Stop(void) = 0;

protected:
    InferenceInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_INFERENCE_INTERFACE_H__
