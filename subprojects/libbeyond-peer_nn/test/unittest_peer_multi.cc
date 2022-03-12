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
#include <cstdlib>
#include <exception>
#include <gtest/gtest.h>

#include <unistd.h>
#include <dlfcn.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"
#include "beyond/plugin/authenticator_ssl_plugin.h"

#include "peer.h"
#include "peer_model.h"
#include "unittest.h"

static struct beyond_peer_info s_valid_info = {
    .name = const_cast<char *>("name"),
    .host = const_cast<char *>("127.0.0.1"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

TEST_F(PeerTest, PositiveActivateMultipleDevices)
{
    StartGrpcServer();
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer1 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer1, nullptr);

    int ret = peer1->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer1->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer1->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer1->Prepare();
    EXPECT_EQ(ret, 0);

    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    ret = peer2->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer2->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer2->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer2->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer1->Deactivate();
    EXPECT_EQ(ret, 0);

    peer1->Destroy();

    ret = peer2->Deactivate();
    EXPECT_EQ(ret, 0);

    peer2->Destroy();
    StopGrpcServer();
}
