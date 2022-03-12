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

#ifndef __BEYOND_PRIVATE_DISCOVERY_INTERFACE_H__
#define __BEYOND_PRIVATE_DISCOVERY_INTERFACE_H__

#include <functional>
#include <beyond/common.h>
#include <beyond/private/event_object_interface_private.h>

namespace beyond {

class API DiscoveryInterface : public EventObjectInterface {
public:
    struct EventData : public EventObjectInterface::EventData {
    };

public:
    class RuntimeInterface;

public:
    virtual ~DiscoveryInterface(void) = default;

    virtual int Configure(const beyond_config *options = nullptr) = 0;

    virtual int Activate(void) = 0;
    virtual int Deactivate(void) = 0;

    virtual int SetItem(const char *key, const void *value, uint8_t valueSize) = 0;
    virtual int RemoveItem(const char *key) = 0;

protected:
    DiscoveryInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_DISCOVERY_INTERFACE_H__
