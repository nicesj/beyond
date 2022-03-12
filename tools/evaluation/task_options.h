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
#ifndef _BEYOND_TOOLS_EVALUATION_TASK_OPTIONS_H_
#define _BEYOND_TOOLS_EVALUATION_TASK_OPTIONS_H_

#include <functional>
#include <string>
#include <vector>

namespace beyond {
namespace evaluation {
class Option {
public:
    template <typename T>
    static Option CreateOption(const char *name, T *val, const char *usage)
    {
        return Option(
            name, [val](const T &v) { *val = v; }, *val, usage);
    }

    Option(const char *name, const std::function<void(const std::string &)> &hook,
           const std::string &default_value, const std::string &usage);
    Option(const char *name, const std::function<void(const int16_t &)> &hook,
           int16_t default_value, const std::string &usage);
    Option(const char *name, const std::function<void(const int32_t &)> &hook,
           int32_t default_value, const std::string &usage);
    Option(const char *name, const std::function<void(const int64_t &)> &hook,
           int64_t default_value, const std::string &usage);

    bool Parse(const std::string &arg, bool *val_ok) const;

private:
    friend class Options;

    std::string name_;
    std::string usage_;
    std::string default_;
    std::function<bool(const std::string &)> convert_hook_;
};

class Options {
public:
    static bool Parse(int *argc, const char **argv, const std::vector<Option> &opt_list);
    static void PrintUsage(const std::vector<Option> &opt_list);
};
} // namespace evaluation
} // namespace beyond
#endif // _BEYOND_TOOLS_EVALUATION_TASK_OPTIONS_H_
