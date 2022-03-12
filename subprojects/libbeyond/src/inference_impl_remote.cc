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
#include <cstring>
#include <cassert>

#include <exception>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/event_object_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/event_loop_private.h"

#include "inference_impl.h"
#include "inference_impl_remote.h"
#include "inference_impl_event_object.h"

namespace beyond {

Inference::impl::remote *Inference::impl::remote::Create(bool autoSplit)
{
    Inference::impl::remote *impl;

    try {
        impl = new Inference::impl::remote();
    } catch (std::exception &e) {
        ErrPrint("new inference impl remote: %s", e.what());
        return nullptr;
    }

    // NOTE:
    // Even though we need minimize the implementation for the lowering the complexity,
    // we should manage the very basic frame how the implementation will go on.
    // This event loop is a basic one that going to be used for managing multiple events from multiple sources.
    //
    // NOTE:
    // The eventObject is going to be used for manage the event handlers added to the Co-inference (remote mode)
    impl->eventObject = Inference::impl::EventObject::Create();
    if (impl->eventObject == nullptr) {
        impl->Destroy();
        impl = nullptr;
        return nullptr;
    }

    impl->autoSplit = autoSplit;
    return impl;
}

void Inference::impl::remote::Destroy(void)
{
    if (eventObject != nullptr) {
        eventObject->Destroy();
        eventObject = nullptr;
    }

    delete this;
}

Inference::impl::remote::remote(void)
    : eventObject(nullptr)
    , peer(nullptr)
    , autoSplit(false)
{
}

int Inference::impl::remote::Configure(const beyond_config *options)
{
    // TODO:
    // The added peers are configured by caller already.
    // At here, we only care about BeyonD remote mode co-inference itself.
    return 0;
}

int Inference::impl::remote::LoadModel(const char *model)
{
    if (model == nullptr) {
        ErrPrint("Invalid argument (%p)", model);
        return -EINVAL;
    }

    return LoadModel(&model, 1);
}

int Inference::impl::remote::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->GetInputTensorInfo(info, size);
}

int Inference::impl::remote::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->GetOutputTensorInfo(info, size);
}

int Inference::impl::remote::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->SetInputTensorInfo(info, size);
}

// TODO:
// The implementation must be changed in next phase of implementation.
// This is not what we want.
int Inference::impl::remote::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->SetOutputTensorInfo(info, size);
}

int Inference::impl::remote::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->AllocateTensor(info, size, tensor);
}

void Inference::impl::remote::FreeTensor(beyond_tensor *&tensor, int size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return;
    }
    return peer->FreeTensor(tensor, size);
}

// TODO:
// This implementaion is not what we want.
// The invocation must be run asynchronously.
// And the invocation chain must be managed by BeyonD properly.
int Inference::impl::remote::Prepare(void)
{
    if (peerVector.empty() == true) {
        ErrPrint("There is not usable peer found");
        return -EINVAL;
    }

    if (peer != nullptr) {
        DbgPrint("There is a peer already selected");
        return peer->Prepare();
    }

    struct Selected {
        unsigned long long freeStorage;
        unsigned long long freeMemory;
        InferenceInterface::PeerInterface *peer;
    } selected = {
        .freeStorage = 0,
        .freeMemory = 0,
        .peer = nullptr,
    };

    // NOTE:
    // This implementation is based on the current peer information.
    // Until we decide what factors could be used,
    // the comparison conditions could be added or replaced.
    std::vector<InferenceInterface::PeerInterface *>::iterator it;
    for (it = peerVector.begin(); it != peerVector.end(); ++it) {
        InferenceInterface::PeerInterface *_peer = *it;
        const beyond_peer_info *info = nullptr;

        if (_peer->GetInfo(info) < 0 || info == nullptr) {
            continue;
        }

        InfoPrint("name: %s, free memory: %llu, free storage: %llu", info->name, info->free_memory, info->free_storage);

        // We know memory and storage information
        if (info->free_memory > selected.freeMemory) {
            InfoPrint("%s is selected", info->name);
            selected.peer = _peer;
            selected.freeStorage = info->free_storage;
            selected.freeMemory = info->free_memory;
        } else if ((info->free_memory == selected.freeMemory) && (info->free_storage > selected.freeStorage)) {
            InfoPrint("%s is selected", info->name);
            selected.peer = _peer;
            selected.freeStorage = info->free_storage;
            selected.freeMemory = info->free_memory;
        }
    }

    if (selected.peer == nullptr) {
        ErrPrint("Unable to find the best peer, just select the first peer");
        selected.peer = peerVector[0];
    }

    if (modelFile.empty() == false) {
        int ret = selected.peer->LoadModel(modelFile.c_str());
        if (ret < 0) {
            ErrPrint("Unable to load the model[%s]", modelFile.c_str());
            return ret;
        }

        modelFile.clear();
    }

    peer = selected.peer;
    return peer->Prepare();
}

int Inference::impl::remote::Invoke(const beyond_tensor *input, int size, const void *context)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->Invoke(input, size, context);
}

int Inference::impl::remote::GetOutput(beyond_tensor *&tensor, int &size)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->GetOutput(tensor, size);
}

int Inference::impl::remote::Stop(void)
{
    if (peer == nullptr) {
        ErrPrint("Peer is not ready to use");
        return -EINVAL;
    }
    return peer->Stop();
}

int Inference::impl::remote::LoadModel(const char **model, int size)
{
    if (size <= 0 || model == nullptr || *model == nullptr) {
        ErrPrint("invalid argument: size(%d), model(%p), model[0](%p)", size, model, model != nullptr ? *model : nullptr);
        return -EINVAL;
    }

    if (size != 1) {
        DbgPrint("TBD: handling the multiple model and multiple peers");
        return -ENOTSUP;
    }

    int ret = 0;
    if (peer == nullptr) {
        // NOTE:
        // Keep the model filename until we choose the best peer (in the prepare() stage)
        modelFile = std::string(model[0]);
    } else {
        ret = peer->LoadModel(model[0]);
    }

    return ret;
}

int Inference::impl::remote::AddRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    ErrPrint("Remote inference does not require the runtime object");
    return -EINVAL;
}

int Inference::impl::remote::RemoveRuntime(InferenceInterface::RuntimeInterface *runtime)
{
    ErrPrint("Remote inference does not require the runtime object");
    return -EINVAL;
}

beyond_handler_return Inference::impl::remote::PeerEventHandler(beyond_object_h obj, int type, beyond_event_info *eventInfo, void *data)
{
    if ((type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        DbgPrint("BeyonD Error Event");
        return BEYOND_HANDLER_RETURN_RENEW;
    }

    Inference::impl::remote *remote = static_cast<Inference::impl::remote *>(data);

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
    if (remote->eventObject->PublishEventData(eventData) < 0) {
        assert(!"Failed to publish the event data");
        DbgPrint("Failed to publish the event data");
        delete eventData;
        eventData = nullptr;
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

// Add peer modules for invoke remote inference
int Inference::impl::remote::AddPeer(InferenceInterface::PeerInterface *peer)
{
    int ret;

    ret = peer->AddHandler(
        Inference::impl::remote::PeerEventHandler,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        static_cast<void *>(this));
    if (ret < 0) {
        ErrPrint("Failed to add event handler");
        return ret;
    }

    // TODO:
    // The peer should be activated when the co-inference needs to use it.
    // At least now, just activate each peer when it is added simply.
    ret = peer->Activate();
    if (ret < 0) {
        (void)peer->RemoveHandler(
            Inference::impl::remote::PeerEventHandler,
            beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
            static_cast<void *>(this));
        return ret;
    }

    peerVector.push_back(peer);
    return ret;
}

int Inference::impl::remote::RemovePeer(InferenceInterface::PeerInterface *peer)
{
    std::vector<InferenceInterface::PeerInterface *>::iterator it;
    it = std::find(peerVector.begin(), peerVector.end(), peer);
    if (it == peerVector.end()) {
        DbgPrint("Peer is not found");
        return -ENOENT;
    }

    // TODO:
    // Even not the removing case, the peer can be deactivated when the co-inference does not
    // need to use the peer anymore while doing inference operation.
    // For an example, the peer getting too slow (or a new one added faster than the old one)
    peer->Deactivate();

    int ret = peer->RemoveHandler(
        Inference::impl::remote::PeerEventHandler,
        beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
        static_cast<void *>(this));

    if (this->peer == peer) {
        this->peer = nullptr;
    }

    peerVector.erase(it);

    return ret;
}

int Inference::impl::remote::GetHandle(void) const
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->GetHandle();
}

int Inference::impl::remote::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->AddHandler(handler, type, data);
}

int Inference::impl::remote::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->RemoveHandler(handler, type, data);
}

int Inference::impl::remote::FetchEventData(EventObjectInterface::EventData *&data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->FetchEventData(data);
}

int Inference::impl::remote::DestroyEventData(EventObjectInterface::EventData *&data)
{
    assert(eventObject != nullptr && "eventObject is nullptr");
    return eventObject->DestroyEventData(data);
}

} // namespace beyond
