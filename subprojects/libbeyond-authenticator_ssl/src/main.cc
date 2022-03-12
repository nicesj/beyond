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

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <string>

#include <getopt.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "authenticator.h"
#include "beyond/plugin/authenticator_ssl_plugin.h"

extern "C" {

// ApplicationContext
API void *_main(int argc, char *argv[])
{
    const option opts[] = {
        {
            .name = "help",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'h',
        },
        {
            .name = "async",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'a',
        },
        // TODO:
        // Add more options
    };

    int c;
    int idx;
    bool enableAsync = false;

    while ((c = getopt_long(argc, argv, "-:ha", opts, &idx)) != -1) {
        switch (c) {
        case 'h':
            InfoPrint("Help message");
            break;
        case 'a':
            InfoPrint("Enable asynchronous mode");
            enableAsync = true;
            break;
        default:
            break;
        }
    }

    Authenticator *authenticator = Authenticator::Create(enableAsync);
    if (authenticator == nullptr) {
        // TODO:
        // Unable to create a authenticator module instance
        ErrPrint("Authenticator is not created successfully.");
    }

    return reinterpret_cast<void *>(authenticator);
}

} // extern "C"

#ifndef NDEBUG
int main(void)
{
    char *argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);
    Authenticator *authenticator = static_cast<Authenticator *>(_main(argc, argv));

    printf(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME ": %p (argc: %d)\n", authenticator, argc);

    if (authenticator != nullptr) {
        authenticator->Destroy();
        authenticator = nullptr;
    }

    _exit(0);
}
#endif // NDEBUG
