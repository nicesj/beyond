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

#ifndef __BEYOND_MOCK_H__
#define __BEYOND_MOCK_H__

namespace mock {
class Type {
public:
    // NOTE:
    // Using constexpr allocates new space for storing data for the case of a
    // library and a program respectively. I found a way to declare a variable
    // with constexpr to use a unique address, but it didn't work for me.
    // Reference:
    // https://stackoverflow.com/questions/14872240/unique-address-for-constexpr-variable
    static const void *AnyPtr;
    static const char *AnyStr;
    static const int AnyInt;
};
} // namespace mock

#if __cplusplus
#define restrict
#endif

#define TYPE_PREFIX(type) mock::Ctrl::##type

#define CTRL_CLIENT(TYPE, NAME, COMPARE, RETVAL, NATIVE)                                               \
    do {                                                                                               \
        __typeof__(TYPE) *native_func = reinterpret_cast<__typeof__(TYPE) *>(dlsym(RTLD_NEXT, #TYPE)); \
        if (mock::Ctrl::instance == nullptr) {                                                         \
            return native_func NATIVE;                                                                 \
        }                                                                                              \
                                                                                                       \
        mock::Ctrl::Expect *i = mock::Ctrl::instance->head;                                            \
        mock::Ctrl::Expect *n;                                                                         \
                                                                                                       \
        do {                                                                                           \
            n = i->next;                                                                               \
                                                                                                       \
            if (strncmp(i->name, __func__, strlen(__func__)) == 0) {                                   \
                mock::Ctrl::args_##TYPE *NAME =                                                        \
                    static_cast<mock::Ctrl::args_##TYPE *>(i->args);                                   \
                                                                                                       \
                if (COMPARE) {                                                                         \
                    assert(i->count > 0);                                                              \
                    i->count--;                                                                        \
                    if (i->use_native == true) {                                                       \
                        return native_func NATIVE;                                                     \
                    } else {                                                                           \
                        return i->ret.RETVAL;                                                          \
                    }                                                                                  \
                }                                                                                      \
            }                                                                                          \
                                                                                                       \
            i = n;                                                                                     \
        } while (i != nullptr);                                                                        \
                                                                                                       \
        if (mock::Ctrl::instance->fallbackNative == true) {                                            \
            return native_func NATIVE;                                                                 \
        }                                                                                              \
    } while (0)

#endif // __BEYOND_MOCK_H__
