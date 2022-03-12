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
#pragma once

#include <functional>
#include <dns_sd.h>
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include "discoveryinfo.h"
#include "nsdeventloop.h"

class NsdManager {
public:
    typedef std::function<void(std::string)> CallbackWithString;
    typedef std::function<void(beyond_event_type, DiscoveryInfo &)> CallbackWithInfo;

    NsdManager();
    ~NsdManager();

    int discoverServices(CallbackWithInfo cb);
    int stopServiceDiscovery();
    int registerService(DiscoveryInfo &serviceInfo, CallbackWithString cb);
    int unregisterService();

private:
    struct NsdCallbackData {
        virtual ~NsdCallbackData() = default;
        NsdManager *nsd;
    };
    struct NsdDiscoverData : public NsdCallbackData {
        NsdDiscoverData(NsdManager *obj, CallbackWithInfo cb);
        CallbackWithInfo discovered;
    };
    struct NsdResolveData : public NsdCallbackData {
        NsdResolveData(NsdManager *obj, CallbackWithInfo cb);
        DiscoveryInfo info;
        CallbackWithInfo resolved;
    };
    struct NsdRegisterData : public NsdCallbackData {
        NsdRegisterData(NsdManager *obj, CallbackWithString cb);
        CallbackWithString registered;
    };

    static void discover_reply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                               DNSServiceErrorType errorCode, const char *serviceName,
                               const char *regtype, const char *replyDomain, void *context);
    static void resolve_reply(DNSServiceRef sdref, const DNSServiceFlags flags,
                              uint32_t ifIndex, DNSServiceErrorType errorCode,
                              const char *fullname, const char *hosttarget, uint16_t opaqueport,
                              uint16_t txtLen, const unsigned char *txtRecord, void *context);
    static void addrinfo_reply(DNSServiceRef sdref, const DNSServiceFlags flags,
                               uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                               const char *hostname, const struct sockaddr *address, uint32_t ttl,
                               void *context);
    static void register_reply(DNSServiceRef sdref, const DNSServiceFlags flags,
                               DNSServiceErrorType errorCode, const char *name,
                               const char *regtype, const char *domain, void *context);
    static void process_result(DNSServiceRef sdRef);
    int resolveService(std::string serviceName, CallbackWithInfo cb);
    int stopService();

    static constexpr const char *REGTYPE = "_beyond._tcp";
    static constexpr const char *DOMAIN_LOCAL = "local";

    DNSServiceRef svc;
    NsdEventLoop eventLoop;
    NsdCallbackData *callbackData;
};
