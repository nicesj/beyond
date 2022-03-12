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
#include "discovery_client.h"

#include <fcntl.h>

DiscoveryClient::DiscoveryClient()
{
    int err = pipe2(eventPipe, O_CLOEXEC | O_NONBLOCK);
    if (err < 0) {
        ErrPrintCode(errno, "pipe2");
        throw std::runtime_error("pipe2() Fail");
    }
}

DiscoveryClient::~DiscoveryClient()
{
    close(eventPipe[1]);
    close(eventPipe[0]);
}

void DiscoveryClient::Destroy(void)
{
    DbgPrint("DiscoveryClient Destroy");
    delete this;
}

int DiscoveryClient::Activate(void)
{
    return nsd.discoverServices(std::bind(&DiscoveryClient::onServiceDiscovered, this,
                                          std::placeholders::_1, std::placeholders::_2));
}

void DiscoveryClient::onServiceDiscovered(beyond_event_type event, DiscoveryInfo &info)
{
    EventData *eventData = newEventData();
    if (eventData == nullptr)
        return;

    eventData->type = event;
        auto *clientData = static_cast<client_event_data *>(eventData->data);
        clientData->name = strdup(info.name.c_str());
    switch (eventData->type) {
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED:
        memcpy(&(clientData->address), static_cast<const void *>(info.address), sizeof(struct sockaddr));
        clientData->port = info.port;
        snprintf(clientData->uuid, sizeof(clientData->uuid), "%s", info.uuid.c_str());
        break;
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REMOVED:
        // Do nothing
        break;
    default:
        break;
    }

    int ret = write(eventPipe[1], &eventData, sizeof(eventData));
    if (ret < 0) {
        ErrPrintCode(errno, "write() Fail");
    }
}

DiscoveryClient::EventData *DiscoveryClient::newEventData()
{
    EventData *eventData = nullptr;
    try {
        eventData = new EventData();
        eventData->data = new client_event_data();
    } catch (std::exception &e) {
        ErrPrint("new() Fail(%s)", e.what());
        delete eventData;
        return nullptr;
    }
    return eventData;
}

int DiscoveryClient::Deactivate(void)
{
    return nsd.stopServiceDiscovery();
}

int DiscoveryClient::SetItem(const char *key, const void *value, uint8_t valueSize)
{
    ErrPrint("DiscoveryClient SetItem unsupported");
    return -ENOTSUP;
}

int DiscoveryClient::RemoveItem(const char *key)
{
    ErrPrint("DiscoveryClient RemoveItem unsupported");
    return -ENOTSUP;
}

int DiscoveryClient::FetchEventData(EventObjectInterface::EventData *&data)
{
    InfoPrint("DiscoveryClient FetchEventData");

    int ret = read(eventPipe[0], &data, sizeof(data));
    if (ret < 0) {
        ErrPrintCode(errno, "read() Fail");
        return -errno;
    }

    return 0;
}

int DiscoveryClient::DestroyEventData(EventObjectInterface::EventData *&data)
{
    InfoPrint("DiscoveryClient DestroyEventData");
    if (data == nullptr)
        return 0;

    switch (data->type) {
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED: {
        auto clientData = static_cast<client_event_data *>(data->data);
        free(clientData->name);
        delete clientData;
    } break;
    case beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REMOVED:
        auto clientData = static_cast<client_event_data *>(data->data);
        free(clientData->name);
        delete clientData;
        break;
    }

    delete data;
    data = nullptr;

    return 0;
}

int DiscoveryClient::GetHandle(void) const
{
    return eventPipe[0];
}
