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

#ifndef __BEYOND_PRIVATE_DISCOVERY_RUNTIME_INTERFACE_H__
#define __BEYOND_PRIVATE_DISCOVERY_RUNTIME_INTERFACE_H__

#include "beyond/private/module_interface_private.h"
#include "beyond/private/discovery_interface_private.h"
#include <arpa/inet.h>

namespace beyond {
class DiscoveryInterface::RuntimeInterface : public ModuleInterface, public DiscoveryInterface {
public:
    struct server_event_data {
        char *name;
    };

    struct client_event_data {
        char *name;
        struct sockaddr address;
        uint16_t port;
        char uuid[BEYOND_UUID_LEN];
    };

    virtual ~RuntimeInterface(void) = default;

protected:
    RuntimeInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_DISCOVERY_RUNTIME_INTERFACE_H__
