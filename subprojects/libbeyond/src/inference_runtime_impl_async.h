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

#ifndef __BEYOND_INTERNAL_RUNTIME_IMPL_ASYNC_H__
#define __BEYOND_INTERNAL_RUNTIME_IMPL_ASYNC_H__

#include <beyond/common.h>

#include <beyond/private/event_object_base_interface_private.h>
#include <beyond/private/event_object_interface_private.h>
#include <beyond/private/event_object_private.h>
#include <beyond/private/event_loop_private.h>
#include <beyond/private/command_object_private.h>
#include <beyond/private/inference_interface_private.h>
#include <beyond/private/inference_runtime_interface_private.h>

#include "inference_impl.h"
#include "inference_impl_event_object.h"

namespace beyond {

// NOTE:
// This class is going to emulate the asynchronous mode if the runtime does not support async mode.
// Otherwise, it will just call the runtime method transparently.
//
class Inference::Runtime::impl::Async : public InferenceInterface {
public:
    static Async *Create(InferenceInterface::RuntimeInterface *module);
    virtual void Destroy(void);

private:
    explicit Async(InferenceInterface::RuntimeInterface *module);
    virtual ~Async(void);

private:
    enum Command : int {
        IdConfigure = 0x00,
        IdLoadModel = 0x01,
        IdGetInputTensorInfo = 0x02,
        IdGetOutputTensorInfo = 0x03,
        IdSetInputTensorInfo = 0x04,
        IdSetOutputTensorInfo = 0x05,
        IdAllocateTensor = 0x06,
        IdFreeTensor = 0x07,
        IdPrepare = 0x08,
        IdInvoke = 0x09,
        IdStop = 0x0A,
        IdLast = 0x0B,

        IdGetOutput = 0x0C,
    };
    typedef int (*CommandHandler)(Async *moduleAsync, void *data);

    static int CommandConfigureHandler(Async *moduleAsync, void *data);
    static int CommandLoadModelHandler(Async *moduleAsync, void *data);
    static int CommandGetInputTensorInfoHandler(Async *moduleAsync, void *data);
    static int CommandGetOutputTensorInfoHandler(Async *moduleAsync, void *data);
    static int CommandSetInputTensorInfoHandler(Async *moduleAsync, void *data);
    static int CommandSetOutputTensorInfoHandler(Async *moduleAsync, void *data);
    static int CommandAllocateTensorHandler(Async *moduleAsync, void *data);
    static int CommandFreeTensorHandler(Async *moduleAsync, void *data);
    static int CommandPrepareHandler(Async *moduleAsync, void *data);
    static int CommandInvokeHandler(Async *moduleAsync, void *data);
    static int CommandStopHandler(Async *moduleAsync, void *data);

    static beyond_handler_return Main(EventObjectBaseInterface *obj, int type, void *data);

public: // Inference interface
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

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&info) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

private:
    CommandObject *command;
    CommandObject *outputConsumer;
    Inference::impl::EventObject *eventObject;
    EventLoop::HandlerObject *handlerObject;

    struct CommandData {
        union Arguments {
            struct TensorInfo {
                beyond_tensor_info *info;
                int size;
            } tensorInfo;
            struct Tensor {
                beyond_tensor *tensor;
                int size;
                void *context;
            } tensor;
            void *ptr;
        } args;

        union Return {
            int value;
            void *ptr;
            struct Object {
                int value;
                void *ptr;
            } obj;
        } ret;
    };

    struct Context {
        CommandObject *command;
        CommandObject *outputProducer;
        EventLoop *eventLoop;
        CommandHandler cmdTable[Command::IdLast];
        InferenceInterface::RuntimeInterface *module;
    } context;

    struct EventData : public EventObjectInterface::EventData {
    };
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_RUNTIME_IMPL_ASYNC_H__
