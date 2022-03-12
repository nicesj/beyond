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
#include "task_options.h"

#include <functional>
#include <sstream>
#include <unordered_map>

namespace beyond {
namespace evaluation {

template <typename T>
std::string ToString(T value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

template <typename T>
bool ConvertOpt(const std::string &value,
                const std::function<void(const T &)> &hook)
{
    std::istringstream stream(value);
    T read_value;
    stream >> read_value;
    if (!stream.eof() && !stream.good()) {
        return false;
    }
    hook(read_value);
    return true;
}

bool ParseOpt(const std::string &arg, const std::string &name,
              const std::function<bool(const std::string &)> &func,
              bool *value_ok)
{
    *value_ok = true;

    std::string prefix = "--" + name + "=";
    if (arg.find(prefix) == std::string::npos) {
        return false;
    }

    bool has_value = arg.size() >= prefix.size();
    *value_ok = has_value;
    if (has_value) {
        *value_ok = func(arg.substr(prefix.size()));
    }
    return true;
}

Option::Option(const char *name,
               const std::function<void(const std::string &)> &hook,
               const std::string &default_value,
               const std::string &usage)
    : name_(name)
    , usage_(usage)
    , default_(default_value)
    , convert_hook_([hook](const std::string &val) {
        hook(val);
        return true;
    })
{
}

Option::Option(const char *name, const std::function<void(const int16_t &)> &hook,
               int16_t default_value, const std::string &usage)
    : name_(name)
    , usage_(usage)
    , default_(ToString(default_value))
    , convert_hook_([hook](const std::string &val) {
        return ConvertOpt<int16_t>(val, hook);
    })
{
}

Option::Option(const char *name, const std::function<void(const int32_t &)> &hook,
               int32_t default_value, const std::string &usage)
    : name_(name)
    , usage_(usage)
    , default_(ToString(default_value))
    , convert_hook_([hook](const std::string &val) {
        return ConvertOpt<int32_t>(val, hook);
    })
{
}

Option::Option(const char *name, const std::function<void(const int64_t &)> &hook,
               int64_t default_value, const std::string &usage)
    : name_(name)
    , usage_(usage)
    , default_(ToString(default_value))
    , convert_hook_([hook](const std::string &val) {
        return ConvertOpt<int64_t>(val, hook);
    })
{
}

bool Option::Parse(const std::string &arg, bool *value_ok) const
{
    return ParseOpt(arg, name_, convert_hook_, value_ok);
}

bool Options::Parse(int *argc, const char **argv, const std::vector<Option> &opt_list)
{
    bool result = true;

    // compare options defined vs given
    // print error when meets unexpected, incorrect params
    std::vector<bool> parsed_argvs(*argc, false);
    std::unordered_map<std::string, int> parsed_opts;

    // std::vector<int> opt_idx(opt_list.size());
    // for (unsigned int idx = 0; idx < opt_idx.size(); idx++) {
    for (unsigned int idx = 0; idx < opt_list.size(); idx++) {
        const Option &option = opt_list[idx];
        const auto it = parsed_opts.find(option.name_);
        if (it != parsed_opts.end()) {
            printf("Duplicate option found in a task : %s \n", option.name_.c_str());
            result = false;
        }

        bool found = false;
        for (int i = 1; i < *argc; ++i) {
            if (parsed_argvs[i] == true)
                continue;
            bool value_ok;
            found = option.Parse(argv[i], &value_ok);
            if (value_ok == false) {
                printf("Failed parse option [%s], against argv %s \n", option.name_.c_str(), argv[i]);
                result = false;
            }
            if (found == true) {
                parsed_argvs[i] = true;
                parsed_opts[option.name_] = i;
                break;
            }
        }

        if (found == true)
            continue;
        parsed_opts[option.name_] = -1;
#ifdef DEBUG
        printf("[%s] option is not entered\n", option.name_.c_str());
#endif
    }

    for (int i = 1; i < *argc; ++i) {
        if (parsed_argvs[i] == false) {
            printf("Warnning: the command line option is not consumed :\n \t\t %s \n", argv[i]);
        }
    }

    return result;
}

void Options::PrintUsage(const std::vector<Option> &opt_list)
{
    printf("Usage : [option]\n");
    for (unsigned int idx = 0; idx < opt_list.size(); idx++) {
        const Option &option = opt_list[idx];
        printf(" %s : %s\n", option.name_.c_str(), option.usage_.c_str());
    }
}

} // namespace evaluation
} // namespace beyond
