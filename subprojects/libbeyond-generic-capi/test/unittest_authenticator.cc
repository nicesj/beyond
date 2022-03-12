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

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include <beyond/beyond.h>

#include <beyond/plugin/authenticator_ssl_plugin.h>
#include <beyond/plugin/peer_nn_plugin.h>

class AuthenticatorGeneric : public testing::Test {
protected:
    beyond_session_h session;
    beyond_authenticator_h handle;

protected:
    void SetUp() override
    {
        session = beyond_session_create(0, 0);
        ASSERT_NE(session, nullptr);

        char *auth_argv[] = { const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME) };
        beyond_argument auth_args = {
            .argc = sizeof(auth_argv) / sizeof(char *),
            .argv = auth_argv,
        };

        handle = beyond_authenticator_create(session, &auth_args);
        ASSERT_NE(handle, nullptr);
    }

    void TearDown() override
    {
        beyond_authenticator_destroy(handle);
        beyond_session_destroy(session);
    }
};

TEST_F(AuthenticatorGeneric, positive_beyond_authenticator_configure)
{
    int ret;

    beyond_config option = {
        .type = BEYOND_CONFIG_TYPE_AUTHENTICATOR,
        .object = nullptr,
    };
    ret = beyond_authenticator_configure(handle, &option);
    EXPECT_EQ(ret, -EINVAL);
}

TEST_F(AuthenticatorGeneric, positive_beyond_authenticator_for_peer)
{
    beyond_config option;

    int ret = beyond_authenticator_configure(handle, &option);
    EXPECT_EQ(ret, -EINVAL);

    ret = beyond_authenticator_activate(handle);
    EXPECT_EQ(ret, 0);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    beyond_argument peer_args = {
        .argc = sizeof(peer_argv) / sizeof(char *),
        .argv = peer_argv,
    };

    auto peerHandle = beyond_peer_create(session, &peer_args);
    ASSERT_NE(peerHandle, nullptr);

    option.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
    option.object = handle;
    ret = beyond_peer_configure(peerHandle, &option);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_activate(peerHandle);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_deactivate(peerHandle);
    EXPECT_EQ(ret, 0);

    beyond_peer_destroy(peerHandle);

    ret = beyond_authenticator_deactivate(handle);
    EXPECT_EQ(ret, 0);
}
