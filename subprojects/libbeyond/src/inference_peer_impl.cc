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
#include <getopt.h>

#include <exception>

#include <dlfcn.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/private/module_interface_private.h"

#include "inference_peer_impl.h"

// NOTE:
// This implementation is designed to isolate the module function call.
// Before and after call the module's function, we can add our safety guard.
// We can esitmate time and give the deadline of function execution.

namespace beyond {

Inference::Peer::impl *Inference::Peer::impl::Create(const beyond_argument *arg)
{
    if (arg == nullptr || arg->argc < 1 || arg->argv == nullptr || arg->argv[0] == nullptr) {
        ErrPrint("Invalid arguments");
        return nullptr;
    }

    Inference::Peer::impl *impl;

    try {
        impl = new Inference::Peer::impl();
    } catch (std::exception &e) {
        ErrPrint("new Inference::Peer::impl(): %s", e.what());
        return nullptr;
    }

    int namelen = strlen(ModuleInterface::FilenameFormat) + strlen(arg->argv[0]) - 1;
    char *filename = static_cast<char *>(malloc(namelen));
    if (filename == nullptr) {
        // error
        ErrPrintCode(errno, "malloc");
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    int ret = snprintf(filename, namelen, ModuleInterface::FilenameFormat, arg->argv[0]);
    if (ret < 0) {
        // error
        ErrPrintCode(errno, "snprintf: %d (%d) - %s (%s) %s", ret, namelen, filename, arg->argv[0], ModuleInterface::FilenameFormat);
        free(filename);
        filename = nullptr;
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    DbgPrint("Load module: %s", filename);
    impl->dlHandle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    free(filename);
    filename = nullptr;
    if (impl->dlHandle == nullptr) {
        ErrPrint("dlopen failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    ModuleInterface::EntryPoint main = reinterpret_cast<ModuleInterface::EntryPoint>(dlsym(impl->dlHandle, ModuleInterface::EntryPointSymbol));
    if (main == nullptr) {
        // error, not found main symbol
        ErrPrint("dlsym failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    optind = 0;
    opterr = 0;

    InferenceInterface::PeerInterface *peer = reinterpret_cast<InferenceInterface::PeerInterface *>(main(arg->argc, arg->argv));
    if (peer == nullptr) {
        ErrPrint("Initialization failed");
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    const char *moduleType = peer->GetModuleType();
    if (moduleType == nullptr) {
        peer->Destroy();
        peer = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (strcmp(moduleType, ModuleInterface::TYPE_PEER) != 0) {
        peer->Destroy();
        peer = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->module = peer;
    return impl;
}

Inference::Peer::impl::impl(void)
    : dlHandle(nullptr)
    , module(nullptr)
{
}

Inference::Peer::impl::~impl(void)
{
    if (dlHandle != nullptr) {
        if (dlclose(dlHandle) != 0) {
            ErrPrint("dlclose: %s", dlerror());
        }
        dlHandle = nullptr;
    }
}

void Inference::Peer::impl::Destroy(void)
{
    module->Destroy();
    module = nullptr;
    delete this;
}

int Inference::Peer::impl::Activate(void)
{
    return module->Activate();
}

int Inference::Peer::impl::Deactivate(void)
{
    return module->Deactivate();
}

int Inference::Peer::impl::GetInfo(const beyond_peer_info *&info)
{
    return module->GetInfo(info);
}

// NOTE:
// SetInfo() method must invoke the event handler with BEYOND_PEER_EVENT_INFO_UPDATED event type.
int Inference::Peer::impl::SetInfo(beyond_peer_info *info)
{
    return module->SetInfo(info);
}

const char *Inference::Peer::impl::GetModuleName(void) const
{
    return module->GetModuleName();
}

const char *Inference::Peer::impl::GetModuleType(void) const
{
    return module->GetModuleType();
}

int Inference::Peer::impl::Configure(const beyond_config *options)
{
    return module->Configure(options);
}

int Inference::Peer::impl::LoadModel(const char *model)
{
    return module->LoadModel(model);
}

int Inference::Peer::impl::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return module->GetInputTensorInfo(info, size);
}

int Inference::Peer::impl::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return module->GetOutputTensorInfo(info, size);
}

int Inference::Peer::impl::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    return module->SetInputTensorInfo(info, size);
}

int Inference::Peer::impl::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    return module->SetOutputTensorInfo(info, size);
}

int Inference::Peer::impl::Prepare(void)
{
    return module->Prepare();
}

int Inference::Peer::impl::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    return module->AllocateTensor(info, size, tensor);
}

void Inference::Peer::impl::FreeTensor(beyond_tensor *&tensor, int size)
{
    return module->FreeTensor(tensor, size);
}

int Inference::Peer::impl::Invoke(const beyond_tensor *input, int size, const void *context)
{
    return module->Invoke(input, size, context);
}

int Inference::Peer::impl::GetOutput(beyond_tensor *&tensor, int &size)
{
    return module->GetOutput(tensor, size);
}

int Inference::Peer::impl::Stop(void)
{
    return module->Stop();
}

int Inference::Peer::impl::GetHandle(void) const
{
    return module->GetHandle();
}

int Inference::Peer::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->AddHandler(handler, type, data);
}

int Inference::Peer::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->RemoveHandler(handler, type, data);
}

int Inference::Peer::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    return module->FetchEventData(data);
}

int Inference::Peer::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return module->DestroyEventData(data);
}

} // namespace beyond
