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
#include "discoveryinfo.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

DiscoveryInfo::DiscoveryInfo()
    : address(nullptr)
    , name("BeyonD")
    , port(3000)
{
}

int DiscoveryInfo::setValue(const std::string &key, Value val)
{
    auto result = valueMap.insert(std::make_pair(key, std::move(val)));
    if (result.second == false) {
        ErrPrint("Invalid key(%s), already exists", key.c_str());
        return -EEXIST;
    }

    return 0;
}

int DiscoveryInfo::removeValue(const std::string &key)
{
    auto result = valueMap.find(key);
    if (result == valueMap.end()) {
        ErrPrint("Invalid key(%s), not found", key.c_str());
        return -EINVAL;
    }

    valueMap.erase(result);
    return 0;
}

DiscoveryInfo::Value::Value(const void *val, int len)
    : size(len)
{
    value = new char[size];
    memcpy(value, val, size);
}

DiscoveryInfo::Value::Value(const Value &val)
    : size(val.size)
{
    value = new char[size];
    memcpy(value, val.value, size);
}

DiscoveryInfo::Value::~Value()
{
    delete[] static_cast<char*>(value);
}
