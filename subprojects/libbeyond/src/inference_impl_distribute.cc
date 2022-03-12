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

#include "inference_impl_distribute.h"
#include "inference_impl.h"

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/event_object_private.h"
#include "beyond/private/inference_interface_private.h"

#include <cerrno>

namespace beyond {

Inference::impl::distribute *Inference::impl::distribute::Create(bool autoSplit)
{
    return nullptr;
}

void Inference::impl::distribute::Destroy(void)
{
}

int Inference::impl::distribute::Configure(const beyond_config *options)
{
    return -ENOSYS;
}

int Inference::impl::distribute::LoadModel(const char *model)
{
    return -ENOSYS;
}

int Inference::impl::distribute::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    return -ENOSYS;
}

void Inference::impl::distribute::FreeTensor(beyond_tensor *&tensor, int size)
{
    return;
}

int Inference::impl::distribute::Prepare(void)
{
    return -ENOSYS;
}

int Inference::impl::distribute::Invoke(const beyond_tensor *input, int size, const void *context)
{
    return -ENOSYS;
}

int Inference::impl::distribute::GetOutput(beyond_tensor *&tensor, int &size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::Stop(void)
{
    return -ENOSYS;
}

int Inference::impl::distribute::LoadModel(const char **model, int size)
{
    return -ENOSYS;
}

int Inference::impl::distribute::AddRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return -ENOSYS;
}

int Inference::impl::distribute::RemoveRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    return -ENOSYS;
}

// Add peer modules for invoke remote inference
int Inference::impl::distribute::AddPeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOSYS;
}

int Inference::impl::distribute::RemovePeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOSYS;
}

int Inference::impl::distribute::GetHandle(void) const
{
    return -1;
}

int Inference::impl::distribute::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOSYS;
}

int Inference::impl::distribute::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOSYS;
}

int Inference::impl::distribute::FetchEventData(EventObjectInterface::EventData *&data)
{
    return -ENOSYS;
}

int Inference::impl::distribute::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return -ENOSYS;
}

} // namespace beyond
