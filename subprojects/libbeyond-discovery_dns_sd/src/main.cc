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
#include <getopt.h>
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include "discovery_client.h"
#include "discovery_server.h"
#include "discoveryinfo.h"

// ApplicationContext
extern "C" {

static const option opts[] = {
    {
        .name = "server",
        .has_arg = no_argument,
        .flag = nullptr,
        .val = 's',
    },
    {
        .name = "client",
        .has_arg = no_argument,
        .flag = nullptr,
        .val = 'c',
    },
    {
        .name = "name",
        .has_arg = required_argument,
        .flag = nullptr,
        .val = 'n',
    },
    {
        .name = "port",
        .has_arg = required_argument,
        .flag = nullptr,
        .val = 'p',
    }
};

API void *_main(int argc, char *argv[])
{
    int c;
    int idx;
    bool isServer = false;
    DiscoveryInfo info;
    Discovery *discovery = nullptr;

    if (0 < optind) {
        ErrPrint("Invalid getopt status(%d)", optind);
    }
    while ((c = getopt_long(argc, argv, "-:scn:p:", opts, &idx)) != -1) {
        switch (c) {
        case 's':
            isServer = true;
            InfoPrint("Set DiscoveryType to SERVER");
            break;
        case 'c':
            isServer = false;
            InfoPrint("Set DiscoveryType to CLIENT");
            break;
        case 'n':
            InfoPrint("Name: %s", optarg);
            if (optarg) {
                info.name = optarg;
            }
            break;
        case 'p':
            InfoPrint("Port: %s", optarg);
            if (optarg) {
                info.port = atoi(optarg);
            }
            break;
        default:
            break;
        }
    }

    try {
        if (isServer) {
            discovery = new DiscoveryServer(info.name, info.port);
        } else {
            discovery = new DiscoveryClient();
        }
    } catch (const std::exception &e) {
        ErrPrint("exception(%s)", e.what());
    }

    if (discovery == nullptr) {
        ErrPrint("discovery dns-sd is not created successfully.");
    }

    return discovery;
}
}

#if 0
int main(void)
{
    char *argv[] = { strdup("discovery") };
    int argc = 1;
    auto discovery = static_cast<Discovery *>(_main(argc, argv));

    printf("discovery: %p\n", discovery);

    if (discovery != nullptr) {
        discovery->Destroy();
        discovery = nullptr;
    }

    free(argv[0]);
    argv[0] = nullptr;

    _exit(0);
}
#endif // NDEBUG
