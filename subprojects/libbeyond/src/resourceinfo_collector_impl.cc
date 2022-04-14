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
#include "resourceinfo_collector_impl.h"

#include <unistd.h>
#include <limits.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"

#include "beyond/private/resourceinfo_collector.h"

namespace beyond {

ResourceInfoCollector::impl::impl(std::string &storagePath)
    : storagePath(storagePath)
{
}

void ResourceInfoCollector::impl::collectResourceInfo(beyond_peer_info *info)
{
    if (info->free_memory <= 0) {
        struct sysinfo _info = {0};
        if (sysinfo(&_info) < 0) {
            ErrPrintCode(errno, "sysinfo");
        } else {
            info->free_memory = _info.freeram;
        }
    }

    if (info->free_storage <= 0) {
        unsigned long long free_storage = 0;
        if (storagePath.empty() == true) {
            char _path[PATH_MAX];
            if (getcwd(_path, sizeof(_path)) == nullptr) {
                ErrPrintCode(errno, "getcwd");
            } else {
                storagePath = std::string(_path);
            }
        }

        if (storagePath.empty() == false) {
            struct statvfs _stat;
            if (statvfs(storagePath.c_str(), &_stat) < 0) {
                ErrPrintCode(errno, "statvfs");
            } else {
                free_storage = _stat.f_bsize * _stat.f_bfree;
            }
        }
        info->free_storage = free_storage;
    }
}

} // namespace beyond
