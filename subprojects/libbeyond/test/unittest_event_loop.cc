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
#include <exception>
#include <cerrno>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(EventLoop, Create)
{
    auto loop = beyond::EventLoop::Create();
    ASSERT_NE(loop, nullptr);
    loop->Destroy();
}

TEST(EventLoop, Run)
{
    auto loop = beyond::EventLoop::Create();
    ASSERT_NE(loop, nullptr);

    int ret = loop->Run(10, -1, 1000);
    EXPECT_EQ(ret, -ETIMEDOUT);

    loop->Destroy();
}

TEST(EventLoop, RunAsync)
{
    auto loop = beyond::EventLoop::Create(true);
    ASSERT_NE(loop, nullptr);

    int ret = loop->Run(10, -1, -1);
    EXPECT_EQ(ret, 0);

    ret = loop->Stop();
    EXPECT_EQ(ret, 0);

    loop->Destroy();
}

TEST(EventLoop, Stop)
{
    auto loop = beyond::EventLoop::Create();
    ASSERT_NE(loop, nullptr);

    auto timer = beyond::Timer::Create();
    timer->SetTimer(0.01);
    int type = beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    loop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(timer), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            auto ptr = static_cast<beyond::EventLoop *>(data);
            EXPECT_EQ(ptr->Stop(), 0);
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        loop);

    EXPECT_EQ(loop->Run(10, -1, 1000), 0);

    loop->Destroy();
}

TEST(EventLoop, StopAsync)
{
    auto loop = beyond::EventLoop::Create(true);
    ASSERT_NE(loop, nullptr);

    auto timer = beyond::Timer::Create();
    timer->SetTimer(0.01);
    int type = beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    loop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(timer), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            auto ptr = static_cast<beyond::EventLoop *>(data);
            EXPECT_EQ(ptr->Stop(), 0);
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        loop);

    EXPECT_EQ(loop->Run(10, -1, 1000), 0);
    loop->Destroy();
}

TEST(EventLoop, AddEventHandler_Basic)
{
    auto loop = beyond::EventLoop::Create();
    ASSERT_NE(loop, nullptr);

    auto timer = beyond::Timer::Create();
    timer->SetTimer(0.01);
    int value = 0;
    int type = beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    loop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(timer), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            int *ptr = static_cast<int *>(data);
            *ptr = 1;
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        &value);

    int ret = loop->Run(1, 1, 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(value, 1);

    loop->Destroy();
}

TEST(EventLoop, AddEventHandler_BasicAsync)
{
    auto loop = beyond::EventLoop::Create(true);
    ASSERT_NE(loop, nullptr);

    auto timer = beyond::Timer::Create();
    timer->SetTimer(0.01);
    int value = 0;
    int type = beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    loop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(timer), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            int *ptr = static_cast<int *>(data);
            *ptr = 1;
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        &value);

    int ret = loop->Run(1, 1, 1000);
    EXPECT_EQ(ret, 0);
    sleep(1);
    loop->Destroy();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(value, 1);
}

TEST(EventLoop, AddEventHandler_subLoop)
{
    auto loop = beyond::EventLoop::Create();
    ASSERT_NE(loop, nullptr);

    auto subloop = beyond::EventLoop::Create();
    ASSERT_NE(subloop, nullptr);

    auto timer = beyond::Timer::Create();
    timer->SetTimer(0.01);
    int value1 = 0;
    int value2 = 0;
    int type = beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    subloop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(timer), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            int *ptr = static_cast<int *>(data);
            *ptr = 1;
            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        &value1);
    loop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(subloop), type, [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
            auto subloop = dynamic_cast<beyond::EventLoop *>(eventObject);
            if (subloop == nullptr) {
                return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
            }

            subloop->Run(1, 1, 1000);

            int *ptr = static_cast<int *>(data);
            *ptr = 2;

            return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
        },
        &value2);

    int ret = loop->Run(1, 1, 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(value1, 1);
    EXPECT_EQ(value2, 2);

    loop->Destroy();
    subloop->Destroy();
}
