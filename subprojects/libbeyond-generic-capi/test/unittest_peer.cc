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

#include <beyond/beyond.h>
#include <gtest/gtest.h>

#include "beyond/plugin/peer_nn_plugin.h"

#define TEST_MODULE_EDGE_OPTION (const_cast<char *>("--server"))

class PeerGeneric : public testing::Test {
protected:
    beyond_session_h session;
    beyond_peer_h peer;

protected:
    void SetUp() override
    {
        session = beyond_session_create(0, 0);
        ASSERT_NE(session, nullptr);

        char *argv[] = {
            const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
        };
        int argc = sizeof(argv) / sizeof(char *);
        beyond_argument arg = {
            .argc = argc,
            .argv = argv,
        };

        peer = beyond_peer_create(session, &arg);
        ASSERT_NE(peer, nullptr);
    }

    void TearDown() override
    {
        beyond_peer_destroy(peer);
        beyond_session_destroy(session);
    }
};

TEST_F(PeerGeneric, positve_beyond_peer_create_edge)
{
    beyond_peer_h peer;
    char *argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
        TEST_MODULE_EDGE_OPTION,
    };
    int argc = sizeof(argv) / sizeof(char *);
    beyond_argument arg = {
        .argc = argc,
        .argv = argv,
    };
    peer = beyond_peer_create(session, &arg);
    ASSERT_NE(peer, nullptr);

    beyond_peer_destroy(peer);
}

TEST_F(PeerGeneric, negative_beyond_peer_create_invalidArgv)
{
    beyond_peer_h peer;
    beyond_argument arg = {
        .argc = 1,
        .argv = nullptr,
    };
    peer = beyond_peer_create(session, &arg);
    EXPECT_EQ(peer, nullptr);
}

TEST_F(PeerGeneric, negative_beyond_peer_create_invalidArgc)
{
    beyond_peer_h peer;
    char *argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
        TEST_MODULE_EDGE_OPTION,
    };
    int argc = -1;
    beyond_argument arg = {
        .argc = argc,
        .argv = argv,
    };
    peer = beyond_peer_create(session, &arg);
    EXPECT_EQ(peer, nullptr);
}

TEST_F(PeerGeneric, negative_beyond_peer_create_nullSession)
{
    beyond_peer_h peer;
    char *argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);
    beyond_argument arg = {
        .argc = argc,
        .argv = argv,
    };

    peer = beyond_peer_create(nullptr, &arg);
    EXPECT_EQ(peer, nullptr);
}

TEST_F(PeerGeneric, negative_beyond_peer_create_nullArg)
{
    beyond_peer_h peer;
    peer = beyond_peer_create(session, nullptr);
    EXPECT_EQ(peer, nullptr);
}

TEST_F(PeerGeneric, positive_beyond_peer_set_event_callback)
{
    int ret = beyond_peer_set_event_callback(
        peer, [](beyond_peer_h peer_, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        reinterpret_cast<void *>(0xdeadbeef));
    EXPECT_EQ(ret, 0);
}

TEST_F(PeerGeneric, negative_beyond_peer_set_event_callback_nullHandle)
{
    int ret = beyond_peer_set_event_callback(
        nullptr, [](beyond_peer_h peer_, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        reinterpret_cast<void *>(0xdeadbeef));
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(PeerGeneric, negative_beyond_peer_set_event_callback_nullCallback)
{
    int ret = beyond_peer_set_event_callback(peer, nullptr, reinterpret_cast<void *>(0xdeadbeef));
    EXPECT_EQ(ret, -EINVAL);
}
