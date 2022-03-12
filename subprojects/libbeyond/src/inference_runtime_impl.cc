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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <exception>

#include <dlfcn.h>
#include <getopt.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "inference_runtime_impl.h"
#include "inference_runtime_impl_async.h"

namespace beyond {

Inference::Runtime::impl *Inference::Runtime::impl::Create(const beyond_argument *arg)
{
    if (arg == nullptr || arg->argc < 1 || arg->argv == nullptr || arg->argv[0] == nullptr) {
        return nullptr;
    }

    Inference::Runtime::impl *impl;

    try {
        impl = new Inference::Runtime::impl();
    } catch (std::exception &e) {
        ErrPrint("new Inference::Runtime::impl(): %s", e.what());
        return nullptr;
    }

    int namelen = strlen(ModuleInterface::FilenameFormat) + strlen(arg->argv[0]) - 1;
    char *filename = static_cast<char *>(malloc(namelen));
    if (filename == nullptr) {
        // error
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    int ret = snprintf(filename, namelen, ModuleInterface::FilenameFormat, arg->argv[0]);
    if (ret < 0) {
        // error
        free(filename);
        filename = nullptr;
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    DbgPrint("Load runtime module: %s", filename);
    impl->dlHandle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    free(filename);
    filename = nullptr;
    if (impl->dlHandle == nullptr) {
        ErrPrint("dlopen: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    ModuleInterface::EntryPoint main = reinterpret_cast<ModuleInterface::EntryPoint>(dlsym(impl->dlHandle, ModuleInterface::EntryPointSymbol));
    if (main == nullptr) {
        ErrPrint("dlsym: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    optind = 0;
    opterr = 0;

    InferenceInterface::RuntimeInterface *runtime = reinterpret_cast<InferenceInterface::RuntimeInterface *>(main(arg->argc, arg->argv));
    if (runtime == nullptr) {
        ErrPrint("Failed to create the runtime instance");
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    const char *moduleType = runtime->GetModuleType();
    if (moduleType == nullptr) {
        ErrPrint("Module type is nullptr");
        runtime->Destroy();
        runtime = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (strcmp(moduleType, ModuleInterface::TYPE_RUNTIME) != 0) {
        ErrPrint("Invalid module type: %s\n", moduleType);
        runtime->Destroy();
        runtime = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->module = runtime;

    if (impl->module->GetHandle() == -ENOTSUP) {
        DbgPrint("Runtime module does not support asynchornous mode, activate the async mode emulator");
        impl->asyncEmulator = Inference::Runtime::impl::Async::Create(impl->module);
        impl->async = impl->asyncEmulator;
        if (impl->async == nullptr) {
            ErrPrint("Unable to activate the async mode emulator");
        }
    } else {
        DbgPrint("Runtime module support async mode, do not activate the async mode emulator");
        impl->async = runtime;
    }

    return impl;
}

Inference::Runtime::impl::impl(void)
    : dlHandle(nullptr)
    , module(nullptr)
    , asyncEmulator(nullptr)
    , async(nullptr)
{
}

Inference::Runtime::impl::~impl(void)
{
    if (dlHandle != nullptr) {
        if (dlclose(dlHandle) != 0) {
            ErrPrint("dlclose: %s", dlerror());
        }
        dlHandle = nullptr;
    }
}

void Inference::Runtime::impl::Destroy(void)
{
    if (asyncEmulator != nullptr) {
        asyncEmulator->Destroy();
        asyncEmulator = nullptr;
    }

    module->Destroy();

    module = nullptr;
    async = nullptr;

    delete this;
}

const char *Inference::Runtime::impl::GetModuleName(void) const
{
    return module->GetModuleName();
}

const char *Inference::Runtime::impl::GetModuleType(void) const
{
    return module->GetModuleType();
}

int Inference::Runtime::impl::Configure(const beyond_config *options)
{
    return async->Configure(options);
}

int Inference::Runtime::impl::LoadModel(const char *model)
{
    return async->LoadModel(model);
}

int Inference::Runtime::impl::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return async->GetInputTensorInfo(info, size);
}

int Inference::Runtime::impl::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return async->GetOutputTensorInfo(info, size);
}

int Inference::Runtime::impl::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    return async->SetInputTensorInfo(info, size);
}

int Inference::Runtime::impl::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    return async->SetOutputTensorInfo(info, size);
}

int Inference::Runtime::impl::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    return async->AllocateTensor(info, size, tensor);
}

void Inference::Runtime::impl::FreeTensor(beyond_tensor *&tensor, int size)
{
    return async->FreeTensor(tensor, size);
}

int Inference::Runtime::impl::Prepare(void)
{
    return async->Prepare();
}

int Inference::Runtime::impl::Invoke(const beyond_tensor *input, int size, const void *context)
{
    return async->Invoke(input, size, context);
}

int Inference::Runtime::impl::GetOutput(beyond_tensor *&tensor, int &size)
{
    return async->GetOutput(tensor, size);
}

int Inference::Runtime::impl::Stop(void)
{
    return async->Stop();
}

int Inference::Runtime::impl::GetHandle(void) const
{
    return async->GetHandle();
}

int Inference::Runtime::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return async->AddHandler(handler, type, data);
}

int Inference::Runtime::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return async->RemoveHandler(handler, type, data);
}

int Inference::Runtime::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    return async->FetchEventData(data);
}

int Inference::Runtime::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return async->DestroyEventData(data);
}

} // namespace beyond
