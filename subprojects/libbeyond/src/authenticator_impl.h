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

#ifndef __BEYOND_INTERNAL_AUTHENTICATOR_IMPL_H__
#define __BEYOND_INTERNAL_AUTHENTICATOR_IMPL_H__

#include <beyond/common.h>
#include <beyond/private/authenticator_private.h>

namespace beyond {

class Authenticator::impl final : public Authenticator {
public: // Authenticator interface
    static impl *Create(const beyond_argument *arg);
    void Destroy(void) override;

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

public: // AuthenticatorInterface interface
    int Configure(const beyond_config *options = nullptr) override;

    int Activate(void) override;
    int Prepare(void) override;
    int Deactivate(void) override;

    int Encrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) override;
    int Decrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) override;
    int GetResult(void *&outData, int &outSize) override;
    int GetKey(beyond_authenticator_key_id id, void *&key, int &size) override;

    int GenerateSignature(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize) override;
    int VerifySignature(unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic) override;

public: // ModuleInterface interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;

private:
    impl(void);
    virtual ~impl(void);

    void *dlHandle;
    AuthenticatorInterface *module;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_AUTHENTICATOR_IMPL_H__
