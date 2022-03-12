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
#include <cerrno>
#include <beyond/beyond.h>

#include "beyond/plugin/discovery_dns_sd_plugin.h"

#define TEST_MODULE_OPTION_EDGE (const_cast<char *>("--server"))

static char *s_device_argv[] = { const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME) };
static beyond_argument s_device_arg = {
    .argc = sizeof(s_device_argv) / sizeof(char *),
    .argv = s_device_argv,
};

static char *s_edge_argv[] = {
    const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME),
    TEST_MODULE_OPTION_EDGE,
};
static beyond_argument s_edge_arg = {
    .argc = sizeof(s_edge_argv) / sizeof(char *),
    .argv = s_edge_argv
};

class DiscoveryGeneric : public testing::Test {
protected:
    beyond_session_h session;
    beyond_discovery_h handle;

    struct EdgeContext {
        beyond_session_h session;
        beyond_discovery_h handle;
    } edgeCtx;

protected:
    void SetUp() override
    {
        session = beyond_session_create(0, 0);
        ASSERT_NE(session, nullptr);

        handle = beyond_discovery_create(session, &s_device_arg);
        ASSERT_NE(handle, nullptr);
    }

    void TearDown() override
    {
        beyond_discovery_destroy(handle);
        beyond_session_destroy(session);
    }

    int ActivateEdge(void)
    {
        edgeCtx.session = beyond_session_create(1, 0);
        if (edgeCtx.session == nullptr) {
            return -EFAULT;
        }

        edgeCtx.handle = beyond_discovery_create(edgeCtx.session, &s_edge_arg);
        if (edgeCtx.handle == nullptr) {
            return -EFAULT;
        }

        int ret = beyond_discovery_activate(edgeCtx.handle);
        if (ret < 0) {
            beyond_discovery_destroy(edgeCtx.handle);
            edgeCtx.handle = nullptr;
            return ret;
        }

        ret = beyond_session_run(edgeCtx.session, 10, -1, -1);
        if (ret < 0) {
            beyond_discovery_destroy(edgeCtx.handle);
            edgeCtx.handle = nullptr;

            beyond_session_destroy(edgeCtx.session);
            edgeCtx.session = nullptr;
        }

        return ret;
    }

    int DeactivateEdge(void)
    {
        if (edgeCtx.session == nullptr) {
            return -EINVAL;
        }

        if (edgeCtx.handle == nullptr) {
            return -EINVAL;
        }

        int ret = beyond_discovery_deactivate(edgeCtx.handle);
        if (ret < 0) {
            return ret;
        }

        beyond_discovery_destroy(edgeCtx.handle);
        edgeCtx.handle = nullptr;

        beyond_session_destroy(edgeCtx.session);
        edgeCtx.session = nullptr;
        return ret;
    }
};

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_invalidSession)
{
    auto handle = beyond_discovery_create(nullptr, &s_device_arg);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(DiscoveryGeneric, positive_beyond_discovery_create_edge)
{
    auto handle = beyond_discovery_create(session, &s_edge_arg);
    ASSERT_NE(handle, nullptr);

    beyond_discovery_destroy(handle);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_edge_invalidSession)
{
    auto handle = beyond_discovery_create(nullptr, &s_edge_arg);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_unknownModule)
{
    char *argv[] = {
        const_cast<char *>("hello my module"),
    };
    int argc = sizeof(argv) / sizeof(char *);

    beyond_argument arg = {
        .argc = argc,
        .argv = argv,
    };

    auto handle = beyond_discovery_create(session, &arg);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_nullArg)
{
    auto handle = beyond_discovery_create(session, nullptr);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_invalidArgc)
{
    char *argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME),
    };

    beyond_argument arg = {
        .argc = -1,
        .argv = argv,
    };

    auto handle = beyond_discovery_create(session, &arg);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_create_invalidArgv)
{
    beyond_argument arg = {
        .argc = 1,
        .argv = nullptr,
    };

    auto handle = beyond_discovery_create(session, &arg);
    ASSERT_NE(handle, nullptr);

    beyond_discovery_destroy(handle);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_destroy_invalidHandle)
{
    beyond_discovery_destroy(nullptr);
}

TEST_F(DiscoveryGeneric, positive_beyond_discovery_configure)
{
    beyond_config config = { 's', nullptr };
    auto ret = beyond_discovery_configure(handle, &config);
    EXPECT_EQ(ret, 0);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_configure_invalidHandle)
{
    beyond_config config = { 's', nullptr };

    auto ret = beyond_discovery_configure(nullptr, &config);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(DiscoveryGeneric, positive_beyond_discovery_set_event_callback)
{
    int ret = beyond_discovery_set_event_callback(
        handle,
        [](beyond_discovery_h handle, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        nullptr);
    EXPECT_EQ(ret, 0);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_set_event_callback_invalidHandle)
{
    int ret = beyond_discovery_set_event_callback(
        nullptr,
        [](beyond_discovery_h handle, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_set_event_callback_invalidCallback)
{
    int ret = beyond_discovery_set_event_callback(handle, nullptr, nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(DiscoveryGeneric, positive_beyond_discovery_activate)
{
    int ret = beyond_discovery_activate(handle);
    EXPECT_EQ(ret, 0);

    ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, 0);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_deactivate_noActivate)
{
    int ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, -EILSEQ);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_activate_invalidHandle)
{
    int ret = beyond_discovery_activate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(DiscoveryGeneric, negative_beyond_discovery_deactivate_invalidHandle)
{
    int ret = beyond_discovery_deactivate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(DiscoveryGeneric, postive_beyond_discovery_event_processing)
{
    int ret;
    int discovered = 0;

    ret = ActivateEdge();
    ASSERT_EQ(ret, 0);

    ret = beyond_discovery_set_event_callback(
        handle,
        [](beyond_discovery_h handle, beyond_event_info *event, void *data) -> int {
            int *discovered = reinterpret_cast<int *>(data);
            (*discovered)++;
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        reinterpret_cast<void *>(&discovered));
    EXPECT_EQ(ret, 0);

    ret = beyond_discovery_activate(handle);
    EXPECT_EQ(ret, 0);

    ret = beyond_session_run(session, 10, 1, -1);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(discovered, 0);

    ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, 0);

    ret = DeactivateEdge();
    EXPECT_EQ(ret, 0);
}
