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
#pragma once

#include <arpa/inet.h>
#include <map>
#include <string>

class DiscoveryInfo {
public:
    struct Value {
        Value(const void *value, int length);
        Value(const Value &val);
        ~Value();

        void *value;
        int size;
    };

    DiscoveryInfo();

    int setValue(const std::string &key, Value val);
    int removeValue(const std::string &key);

    const struct sockaddr *address;
    std::string name;
    uint16_t port;
    std::string uuid;
    std::map<std::string, Value> valueMap;
};
