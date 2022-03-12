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

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/private/discovery_private.h"
#include "beyond/common.h"
#include "beyond/session.h"
#include "beyond/discovery.h"

#include "session_internal.h"
#include "authenticator_internal.h"
#include "beyond_generic_internal.h"

struct beyond_discovery {
    beyond_generic_handle _generic;
};

beyond::DiscoveryInterface *beyond_discovery_get_discovery(beyond_discovery_h handle)
{
    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_generic_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0) {
        return nullptr;
    }
    return discovery;
}

beyond_discovery_h beyond_discovery_create(beyond_session_h session, struct beyond_argument *arg)
{
    return nullptr;
}

// Create a peer from this event callback
// Update peer information from this event callback
// event structure is able to contain the configuration parameter for creating a peer instance
// the configuration parameter could be an instance of some kind of connection session to make a channel of control between each other
int beyond_discovery_set_event_callback(beyond_discovery_h handle, int (*event)(beyond_discovery_h handle, struct beyond_event_info *event, void *data), void *data)
{
    return -ENOSYS;
}

int beyond_discovery_configure(beyond_discovery_h handle, struct beyond_config *config)
{
    if (handle == nullptr) {
        ErrPrint("Invalid handle");
        return -EINVAL;
    }

    beyond::DiscoveryInterface *discovery = nullptr;
    if (beyond_generic_handle_get_handle<beyond::DiscoveryInterface>(handle, discovery) < 0 || discovery == nullptr) {
        return -EINVAL;
    }

    struct beyond_config _config;
    if (config != nullptr) {
        _config.type = config->type;
        if (beyond_generic_handle_get_handle<void>(config->object, _config.object) < 0 || _config.object == nullptr) {
            _config.object = config->object;
        }
        config = &_config;
    }

    return discovery->Configure(config);
}

// Activate discovery module
int beyond_discovery_activate(beyond_discovery_h handle)
{
    return -ENOSYS;
}

// Deactivate discovery module
int beyond_discovery_deactivate(beyond_discovery_h handle)
{
    return -ENOSYS;
}

void beyond_discovery_destroy(beyond_discovery_h handle)
{
    return;
}
