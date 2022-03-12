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

#ifndef __BEYOND_PRIVATE_MOUDLE_INTERFACE_H__
#define __BEYOND_PRIVATE_MOUDLE_INTERFACE_H__

#include <beyond/common.h>

namespace beyond {

class ModuleInterface {
public:
    virtual ~ModuleInterface(void) = default;

    static constexpr const char *FilenameFormat = "libbeyond-%s.so";
    static constexpr const char *EntryPointSymbol = "_main";
    static constexpr const char *TYPE_RUNTIME = "runtime";
    static constexpr const char *TYPE_PEER = "peer";
    static constexpr const char *TYPE_DISCOVERY = "discovery";
    static constexpr const char *TYPE_AUTHENTICATOR = "authenticator";

    typedef void *(*EntryPoint)(int argc, char *argv[]);

    virtual void Destroy(void) = 0;

    virtual const char *GetModuleName(void) const = 0;
    virtual const char *GetModuleType(void) const = 0;

protected:
    ModuleInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_MOUDLE_INTERFACE_H__
