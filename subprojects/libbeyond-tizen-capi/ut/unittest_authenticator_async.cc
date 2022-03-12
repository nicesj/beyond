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

#include <glib.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include <beyond/beyond.h>

#include <beyond/plugin/authenticator_ssl_plugin.h>
#include <beyond/plugin/peer_nn_plugin.h>

static char *s_auth_argv[] = {
    const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME),
    const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
};

static beyond_argument s_auth_args = {
    .argc = sizeof(s_auth_argv) / sizeof(char *),
    .argv = s_auth_argv,
};

static char *s_peer_argv[] = {
    const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
};
static beyond_argument s_peer_args = {
    .argc = sizeof(s_peer_argv) / sizeof(char *),
    .argv = s_peer_argv,
};

class AuthenticatorGenericAsync : public testing::Test {
protected:
    GMainLoop *loop;

protected:
    void SetUp() override
    {
        loop = g_main_loop_new(nullptr, FALSE);
        ASSERT_NE(loop, nullptr);

        g_timeout_add(
            1000, [](gpointer user_data) -> gboolean {
                g_main_loop_quit(static_cast<GMainLoop *>(user_data));
                return FALSE;
            },
            static_cast<gpointer>(loop));
    }

    void TearDown() override
    {
        g_main_loop_unref(loop);
        loop = nullptr;
    }
};

TEST_F(AuthenticatorGenericAsync, positive_beyond_authenticator_create)
{
    auto handle = beyond_authenticator_create(&s_auth_args);
    ASSERT_NE(handle, nullptr);

    g_main_loop_run(loop);

    beyond_authenticator_destroy(handle);
}

TEST_F(AuthenticatorGenericAsync, positive_beyond_authenticator_configure)
{
    int ret;
    auto handle = beyond_authenticator_create(&s_auth_args);
    ASSERT_NE(handle, nullptr);

    beyond_config option;

    ret = beyond_authenticator_configure(handle, &option);
    EXPECT_EQ(ret, -EINVAL);

    g_main_loop_run(loop);

    beyond_authenticator_destroy(handle);
}

TEST_F(AuthenticatorGenericAsync, positive_beyond_authenticator_for_peer)
{
    int invoked = 0;
    beyond_authenticator_h authHandle = beyond_authenticator_create(&s_auth_args);
    ASSERT_NE(authHandle, nullptr);

    beyond_authenticator_set_event_callback(
        authHandle, [](beyond_authenticator_h auth, beyond_event_info *event, void *data) -> int {
            int *invoked = static_cast<int *>(data);
            (*invoked)++;
            EXPECT_TRUE(!!(event->type & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE));
            InfoPrint("Invoked: %d 0x%X", *invoked, event->type);
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);

    beyond_config option;

    int ret = beyond_authenticator_configure(authHandle, &option);
    EXPECT_EQ(ret, -EINVAL);

    ret = beyond_authenticator_activate(authHandle);
    EXPECT_EQ(ret, 0);

    ret = beyond_authenticator_prepare(authHandle);
    EXPECT_EQ(ret, 0);

    auto handle = beyond_peer_create(&s_peer_args);
    ASSERT_NE(handle, nullptr);

    option.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
    option.object = authHandle;
    ret = beyond_peer_configure(handle, &option);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_activate(handle);
    EXPECT_EQ(ret, 0);

    g_main_loop_run(loop);

    ret = beyond_peer_deactivate(handle);
    EXPECT_EQ(ret, 0);

    beyond_peer_destroy(handle);

    ret = beyond_authenticator_deactivate(authHandle);
    EXPECT_EQ(ret, 0);

    beyond_authenticator_destroy(authHandle);
}
