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

#ifndef __BEYOND_DISCOVERY_DNS_SD_H__
#define __BEYOND_DISCOVERY_DNS_SD_H__

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include <beyond/plugin/discovery_dns_sd_plugin.h>

class API Discovery : public beyond::DiscoveryInterface::RuntimeInterface {
public:
    Discovery(void) = default;
    virtual ~Discovery(void) = default;

public:
    static constexpr const char *NAME = "discovery_dns_sd";

    // module interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int Configure(const beyond_config *options = nullptr) override;
};

#endif // __BEYOND_DISCOVERY_DNS_SD_H__
