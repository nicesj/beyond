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

#include <gst/gst.h>

#include <getopt.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "peer.h"

extern "C" {

static void dup_opt(char *&dst, char *arg) {
    assert(arg != nullptr && "optarg is nullptr");
    if (arg != nullptr) {
        free(dst);
        dst = strdup(arg);
        if (dst == nullptr) {
            ErrPrintCode(errno, "strdup");
        }
    }
}

// ApplicationContext
API void *_main(int argc, char *argv[])
{
    const option opts[] = {
        {
            .name = "server",
            .has_arg = 0,
            .flag = nullptr,
            .val = 's',
        },
        {
            .name = "path",
            .has_arg = 1,
            .flag = nullptr,
            .val = 'p',
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_OPTION_FRAMEWORK),
            .has_arg = 1,
            .flag = nullptr,
            .val = 'f',
        },
        {
            .name = BEYOND_GET_OPTION_NAME(BEYOND_INFERENCE_OPTION_FRAMEWORK_ACCEL),
            .has_arg = 1,
            .flag = nullptr,
            .val = 'a',
        },
        // TODO:
        // Add more options
    };

    static bool gst_initialized = false;

    if (gst_initialized == false) {
        gst_initialized = true;
        gst_init(nullptr, nullptr);
    }

    int c;
    int idx;
    bool isServer = false;
    char *storagePath = nullptr;
    char *framework = nullptr;
    char *accel = nullptr;
    while ((c = getopt_long(argc, argv, "-:sp:f:a:", opts, &idx)) != -1) {
        switch (c) {
        case 's':
            isServer = true;
            break;
        case 'p':
            dup_opt(storagePath, optarg);
            break;
        case 'f':
            dup_opt(framework, optarg);
            break;
        case 'a':
            dup_opt(accel, optarg);
            break;
        default:
            break;
        }
    }

    Peer *peer = Peer::Create(isServer, framework, accel, storagePath);
    free(framework);
    framework = nullptr;
    free(accel);
    accel = nullptr;
    free(storagePath);
    storagePath = nullptr;
    if (peer == nullptr) {
        // TODO:
        // Unable to create a peer module instance
        ErrPrint("Peer is not created successfully.");
    }

    return reinterpret_cast<void *>(peer);
}

} // extern "C"

#ifndef NDEBUG
int main(void)
{
    char *argv[] = { strdup("peer_nn") };
    int argc = 1;
    Peer *peer = static_cast<Peer *>(_main(argc, argv));

    printf("peer: %p\n", peer);

    if (peer != nullptr) {
        peer->Destroy();
        peer = nullptr;
    }

    free(argv[0]);
    argv[0] = nullptr;

    _exit(0);
}
#endif // NDEBUG
