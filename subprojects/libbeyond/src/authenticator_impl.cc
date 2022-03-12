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

#include <cstdio>
#include <cerrno>
#include <exception>
#include <functional>
#include <cstdlib>
#include <cstring>

#include <dlfcn.h>
#include <getopt.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"

#include "authenticator_impl.h"

// NOTE:
// This implementation is going to help loading an authenticator module
namespace beyond {

Authenticator::impl *Authenticator::impl::Create(const beyond_argument *arg)
{
    if (arg == nullptr || arg->argc < 1 || arg->argv[0] == nullptr) {
        return nullptr;
    }

    Authenticator::impl *impl;

    try {
        impl = new Authenticator::impl();
    } catch (std::exception &e) {
        ErrPrint("new Authenticator::impl(): %s", e.what());
        return nullptr;
    }

    int namelen = strlen(ModuleInterface::FilenameFormat) + strlen(arg->argv[0]) - 1;
    char *filename = static_cast<char *>(malloc(namelen));
    if (filename == nullptr) {
        // error
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    int ret = snprintf(filename, namelen, ModuleInterface::FilenameFormat, arg->argv[0]);
    if (ret < 0) {
        // error
        free(filename);
        filename = nullptr;
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->dlHandle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    free(filename);
    filename = nullptr;
    if (impl->dlHandle == nullptr) {
        ErrPrint("dlopen failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    ModuleInterface::EntryPoint main = reinterpret_cast<ModuleInterface::EntryPoint>(dlsym(impl->dlHandle, ModuleInterface::EntryPointSymbol));
    if (main == nullptr) {
        ErrPrint("dlsym failed: %s", dlerror());
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    optind = 0;
    opterr = 0;

    AuthenticatorInterface *authenticator = reinterpret_cast<AuthenticatorInterface *>(main(arg->argc, arg->argv));
    if (authenticator == nullptr) {
        delete impl;
        impl = nullptr;
        return nullptr;
    }

    const char *moduleType = authenticator->GetModuleType();
    if (moduleType == nullptr) {
        authenticator->Destroy();
        authenticator = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (strcmp(moduleType, ModuleInterface::TYPE_AUTHENTICATOR) != 0) {
        authenticator->Destroy();
        authenticator = nullptr;

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    impl->module = authenticator;
    return impl;
}

Authenticator::impl::impl(void)
    : dlHandle(nullptr)
    , module(nullptr)
{
}

Authenticator::impl::~impl(void)
{
    if (dlHandle != nullptr) {
        if (dlclose(dlHandle) < 0) {
            ErrPrint("dlclose: %s", dlerror());
        }
        dlHandle = nullptr;
    }
}

void Authenticator::impl::Destroy(void)
{
    module->Destroy();
    module = nullptr;

    delete this;
}

int Authenticator::impl::Configure(const beyond_config *options)
{
    return module->Configure(options);
}

int Authenticator::impl::Activate(void)
{
    return module->Activate();
}

int Authenticator::impl::Prepare(void)
{
    return module->Prepare();
}

int Authenticator::impl::Deactivate(void)
{
    return module->Deactivate();
}

int Authenticator::impl::Encrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize)
{
    return module->Encrypt(id, data, size, iv, ivsize);
}

int Authenticator::impl::Decrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize)
{
    return module->Decrypt(id, data, size, iv, ivsize);
}

int Authenticator::impl::GetResult(void *&outData, int &outSize)
{
    return module->GetResult(outData, outSize);
}

int Authenticator::impl::GetKey(beyond_authenticator_key_id id, void *&key, int &size)
{
    return module->GetKey(id, key, size);
}

int Authenticator::impl::GenerateSignature(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize)
{
    return module->GenerateSignature(data, dataSize, encoded, encodedSize);
}

int Authenticator::impl::VerifySignature(unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic)
{
    return module->VerifySignature(signedData, signedDataSize, original, originalSize, authentic);
}

const char *Authenticator::impl::GetModuleName(void) const
{
    return module->GetModuleName();
}

const char *Authenticator::impl::GetModuleType(void) const
{
    return module->GetModuleType();
}

int Authenticator::impl::GetHandle(void) const
{
    return module->GetHandle();
}

int Authenticator::impl::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->AddHandler(handler, type, data);
}

int Authenticator::impl::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return module->RemoveHandler(handler, type, data);
}

int Authenticator::impl::FetchEventData(EventObjectInterface::EventData *&data)
{
    return module->FetchEventData(data);
}

int Authenticator::impl::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return module->DestroyEventData(data);
}

} // namespace beyond
