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

#include "inference_impl_edge.h"
#include "inference_impl.h"

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/event_object_private.h"
#include "beyond/private/inference_interface_private.h"

#include <cerrno>

namespace beyond {

Inference::impl::edge *Inference::impl::edge::Create(bool autoSplit)
{
    return nullptr;
}

void Inference::impl::edge::Destroy(void)
{
}

int Inference::impl::edge::Configure(const beyond_config *options)
{
    return -ENOSYS;
}

int Inference::impl::edge::LoadModel(const char *model)
{
    return -ENOSYS;
}

int Inference::impl::edge::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return -ENOSYS;
}

int Inference::impl::edge::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return -ENOSYS;
}

int Inference::impl::edge::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    return -ENOSYS;
}

int Inference::impl::edge::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    return -ENOSYS;
}

int Inference::impl::edge::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    return -ENOSYS;
}

void Inference::impl::edge::FreeTensor(beyond_tensor *&tensor, int size)
{
    return;
}

int Inference::impl::edge::Prepare(void)
{
    return -ENOSYS;
}

int Inference::impl::edge::Invoke(const beyond_tensor *input, int size, const void *context)
{
    return -ENOSYS;
}

int Inference::impl::edge::GetOutput(beyond_tensor *&tensor, int &size)
{
    return -ENOSYS;
}

int Inference::impl::edge::Stop(void)
{
    return -ENOSYS;
}

int Inference::impl::edge::LoadModel(const char **model, int size)
{
    return -ENOSYS;
}

int Inference::impl::edge::AddRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return -ENOSYS;
}

int Inference::impl::edge::RemoveRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return -ENOSYS;
}

// Add peer modules for invoke remote inference
int Inference::impl::edge::AddPeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOSYS;
}

int Inference::impl::edge::RemovePeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOSYS;
}

int Inference::impl::edge::GetHandle(void) const
{
    return -1;
}

int Inference::impl::edge::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOSYS;
}

int Inference::impl::edge::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOSYS;
}

int Inference::impl::edge::FetchEventData(EventObjectInterface::EventData *&data)
{
    return -ENOSYS;
}

int Inference::impl::edge::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return -ENOSYS;
}

} // namespace beyond
