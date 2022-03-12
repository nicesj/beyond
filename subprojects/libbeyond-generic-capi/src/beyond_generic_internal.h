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

#ifndef __BEYOND_GENERIC_INTERNAL_H__
#define __BEYOND_GENERIC_INTERNAL_H__

#include <cerrno>

#define BEYOND_GENERIC_HANDLE_MAGIC_LEN 8
#define BEYOND_GENERIC_HANDLE_MAGIC "_B_GEH!_"
#define BEYOND_GENERIC_HANDLE_MAGIC_FREE "DEADBEEF"

struct beyond_generic_handle {
    char magic[BEYOND_GENERIC_HANDLE_MAGIC_LEN];
    void *handle;
};

extern void beyond_generic_handle_init(void *handle);
extern void beyond_generic_handle_deinit(void *handle);
extern int beyond_generic_handle_set_handle(void *handle, void *_beyond_handle);
extern int beyond_generic_handle_is_valid(void *handle);

template <typename T>
int beyond_generic_handle_get_handle(void *handle, T *&_beyond_handle)
{
    beyond_generic_handle *_handle = static_cast<beyond_generic_handle *>(handle);
    if (beyond_generic_handle_is_valid(handle) == 0) {
        return -EINVAL;
    }

    _beyond_handle = static_cast<T *>(_handle->handle);
    return 0;
}

#endif // __BEYOND_GENERIC_INTERNAL_H__
