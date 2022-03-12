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

int main(int argc, char *argv[])
{
    int result = -1;

    try {
        testing::InitGoogleTest(&argc, argv);
    } catch (std::exception &e) {
        ErrPrint("catch 'testing::internal::<unnamed>::ClassUniqueToAlwaysTrue': %s", e.what());
    }

    try {
        result = RUN_ALL_TESTS();
    } catch (std::exception &e) {
        ErrPrint("catch `testing::internal::GoogleTestFailureException`: %s", e.what());
    }

    return result;
}
