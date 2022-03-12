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

#ifndef __BEYOND_INTERNAL_INFERENCE_RUNTIME_IMPL_H__
#define __BEYOND_INTERNAL_INFERENCE_RUNTIME_IMPL_H__

#include "beyond/private/module_interface_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_runtime_private.h"

namespace beyond {

class Inference::Runtime::impl final : public Inference::Runtime {
private:
    class Async;

public: // Inference::Runtime interface
    static impl *Create(const beyond_argument *arg);

    void Destroy(void) override;

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

public: // ModuleInterface interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;

public: // InferenceInterface interface
    int Configure(const beyond_config *options = nullptr) override;

    int LoadModel(const char *model) override;

    int GetInputTensorInfo(const beyond_tensor_info *&info, int &size) override;
    int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size) override;

    int SetInputTensorInfo(const beyond_tensor_info *info, int size) override;
    int SetOutputTensorInfo(const beyond_tensor_info *info, int size) override;

    int AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor) override;
    void FreeTensor(beyond_tensor *&tensor, int size) override;

    int Prepare(void) override;

    int Invoke(const beyond_tensor *input, int size, const void *context = nullptr) override;

    int GetOutput(beyond_tensor *&tensor, int &size) override;

    int Stop(void) override;

private:
    impl(void);
    virtual ~impl(void);

    void *dlHandle;
    InferenceInterface::RuntimeInterface *module;
    Inference::Runtime::impl::Async *asyncEmulator;

    InferenceInterface *async;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_INFERENCE_RUNTIME_IMPL_H__
