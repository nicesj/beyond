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
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("127.0.0.1"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

static struct beyond_peer_info s_invalid_info_port = {
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("127.0.0.1"),
    .port = { 0 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

static struct beyond_peer_info s_valid_edge_info = {
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("0.0.0.0"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

static struct beyond_peer_info s_valid_edge_info2 = {
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("0.0.0.0"),
    .port = { 50001 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

static struct beyond_peer_info s_edge_info_port = {
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("0.0.0.0"),
    .port = { 0 },
    .free_memory = 0llu,
    .free_storage = 0llu,
};

TEST_F(PeerTest, PositiveConstructorDevice)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveConstructorDevice2)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer1 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer1, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    peer1->Destroy();

    peer2->Destroy();
}

TEST_F(PeerTest, PositiveConstructorDevice_Framework)
{
    int argc = 3;
    char *argv[3];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_INFERENCE_OPTION_FRAMEWORK);
    argv[2] = const_cast<char *>("nnfw");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveConstructorDevice_Framework2)
{
    int argc = 5;
    char *argv[5];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_INFERENCE_OPTION_FRAMEWORK);
    argv[2] = const_cast<char *>("nnfw");
    argv[3] = const_cast<char *>(BEYOND_INFERENCE_OPTION_FRAMEWORK);
    argv[4] = const_cast<char *>("snpe");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveConstructorEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveConstructorEdge2)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer1 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer1, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    peer1->Destroy();

    peer2->Destroy();
}

TEST_F(PeerTest, PositiveGetModuleName)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    const char *moduleName = peer->GetModuleName();
    EXPECT_STREQ(moduleName, BEYOND_PLUGIN_PEER_NN_NAME);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoEdge_Event)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret;
    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [peer](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer_ = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);
            EXPECT_EQ(peer, peer_);

            int ret;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED);

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                EXPECT_EQ(evtData->data, nullptr);

                const struct beyond_peer_info *stored_info;
                ret = peer->GetInfo(stored_info);
                EXPECT_EQ(ret, 0);

                EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
                EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        nullptr);

    ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(10, 1, -1);
    ASSERT_EQ(ret, 0);

    eventLoop->Destroy();

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoEdge2)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = peer2->SetInfo(&s_valid_edge_info2);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);

    ret = peer2->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info2.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info2.port[0], stored_info->port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoEdge2_Event)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    int ret;
    int invocation_count = 0;
    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [peer, &invocation_count](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer_ = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);
            EXPECT_EQ(peer, peer_);

            int ret;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED);

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                EXPECT_EQ(evtData->data, nullptr);

                invocation_count++;

                const struct beyond_peer_info *stored_info;
                ret = peer->GetInfo(stored_info);
                EXPECT_EQ(ret, 0);

                EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
                EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer2),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [peer2, &invocation_count](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ(peer, peer2);
            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);

            int ret;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED);

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                invocation_count++;

                EXPECT_EQ(evtData->data, nullptr);
                const struct beyond_peer_info *stored_info;
                ret = peer->GetInfo(stored_info);
                EXPECT_EQ(ret, 0);

                EXPECT_STREQ(s_valid_edge_info2.host, stored_info->host);
                EXPECT_EQ(s_valid_edge_info2.port[0], stored_info->port[0]);
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        nullptr);

    ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = peer2->SetInfo(&s_valid_edge_info2);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(10, 1, -1);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(invocation_count, 2);

    eventLoop->Destroy();

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info.port[0], stored_info->port[0]);

    ret = peer2->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(s_valid_edge_info2.host, stored_info->host);
    EXPECT_EQ(s_valid_edge_info2.port[0], stored_info->port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoEdge_Port)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_edge_info_port);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    const beyond_peer_info *info = nullptr;
    ret = peer->GetInfo(info);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(info->port[0], 0);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeSetInfoEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(nullptr);
    EXPECT_EQ(ret, -EINVAL);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveActivateEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(stored_info->host, s_valid_edge_info.host);
    EXPECT_EQ(stored_info->port[0], s_valid_edge_info.port[0]);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveActivateEdge2)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer2->SetInfo(&s_valid_edge_info2);
    EXPECT_EQ(ret, 0);

    ret = peer2->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = peer2->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    peer2->Destroy();
}

TEST_F(PeerTest, NegativeActivateEdge2_PortInUse)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer2 = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer2, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer2->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    ret = peer2->Activate();
    EXPECT_NE(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = peer2->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();

    peer2->Destroy();
}

TEST_F(PeerTest, NegativeActivateEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(stored_info->host, s_valid_edge_info.host);
    EXPECT_EQ(stored_info->port[0], s_valid_edge_info.port[0]);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, -EALREADY);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeDeactivateEdge_NoActivate)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_edge_info);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(stored_info->host, s_valid_edge_info.host);
    EXPECT_EQ(stored_info->port[0], s_valid_edge_info.port[0]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeSetInfoDevice)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = nullptr;

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(nullptr);
    EXPECT_EQ(ret, -EINVAL);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeSetInfoDevice_InvalidPort)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_invalid_info_port);
    EXPECT_EQ(ret, -EINVAL);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoDevice)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(stored_info->host, s_valid_info.host);
    EXPECT_EQ(stored_info->port[0], s_valid_info.port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetInfoDevice_Event)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret;
    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [peer](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer_ = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);
            EXPECT_EQ(peer, peer_);

            int ret;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED);

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                EXPECT_EQ(evtData->data, nullptr);

                const struct beyond_peer_info *stored_info;
                ret = peer->GetInfo(stored_info);
                EXPECT_EQ(ret, 0);

                EXPECT_STREQ(s_valid_info.host, stored_info->host);
                EXPECT_EQ(s_valid_info.port[0], stored_info->port[0]);
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        nullptr);

    ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(10, 1, -1);
    ASSERT_EQ(ret, 0);

    eventLoop->Destroy();

    const struct beyond_peer_info *stored_info;
    ret = peer->GetInfo(stored_info);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(stored_info->host, s_valid_info.host);
    EXPECT_EQ(stored_info->port[0], s_valid_info.port[0]);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveActivateDevice)
{
    StartGrpcServer();
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
    StopGrpcServer();
}

TEST_F(PeerTest, PositivePrepareDevice_Framework)
{
    StartGrpcServer();
    int argc = 3;
    char *argv[3];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_INFERENCE_OPTION_FRAMEWORK);
    argv[2] = const_cast<char *>("snpe");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, -EFAULT);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
    StopGrpcServer();
}

TEST_F(PeerTest, NegativeActivateDevice)
{
    StartGrpcServer();
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, -EALREADY);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
    StopGrpcServer();
}

TEST_F(PeerTest, NegativeDeactivateDevice_NoActivate)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveLoadModelDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeLoadModelDevice_NoActivate)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, -EILSEQ);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeLoadModelEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, -ENOTSUP);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveAllocateTensorDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor = nullptr;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(tensor, nullptr);

    peer->FreeTensor(tensor, 1);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeAllocateTensorDevice_nullInfo)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor = nullptr;
    ret = peer->AllocateTensor(nullptr, 1, tensor);
    EXPECT_EQ(ret, -EINVAL);
    EXPECT_EQ(tensor, nullptr);

    peer->FreeTensor(tensor, 1);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeAllocateTensorDevice_invalidSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor = nullptr;
    ret = peer->AllocateTensor(&tensorInfo, -1, tensor);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(tensor, nullptr);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveGetInputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    // FIXME:
    // The pipeline must be prepared for getting the tensor shape of the model
    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveGetOutputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    // FIXME:
    // The pipeline must be prepared for getting the tensor shape of the model
    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 2;
    dims->data[0] = 1;
    dims->data[1] = 1001;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };

    const beyond_tensor_info *outputTensorInfo;
    int size;
    ret = peer->GetOutputTensorInfo(outputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(outputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(outputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(outputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(outputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(outputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveSetInputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveSetInputTensorInfoDevice_withPrepare)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    ASSERT_EQ(ret, 0);

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_NoActivate)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, -EILSEQ);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EILSEQ);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();

    free(dims);
    dims = nullptr;
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->SetInputTensorInfo(nullptr, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_Size)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo;
    ret = peer->SetInputTensorInfo(&tensorInfo, -1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_TensorSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = 0,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_DimensionsSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 0;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_DimensionsNull)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = nullptr,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->SetInputTensorInfo(nullptr, 0);
    EXPECT_EQ(ret, -ENOTSUP);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveSetOutputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    ASSERT_EQ(ret, 0);

    const beyond_tensor_info *outputTensorInfo;
    int size;
    ret = peer->GetOutputTensorInfo(outputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(outputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(outputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(outputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(outputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(outputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(outputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(outputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_NoActivate)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, -EILSEQ);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EILSEQ);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->SetOutputTensorInfo(nullptr, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_Size)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo;
    ret = peer->SetOutputTensorInfo(&tensorInfo, -1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_TensorSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 2;
    dims->data[2] = 3;
    dims->data[3] = 4;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 0,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_DimensionsSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 0;
    dims->data[0] = 1;
    dims->data[1] = 2;
    dims->data[2] = 3;
    dims->data[3] = 4;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_DimensionsNull)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = nullptr,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_BeforeActivate_ImageInput)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
        .server = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
            .framework = const_cast<char *>("tensorflow-lite"),
            .accel = nullptr,
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_BeforeActivate_VideoInput)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_VIDEO,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
        .server = {
            .input_type = BEYOND_INPUT_TYPE_VIDEO,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
            .framework = const_cast<char *>("tensorflow-lite"),
            .accel = nullptr,
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_ImageConfig_BeforeActivate)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "RGB",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_VideoConfig_BeforeActivate)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_VIDEO,
        .config = {
            .video = {
                .frame = {
                    .format = "NV21",
                    .width = 224,
                    .height = 224,
                    .convert_format = "RGB",
                    .convert_width = 224,
                    .convert_height = 224,
                    .transform_mode = "typecast",
                    .transform_option = "uint8" },
                .fps = 30,
                .duration = -1, // Live video - no duration
            },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_ImageConfig)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "RGB",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveConfigureDevice_VideoConfig)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_VIDEO,
        .config = {
            .video = {
                .frame = {
                    .format = "NV21",
                    .width = 224,
                    .height = 224,
                    .convert_format = "RGB",
                    .convert_width = 224,
                    .convert_height = 224,
                    .transform_mode = "typecast",
                    .transform_option = "uint8" },
                .fps = 30,
                .duration = -1, // Live video - no duration
            },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositivePrepareWithConfigureDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativePrepareWithConfigureDevice_InvalidPreprocessing)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>("I'm not a valid pipe description"),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, -EFAULT);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositivePrepareWithoutConfigureDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveInvokeDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 1;
    dims->data[2] = 1;
    dims->data[3] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    // TODO:
    // Use the sample data for the tensor to make real inference
    ret = peer->Invoke(tensor, 1, nullptr);
    EXPECT_EQ(ret, 0);

    beyond_tensor *output;
    int outputSize;
    ret = peer->GetOutput(output, outputSize);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outputSize, 0);
    EXPECT_NE(output, nullptr);

    // TODO:
    // Validate output and outputSize using real sample data
    peer->FreeTensor(output, outputSize);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveInvokeDevice_ImageConfig)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "RGB",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 1;
    dims->data[2] = 1;
    dims->data[3] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    // TODO:
    // Use the sample data for the tensor to make real inference
    ret = peer->Invoke(tensor, 1, nullptr);
    EXPECT_EQ(ret, 0);

    beyond_tensor *output;
    int outputSize;
    ret = peer->GetOutput(output, outputSize);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outputSize, 0);
    EXPECT_NE(output, nullptr);

    // TODO:
    // Validate output and outputSize using real sample data
    peer->FreeTensor(output, outputSize);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveInvokeDevice_Async)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    dims->size = 2;
    dims->data[0] = 1;
    dims->data[1] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    // TODO:
    // Use the sample data for the tensor to make real inference
    ret = peer->Invoke(tensor, 1, reinterpret_cast<void *>(0x1004beef));
    EXPECT_EQ(ret, 0);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);

            int ret;
            beyond_handler_return handler_ret = BEYOND_HANDLER_RETURN_RENEW;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE(((evtData->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) == BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) || ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED));

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                EXPECT_EQ(evtData->data, nullptr);
            } else if ((evtData->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) == BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
                EXPECT_EQ(evtData->data, reinterpret_cast<void *>(0x1004beef));

                beyond_tensor *output;
                int outputSize;
                ret = peer->GetOutput(output, outputSize);
                EXPECT_EQ(ret, 0);

                EXPECT_GT(outputSize, 0);
                EXPECT_NE(output, nullptr);

                // TODO:
                // Validate output and outputSize using real sample data
                peer->FreeTensor(output, outputSize);
                handler_ret = BEYOND_HANDLER_RETURN_CANCEL;
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return handler_ret;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        tensor);

    ret = eventLoop->Run(10, 2, -1);
    ASSERT_EQ(ret, 0);

    eventLoop->Destroy();

    peer->FreeTensor(tensor, 1);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensor)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Invoke(nullptr, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    beyond_tensor tensor;
    ret = peer->Invoke(&tensor, 0, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensorSize)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    char buffer[4096];
    beyond_tensor tensor = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = -1,
        .data = static_cast<char *>(buffer),
    };
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensorData)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    char buffer[4096];
    beyond_tensor tensor = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = sizeof(buffer),
        .data = nullptr,
    };
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeInvokeDevice_NoActivate)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, -EILSEQ);

    ret = peer->Prepare();
    EXPECT_EQ(ret, -EILSEQ);

    beyond_tensor tensor;
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EILSEQ);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();
}

TEST_F(PeerTest, NegativeInvokeWithoutPrepareDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor tensor;
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, NegativeConfigureDevice)
{
    StartGrpcServer();

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    // TODO: Prepare mock

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
}

TEST_F(PeerTest, PositiveGetHandleDevice)
{
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->GetHandle();
    EXPECT_TRUE(ret >= 0);

    peer->Destroy();
}

TEST_F(PeerTest, PositiveGetHandleEdge)
{
    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->GetHandle();
    EXPECT_TRUE(ret >= 0);

    peer->Destroy();
}
