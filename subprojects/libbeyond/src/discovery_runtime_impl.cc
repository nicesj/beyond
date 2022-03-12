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
#include <exception>
#include <functional>
#include <cstdlib>
#include <cstring>

#include <dlfcn.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "beyond/private/module_interface_private.h"

#include "discovery_runtime_impl.h"

#define HOST_LEN 32
// NOTE:
// This implementation is going to help loading a discovery module
// that is implemented using various kind of protocols such as
// smartthings, vine(based on mDNSResponder), and customized protocols

namespace beyond {

Discovery::Runtime::impl *Discovery::Runtime::impl::Create(const beyond_argument *arg)
{
    if (arg == nullptr || arg->argc < 1 || arg->argv == nullptr || arg->argv[0] == nullptr) {
        return nullptr;
    }

    Discovery::Runtime::impl *impl;

    try {
        impl = new Discovery::Runtime::impl();
    } catch (std::exception &e) {
        ErrPrint("new Discovery::Runtime::impl(): %s", e.what());
        return nullptr;
    }

    int namelen = strlen(ModuleInterface::FilenameFormat) + strlen(arg->argv[0]) - 1;
    char *filename = static_cast<char *>(malloc(namelen));
    if (filename == nullptr) {
        // error
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    int ret = snprintf(filename, namelen, ModuleInterface::FilenameFormat, arg->argv[0]);
    if (ret < 0) {
        // error
        free(filename);
        filename = nullptr;
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->dlHandle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    free(filename);
    filename = nullptr;
    if (impl->dlHandle == nullptr) {
        ErrPrint("dlopen failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    ModuleInterface::EntryPoint main = reinterpret_cast<ModuleInterface::EntryPoint>(dlsym(impl->dlHandle, ModuleInterface::EntryPointSymbol));
    if (main == nullptr) {
        ErrPrint("dlsym failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    optind = 0;
    opterr = 0;

    DiscoveryInterface::RuntimeInterface *discovery = reinterpret_cast<DiscoveryInterface::RuntimeInterface *>(main(arg->argc, arg->argv));
    if (discovery == nullptr) {
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    const char *moduleType = discovery->GetModuleType();
    if (moduleType == nullptr) {
        discovery->Destroy();
        discovery = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (strcmp(moduleType, ModuleInterface::TYPE_DISCOVERY) != 0) {
        discovery->Destroy();
        discovery = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->module = discovery;
    return impl;
}

Discovery::Runtime::impl::impl(void)
    : dlHandle(nullptr)
    , module(nullptr)
{
}

Discovery::Runtime::impl::~impl(void)
{
    if (dlHandle != nullptr) {
        if (dlclose(dlHandle) < 0) {
            ErrPrint("dlclose: %s", dlerror());
        }
        dlHandle = nullptr;
    }
}

void Discovery::Runtime::impl::Destroy(void)
{
    module->Destroy();
    module = nullptr;

    delete this;
}

int Discovery::Runtime::impl::Configure(const beyond_config *options)
{
    return module->Configure(options);
}

int Discovery::Runtime::impl::Activate(void)
{
    return module->Activate();
}

int Discovery::Runtime::impl::Deactivate(void)
{
    return module->Deactivate();
}

int Discovery::Runtime::impl::SetItem(const char *key, const void *value, uint8_t valueSize)
{
    return module->SetItem(key, value, valueSize);
}

int Discovery::Runtime::impl::RemoveItem(const char *key)
{
    return module->RemoveItem(key);
}

const char *Discovery::Runtime::impl::GetModuleName(void) const
{
    return module->GetModuleName();
}

const char *Discovery::Runtime::impl::GetModuleType(void) const
{
    return module->GetModuleType();
}

int Discovery::Runtime::impl::GetHandle(void) const
{
    return module->GetHandle();
}

int Discovery::Runtime::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->AddHandler(handler, type, data);
}

int Discovery::Runtime::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->RemoveHandler(handler, type, data);
}

int Discovery::Runtime::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    EventObjectInterface::EventData *sub_data = nullptr;
    int ret = module->FetchEventData(sub_data);
    if (ret < 0) {
        ErrPrint("FetchEventData failed: %d", ret);
        return ret;
    } else if (sub_data == nullptr || sub_data->data == nullptr) {
        ErrPrint("FetchEventData failed: %d", -EFAULT);
        return -EFAULT;
    }

    try {
        data = new EventObjectInterface::EventData();
        data->type = sub_data->type;
    } catch (std::exception &e) {
        ErrPrint("Discovery::Runtime::impl::FetchEventData(): %s", e.what());
        module->DestroyEventData(sub_data);
        return -ENOMEM;
    }

    switch (sub_data->type) {
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED: {
        auto event_data = static_cast<DiscoveryInterface::RuntimeInterface::server_event_data *>(sub_data->data);
        data->data = strdup(event_data->name);
        if (data->data == nullptr) {
            ret = -errno;
            ErrPrint("strndup");
            delete data;
            data = nullptr;
        }
    } break;
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED: {
        beyond_peer_info *info = nullptr;
        try {
            info = new beyond_peer_info();
        } catch (std::exception &e) {
            ErrPrint("new beyond_peer_info(): %s", e.what());
            ret = -ENOMEM;
            delete data;
            data = nullptr;
            break;
        }

        auto event_data = static_cast<DiscoveryInterface::RuntimeInterface::client_event_data *>(sub_data->data);
        auto addr_in = reinterpret_cast<struct sockaddr_in *>(&(event_data->address));
        try {
            info->host = new char[HOST_LEN];
        } catch (std::exception &e) {
            ErrPrint("new char[%d]: %s", HOST_LEN, e.what());
            ret = -ENOMEM;
            delete info;
            delete data;
            data = nullptr;
            break;
        }

        info->name = strdup(event_data->name);
        if (info->name == nullptr) {
            ret = -ENOMEM;
            delete info;
            delete data;
            data = nullptr;
            break;
        }

        if (inet_ntop(event_data->address.sa_family, &(addr_in->sin_addr), info->host, HOST_LEN) == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "inet_ntop");
            delete[] info->host;
            delete info;
            delete data;
            data = nullptr;
            break;
        }
        info->port[0] = event_data->port;
        snprintf(info->uuid, BEYOND_UUID_LEN, "%s", event_data->uuid);
        data->data = info;
    } break;
    case BEYOND_EVENT_TYPE_DISCOVERY_REMOVED: {
        beyond_peer_info *info = nullptr;
        auto event_data = static_cast<DiscoveryInterface::RuntimeInterface::client_event_data *>(sub_data->data);
        try {
            info = new beyond_peer_info();
            info->port[0] = 0;
            data->data = info;
        } catch (std::exception &e) {
            ErrPrint("new beyond_peer_info(): %s", e.what());
            ret = -ENOMEM;
            delete data;
            data = nullptr;
            break;
        }

        info->name = strdup(event_data->name);
        if (info->name == nullptr) {
            ret = -ENOMEM;
            delete info;
            delete data;
            data = nullptr;
            break;
        }
    } break;
    default:
        ErrPrint("Invalid type");
        ret = -EFAULT;
        delete data;
        data = nullptr;
        break;
    }

    module->DestroyEventData(sub_data);
    return ret;
}

int Discovery::Runtime::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    if (data == nullptr) {
        ErrPrint("Try to destroy nullptr");
        return -EINVAL;
    }

    if (data->data == nullptr) {
        ErrPrint("event data is nullptr");
        delete data;
        data = nullptr;
        return -EINVAL;
    }

    switch (data->type) {
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED: {
        auto name = static_cast<char *>(data->data);
        free(name);
    } break;
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED: {
        auto info = static_cast<beyond_peer_info *>(data->data);
        free(info->name);
        delete[] info->host;
        delete info;
    } break;
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REMOVED: {
        auto info = static_cast<beyond_peer_info *>(data->data);
        free(info->name);
        delete info;
    } break;
    default:
        break;
    }

    delete data;
    data = nullptr;
    return 0;
}

} // namespace beyond
