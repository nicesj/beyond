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

#include <gtest/gtest.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>
#include <beyond/plugin/discovery_dns_sd_plugin.h>
#include "discovery.h"
#include "../src/discovery_client.h"
#include "../src/discovery_server.h"

#define STR_EQ 0
extern "C" void *_main(int argc, char *argv[]);

const char *serverName = "servernametest";

TEST(Discovery, Server)
{
    char *argv_server[] = {
        const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME),
        const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_SERVER),
        const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_NAME),
        const_cast<char *>(serverName),
    };
    int argc_server = (sizeof(argv_server) / sizeof(char *));
    Discovery *server = static_cast<Discovery *>(_main(argc_server, argv_server));

    ASSERT_NE(server, nullptr);
    ASSERT_EQ(server->Activate(), 0);

    beyond::EventObjectInterface::EventData *evtData = nullptr;
    sleep(1);
    server->FetchEventData(evtData);
    ASSERT_NE(evtData, nullptr);

    ASSERT_EQ(static_cast<beyond_event_type>(evtData->type), beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED);
    auto data = static_cast<struct beyond::DiscoveryInterface::RuntimeInterface::server_event_data *>(evtData->data);
    EXPECT_STREQ(data->name, serverName);
    server->DestroyEventData(evtData);
    delete server;
}

class DiscoveryBasicTest : public testing::Test {
protected:
    void SetUp() override
    {
        char *argv_server[] = {
            const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME),
            const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_SERVER),
            const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_NAME),
            const_cast<char *>(serverName),
        };
        int argc_server = (sizeof(argv_server) / sizeof(char *));
        optind = opterr = 0; // For rewinding getopt
        server = static_cast<Discovery *>(_main(argc_server, argv_server));
        ASSERT_NE(server, nullptr);

        char *argv_client[] = {
            const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME),
            const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_CLIENT)
        };
        int argc_client = (sizeof(argv_client) / sizeof(char *));
        optind = opterr = 0; // For rewinding getopt
        client = static_cast<Discovery *>(_main(argc_client, argv_client));
        ASSERT_NE(client, nullptr);
    }

    void TearDown() override
    {
        delete server;
        delete client;
    }

protected:
    Discovery *server;
    Discovery *client;
};

TEST_F(DiscoveryBasicTest, GetHandle)
{
    EXPECT_GT(server->GetHandle(), 0);
    EXPECT_GT(client->GetHandle(), 0);
}

TEST_F(DiscoveryBasicTest, Activate)
{
    EXPECT_EQ(server->Activate(), 0);
    EXPECT_EQ(server->Deactivate(), 0);

    EXPECT_EQ(client->Activate(), 0);
    EXPECT_EQ(client->Deactivate(), 0);
}

TEST_F(DiscoveryBasicTest, ActivateTwice)
{
    EXPECT_EQ(server->Activate(), 0);
    EXPECT_EQ(server->Activate(), -EALREADY);
    EXPECT_EQ(server->Deactivate(), 0);

    EXPECT_EQ(client->Activate(), 0);
    EXPECT_EQ(client->Activate(), -EALREADY);
    EXPECT_EQ(client->Deactivate(), 0);
}

TEST_F(DiscoveryBasicTest, DeactivateWithoutActivate)
{
    EXPECT_EQ(server->Deactivate(), -EALREADY);

    EXPECT_EQ(client->Deactivate(), -EALREADY);
}

TEST_F(DiscoveryBasicTest, SetItem)
{
    EXPECT_EQ(server->SetItem("test", "1234", 4), 0);
}

TEST_F(DiscoveryBasicTest, SetItemClient)
{
    EXPECT_EQ(client->SetItem("test", "1234", 4), -ENOTSUP);
}

TEST_F(DiscoveryBasicTest, SetItemNullptrKey)
{
    EXPECT_EQ(server->SetItem(nullptr, "1234", 4), -EINVAL);
}

TEST_F(DiscoveryBasicTest, SetItemWithSameKey)
{
    EXPECT_EQ(server->SetItem("test", "1234", 4), 0);
    EXPECT_EQ(server->SetItem("test", "5678", 4), -EEXIST);
}

TEST_F(DiscoveryBasicTest, RemoveItem)
{
    EXPECT_EQ(server->SetItem("test", "1234", 4), 0);
    EXPECT_EQ(server->RemoveItem("test"), 0);
}

TEST_F(DiscoveryBasicTest, RemoveItemClient)
{
    EXPECT_EQ(client->RemoveItem("test"), -ENOTSUP);
}

TEST_F(DiscoveryBasicTest, RemoveItemWithoutSetItem)
{
    EXPECT_EQ(server->RemoveItem("test"), -EINVAL);
}

TEST_F(DiscoveryBasicTest, RemoveItemNullptrKey)
{
    EXPECT_EQ(server->RemoveItem(nullptr), -EINVAL);
}

TEST_F(DiscoveryBasicTest, FetchEventData)
{
    EXPECT_EQ(server->SetItem("uuid", "testuuid", 8), 0);
    ASSERT_EQ(server->Activate(), 0);

    beyond::EventObjectInterface::EventData *evtData = nullptr;

    int ret;
    for (int i = 0; i < 10; i++) {
        ret = server->FetchEventData(evtData);
        if (ret == -EAGAIN) {
            usleep(100000);
            continue;
        } else {
            break;
        }
    }

    EXPECT_EQ(ret, 0);
    ASSERT_NE(evtData, nullptr);
    server->DestroyEventData(evtData);

    EXPECT_EQ(client->Activate(), 0);

    for (int i = 0; i < 10; i++) {
        ret = client->FetchEventData(evtData);
        if (ret == -EAGAIN) {
            usleep(100000);
            continue;
        }

        ASSERT_EQ(ret, 0);
        ASSERT_NE(evtData, nullptr);
        ASSERT_EQ(static_cast<beyond_event_type>(evtData->type), beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED);

        auto data = static_cast<struct beyond::DiscoveryInterface::RuntimeInterface::client_event_data *>(evtData->data);
        if (strcmp(data->name, serverName) != STR_EQ) {
            printf("data->name(%s) is not matched with serverName(%s). try again\n", data->name, serverName);
            client->DestroyEventData(evtData);
            ret = -EAGAIN;
            continue;
        }
        auto addr_in = reinterpret_cast<struct sockaddr_in *>(&(data->address));
        char b[32] = "";
        inet_ntop(data->address.sa_family, &(addr_in->sin_addr), b, sizeof(b));
        printf("b: %s\n", b);
        EXPECT_STRNE(b, "");
        if (strcmp(data->uuid, "testuuid") != STR_EQ) {
            printf("uuid: %s is not matched. try again\n", data->uuid);
            client->DestroyEventData(evtData);
            ret = -EAGAIN;
            continue;
        }
        EXPECT_EQ(data->port, 3000);

        client->DestroyEventData(evtData);
        break;
    }

    ASSERT_EQ(ret, 0);

    server->Deactivate();

    for (int i = 0; i < 10; i++) {
        ret = client->FetchEventData(evtData);
        if (ret == -EAGAIN) {
            usleep(200000);
            continue;
        }

        ASSERT_EQ(ret, 0);
        ASSERT_NE(evtData, nullptr);
        ASSERT_EQ(static_cast<beyond_event_type>(evtData->type), beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REMOVED);

        auto data = static_cast<struct beyond::DiscoveryInterface::RuntimeInterface::client_event_data *>(evtData->data);
        if (strcmp(data->name, serverName) != 0) {
            printf("data->name(%s) is not matched with serverName(%s). try again\n", data->name, serverName);
            client->DestroyEventData(evtData);
            ret = -EAGAIN;
            continue;
        }

        client->DestroyEventData(evtData);
        break;
    }

    EXPECT_EQ(ret, 0);

    client->Deactivate();
}
