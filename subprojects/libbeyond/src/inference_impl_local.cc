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

#include <cerrno>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/event_object_private.h"
#include "beyond/private/inference_interface_private.h"

#include "inference_impl.h"
#include "inference_impl_local.h"
#include "inference_impl_event_object.h"

namespace beyond {

Inference::impl::local *Inference::impl::local::Create(bool autoSplit)
{
    Inference::impl::local *impl;

    try {
        impl = new Inference::impl::local();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return nullptr;
    }

    impl->eventObject = Inference::impl::EventObject::Create();
    if (impl->eventObject == nullptr) {
        impl->Destroy();
        impl = nullptr;
        return nullptr;
    }

    impl->autoSplit = autoSplit;
    return impl;
}

void Inference::impl::local::Destroy(void)
{
    if (eventObject != nullptr) {
        eventObject->Destroy();
        eventObject = nullptr;
    }

    delete this;
}

Inference::impl::local::local(void)
    : autoSplit(false)
    , eventObject(nullptr)
    , runtime(nullptr)
{
}

int Inference::impl::local::Configure(const beyond_config *options)
{
    // TODO:
    // The added runtimes are configured by caller already.
    // At here, we only care about BeyonD local mode co-inference itself
    return 0;
}

int Inference::impl::local::LoadModel(const char *model)
{
    if (model == nullptr) {
        ErrPrint("Invalid argument (%p)", model);
        return -EINVAL;
    }

    return LoadModel(model);
}

int Inference::impl::local::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (runtime == nullptr) {
        ErrPrint("runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->GetInputTensorInfo(info, size);
}

int Inference::impl::local::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (runtime == nullptr) {
        ErrPrint("runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->GetOutputTensorInfo(info, size);
}

int Inference::impl::local::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (runtime == nullptr) {
        ErrPrint("runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->SetInputTensorInfo(info, size);
}

int Inference::impl::local::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (runtime == nullptr) {
        ErrPrint("runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->SetOutputTensorInfo(info, size);
}

int Inference::impl::local::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->AllocateTensor(info, size, tensor);
}

void Inference::impl::local::FreeTensor(beyond_tensor *&tensor, int size)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return;
    }
    return runtime->FreeTensor(tensor, size);
}

int Inference::impl::local::Prepare(void)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->Prepare();
}

int Inference::impl::local::Invoke(const beyond_tensor *input, int size, const void *context)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->Invoke(input, size, context);
}

int Inference::impl::local::GetOutput(beyond_tensor *&tensor, int &size)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->GetOutput(tensor, size);
}

int Inference::impl::local::Stop(void)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }
    return runtime->Stop();
}

int Inference::impl::local::LoadModel(const char **model, int size)
{
    if (runtime == nullptr) {
        ErrPrint("Runtime is not ready to use");
        return -EINVAL;
    }

    if (size <= 0 || model == nullptr || *model == nullptr) {
        ErrPrint("invalid argument: size(%d), model(%p), model[0](%p)", size, model, model != nullptr ? *model : nullptr);
        return -EINVAL;
    }

    if (size != 1) {
        DbgPrint("TBD: handling the multiple model and multiple peers");
        return -ENOTSUP;
    }

    return runtime->LoadModel(model[0]);
}

beyond_handler_return Inference::impl::local::RuntimeEventHandler(beyond_object_h obj, int type, beyond_event_info *eventInfo, void *data)
{
    if ((type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        DbgPrint("BeyonD Error Event");
        return BEYOND_HANDLER_RETURN_RENEW;
    }

    Inference::impl::local *local = static_cast<Inference::impl::local *>(data);
    EventObjectInterface::EventData *eventData;

    try {
        eventData = new EventObjectInterface::EventData();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    eventData->type = eventInfo->type;
    eventData->data = eventInfo->data;

    DbgPrint("Publish event data! (0x%.8X, %p)", eventData->type, eventData->data);
    if (local->eventObject->PublishEventData(eventData) < 0) {
        assert(!"Failed to publish the event data");
        DbgPrint("Failed to publish the event data");
        delete eventData;
        eventData = nullptr;
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

int Inference::impl::local::AddRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    int ret;

    if (this->runtime != nullptr) {
        DbgPrint("Not yet support multiple runtimes");
        return -ENOTSUP;
    }

    ret = runtime->AddHandler(
        Inference::impl::local::RuntimeEventHandler,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        static_cast<void *>(this));
    if (ret == -ENOTSUP) {
        DbgPrint("Added runtime does not support asynchronous mode");
    } else if (ret < 0) {
        ErrPrint("Unable to add runtime event handler: %d", ret);
        return ret;
    }

    this->runtime = runtime;
    return 0;
}

int Inference::impl::local::RemoveRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    if (this->runtime != runtime) {
        DbgPrint("Runtime is not found");
        return -ENOENT;
    }

    int ret = runtime->RemoveHandler(
        Inference::impl::local::RuntimeEventHandler,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        static_cast<void *>(this));
    if (ret == -ENOTSUP) {
        DbgPrint("Added runtime does not support asynchronous mode");
    } else if (ret < 0) {
        ErrPrint("Unable to remove runtime event handler");
        return ret;
    }

    this->runtime = nullptr;
    return 0;
}

// Add peer modules for invoke remote inference
int Inference::impl::local::AddPeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOTSUP;
}

int Inference::impl::local::RemovePeer(InferenceInterface::PeerInterface *peer)
{
    return -ENOTSUP;
}

int Inference::impl::local::GetHandle(void) const
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->GetHandle();
}

int Inference::impl::local::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->AddHandler(handler, type, data);
}

int Inference::impl::local::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->RemoveHandler(handler, type, data);
}

int Inference::impl::local::FetchEventData(EventObjectInterface::EventData *&data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->FetchEventData(data);
}

int Inference::impl::local::DestroyEventData(EventObjectInterface::EventData *&data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->DestroyEventData(data);
}

} // namespace beyond
