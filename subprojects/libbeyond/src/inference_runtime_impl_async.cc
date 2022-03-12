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

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif // !defined(_GNU_SOURCE)

#if defined(__APPLE__)
#define SOCK_CLOEXEC 0
#define pipe2(a, b) pipe(a)
#endif // __APPLE__

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"

#include "beyond/private/event_object_base_interface_private.h"
#include "beyond/private/event_object_interface_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/command_object_interface_private.h"
#include "beyond/private/command_object_private.h"

#include "beyond/private/module_interface_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_runtime_interface_private.h"
#include "beyond/private/inference_runtime_private.h"
#include "inference_runtime_impl.h"
#include "inference_runtime_impl_async.h"

namespace beyond {

int Inference::Runtime::impl::Async::CommandConfigureHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->Configure(static_cast<beyond_config *>(arg->args.ptr));

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdConfigure, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandLoadModelHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->LoadModel(static_cast<char *>(arg->args.ptr));

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdLoadModel, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandGetInputTensorInfoHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    const beyond_tensor_info *info = nullptr;
    int size = 0;

    arg->ret.value = async->context.module->GetInputTensorInfo(info, size);
    arg->args.tensorInfo.info = const_cast<beyond_tensor_info *>(info);
    arg->args.tensorInfo.size = size;

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdGetInputTensorInfo, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandGetOutputTensorInfoHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    const beyond_tensor_info *info;
    int size;

    arg->ret.value = async->context.module->GetOutputTensorInfo(info, size);
    arg->args.tensorInfo.info = const_cast<beyond_tensor_info *>(info);
    arg->args.tensorInfo.size = size;

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdGetOutputTensorInfo, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandSetInputTensorInfoHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->SetInputTensorInfo(arg->args.tensorInfo.info, arg->args.tensorInfo.size);

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdSetInputTensorInfo, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandSetOutputTensorInfoHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->SetOutputTensorInfo(arg->args.tensorInfo.info, arg->args.tensorInfo.size);

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdSetOutputTensorInfo, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandAllocateTensorHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;
    beyond_tensor *tensor = nullptr;

    arg->ret.obj.value = async->context.module->AllocateTensor(arg->args.tensorInfo.info, arg->args.tensorInfo.size, tensor);
    arg->ret.obj.ptr = tensor;

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdAllocateTensor, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandFreeTensorHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    async->context.module->FreeTensor(arg->args.tensor.tensor, arg->args.tensor.size);
    arg->ret.value = 0;

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdFreeTensor, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandPrepareHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->Prepare();

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdPrepare, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandInvokeHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);

    EventData *eventData;
    try {
        eventData = new EventData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    eventData->data = arg->args.tensor.context;

    int ret = async->context.module->Invoke(arg->args.tensor.tensor, arg->args.tensor.size, arg->args.tensor.context);
    if (ret < 0) {
        eventData->type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
    } else {
        eventData->type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_SUCCESS;
    }

    if (async->context.module->GetOutput(arg->args.tensor.tensor, arg->args.tensor.size) < 0) {
        // Rewrite the event type
        eventData->type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
    } else {
        // NOTE:
        // Asynchronous runtime module will have the output tensor after return from the Invoke() method call.
        // Therefore, we have to push the output tensor to the outputProducer (queue),
        // and then publish the event.
        // If we publish the event first, there is a chance to that the outputConsumer failed to get the output tensor.

        // NOTE:
        // "arg" ownership transferred from Invoke()
        // The ownership of the "arg" will be moved to GetOutput()
        ret = async->context.outputProducer->Send(Command::IdGetOutput, arg);
        if (ret < 0) {
            ErrPrint("Unable to queue the output tensor: %d", ret);
            delete arg;
            arg = nullptr;

            // Rewrite the event type
            eventData->type = beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_ERROR;
        }
    }

    if (async->eventObject->PublishEventData(eventData) < 0) {
        ErrPrint("Failed to publish the event data!");
        delete eventData;
        eventData = nullptr;
    }

    return ret;
}

int Inference::Runtime::impl::Async::CommandStopHandler(Inference::Runtime::impl::Async *async, void *data)
{
    assert(async != nullptr && data != nullptr && "async and data must not be nullptr");

    CommandData *arg = static_cast<CommandData *>(data);
    int ret;

    arg->ret.value = async->context.module->Stop();

    // WARN:
    // Do not access the "arg" after Send it
    ret = async->context.command->Send(Command::IdStop, arg);
    arg = nullptr; // for the safety.
    if (ret < 0) {
        ErrPrint("send failed: %d", ret);
    }
    return ret;
}

Inference::Runtime::impl::Async *Inference::Runtime::impl::Async::Create(InferenceInterface::RuntimeInterface *module)
{
    Inference::Runtime::impl::Async *async;
    int spfd[2];
    int pfd[2];

    try {
        async = new Inference::Runtime::impl::Async(module);
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, spfd) < 0) {
        ErrPrintCode(errno, "socketpair");
        delete async;
        async = nullptr;
        return nullptr;
    }

    if (pipe2(pfd, O_CLOEXEC) < 0) {
        ErrPrintCode(errno, "pipe2");
        if (close(spfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        delete async;
        async = nullptr;
        return nullptr;
    }

    try {
        async->command = new beyond::CommandObject(spfd[0]);
    } catch (std::exception &e) {
        delete async;
        async = nullptr;
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(spfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    try {
        async->context.command = new beyond::CommandObject(spfd[1]);
    } catch (std::exception &e) {
        delete async;
        async = nullptr;
        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(spfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }
        return nullptr;
    }

    try {
        async->context.outputProducer = new beyond::CommandObject(pfd[1]);
    } catch (std::exception &e) {
        delete async;
        async = nullptr;

        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        return nullptr;
    }

    try {
        async->outputConsumer = new beyond::CommandObject(pfd[0]);
    } catch (std::exception &e) {
        delete async;
        async = nullptr;

        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }

        return nullptr;
    }

    // TODO: Inherit class required
    async->eventObject = Inference::impl::EventObject::Create();
    if (async->eventObject == nullptr) {
        delete async;
        async = nullptr;
        return nullptr;
    }

    async->context.eventLoop = beyond::EventLoop::Create(true, false);
    if (async->context.eventLoop == nullptr) {
        delete async;
        async = nullptr;
        return nullptr;
    }

    async->handlerObject = async->context.eventLoop->AddEventHandler(static_cast<EventObjectBaseInterface *>(async->context.command), beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR, Main, static_cast<void *>(async));
    if (async->handlerObject == nullptr) {
        delete async;
        async = nullptr;
        return nullptr;
    }

    int ret = async->context.eventLoop->Run();
    if (ret < 0) {
        ErrPrint("Failed to run the event loop: %d", ret);
        delete async;
        async = nullptr;
        return nullptr;
    }

    return async;
}

// RUN_ON_THE_THREAD
beyond_handler_return Inference::Runtime::impl::Async::Main(EventObjectBaseInterface *obj, int type, void *data)
{
    Async *async = static_cast<Async *>(data);

    if ((type & beyond_event_type::BEYOND_EVENT_TYPE_ERROR) == beyond_event_type::BEYOND_EVENT_TYPE_ERROR) {
        ErrPrint("Error! 0x%.8x", type);
        return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
    }

    CommandObject *command = static_cast<CommandObject *>(obj);

    int cmdId;
    void *cmdData;
    int ret;

    ret = command->Recv(cmdId, cmdData);
    if (ret < 0) {
        ErrPrint("Unable to recv command data");
        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    }

    if (cmdId < 0 || cmdId >= Command::IdLast) {
        ErrPrint("Invalid type: 0x%.8x", cmdId);
        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    }

    ret = async->context.cmdTable[cmdId](async, cmdData);
    if (ret < 0) {
        ErrPrint("Command returns %d", ret);
        return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
    }

    return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
}

void Inference::Runtime::impl::Async::Destroy(void)
{
    assert(handlerObject != nullptr && "handlerObject cannot be nullptr");
    if (handlerObject != nullptr) {
        int ret = context.eventLoop->RemoveEventHandler(handlerObject);
        handlerObject = nullptr;
        if (ret < 0) {
            DbgPrint("removeEventHandler: %d", ret);
        }
    }

    assert(context.eventLoop != nullptr && "eventLoop cannot be nullptr");
    if (context.eventLoop != nullptr) {
        context.eventLoop->SetStopHandler([](EventLoop *eventLoop, void *data) -> void {
            DbgPrint("Event Loop is stopped");
            eventLoop->Destroy();
        },
                                          nullptr);

        int ret = context.eventLoop->Stop();
        if (ret < 0) {
            DbgPrint("Stop the event loop: %d", ret);
        }

        // The event loop will be destoryed when it is stopped.
        context.eventLoop = nullptr;
    }

    delete this;
}

Inference::Runtime::impl::Async::Async(InferenceInterface::RuntimeInterface *module)
    : command(nullptr)
    , outputConsumer(nullptr)
    , eventObject(nullptr)
    , handlerObject(nullptr)
    , context{
        .command = nullptr,
        .outputProducer = nullptr,
        .eventLoop = nullptr,
        .cmdTable = {
            CommandConfigureHandler,
            CommandLoadModelHandler,
            CommandGetInputTensorInfoHandler,
            CommandGetOutputTensorInfoHandler,
            CommandSetInputTensorInfoHandler,
            CommandSetOutputTensorInfoHandler,
            CommandAllocateTensorHandler,
            CommandFreeTensorHandler,
            CommandPrepareHandler,
            CommandInvokeHandler,
            CommandStopHandler,
        },
        .module = module,
    }
{
}

Inference::Runtime::impl::Async::~Async(void)
{
    if (context.command != nullptr) {
        if (close(context.command->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }

        delete context.command;
        context.command = nullptr;
    }

    if (command != nullptr) {
        if (close(command->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }
        delete command;
        command = nullptr;
    }

    if (context.outputProducer != nullptr) {
        if (close(context.outputProducer->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }

        delete context.outputProducer;
        context.outputProducer = nullptr;
    }

    if (outputConsumer != nullptr) {
        if (close(outputConsumer->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }

        delete outputConsumer;
        outputConsumer = nullptr;
    }

    if (eventObject != nullptr) {
        eventObject->Destroy();
        eventObject = nullptr;
    }
}

int Inference::Runtime::impl::Async::Configure(const beyond_config *options)
{
    int ret;
    CommandData arg = {
        .args = { .ptr = static_cast<void *>(const_cast<beyond_config *>(options)) }
    };

    ret = command->Send(Command::IdConfigure, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;

    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdConfigure) {
        return ret;
    }
    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::LoadModel(const char *model)
{
    int ret;
    CommandData arg = {
        .args = { .ptr = static_cast<void *>(const_cast<char *>(model)) },
    };

    ret = command->Send(Command::IdLoadModel, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdLoadModel) {
        return ret;
    }
    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    int ret;

    CommandData arg = {
        .args = {
            .tensorInfo = {
                .info = const_cast<beyond_tensor_info *>(info),
                .size = size,
            } },
        .ret = {
            .value = 0,
        }
    };

    ret = command->Send(Command::IdGetInputTensorInfo, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdGetInputTensorInfo) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    if (argPtr->ret.value == 0) {
        info = argPtr->args.tensorInfo.info;
        size = argPtr->args.tensorInfo.size;
    }

    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    int ret;

    CommandData arg = {
        .args = {
            .tensorInfo = {
                .info = const_cast<beyond_tensor_info *>(info),
                .size = size,
            } },
        .ret = {
            .value = 0,
        }
    };

    ret = command->Send(Command::IdGetOutputTensorInfo, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdGetOutputTensorInfo) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    if (argPtr->ret.value == 0) {
        info = argPtr->args.tensorInfo.info;
        size = argPtr->args.tensorInfo.size;
    }

    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    CommandData arg = {
        .args = {
            .tensorInfo = {
                .info = const_cast<beyond_tensor_info *>(info),
                .size = size,
            } },
        .ret = {
            .value = 0,
        }
    };

    int ret = command->Send(Command::IdSetInputTensorInfo, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdSetInputTensorInfo) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    CommandData arg = {
        .args = {
            .tensorInfo = {
                .info = const_cast<beyond_tensor_info *>(info),
                .size = size,
            } },
        .ret = {
            .value = 0,
        }
    };

    int ret = command->Send(Command::IdSetOutputTensorInfo, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdSetOutputTensorInfo) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    CommandData arg = {
        .args = {
            .tensorInfo = {
                .info = const_cast<beyond_tensor_info *>(info),
                .size = size } },
    };

    int ret = command->Send(Command::IdAllocateTensor, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdAllocateTensor) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    if (argPtr->ret.obj.value < 0) {
        return argPtr->ret.obj.value;
    }

    tensor = static_cast<beyond_tensor *>(argPtr->ret.obj.ptr);
    return argPtr->ret.obj.value;
}

void Inference::Runtime::impl::Async::FreeTensor(beyond_tensor *&tensor, int size)
{
    CommandData arg = {
        .args = {
            .tensor = {
                .tensor = tensor,
                .size = size } },
    };

    int ret = command->Send(Command::IdFreeTensor, &arg);
    if (ret < 0) {
        ErrPrint("Unable to send a command: %d", ret);
        return;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdFreeTensor) {
        ErrPrint("Unable to recv a command: %d", ret);
        return;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    if (argPtr != nullptr) {
        DbgPrint("Waiting result: %d", argPtr->ret.value);
    }
}

int Inference::Runtime::impl::Async::Prepare(void)
{
    CommandData arg = {
        .args = {
            .ptr = nullptr },
    };

    int ret = command->Send(Command::IdPrepare, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdPrepare) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::Invoke(const beyond_tensor *input, int size, const void *context)
{
    CommandData *arg;

    try {
        arg = new CommandData();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    arg->args.tensor.tensor = const_cast<beyond_tensor *>(input);
    arg->args.tensor.size = size;
    arg->args.tensor.context = const_cast<void *>(context);

    // NOTE:
    // The ownership of the "arg" will be moved to the InvokeHandler
    int ret = command->Send(Command::IdInvoke, arg);
    if (ret < 0) {
        ErrPrint("Unable to send a command: %d", ret);
        delete arg;
        arg = nullptr;
    }

    // NOTE:
    // In case of the Invoke(),
    // we have not to wait a result command.
    // Let it go.. ;)
    return ret;
}

int Inference::Runtime::impl::Async::GetOutput(beyond_tensor *&tensor, int &size)
{
    int cmdId = Command::IdLast;
    void *cmdData = nullptr;

    int ret = outputConsumer->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdGetOutput) {
        return ret;
    }

    assert(cmdData != nullptr);
    CommandData *arg = static_cast<CommandData *>(cmdData);

    if (arg->ret.value < 0) {
        return arg->ret.value;
    }

    tensor = arg->args.tensor.tensor;
    size = arg->args.tensor.size;
    ret = arg->ret.value;

    // NOTE:
    // The ownership of the "arg" is here.
    // Now, delete it and clean up the resource.
    delete arg;
    arg = nullptr;

    return ret;
}

int Inference::Runtime::impl::Async::Stop(void)
{
    CommandData arg = {};
    int ret = command->Send(Command::IdStop, &arg);
    if (ret < 0) {
        return ret;
    }

    int cmdId = Command::IdLast;
    void *cmdData = nullptr;
    ret = command->Recv(cmdId, cmdData);
    if (ret < 0 || cmdId != Command::IdStop) {
        return ret;
    }

    assert(cmdData == &arg && "Invalid argument pointer");

    CommandData *argPtr = static_cast<CommandData *>(cmdData);
    return argPtr->ret.value;
}

int Inference::Runtime::impl::Async::GetHandle(void) const
{
    return eventObject->GetHandle();
}

int Inference::Runtime::impl::Async::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject->AddHandler(handler, type, data);
}

int Inference::Runtime::impl::Async::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject->RemoveHandler(handler, type, data);
}

int Inference::Runtime::impl::Async::FetchEventData(EventObjectInterface::EventData *&info)
{
    return eventObject->FetchEventData(info);
}

int Inference::Runtime::impl::Async::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return eventObject->DestroyEventData(data);
}

} // namespace beyond
