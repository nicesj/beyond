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

#ifndef __BEYOND_PRIVATE_AUTHENTICATOR_INTERFACE_H
#define __BEYOND_PRIVATE_AUTHENTICATOR_INTERFACE_H

#include <beyond/private/event_object_interface_private.h>
#include <beyond/private/module_interface_private.h>

namespace beyond {

class AuthenticatorInterface : public ModuleInterface, public EventObjectInterface {
public:
    struct EventData : public EventObjectInterface::EventData {
    };

public:
    virtual ~AuthenticatorInterface(void) = default;

    virtual int Configure(const beyond_config *options = nullptr) = 0;

    virtual int Activate(void) = 0;

    virtual int Prepare(void) = 0;

    virtual int Encrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) = 0;
    virtual int Decrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) = 0;

    virtual int GetResult(void *&outData, int &outSize) = 0;
    virtual int GetKey(beyond_authenticator_key_id id, void *&key, int &size) = 0;

    virtual int Deactivate(void) = 0;

    virtual int GenerateSignature(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize) = 0;
    virtual int VerifySignature(unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic) = 0;

protected:
    AuthenticatorInterface(void) = default;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_AUTHENTICATOR_INTERFACE_H
