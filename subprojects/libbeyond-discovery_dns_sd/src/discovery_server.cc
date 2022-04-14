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
#include "discovery_server.h"

#include <fcntl.h>
#include <cstring>

DiscoveryServer::DiscoveryServer(const std::string name, uint16_t port)
{
    int err = pipe(eventPipe);
    if (err < 0) {
        ErrPrintCode(errno, "pipe");
        throw std::runtime_error("pipe() Fail");
    }

    if (fcntl(eventPipe[0], F_SETFL, O_NONBLOCK) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(eventPipe[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(eventPipe[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        throw std::runtime_error("fcntl() Fail");
    }

    if (fcntl(eventPipe[1], F_SETFL, O_NONBLOCK) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(eventPipe[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(eventPipe[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        throw std::runtime_error("fcntl() Fail");
    }

    if (fcntl(eventPipe[0], F_SETFD, FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(eventPipe[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(eventPipe[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        throw std::runtime_error("fcntl() Fail");
    }

    if (fcntl(eventPipe[1], F_SETFD, FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");
        if (close(eventPipe[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(eventPipe[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        throw std::runtime_error("fcntl() Fail");
    }

    info.name = name;
    info.port = port;
}

DiscoveryServer::~DiscoveryServer()
{
    if (close(eventPipe[1]) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(eventPipe[0]) < 0) {
        ErrPrintCode(errno, "close");
    }
}

void DiscoveryServer::Destroy(void)
{
    DbgPrint("DiscoveryServer Destroy");
    delete this;
}

int DiscoveryServer::Activate(void)
{
    return nsd.registerService(info, std::bind(&DiscoveryServer::onServiceRegistered, this, std::placeholders::_1));
}

void DiscoveryServer::onServiceRegistered(std::string serviceName)
{
    if (serviceName != info.name) {
        DbgPrint("%s is renamed as %s", info.name.c_str(), serviceName.c_str());
        info.name = serviceName;
    }

    EventObjectInterface::EventData *eventData = newEventData();
    if (eventData == nullptr)
        return;

    eventData->type = beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED;
    auto *serverData = static_cast<server_event_data *>(eventData->data);
    serverData->name = strdup(info.name.c_str());

    int ret = write(eventPipe[1], &eventData, sizeof(eventData));
    if (ret < 0) {
        ErrPrintCode(errno, "write() Fail");
        DestroyEventData(eventData);
    }
}

DiscoveryServer::EventData *DiscoveryServer::newEventData()
{
    EventData *eventData = nullptr;
    try {
        eventData = new EventData();
        eventData->data = new server_event_data();
    } catch (std::exception &e) {
        ErrPrint("new() Fail(%s)", e.what());
        delete eventData;
        return nullptr;
    }
    return eventData;
}

int DiscoveryServer::Deactivate(void)
{
    return nsd.unregisterService();
}

int DiscoveryServer::SetItem(const char *key, const void *value, uint8_t valueSize)
{
    InfoPrint("DiscoveryServer SetItem");
    if (key == nullptr) {
        ErrPrint("key is nullptr");
        return -EINVAL;
    }

    return info.setValue(key, DiscoveryInfo::Value(value, valueSize));
}

int DiscoveryServer::RemoveItem(const char *key)
{
    InfoPrint("DiscoveryServer RemoveItem");
    if (key == nullptr) {
        ErrPrint("key is nullptr");
        return -EINVAL;
    }

    return info.removeValue(key);
}

int DiscoveryServer::FetchEventData(EventObjectInterface::EventData *&data)
{
    InfoPrint("DiscoveryServer FetchEventData");
    int ret = read(eventPipe[0], &data, sizeof(data));
    if (ret < 0) {
        ErrPrintCode(errno, "read() Fail");
        return -errno;
    }

    return 0;
}

int DiscoveryServer::DestroyEventData(EventObjectInterface::EventData *&data)
{
    InfoPrint("DiscoveryServer DestroyEventData");
    if (data == nullptr)
        return 0;

    auto serverData = static_cast<server_event_data *>(data->data);
    free(serverData->name);
    delete serverData;
    delete data;
    data = nullptr;

    return 0;
}

int DiscoveryServer::GetHandle(void) const
{
    return eventPipe[0];
}
