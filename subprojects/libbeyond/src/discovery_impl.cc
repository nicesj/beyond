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
#include <cstring>
#include <cerrno>

#include <exception>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/private/module_interface_private.h"
#include "beyond/private/discovery_interface_private.h"

#include "discovery_impl.h"

// TODO: Implement the internal discovery service code of BeyonD framework

namespace beyond {
Discovery::impl::impl(void)
    : runtime(nullptr)
{
}

Discovery::impl *Discovery::impl::Create(const beyond_argument *arg)
{
    Discovery::impl *impl;
    auto *runtime = Discovery::Runtime::Create(arg);
    if (runtime == nullptr) {
        return nullptr;
    }

    try {
        impl = new Discovery::impl();
    } catch (std::exception &e) {
        ErrPrint("new DiscoveryImpl: %s", e.what());
        return nullptr;
    }

    impl->runtime = runtime;
    return impl;
}

// TODO:
// This method and its implementation will be removed
void Discovery::impl::Destroy(void)
{
    if (runtime != nullptr) {
        runtime->Destroy();
    }

    delete this;
}

int Discovery::impl::Configure(const beyond_config *options)
{
    if (runtime) {
        return runtime->Configure(options);
    }

    return -ENOSYS;
}

int Discovery::impl::Activate(void)
{
    if (runtime) {
        return runtime->Activate();
    }

    return -ENOSYS;
}

int Discovery::impl::Deactivate(void)
{
    if (runtime) {
        return runtime->Deactivate();
    }

    return -ENOSYS;
}

int Discovery::impl::SetItem(const char *key, const void *value, uint8_t valueSize)
{
    if (runtime) {
        return runtime->SetItem(key, value, valueSize);
    }

    return -ENOSYS;
}

int Discovery::impl::RemoveItem(const char *key)
{
    if (runtime) {
        return runtime->RemoveItem(key);
    }

    return -ENOSYS;
}

int Discovery::impl::GetHandle(void) const
{
    if (runtime) {
        return runtime->GetHandle();
    }

    return -1;
}

int Discovery::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    if (runtime) {
        return runtime->AddHandler(handler, type, data);
    }

    return -ENOSYS;
}

int Discovery::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    if (runtime) {
        return runtime->RemoveHandler(handler, type, data);
    }

    return -ENOSYS;
}

int Discovery::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    if (runtime) {
        return runtime->FetchEventData(data);
    }

    return -ENOSYS;
}

int Discovery::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    if (runtime) {
        return runtime->DestroyEventData(data);
    }

    return -ENOSYS;
}

} // namespace beyond
