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

#ifndef __BEYOND_PRIVATE_INFERENCE_PEER_INTERFACE_H__
#define __BEYOND_PRIVATE_INFERENCE_PEER_INTERFACE_H__

#include <functional>

#include <beyond/common.h>
#include <beyond/private/module_interface_private.h>
#include <beyond/private/inference_interface_private.h>

namespace beyond {

class InferenceInterface::PeerInterface : public ModuleInterface, public InferenceInterface {
public:
    struct EventData : public InferenceInterface::EventData {
    };

public:
    virtual ~PeerInterface(void) = default;

    virtual int Activate(void) = 0;
    virtual int Deactivate(void) = 0;

    virtual int GetInfo(const beyond_peer_info *&info) = 0;

    // NOTE:
    // SetInfo() method must invoke the event handler with BEYOND_PEER_EVENT_INFO_UPDATED event type.
    virtual int SetInfo(beyond_peer_info *info) = 0;

protected:
    PeerInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_INFERENCE_PEER_INTERFACE_H__
