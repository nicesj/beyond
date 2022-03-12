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

#include <glib.h>

#define TEST_MODULE_NAME (const_cast<char *>("discovery_dns_sd"))
#define TEST_MODULE_OPTION_EDGE (const_cast<char *>("--server"))
#define TEST_MODULE_OPTION_NAME (const_cast<char *>("--name"))
#define TEST_MODULE_VALUE_NAME (const_cast<char *>("testServiceName"))

static char *s_device_argv[] = { TEST_MODULE_NAME };

static beyond_argument s_device_arg = {
    .argc = sizeof(s_device_argv) / sizeof(char *),
    .argv = s_device_argv,
};

static char *s_edge_argv[] = {
    TEST_MODULE_NAME,
    TEST_MODULE_OPTION_EDGE,
    TEST_MODULE_OPTION_NAME,
    TEST_MODULE_VALUE_NAME
};
static beyond_argument s_edge_arg = {
    .argc = sizeof(s_edge_argv) / sizeof(char *),
    .argv = s_edge_argv
};

static struct EdgeContext {
    beyond_discovery_h handle;
} s_EdgeCtx;

static int ActivateEdge(void)
{
    s_EdgeCtx.handle = beyond_discovery_create(&s_edge_arg);
    if (s_EdgeCtx.handle == nullptr) {
        return -EFAULT;
    }

    int ret = beyond_discovery_set_item(s_EdgeCtx.handle, "test", "1234", 4);
    if (ret < 0) {
        beyond_discovery_destroy(s_EdgeCtx.handle);
        s_EdgeCtx.handle = nullptr;
    }

    ret = beyond_discovery_activate(s_EdgeCtx.handle);
    if (ret < 0) {
        beyond_discovery_destroy(s_EdgeCtx.handle);
        s_EdgeCtx.handle = nullptr;
    }

    return ret;
}

static int DeactivateEdge(void)
{
    if (s_EdgeCtx.handle == nullptr) {
        return -EINVAL;
    }

    int ret = beyond_discovery_deactivate(s_EdgeCtx.handle);
    if (ret < 0) {
        return ret;
    }

    beyond_discovery_destroy(s_EdgeCtx.handle);
    return ret;
}

TEST(discovery, positive_beyond_discovery_create)
{
    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

    beyond_discovery_destroy(handle);
}

TEST(discovery, positive_beyond_discovery_create_edge)
{
    auto handle = beyond_discovery_create(&s_edge_arg);
    ASSERT_NE(handle, nullptr);

    beyond_discovery_destroy(handle);
}

TEST(discovery, negative_beyond_discovery_create_unknownModule)
{
    char *argv[] = {
        const_cast<char *>("hello my module"),
    };
    int argc = sizeof(argv) / sizeof(char *);

    beyond_argument arg = {
        .argc = argc,
        .argv = argv,
    };

    auto handle = beyond_discovery_create(&arg);
    EXPECT_EQ(handle, nullptr);
}

TEST(discovery, negative_beyond_discovery_create_nullArg)
{
    auto handle = beyond_discovery_create(nullptr);
    EXPECT_EQ(handle, nullptr);
}

TEST(discovery, negative_beyond_discovery_create_invalidArgc)
{
    char *argv[] = {
        TEST_MODULE_NAME,
    };

    beyond_argument arg = {
        .argc = -1,
        .argv = argv,
    };

    auto handle = beyond_discovery_create(&arg);
    EXPECT_EQ(handle, nullptr);
}

TEST(discovery, negative_beyond_discovery_create_invalidArgv)
{
    beyond_argument arg = {
        .argc = 1,
        .argv = nullptr,
    };

    auto handle = beyond_discovery_create(&arg);
    ASSERT_EQ(handle, nullptr);
}

TEST(discovery, negative_beyond_discovery_destroy_invalidHandle)
{
    beyond_discovery_destroy(nullptr);
}

TEST(discovery, positive_beyond_discovery_configure)
{
    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

    beyond_config config = { 's', nullptr };
    auto ret = beyond_discovery_configure(handle, &config);
    EXPECT_EQ(ret, -ENOSYS);

    beyond_discovery_destroy(handle);
}

TEST(discovery, negative_beyond_discovery_configure_invalidHandle)
{
    beyond_config config = { 's', nullptr };

    auto ret = beyond_discovery_configure(nullptr, &config);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(discovery, positive_beyond_discovery_set_event_callback)
{
    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

    int ret = beyond_discovery_set_event_callback(
        handle,
        [](beyond_discovery_h handle, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        nullptr);
    EXPECT_EQ(ret, 0);

    beyond_discovery_destroy(handle);
}

TEST(discovery, negative_beyond_discovery_set_event_callback_invalidHandle)
{
    int ret = beyond_discovery_set_event_callback(
        nullptr,
        [](beyond_discovery_h handle, beyond_event_info *event, void *data) -> int {
            return BEYOND_HANDLER_RETURN_CANCEL;
        },
        nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(discovery, positive_beyond_discovery_activate)
{
    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

    int ret = beyond_discovery_activate(handle);
    EXPECT_EQ(ret, 0);

    ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, 0);

    beyond_discovery_destroy(handle);
}

TEST(discovery, negative_beyond_discovery_deactivate_noActivate)
{
    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

    int ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, -EALREADY);

    beyond_discovery_destroy(handle);
}

TEST(discovery, negative_beyond_discovery_activate_invalidHandle)
{
    int ret = beyond_discovery_activate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(discovery, negative_beyond_discovery_deactivate_invalidHandle)
{
    int ret = beyond_discovery_deactivate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(discovery, postive_beyond_discovery_event_processing)
{
    GMainLoop *loop;
    loop = g_main_loop_new(nullptr, FALSE);
    ASSERT_NE(loop, nullptr);

    g_timeout_add(
        1000, [](gpointer user_data) -> gboolean {
            g_main_loop_quit(static_cast<GMainLoop *>(user_data));
            return FALSE;
        },
        static_cast<gpointer>(loop));

    int ret;
    int discovered = 0;

    ret = ActivateEdge();
    ASSERT_EQ(ret, 0);

    auto handle = beyond_discovery_create(&s_device_arg);
    ASSERT_NE(handle, nullptr);

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

    g_main_loop_run(loop);

    EXPECT_GT(discovered, 0);

    ret = beyond_discovery_deactivate(handle);
    EXPECT_EQ(ret, 0);

    beyond_discovery_destroy(handle);

    ret = DeactivateEdge();
    EXPECT_EQ(ret, 0);

    g_main_loop_unref(loop);
}

TEST(discovery, positive_beyond_discovery_set_item)
{
    auto handle = beyond_discovery_create(&s_edge_arg);
    ASSERT_NE(handle, nullptr);

    int ret = beyond_discovery_set_item(handle, "test", "1234", 4);
    EXPECT_EQ(ret, 0);

    beyond_discovery_destroy(handle);
}

TEST(discovery, positive_beyond_discovery_remove_item)
{
    auto handle = beyond_discovery_create(&s_edge_arg);
    ASSERT_NE(handle, nullptr);

    int ret = beyond_discovery_set_item(handle, "test", "1234", 4);
    EXPECT_EQ(ret, 0);

    ret = beyond_discovery_remove_item(handle, "test");
    EXPECT_EQ(ret, 0);

    beyond_discovery_destroy(handle);
}
