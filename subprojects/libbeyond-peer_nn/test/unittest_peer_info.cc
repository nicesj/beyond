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
};

TEST_F(PeerTest, PositiveResourceInfo_Auto)
{
    beyond_peer_info_device tflite_devices[] = {
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_GPU),
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_CPU),
        },
    };

    beyond_peer_info_device snpe_devices[] = {
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_DSP),
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_NPU),
        },
    };

    beyond_peer_info_runtime runtimes[] = {
        {
            .name = const_cast<char *>("tensorflow-lite"),
            .count_of_devices = sizeof(tflite_devices) / sizeof(*tflite_devices),
            .devices = static_cast<beyond_peer_info_device *>(tflite_devices),
        },
        {
            .name = const_cast<char *>("snpe"),
            .count_of_devices = sizeof(snpe_devices) / sizeof(*snpe_devices),
            .devices = static_cast<beyond_peer_info_device *>(snpe_devices),
        },
    };

    beyond_peer_info info = {
        .name = const_cast<char *>("name"),
        .host = const_cast<char *>("0.0.0.0"),
        .port = { 50000 },
        .free_memory = 0,
        .free_storage = 0,
        .uuid = "ec0e0cec-d797-4ba5-b698-f2420c74b787",
        .count_of_runtimes = sizeof(runtimes) / sizeof(*runtimes),
        .runtimes = static_cast<beyond_peer_info_runtime *>(runtimes),
    };

    StartGrpcServer(&info);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    beyond_argument args = {
        .argc = sizeof(peer_argv) / sizeof(char *),
        .argv = peer_argv,
    };

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    const beyond_peer_info *resourceInfo = nullptr;
    ret = peer->GetInfo(resourceInfo);
    EXPECT_EQ(ret, 0);
    ASSERT_NE(resourceInfo, nullptr);
    EXPECT_GT(resourceInfo->free_memory, 0llu);
    EXPECT_GT(resourceInfo->free_storage, 0llu);

    for (int i = 0; i < resourceInfo->count_of_runtimes; i++) {
        EXPECT_STREQ(resourceInfo->runtimes[i].name, runtimes[i].name);
        EXPECT_EQ(resourceInfo->runtimes[i].count_of_devices, runtimes[i].count_of_devices);
        for (int j = 0; j < resourceInfo->runtimes[i].count_of_devices; j++) {
            EXPECT_STREQ(resourceInfo->runtimes[i].devices[j].name, runtimes[i].devices[j].name);
        }
    }

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveResourceInfo_Manual)
{
    beyond_peer_info_device tflite_devices[] = {
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_GPU),
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_CPU),
        },
    };

    beyond_peer_info_device snpe_devices[] = {
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_DSP),
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_RUNTIME_DEVICE_NPU),
        },
    };

    beyond_peer_info_runtime runtime[] = {
        {
            .name = const_cast<char *>("tensorflow-lite"),
            .count_of_devices = sizeof(tflite_devices) / sizeof(*tflite_devices),
            .devices = static_cast<beyond_peer_info_device *>(tflite_devices),
        },
        {
            .name = const_cast<char *>("snpe"),
            .count_of_devices = sizeof(snpe_devices) / sizeof(*snpe_devices),
            .devices = static_cast<beyond_peer_info_device *>(snpe_devices),
        },
    };

    beyond_peer_info info = {
        .name = const_cast<char *>("name"),
        .host = const_cast<char *>("0.0.0.0"),
        .port = { 50000 },
        .free_memory = 123456,
        .free_storage = 101010,
        .uuid = "ec0e0cec-d797-4ba5-b698-f2420c74b787",
        .count_of_runtimes = sizeof(runtime) / sizeof(*runtime),
        .runtimes = static_cast<beyond_peer_info_runtime *>(runtime),
    };

    StartGrpcServer(&info);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    beyond_argument args = {
        .argc = sizeof(peer_argv) / sizeof(char *),
        .argv = peer_argv,
    };

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    const beyond_peer_info *resourceInfo = nullptr;
    ret = peer->GetInfo(resourceInfo);
    EXPECT_EQ(ret, 0);
    ASSERT_NE(resourceInfo, nullptr);
    EXPECT_EQ(resourceInfo->free_memory, info.free_memory);
    EXPECT_EQ(resourceInfo->free_storage, info.free_storage);

    for (int i = 0; i < resourceInfo->count_of_runtimes; i++) {
        EXPECT_STREQ(resourceInfo->runtimes[i].name, runtime[i].name);
        EXPECT_EQ(resourceInfo->runtimes[i].count_of_devices, runtime[i].count_of_devices);
        for (int j = 0; j < resourceInfo->runtimes[i].count_of_devices; j++) {
            EXPECT_STREQ(resourceInfo->runtimes[i].devices[j].name, runtime[i].devices[j].name);
        }
    }

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}
