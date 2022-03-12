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
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include <string.h>

char service_name[] = "unittest_discovery";
char port_string[] = "4321";
uint16_t port_uint16 = 4321;

char *argv_server[] = {
    const_cast<char *>("discovery_dns_sd"),
    const_cast<char *>("--server"),
    const_cast<char *>("--name"),
    service_name,
    const_cast<char *>("--port"),
    port_string,
};

int argc_server = sizeof(argv_server) / sizeof(char *);
beyond_argument args_server = {
    .argc = argc_server,
    .argv = argv_server,
};

char *argv_client[] = {
    const_cast<char *>("discovery_dns_sd"),
};

int argc_client = sizeof(argv_client) / sizeof(char *);
beyond_argument args_client = {
    .argc = argc_client,
    .argv = argv_client,
};

TEST(Discovery, CreateServer)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    discovery->Destroy();
}

TEST(Discovery, CreateClient)
{
    auto discovery = beyond::Discovery::Create(&args_client);
    ASSERT_NE(discovery, nullptr);
    discovery->Destroy();
}

TEST(Discovery, CreateWithNullptr)
{
    auto discovery = beyond::Discovery::Create(nullptr);
    ASSERT_EQ(discovery, nullptr);
}

TEST(Discovery, CreateWithInvalidModule)
{
    char *argv_invalid[] = {
        const_cast<char *>("discovery_invalid"),
    };

    int argc_invalid = sizeof(argv_invalid) / sizeof(char *);
    beyond_argument args_invalid = {
        .argc = argc_invalid,
        .argv = argv_invalid,
    };
    auto discovery = beyond::Discovery::Create(&args_invalid);
    ASSERT_EQ(discovery, nullptr);
}

TEST(Discovery, Destroy)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    discovery->Destroy();
}

TEST(Discovery, Configure)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    auto ret = discovery->Configure(nullptr);
    EXPECT_EQ(ret, -ENOSYS);
    discovery->Destroy();
}

TEST(Discovery, Activate)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    auto ret = discovery->Configure(nullptr);
    EXPECT_EQ(ret, -ENOSYS);
    ret = discovery->Activate();
    EXPECT_EQ(ret, 0);
    discovery->Deactivate();
    discovery->Destroy();
}

TEST(Discovery, Deactivate)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    auto ret = discovery->Configure(nullptr);
    EXPECT_EQ(ret, -ENOSYS);
    ret = discovery->Activate();
    EXPECT_EQ(ret, 0);
    ret = discovery->Deactivate();
    EXPECT_EQ(ret, 0);
    discovery->Destroy();
}

TEST(Discovery, GetHandle)
{
    auto discovery = beyond::Discovery::Create(&args_server);
    ASSERT_NE(discovery, nullptr);
    auto ret = discovery->Configure(nullptr);
    EXPECT_EQ(ret, -ENOSYS);
    ret = discovery->Activate();
    EXPECT_EQ(ret, 0);
    ret = discovery->GetHandle();
    EXPECT_NE(ret, -1);
    discovery->Deactivate();
    discovery->Destroy();
}

TEST(Discovery, FetchEventDataServer)
{
    auto server = beyond::Discovery::Create(&args_server);
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->Activate(), 0);

    beyond::EventObjectInterface::EventData *evtData = nullptr;
    int ret;
    for (int i = 0; i < 1000; i++) {
        ret = server->FetchEventData(evtData);
        if (ret == -EAGAIN) {
            usleep(1000);
            continue;
        } else {
            break;
        }
    }
    EXPECT_EQ(ret, 0);
    ASSERT_NE(evtData, nullptr);
    ASSERT_EQ(static_cast<beyond_event_type>(evtData->type), beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED);
    auto name = static_cast<char *>(evtData->data);
    EXPECT_STREQ(name, service_name);

    server->DestroyEventData(evtData);
    server->Deactivate();
    server->Destroy();
}

TEST(Discovery, FetchEventDataClient)
{
    auto server = beyond::Discovery::Create(&args_server);
    ASSERT_NE(server, nullptr);
    auto client = beyond::Discovery::Create(&args_client);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(server->SetItem("uuid", "test", 4), 0);
    EXPECT_EQ(server->Activate(), 0);
    EXPECT_EQ(client->Activate(), 0);

    beyond::EventObjectInterface::EventData *evtData = nullptr;
    int ret;
    for (int i = 0; i < 1000; i++) {
        ret = client->FetchEventData(evtData);
        if (ret == -EAGAIN) {
            usleep(1000);
            continue;
        } else {
            break;
        }
    }
    EXPECT_EQ(ret, 0);
    ASSERT_NE(evtData, nullptr);
    ASSERT_EQ(static_cast<beyond_event_type>(evtData->type), beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED);
    auto info = static_cast<beyond_peer_info *>(evtData->data);
    EXPECT_NE(info, nullptr);
    EXPECT_STREQ(info->name, service_name);
    EXPECT_EQ(info->port[0], port_uint16);
    EXPECT_STREQ(info->uuid, "test");

    client->DestroyEventData(evtData);
    client->Deactivate();
    client->Destroy();
    server->Deactivate();
    server->Destroy();
}
