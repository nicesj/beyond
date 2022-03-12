/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "task_runner.h"

#include <cstring>

#include "task_options.h"
// logging

namespace beyond {
namespace evaluation {

int TaskRunner::Run(int *argc,
                    char *argv[])
{
    auto option_list = GetOptions();

    if (*argc < 2 || std::strstr(argv[1], "-help") != nullptr || std::strcmp(argv[1], "-h") == 0) {
        Options::PrintUsage(option_list);
        return 1;
    }

    bool parse_result = Options::Parse(argc, const_cast<const char **>(argv), option_list);
    if (parse_result == false) {
        //error if find undefined options or their valus is incorrect
        printf("Please check on command line options\n");
        return -1;
    }

    return RunImpl();
}

} // namespace evaluation
} // namespace beyond
