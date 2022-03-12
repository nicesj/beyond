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
#include <cerrno>
#include <exception>
#include <cstring>

#include <unistd.h>
#include <getopt.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/private/module_interface_private.h"
#include "beyond/private/event_object_private.h"
#include "beyond/private/inference_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_runtime_private.h"
#include "beyond/private/inference_peer_interface_private.h"

#include "inference_impl.h"
#include "inference_impl_local.h"
#include "inference_impl_remote.h"
#include "inference_impl_edge.h"
#include "inference_impl_distribute.h"

namespace beyond {

Inference::impl *Inference::impl::Create(const beyond_argument *arg)
{
    Inference::impl *instance;
    char *default_argv = const_cast<char *>(BEYOND_INFERENCE_MODE_LOCAL);
    beyond_argument default_arg = {
        .argc = 1,
        .argv = &default_argv,
    };

    if (arg == nullptr || arg->argc == 0) {
        DbgPrint("Arguments are not provided. Fallback to the local inference");
        arg = &default_arg;
    }

    try {
        instance = new Inference::impl();
    } catch (std::exception &e) {
        ErrPrint("new Inference::impl: %s", e.what());
        return nullptr;
    }

    if (instance->ParseArguments(arg->argc, arg->argv) < 0) {
        delete instance;
        instance = nullptr;
        return nullptr;
    }

    return instance;
}

Inference::impl::impl(void)
    : instance(nullptr)
{
}

int Inference::impl::ParseArguments(int argc, char *argv[])
{
    const option opts[] = {
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_OPTION_AUTO_SPLIT), // Split a model by BeyonD automatically
            .has_arg = 0,
            .flag = nullptr,
            .val = 's',
        },
    };
    int idx;
    int c;
    bool autoSplit = false;

    optind = 0;
    opterr = 0;

    while ((c = getopt_long(argc, argv, "-:s", opts, &idx)) != -1) {
        switch (c) {
        case 's': // split
            autoSplit = true;
            break;
        default:
            break;
        }
    }

    // TODO: Expand inference modes.
    // if (strncmp(argv[0], BEYOND_INFERENCE_MODE_LOCAL, sizeof(BEYOND_INFERENCE_MODE_LOCAL)) == 0) {
    //     instance = Inference::impl::local::Create(autoSplit);
    // } else if (strncmp(argv[0], BEYOND_INFERENCE_MODE_REMOTE, sizeof(BEYOND_INFERENCE_MODE_REMOTE)) == 0) {
    //     instance = Inference::impl::remote::Create(autoSplit);
    // } else if (strncmp(argv[0], BEYOND_INFERENCE_MODE_EDGE, sizeof(BEYOND_INFERENCE_MODE_EDGE)) == 0) {
    //     instance = Inference::impl::edge::Create(autoSplit);
    // } else if (strncmp(argv[0], BEYOND_INFERENCE_MODE_DISTRIBUTE, sizeof(BEYOND_INFERENCE_MODE_DISTRIBUTE)) == 0) {
    //     instance = Inference::impl::distribute::Create(autoSplit);
    // } else {
    //     ErrPrint("Unknown inference mode selected: <%s>", argv[0]);
    //     return -EINVAL;
    // }

    if (strncmp(argv[0], BEYOND_INFERENCE_MODE_LOCAL, sizeof(BEYOND_INFERENCE_MODE_LOCAL)) == 0) {
        instance = Inference::impl::local::Create(autoSplit);
    } else if (strncmp(argv[0], BEYOND_INFERENCE_MODE_REMOTE, sizeof(BEYOND_INFERENCE_MODE_REMOTE)) == 0) {
        instance = Inference::impl::remote::Create(autoSplit);
    } else {
        ErrPrint("Unknown inference mode selected: <%s>", argv[0]);
        return -EINVAL;
    }

    return 0;
}

void Inference::impl::Destroy(void)
{
    instance->Destroy();

    // TODO:
    // Destroy() method will be removed after applying the smart pointer.
    // We will use this until remove all Destroy() method.
    // Reference: https://github.sec.samsung.net/BeyonD/SprintC-next/issues/77
    delete this;
}

int Inference::impl::Configure(const beyond_config *options)
{
    // TODO:
    // Configure the BeyonD framework
    return instance->Configure(options);
}

int Inference::impl::LoadModel(const char *model)
{
    return instance->LoadModel(model);
}

// Load multiple model files
// Model files should be parsed first (extract necessary information if possible)
// and then use the extracted information for mapping the model and the runtime(or peer)
int Inference::impl::LoadModel(const char **model, int size)
{
    if (model == nullptr) {
        return -EINVAL;
    }

    return instance->LoadModel(model, size);
}

int Inference::impl::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return instance->GetInputTensorInfo(info, size);
}

int Inference::impl::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return instance->GetOutputTensorInfo(info, size);
}

int Inference::impl::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    return instance->SetInputTensorInfo(info, size);
}

int Inference::impl::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    return instance->SetOutputTensorInfo(info, size);
}

int Inference::impl::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    return instance->AllocateTensor(info, size, tensor);
}

void Inference::impl::FreeTensor(beyond_tensor *&tensor, int size)
{
    return instance->FreeTensor(tensor, size);
}

int Inference::impl::Prepare(void)
{
    return instance->Prepare();
}

int Inference::impl::Invoke(const beyond_tensor *input, int size, const void *context)
{
    return instance->Invoke(input, size, context);
}

int Inference::impl::GetOutput(beyond_tensor *&tensor, int &size)
{
    // NOTE:
    // can be called
    // in the event callback
    // in the main context
    //
    // TODO:
    // If the output tensor is not ready, the method call should be blocked
    return instance->GetOutput(tensor, size);
}

int Inference::impl::Stop(void)
{
    return instance->Stop();
}

// Add runtime modules for invoke local inference
int Inference::impl::AddRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return instance->AddRuntime(runtime);
}

int Inference::impl::RemoveRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return instance->RemoveRuntime(runtime);
}

// Add peer modules for invoke remote inference
int Inference::impl::AddPeer(InferenceInterface::PeerInterface *peer)
{
    return instance->AddPeer(peer);
}

int Inference::impl::RemovePeer(InferenceInterface::PeerInterface *peer)
{
    return instance->RemovePeer(peer);
}

int Inference::impl::GetHandle(void) const
{
    return instance->GetHandle();
}

int Inference::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return instance->AddHandler(handler, type, data);
}

int Inference::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return instance->RemoveHandler(handler, type, data);
}

int Inference::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    return instance->FetchEventData(data);
}

int Inference::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return instance->DestroyEventData(data);
}

} // namespace beyond
