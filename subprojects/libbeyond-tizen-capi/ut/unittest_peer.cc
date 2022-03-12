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

TEST(peer, positive_beyond_peer_create)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);
    beyond_peer_destroy(handle);
}

TEST(peer, positive_beyond_peer_destroy)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);
    beyond_peer_destroy(handle);
}

TEST(peer, negative_beyond_peer_destroy)
{
    beyond_peer_destroy(nullptr);
}

TEST(peer, positive_beyond_peer_configure)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);

    beyond_config config = { 's', nullptr };
    auto ret = beyond_peer_configure(handle, &config);
    EXPECT_EQ(ret, 0);

    beyond_peer_destroy(handle);
}

TEST(peer, negative_beyond_peer_configure)
{
    beyond_config config = { 's', nullptr };

    auto ret = beyond_peer_configure(nullptr, &config);
    EXPECT_EQ(ret, -EINVAL);

    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);

    ret = beyond_peer_configure(handle, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    beyond_peer_destroy(handle);
}

TEST(peer, positive_beyond_peer_set_event_callback)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);

    beyond_config config = { 's', nullptr };
    auto ret = beyond_peer_configure(handle, &config);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_set_event_callback(handle, nullptr, nullptr);
    EXPECT_EQ(ret, -ENOSYS); // NOT Implmented yet

    beyond_peer_destroy(handle);
}

TEST(peer, negative_beyond_peer_set_event_callback)
{
    auto ret = beyond_peer_set_event_callback(nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(peer, positive_beyond_peer_activate)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);

    beyond_config config = { 's', nullptr };
    auto ret = beyond_peer_configure(handle, &config);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_activate(handle);
    EXPECT_EQ(ret, 0);

    beyond_peer_deactivate(handle);
    beyond_peer_destroy(handle);
}

TEST(peer, negative_beyond_peer_activate)
{
    auto ret = beyond_peer_activate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}

TEST(peer, positive_beyond_peer_deactivate)
{
    auto handle = beyond_peer_create(nullptr);
    ASSERT_NE(handle, nullptr);

    beyond_config config = { 's', nullptr };
    auto ret = beyond_peer_configure(handle, &config);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_activate(handle);
    EXPECT_EQ(ret, 0);

    ret = beyond_peer_deactivate(handle);
    EXPECT_EQ(ret, 0);

    beyond_peer_destroy(handle);
}

TEST(peer, negative_beyond_peer_deactivate)
{
    auto ret = beyond_peer_deactivate(nullptr);
    EXPECT_EQ(ret, -EINVAL);
}
