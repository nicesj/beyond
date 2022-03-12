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

#ifndef __BEYOND_AUTHENTICATOR_SSL_H__
#define __BEYOND_AUTHENTICATOR_SSL_H__

#include <string>
#include <memory>

#include <openssl/evp.h>
#include <openssl/x509v3.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/authenticator_ssl_plugin.h"

class Authenticator final : public beyond::AuthenticatorInterface {
public:
    static constexpr const char *NAME = BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME;
    static Authenticator *Create(bool enableAsync = false);

public: // module interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;
    void Destroy(void) override;

public: // EventObject interface
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

public: // Authenticator interface
    int Configure(const beyond_config *options = nullptr) override;
    int Activate(void) override;
    int Prepare(void) override;
    int Deactivate(void) override;

    int Encrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) override;
    int Decrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv = nullptr, int ivsize = 0) override;
    int GetResult(void *&outData, int &outSize) override;
    int GetKey(beyond_authenticator_key_id id, void *&key, int &size) override;

    int VerifySignature(unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic) override;
    int GenerateSignature(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize) override;

private:
    class EventObject;

    enum CRYPTO_OP {
        ENCRYPT = 0,
        DECRYPT = 1,
    };

    enum OUTPUT {
        CRYPTO_OUTPUT = 0,
    };

    enum COMMAND {
        GENERATE = 0,
        CLEANUP = 1,
        CRYPTO = 2,
        GETKEY = 3,
        GENERATE_SIGN = 4,
        VERIFY_SIGN = 5,
        LAST = 6,
    };

    struct GenerateSignData {
        unsigned char *data;
        int size;

        int status;
    };

    struct VerifySignData {
        const unsigned char *data;
        int size;

        const unsigned char *signedData;
        int signedDataSize;

        int status;
        bool authentic;
    };

    struct CryptoInput {
        CRYPTO_OP op;
        beyond_authenticator_key_id id;
        const void *data;
        int size;
        const void *iv;
        int ivsize;
    };

    struct CryptoOutput {
        void *output;
        int size;
    };

    struct KeyInfo {
        void *key;
        int size;
        int status;
    };

private:
    typedef int (*CommandHandler)(Authenticator *inst, void *data);

    static int CommandGenerate(Authenticator *inst, void *data);
    static int CommandCleanup(Authenticator *inst, void *data);
    static int CommandCrypto(Authenticator *inst, void *data);
    static int CommandGetKey(Authenticator *inst, void *data);
    static int CommandGenerateSign(Authenticator *inst, void *data);
    static int CommandVerifySign(Authenticator *inst, void *data);
    static CommandHandler commandTable[COMMAND::LAST];

private:
    Authenticator(void);
    virtual ~Authenticator(void);

    int AddExtension(X509 *cert, int nid, char *value, X509 *issuer = nullptr);
    int CryptoSymmetric(int opid, beyond_authenticator_key_id id, const void *in, size_t inlen, void *&out, size_t &outlen, const void *iv, size_t ivsize);
    int CryptoAsymmetric(int opid, beyond_authenticator_key_id id, const void *in, size_t inlen, void *&out, size_t &outlen);
    int Crypto(int opid, beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize);
    int GetPrivateKey(void *&key, int &size);
    int GetPublicKey(void *&key, int &size);
    int GetCertificate(void *&key, int &size);
    int GetSecretKey(void *&key, int &size);
    int VerifySign(const unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic);
    int GenerateSign(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize);

    int ConfigureJSON(void *object);
    int ConfigureSSL(void *object);
    int ConfigureSecretKey(void *object);
    int ConfigureAuthenticator(void *object);

    int LoadPrivateKey(const char *keyString, int keyLength, EVP_PKEY *&keypair);
    int LoadCertificate(const char *certificateString, int certificateStringLength, X509 *&x509);
    int LoadCertificate(void);
    int LoadSecretKey(void);

    int GenerateKey(void);
    int GenerateSecretKey(void);
    int GenerateCertificate(void);
    int GenerateCertificateRequest(void);

    int EncodeBase64(char *&output, int &outputLength, const unsigned char *input, int length);
    int DecodeBase64(unsigned char *&output, int &outputLength, const char *input, int length);

#ifdef NDEBUG
#define DumpToFile(filename, buffer, size)
#else
    void DumpToFile(const char *filename, const void *buffer, size_t size);
#endif

private: // SSL
    struct SSLConfig {
        int bits;
        int serial;
        int days;
        bool isCA;
        bool enableBase64;
        int secretKeyBits;
        std::string passphrase;
        std::string privateKey;
        std::string certificate; // X509.Certificate
        std::string alternativeName;
        std::string secretKey;
    } sslConfig;

    struct SSLContext {
        X509 *x509;
        EVP_PKEY *keypair;
        std::string secretKey;
    } sslContext;

private: // Asynchronous mode
    struct AsyncContext {
        beyond::EventLoop *eventLoop;
        beyond::EventLoop::HandlerObject *handlerObject;
        std::unique_ptr<beyond::CommandObject> outputProducer;
        std::unique_ptr<beyond::CommandObject> command;
    } asyncCtx;

private:
    EventObject *eventObject;
    std::unique_ptr<beyond::CommandObject> outputConsumer;
    std::unique_ptr<beyond::CommandObject> command;
    bool activated;
    beyond::AuthenticatorInterface *authenticator;
};

#endif // __BEYOND_DISCOVERY_AUTHENTICATOR_SSL_H__
