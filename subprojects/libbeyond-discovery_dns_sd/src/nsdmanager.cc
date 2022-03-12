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
#include "nsdmanager.h"

#include <errno.h>
#include <functional>

NsdManager::NsdManager()
    : svc(nullptr)
    , callbackData(nullptr)
{
}

NsdManager::~NsdManager()
{
    if (svc) {
        DNSServiceRefDeallocate(svc);
    }
}

int NsdManager::discoverServices(CallbackWithInfo cb)
{
    InfoPrint("discoverServices");
    if (svc) {
        ErrPrint("Discovery is already running");
        return -EALREADY;
    }

    auto *cbData = new NsdDiscoverData(this, cb);
    auto err = DNSServiceBrowse(&svc, 0, kDNSServiceInterfaceIndexAny,
                                REGTYPE, DOMAIN_LOCAL, discover_reply, cbData);
    if (err != kDNSServiceErr_NoError || svc == nullptr) {
        ErrPrint("DNSServiceBrowse failed");
        delete cbData;
        return -EFAULT;
    }

    delete callbackData;
    callbackData = cbData;

    try {
        eventLoop.addWatch(DNSServiceRefSockFD(svc), std::bind(&process_result, svc));
        eventLoop.run();
    } catch (NsdLoopException &e) {
        stopServiceDiscovery();
        return e.returnValue;
    }

    return 0;
}

void NsdManager::discover_reply(DNSServiceRef sdRef, DNSServiceFlags flags,
                                uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                const char *serviceName, const char *regtype,
                                const char *replyDomain, void *context)
{
    auto *cbData = static_cast<NsdDiscoverData *>(context);
    if (cbData == nullptr || cbData->nsd == nullptr) {
        ErrPrint("No Callback Data");
        return;
    }

    if (flags & kDNSServiceFlagsAdd) {
        InfoPrint("%s added", serviceName);
        int ret = cbData->nsd->resolveService(serviceName, cbData->discovered);
        if (ret < 0 && cbData->discovered) {
            DiscoveryInfo info;
            info.name = serviceName;
            info.port = 0;
            cbData->discovered(beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_ERROR, info);
        }
    } else {
        InfoPrint("%s removed", serviceName);
        DiscoveryInfo info;
        info.name = serviceName;
        cbData->discovered(beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_REMOVED, info);
    }
}

int NsdManager::resolveService(std::string serviceName, CallbackWithInfo cb)
{
    InfoPrint("resolveService");
    auto *cbData = new NsdManager::NsdResolveData(this, cb);
    cbData->info.name = serviceName;

    int ret;
    DNSServiceRef extraSvc;
    ret = DNSServiceResolve(&extraSvc, 0, kDNSServiceInterfaceIndexAny, serviceName.c_str(),
                            REGTYPE, DOMAIN_LOCAL, resolve_reply, cbData);
    if (ret != kDNSServiceErr_NoError) {
        ErrPrint("DNSServiceResolve() Fail(%d)", ret);
        delete cbData;
        return -EFAULT;
    }

    try {
        eventLoop.addWatch(DNSServiceRefSockFD(extraSvc), std::bind(&process_result, extraSvc));
    } catch (NsdLoopException &e) {
        ErrPrintCode(errno, "addWatch() Fail(%d)", e.returnValue);
        delete cbData;
        return -EFAULT;
    }

    return 0;
}

void NsdManager::resolve_reply(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex,
                               DNSServiceErrorType errorCode, const char *fullname,
                               const char *hosttarget, uint16_t opaqueport, uint16_t txtLen,
                               const unsigned char *txtRecord, void *context)
{
    DbgPrint("resolve_reply");
    auto *cbData = static_cast<NsdResolveData *>(context);
    if (cbData == nullptr || cbData->nsd == nullptr || cbData->resolved == nullptr) {
        ErrPrint("Invalid Callback Data");
        return;
    }

    uint8_t valueLen = 0;
    auto value = TXTRecordGetValuePtr(txtLen, txtRecord, "uuid", &valueLen);
    if (value)
        cbData->info.uuid = std::string(static_cast<const char *>(value), valueLen);
    cbData->info.port = ntohs(opaqueport);

    DNSServiceRef extraSvc;
    auto err = DNSServiceGetAddrInfo(&extraSvc, 0, kDNSServiceInterfaceIndexAny, kDNSServiceProtocol_IPv4, hosttarget, addrinfo_reply, cbData);
    if (err != kDNSServiceErr_NoError) {
        ErrPrint("DNSServiceGetAddrInfo failed");
        cbData->resolved(beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_ERROR, cbData->info);
        delete cbData;
        return;
    }
    try {
        cbData->nsd->eventLoop.addWatch(DNSServiceRefSockFD(extraSvc), std::bind(&NsdManager::process_result, extraSvc));
    } catch (NsdLoopException &e) {
        ErrPrintCode(errno, "addWatch() Fail(%d)", e.returnValue);
        cbData->resolved(beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_ERROR, cbData->info);
        delete cbData;
    }
}

void NsdManager::addrinfo_reply(DNSServiceRef sdref, const DNSServiceFlags flags,
                                uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                const char *hostname, const struct sockaddr *address,
                                uint32_t ttl, void *context)
{
    DbgPrint("addrinfo_reply");
    auto *cbData = static_cast<NsdResolveData *>(context);
    if (cbData == nullptr || cbData->nsd == nullptr || cbData->resolved == nullptr) {
        ErrPrint("No Callback Data");
        return;
    }

    cbData->nsd->eventLoop.delWatch(DNSServiceRefSockFD(sdref));
    DNSServiceRefDeallocate(sdref);

    cbData->info.address = address;
    cbData->resolved(beyond_event_type::BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED, cbData->info);

    delete cbData;
}

int NsdManager::stopServiceDiscovery()
{
    InfoPrint("DiscoveryClient Deactivate");

    int ret = stopService();

    delete callbackData;
    callbackData = nullptr;

    return ret;
}

int NsdManager::registerService(DiscoveryInfo &info, CallbackWithString cb)
{
    InfoPrint("Request Register(name:%s, port:%u)", info.name.c_str(), info.port);

    if (svc) {
        ErrPrint("Register is already running");
        return -EALREADY;
    }

    auto *cbData = new NsdRegisterData(this, cb);

    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, nullptr);
    for (auto &element : info.valueMap) {
        const std::string &key = element.first;
        DiscoveryInfo::Value &val = element.second;
        auto ret = TXTRecordSetValue(&txtRecord, key.c_str(), val.size, val.value);
        if (ret != kDNSServiceErr_NoError) {
            ErrPrint("TXTRecordSetValue() Fail(%d)", ret);
            TXTRecordDeallocate(&txtRecord);
            return -1;
        }
    }

    auto err = DNSServiceRegister(&svc, 0, kDNSServiceInterfaceIndexAny, info.name.c_str(),
                                  REGTYPE, DOMAIN_LOCAL, NULL, htons(info.port),
                                  TXTRecordGetLength(&txtRecord),
                                  TXTRecordGetBytesPtr(&txtRecord), register_reply, cbData);
    if (err != kDNSServiceErr_NoError || svc == nullptr) {
        ErrPrint("DNSServiceRegister() Fail(%d)", err);
        return -1;
    }

    try {
        eventLoop.addWatch(DNSServiceRefSockFD(svc), std::bind(&process_result, svc));
        eventLoop.run();
    } catch (NsdLoopException &e) {
        unregisterService();
        return e.returnValue;
    }

    return 0;
}

void NsdManager::register_reply(DNSServiceRef sdref, const DNSServiceFlags flags,
                                DNSServiceErrorType errorCode, const char *name,
                                const char *regtype, const char *domain, void *context)
{
    DbgPrint("register_reply");
    auto *cbData = static_cast<NsdRegisterData *>(context);
    if (cbData == nullptr) {
        ErrPrint("No Callback Data");
        return;
    }

    if (cbData->registered)
        cbData->registered(name);
    delete cbData;
}

int NsdManager::unregisterService()
{
    InfoPrint("RegisterService Deactivate");
    return stopService();
}

int NsdManager::stopService()
{
    if (svc == nullptr)
        return -EALREADY;

    int ret = 0;
    try {
        eventLoop.quit();
        eventLoop.delWatch(DNSServiceRefSockFD(svc));
    } catch (NsdLoopException &e) {
        ret = e.returnValue;
    }

    DNSServiceRefDeallocate(svc);
    svc = nullptr;

    return ret;
}

void NsdManager::process_result(DNSServiceRef sdRef)
{
    DNSServiceProcessResult(sdRef);
}

inline NsdManager::NsdDiscoverData::NsdDiscoverData(NsdManager *obj, CallbackWithInfo cb)
    : discovered(cb)
{
    nsd = obj;
}

inline NsdManager::NsdResolveData::NsdResolveData(NsdManager *obj, CallbackWithInfo cb)
    : resolved(cb)
{
    nsd = obj;
}

inline NsdManager::NsdRegisterData::NsdRegisterData(NsdManager *obj, CallbackWithString cb)
    : registered(cb)
{
    nsd = obj;
}
