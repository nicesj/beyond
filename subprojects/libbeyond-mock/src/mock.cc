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

#include "beyond/mock/mock.h"

const void *mock::Type::AnyPtr = reinterpret_cast<const void *>("ANY_POINTER");
const char *mock::Type::AnyStr = reinterpret_cast<const char *>("ANY_STRING");
const int mock::Type::AnyInt = 0xdeadbeef;
