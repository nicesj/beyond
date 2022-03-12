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

#ifndef __BEYOND_INTERNAL_DISCOVERY_RUNTIME_IMPL_H__
#define __BEYOND_INTERNAL_DISCOVERY_RUNTIME_IMPL_H__

#include "beyond/common.h"
#include "beyond/private/discovery_runtime_private.h"

namespace beyond {

class Discovery::Runtime::impl final : public Discovery::Runtime {
public: // Runtime interface
    static impl *Create(const beyond_argument *arg);
    void Destroy(void) override;

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

public: // DiscoveryInterface interface
    int Configure(const beyond_config *options = nullptr) override;

    int Activate(void) override;
    int Deactivate(void) override;

    int SetItem(const char *key, const void *value, uint8_t valueSize) override;
    int RemoveItem(const char *key) override;

public: // ModuleInterface interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;

private:
    impl(void);
    virtual ~impl(void);

    void *dlHandle;
    DiscoveryInterface::RuntimeInterface *module;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_DISCOVERY_RUNTIME_IMPL_H__
