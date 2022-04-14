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

#include "authenticator.h"
#include "authenticator_event_object.h"

#include "beyond/plugin/authenticator_ssl_plugin.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <string>
#include <exception>
#include <memory>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/opensslconf.h>
#include <openssl/opensslv.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>

#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#include <json/json.h>

#define DEFAULT_MSG_BUFSZ 256
#define DEFAULT_PRIME 65537
#define DEFAULT_BIO_BUFSZ 4096
// NOTE:
// At least now, the secret key is used for GST pipeline encryption
// And for the gRPC session authentication.
// But the GST pipeline(srtpenc) element is limit its key size from 30 to 46 bytes.
// Let's keep the secret key length by the requirements
#define DEFAULT_SECRET_KEY_BITS 256
#define DEFAULT_BITS 4096
#define DEFAULT_SERIAL 1
#define DEFAULT_DAYS 365

#define DAYS_TO_SECONDS(days) ((days)*24 * 60 * 60)
#define BITS_TO_BYTES(bits) ((bits) >> 3)

#define SSLErrPrint(name)                                      \
    do {                                                       \
        char msg[DEFAULT_MSG_BUFSZ];                           \
        ERR_error_string_n(ERR_get_error(), msg, sizeof(msg)); \
        ErrPrint(name ": %s", msg);                            \
    } while (0)

Authenticator::CommandHandler Authenticator::commandTable[COMMAND::LAST] = {
    Authenticator::CommandGenerate,
    Authenticator::CommandCleanup,
    Authenticator::CommandCrypto,
    Authenticator::CommandGetKey,
    Authenticator::CommandGenerateSign,
    Authenticator::CommandVerifySign,
};

Authenticator *Authenticator::Create(bool enableAsync)
{
    Authenticator *impl;
    int pfd[2]; // output

    try {
        impl = new Authenticator();
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return nullptr;
    }

    // NOTE:
    // eventObject only be created if the async mode is enabled
    impl->eventObject = nullptr;
    impl->asyncCtx.handlerObject = nullptr;

    // NOTE:
    // pipe2 is not portable,
    // therefore, the pipe2 is replaced with a combination of the "pipe" and "fcntl"
    if (pipe(pfd) < 0) {
        ErrPrintCode(errno, "pipe");

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (fcntl(pfd[0], F_SETFD, FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");

        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        delete impl;
        impl = nullptr;
        return nullptr;
    }

    if (fcntl(pfd[1], F_SETFD, FD_CLOEXEC) < 0) {
        ErrPrintCode(errno, "fcntl");

        if (close(pfd[0]) < 0) {
            ErrPrintCode(errno, "close");
        }
        if (close(pfd[1]) < 0) {
            ErrPrintCode(errno, "close");
        }

        delete impl;
        impl = nullptr;
        return nullptr;
    }


    if (enableAsync == true) {
        impl->eventObject = Authenticator::EventObject::Create();
        if (impl->eventObject == nullptr) {
            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }
            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }

        impl->asyncCtx.eventLoop = beyond::EventLoop::Create(true, false);
        if (impl->asyncCtx.eventLoop == nullptr) {
            ErrPrint("Failed to create an event loop");

            impl->eventObject->Destroy();
            impl->eventObject = nullptr;

            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }
            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }

        impl->asyncCtx.eventLoop->SetStopHandler([](beyond::EventLoop *loop, void *data) -> void {
            Authenticator *impl = static_cast<Authenticator *>(data);
            impl->CommandCleanup(impl, nullptr);
            return;
        }, impl);

        int spfd[2]; // command
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, spfd) < 0) {
            ErrPrintCode(errno, "socketpair");

            impl->asyncCtx.eventLoop->Destroy();
            impl->asyncCtx.eventLoop = nullptr;

            impl->eventObject->Destroy();
            impl->eventObject = nullptr;

            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }

        if (fcntl(F_SETFD, spfd[0], FD_CLOEXEC) < 0) {
            ErrPrintCode(errno, "fcntl");

            if (close(spfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(spfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            impl->asyncCtx.eventLoop->Destroy();
            impl->asyncCtx.eventLoop = nullptr;

            impl->eventObject->Destroy();
            impl->eventObject = nullptr;

            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }

        if (fcntl(F_SETFD, spfd[1], FD_CLOEXEC) < 0) {
            ErrPrintCode(errno, "fcntl");

            if (close(spfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(spfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            impl->asyncCtx.eventLoop->Destroy();
            impl->asyncCtx.eventLoop = nullptr;

            impl->eventObject->Destroy();
            impl->eventObject = nullptr;

            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }

        impl->command = std::make_unique<beyond::CommandObject>(spfd[0]);
        impl->asyncCtx.command = std::make_unique<beyond::CommandObject>(spfd[1]);

        impl->asyncCtx.handlerObject = impl->asyncCtx.eventLoop->AddEventHandler(
            // NOTE:
            // get the unique_ptr's raw pointer
            // Does this break the unique_ptr's concept?
            static_cast<beyond::EventObjectBaseInterface *>(impl->asyncCtx.command.get()),
            beyond_event_type::BEYOND_EVENT_TYPE_READ | beyond_event_type::BEYOND_EVENT_TYPE_ERROR,
            [](beyond::EventObjectBaseInterface *eventObject, int type, void *data) -> beyond_handler_return {
                beyond::CommandObject *cmdObject = static_cast<beyond::CommandObject *>(eventObject);
                Authenticator *auth = static_cast<Authenticator *>(data);

                int cmdId = -1;
                void *_data = nullptr;

                int ret = cmdObject->Recv(cmdId, _data);
                if (ret < 0) {
                    ErrPrint("Failed to get the command");
                    return beyond_handler_return::BEYOND_HANDLER_RETURN_CANCEL;
                }

                if (cmdId >= 0 && cmdId < COMMAND::LAST) {
                    assert(auth->commandTable[cmdId] != nullptr && "Command handler is nullptr");
                    ret = auth->commandTable[cmdId](auth, _data);
                    if (ret < 0) {
                        DbgPrint("Command Handler returns: %d", ret);
                    }
                } else {
                    ErrPrint("Invalid command request");
                }

                return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
            },
            impl);

        if (impl->asyncCtx.handlerObject == nullptr) {
            ErrPrint("Failed to add the event handler");

            if (close(spfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }

            if (close(spfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            impl->asyncCtx.eventLoop->Destroy();
            impl->asyncCtx.eventLoop = nullptr;

            impl->eventObject->Destroy();
            impl->eventObject = nullptr;

            if (close(pfd[0]) < 0) {
                ErrPrintCode(errno, "close");
            }
            if (close(pfd[1]) < 0) {
                ErrPrintCode(errno, "close");
            }

            delete impl;
            impl = nullptr;
            return nullptr;
        }
    }

    impl->outputConsumer = std::make_unique<beyond::CommandObject>(pfd[0]);
    impl->asyncCtx.outputProducer = std::make_unique<beyond::CommandObject>(pfd[1]);
    return impl;
}

const char *Authenticator::GetModuleName(void) const
{
    return Authenticator::NAME;
}

const char *Authenticator::GetModuleType(void) const
{
    return ModuleInterface::TYPE_AUTHENTICATOR;
}

void Authenticator::Destroy(void)
{
    if (asyncCtx.eventLoop != nullptr) {
        asyncCtx.eventLoop->Stop();

        asyncCtx.eventLoop->Destroy();

        asyncCtx.eventLoop = nullptr;

        if (close(command->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }

        if (close(asyncCtx.command->GetHandle()) < 0) {
            ErrPrintCode(errno, "close");
        }
    }

    if (close(outputConsumer->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    if (close(asyncCtx.outputProducer->GetHandle()) < 0) {
        ErrPrintCode(errno, "close");
    }

    delete this;
}

int Authenticator::GetHandle(void) const
{
    return eventObject ? eventObject->GetHandle() : -ENOTSUP;
}

int Authenticator::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject ? eventObject->AddHandler(handler, type, data) : -ENOTSUP;
}

int Authenticator::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return eventObject ? eventObject->RemoveHandler(handler, type, data) : -ENOTSUP;
}

int Authenticator::FetchEventData(EventObjectInterface::EventData *&data)
{
    return eventObject ? eventObject->FetchEventData(data) : -ENOTSUP;
}

int Authenticator::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return eventObject ? eventObject->DestroyEventData(data) : -ENOTSUP;
}

int Authenticator::ConfigureAuthenticator(void *object)
{
    authenticator = static_cast<beyond::AuthenticatorInterface *>(object);
    beyond::ModuleInterface *module = dynamic_cast<beyond::ModuleInterface *>(authenticator);
    if (module == nullptr) {
        ErrPrint("Failed to cast to the module interface");
    } else {
        DbgPrint("Module name is %s", module->GetModuleName());
    }
    return 0;
}

int Authenticator::ConfigureSecretKey(void *object)
{
    beyond_authenticator_ssl_config_secret_key *secrets = static_cast<beyond_authenticator_ssl_config_secret_key *>(object);

    if (secrets->key != nullptr) {
        if (secrets->key_bits <= 0) {
            ErrPrint("Secret key bits must be provided");
            return -EINVAL;
        }

        sslConfig.secretKey = std::string(secrets->key, BITS_TO_BYTES(secrets->key_bits));
        sslConfig.secretKeyBits = secrets->key_bits;
    } else if (secrets->key_bits >= 0) {
        sslConfig.secretKeyBits = secrets->key_bits;
        sslConfig.secretKey.clear();
    } else {
        sslConfig.secretKey.clear();
        sslConfig.secretKeyBits = DEFAULT_SECRET_KEY_BITS;
    }

    if (sslConfig.secretKey.empty() == true) {
        return GenerateSecretKey();
    }

    return LoadSecretKey();
}

int Authenticator::ConfigureJSON(void *object)
{
    Json::Value root;
    std::string errors;
    Json::CharReaderBuilder builder;
    Json::CharReader *reader = builder.newCharReader();

    if (reader == nullptr) {
        ErrPrint("Unable to create a json reader");
        return -EFAULT;
    }

    const char *jsonStr = static_cast<char *>(object);
    bool jsonParseStatus = reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &errors);
    if (jsonParseStatus == false) {
        ErrPrint("Failed to parse the json string: %s", jsonStr);
        return -EINVAL;
    }

    if (errors.empty() == false) {
        ErrPrint("Failed to parse the JSON string: %s", errors.c_str());
        return -EFAULT;
    }

    /*
     * NOTE: Sample JSON configuration ("ssl")
     *
     * {
     *    "ssl": {
     *       "passphrase": "secret key string",
     *       "private_key": "BEGIN PRIVATE KEY ..... END PRIVATE KEY",
     *       "certificate": "BEGIN CERTIFICATE ..... END CERTIFICATE",
     *       "alternative_name": "127.0.0.1",
     *       "bits": -1,
     *       "serial": 1,
     *       "days": 3650,
     *       "is_ca": 1
     *    },
     *    "secret_key": {
     *       "key": "secret key string",
     *       "key_bits": 256
     *    }
     * }
     */
    const Json::Value &secretKey = root["secret_key"];
    if (secretKey.empty() == false) {
        beyond_authenticator_ssl_config_secret_key secretKeyOption;

        const Json::Value &keyValue = secretKey["key"];
        if (keyValue.empty() == false && keyValue.isString() == true) {
            secretKeyOption.key = keyValue.asCString();
        } else {
            secretKeyOption.key = nullptr;
        }

        const Json::Value &keyBitsValue = secretKey["key_bits"];
        if (keyBitsValue.empty() == false && keyBitsValue.isInt() == true) {
            secretKeyOption.key_bits = keyBitsValue.asInt();
        } else {
            secretKeyOption.key_bits = -1;
        }

        int ret = ConfigureSecretKey(&secretKeyOption);
        if (ret < 0) {
            ErrPrint("Failed to configure the secret key");
            return ret;
        }
    }

    const Json::Value &ssl = root["ssl"];
    if (ssl.empty() == false) {
        beyond_authenticator_ssl_config_ssl sslOption;
        const Json::Value &passphraseValue = ssl["passphrase"];
        if (passphraseValue.empty() == false && passphraseValue.isString() == true) {
            sslOption.passphrase = passphraseValue.asCString();
        } else {
            sslOption.passphrase = nullptr;
        }

        const Json::Value &privateKeyValue = ssl["private_key"];
        if (privateKeyValue.empty() == false && privateKeyValue.isString() == true) {
            sslOption.private_key = privateKeyValue.asCString();
        } else {
            sslOption.private_key = nullptr;
        }

        const Json::Value &certificateValue = ssl["certificate"];
        if (certificateValue.empty() == false && certificateValue.isString() == true) {
            sslOption.certificate = certificateValue.asCString();
        } else {
            sslOption.certificate = nullptr;
        }

        const Json::Value &alternativeNameValue = ssl["alternative_name"];
        if (alternativeNameValue.empty() == false && alternativeNameValue.isString() == true) {
            sslOption.alternative_name = alternativeNameValue.asCString();
        } else {
            sslOption.alternative_name = nullptr;
        }

        const Json::Value &bitsValue = ssl["bits"];
        if (bitsValue.empty() == false && bitsValue.isInt() == true) {
            sslOption.bits = bitsValue.asInt();
        } else {
            sslOption.bits = -1;
        }

        const Json::Value &daysValue = ssl["days"];
        if (daysValue.empty() == false && daysValue.isInt() == true) {
            sslOption.days = daysValue.asInt();
        } else {
            sslOption.days = -1;
        }

        const Json::Value &serialValue = ssl["serial"];
        if (serialValue.empty() == false && serialValue.isInt() == true) {
            sslOption.serial = serialValue.asInt();
        } else {
            sslOption.serial = -1;
        }

        const Json::Value &isCAValue = ssl["is_ca"];
        if (isCAValue.empty() == false && isCAValue.isInt() == true) {
            sslOption.isCA = isCAValue.asInt();
        } else {
            sslOption.isCA = -1;
        }

        const Json::Value &enableBase64Value = ssl["enable_base64"];
        if (enableBase64Value.empty() == false && enableBase64Value.isInt() == true) {
            sslOption.enableBase64 = enableBase64Value.asInt();
        } else {
            sslOption.enableBase64 = -1;
        }

        int ret = ConfigureSSL(&sslOption);
        if (ret < 0) {
            ErrPrint("Failed to configure the SSL");
            return ret;
        }
    }

    return 0;
}

int Authenticator::ConfigureSSL(void *object)
{
    beyond_authenticator_ssl_config_ssl *ssl = static_cast<beyond_authenticator_ssl_config_ssl *>(object);

    if (ssl->passphrase != nullptr) {
        sslConfig.passphrase = std::string(ssl->passphrase);
    } else {
        sslConfig.passphrase.clear();
    }

    if (ssl->private_key != nullptr) {
        sslConfig.privateKey = std::string(ssl->private_key);
    } else {
        sslConfig.privateKey.clear();
    }

    if (ssl->certificate != nullptr) {
        sslConfig.certificate = std::string(ssl->certificate);
    } else {
        sslConfig.certificate.clear();
    }

    if (ssl->alternative_name != nullptr) {
        sslConfig.alternativeName = std::string(ssl->alternative_name);
    } else {
        sslConfig.alternativeName.clear();
    }

    if (ssl->bits > 0) {
        sslConfig.bits = ssl->bits;
    } else if (ssl->bits == 0) {
        sslConfig.bits = DEFAULT_BITS;
    }

    if (ssl->days > 0) {
        sslConfig.days = ssl->days;
    } else if (ssl->days == 0) {
        sslConfig.days = DEFAULT_DAYS;
    }

    if (ssl->serial > 0) {
        sslConfig.serial = ssl->serial;
    } else if (ssl->serial == 0) {
        sslConfig.serial = DEFAULT_SERIAL;
    }

    if (ssl->isCA >= 0) {
        sslConfig.isCA = !!ssl->isCA;
    } else {
        sslConfig.isCA = true;
    }

    if (ssl->enableBase64 >= 0) {
        sslConfig.enableBase64 = !!ssl->enableBase64;
    } else {
        sslConfig.enableBase64 = true;
    }

    return 0;
}

int Authenticator::Configure(const beyond_config *options)
{
    if (options == nullptr) {
        return 0;
    }

    if (options->object == nullptr) {
        ErrPrint("Configure object is nullptr");
        return -EINVAL;
    }

    int ret = -EINVAL;
    switch (options->type) {
    case BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL:
        ret = ConfigureSSL(options->object);
        break;
    case BEYOND_CONFIG_TYPE_AUTHENTICATOR:
        ret = ConfigureAuthenticator(options->object);
        break;
    case BEYOND_CONFIG_TYPE_JSON:
        ret = ConfigureJSON(options->object);
        break;
    case BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SECRET_KEY:
        ret = ConfigureSecretKey(options->object);
        break;
    default:
        break;
    }

    return ret;
}

int Authenticator::CommandGenerate(Authenticator *inst, void *data)
{
    int ret;

    if (inst->sslConfig.privateKey.empty() == true && inst->sslConfig.certificate.empty() == true) {
        ret = inst->GenerateKey();
        if (ret == 0) {
            ret = inst->GenerateCertificate();
        }
    } else {
        ret = inst->LoadCertificate();
    }

    if (inst->sslContext.secretKey.empty() == true) {
        ret = inst->GenerateSecretKey();
    }

    if (inst->eventObject != nullptr) {
        if (ret == 0) {
            inst->eventObject->PublishEventData(BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);
        } else {
            inst->eventObject->PublishEventData(BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_ERROR);
        }
    }

    return ret;
}

int Authenticator::CommandCrypto(Authenticator *inst, void *data)
{
    CryptoInput *input = static_cast<CryptoInput *>(data);
    int ret = inst->Crypto(input->op, input->id, input->data, input->size, input->iv, input->ivsize);
    delete input;
    input = nullptr;

    if (inst->eventObject != nullptr) {
        if (ret == 0) {
            inst->eventObject->PublishEventData(BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);
        } else {
            inst->eventObject->PublishEventData(BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_ERROR);
        }
    }

    return ret;
}

int Authenticator::CommandGenerateSign(Authenticator *inst, void *data)
{
    GenerateSignData *signData = static_cast<GenerateSignData *>(data);
    unsigned char *signedData;
    int signedDataSize;

    signData->status = inst->GenerateSign(signData->data, signData->size, signedData, signedDataSize);
    if (signData->status == 0) {
        signData->data = signedData;
        signData->size = signedDataSize;
    }

    int ret = inst->asyncCtx.command->Send(COMMAND::GENERATE_SIGN, static_cast<void *>(signData));
    if (ret < 0) {
        ErrPrint("Failed to send a key info");
        free(signedData);
        signedData = nullptr;
        delete signData;
        signData = nullptr;
    }

    return ret;
}

int Authenticator::CommandVerifySign(Authenticator *inst, void *data)
{
    VerifySignData *signData = static_cast<VerifySignData *>(data);

    signData->status = inst->VerifySign(signData->signedData, signData->signedDataSize, signData->data, signData->size, signData->authentic);

    int ret = inst->asyncCtx.command->Send(COMMAND::VERIFY_SIGN, static_cast<void *>(signData));
    if (ret < 0) {
        ErrPrint("Failed to send a key info");
        delete signData;
        signData = nullptr;
    }

    return ret;
}

int Authenticator::CommandGetKey(Authenticator *inst, void *data)
{
    long _id = reinterpret_cast<long>(data);
    beyond_authenticator_key_id id = static_cast<beyond_authenticator_key_id>(_id);
    KeyInfo *info;
    int ret;

    try {
        info = new KeyInfo();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        return -ENOMEM;
    }

    info->key = nullptr;
    info->size = 0;
    info->status = 0;

    switch (id) {
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY:
        info->status = inst->GetPrivateKey(info->key, info->size);
        break;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY:
        info->status = inst->GetPublicKey(info->key, info->size);
        break;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE:
        info->status = inst->GetCertificate(info->key, info->size);
        break;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY:
        info->status = inst->GetSecretKey(info->key, info->size);
        break;
    default:
        info->status = -EINVAL;
        break;
    }

    ret = inst->asyncCtx.command->Send(COMMAND::GETKEY, info);
    if (ret < 0) {
        ErrPrint("Failed to send a key info");
        free(info->key);
        info->key = nullptr;
        delete info;
        info = nullptr;
    }

    return ret;
}

int Authenticator::Activate(void)
{
    int ret = 0;

    if (asyncCtx.eventLoop != nullptr) {
        ret = asyncCtx.eventLoop->Run(10, -1, -1);
        if (ret < 0) {
            ErrPrint("Failed to run the event loop");
        } else {
            DbgPrint("Event loop is running now");
        }
    } else {
        DbgPrint("Event loop is nullptr");
    }

    activated = (ret == 0);

    return ret;
}

int Authenticator::Prepare(void)
{
    if (activated == false) {
        ErrPrint("Not yet activated");
        return -EILSEQ;
    }

    int ret;

    if (asyncCtx.eventLoop != nullptr) {
        ret = command->Send(COMMAND::GENERATE);
        DbgPrint("Send a generate request: %d", ret);
    } else {
        ret = CommandGenerate(this, nullptr);
    }

    return ret;
}

int Authenticator::CommandCleanup(Authenticator *inst, void *data)
{
    EVP_PKEY_free(inst->sslContext.keypair);
    inst->sslContext.keypair = nullptr;

    X509_free(inst->sslContext.x509);
    inst->sslContext.x509 = nullptr;

    if (inst->eventObject != nullptr) {
        inst->eventObject->PublishEventData(BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);
    }

    return 0;
}

int Authenticator::Deactivate(void)
{
    if (activated == false) {
        ErrPrint("Not yet activated");
        return -EILSEQ;
    }

    int ret;

    if (asyncCtx.eventLoop != nullptr) {
        ret = asyncCtx.eventLoop->Stop();
    } else {
        ret = CommandCleanup(this, nullptr);
    }

    activated = !(ret == 0);
    return ret;
}

int Authenticator::CryptoAsymmetric(int opid, beyond_authenticator_key_id id, const void *in, size_t inlen, void *&out, size_t &outlen)
{
    int (*operation[])(EVP_PKEY_CTX * ctx, unsigned char *out, size_t *outlen, const unsigned char *in, size_t inlen) = {
        EVP_PKEY_encrypt,
        EVP_PKEY_decrypt,
    };

    int (*init[])(EVP_PKEY_CTX * ctx) = {
        EVP_PKEY_encrypt_init,
        EVP_PKEY_decrypt_init,
    };

    int ret;

    EVP_PKEY *key;
    EVP_PKEY *_key = nullptr;

    if (id == BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY) {
        if (sslContext.keypair == nullptr) {
            ErrPrint("Private key is not found");
            return -EINVAL;
        }

        key = sslContext.keypair;
    } else if (id == BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY) {
        if (sslContext.x509 == nullptr) {
            ErrPrint("x509 certificate is not found");
            return -EINVAL;
        }

        key = X509_get_pubkey(sslContext.x509);
        if (key == nullptr) {
            SSLErrPrint("X509_get_pubkey");
            return -EFAULT;
        }

        _key = key;
    } else {
        ErrPrint("Invalid key id");
        return -EINVAL;
    }

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(key, nullptr);
    if (ctx == nullptr) {
        SSLErrPrint("EVP_PKEY_CTX_new");
        EVP_PKEY_free(_key);
        _key = nullptr;
        return -EFAULT;
    }

    ret = 0;
    do {
        if ((init[opid])(ctx) <= 0) {
            SSLErrPrint("EVP_PKEY_encrypt_init");
            ret = -EFAULT;
            break;
        }

        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            SSLErrPrint("EVP_PKEY_CTX_set_rsa_padding");
            ret = -EFAULT;
            break;
        }

        ret = (operation[opid])(ctx, nullptr, &outlen, static_cast<const unsigned char *>(in), inlen);
        if (ret == -2) {
            ErrPrint("Not supported");
            ret = -ENOTSUP;
            break;
        } else if (ret <= 0) {
            SSLErrPrint("EVP_PKEY_encrypt or EVP_PKEY_decrypt");
            ret = -EFAULT;
            break;
        }

        ret = 0;

        out = calloc(1, outlen);
        if (out == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "calloc");
            break;
        }

        if ((operation[opid])(ctx, static_cast<unsigned char *>(out), &outlen, static_cast<const unsigned char *>(in), inlen) <= 0) {
            ret = -EFAULT;
            SSLErrPrint("EVP_PKEY_encrypt or EVP_PKEY_decrypt");
            free(out);
            out = nullptr;
            break;
        }
    } while (0);

    EVP_PKEY_CTX_free(ctx);
    ctx = nullptr;
    EVP_PKEY_free(_key);
    _key = nullptr;

    return ret;
}

int Authenticator::CryptoSymmetric(int opid, beyond_authenticator_key_id id, const void *in, size_t inlen, void *&out, size_t &outlen, const void *iv, size_t ivsize)
{
    int (*operation[])(EVP_CIPHER_CTX *, unsigned char *, int *, const unsigned char *, int) = {
        EVP_EncryptUpdate,
        EVP_DecryptUpdate,
    };

    int (*init[])(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *, const unsigned char *, const unsigned char *) = {
        EVP_EncryptInit_ex,
        EVP_DecryptInit_ex,
    };

    int (*finalize[])(EVP_CIPHER_CTX *, unsigned char *, int *) = {
        EVP_EncryptFinal_ex,
        EVP_DecryptFinal_ex,
    };

    int ret;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return -EFAULT;
    }

    ret = (init[opid])(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(sslContext.secretKey.c_str()), static_cast<const unsigned char *>(iv));
    if (ret != 1) {
        SSLErrPrint("init");
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return -EFAULT;
    }

    // Enable padding: default
    ret = EVP_CIPHER_CTX_set_padding(ctx, 1);
    if (ret != 1) {
        SSLErrPrint("set_padding");
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return -EFAULT;
    }

    size_t blockSize = EVP_CIPHER_block_size(EVP_aes_256_cbc());
    int paddingSize = inlen % blockSize;
    paddingSize = (paddingSize == 0) ? 0 : (blockSize - paddingSize);
    outlen = inlen + paddingSize;

    out = calloc(1, outlen);
    if (out == nullptr) {
        ErrPrintCode(errno, "calloc");
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return -ENOMEM;
    }

    int _outlen = outlen;
    ret = (operation[opid])(ctx, static_cast<unsigned char *>(out), &_outlen, static_cast<const unsigned char *>(in), inlen);
    if (ret != 1) {
        SSLErrPrint("operation");
        free(out);
        out = nullptr;
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return -EFAULT;
    }

    int tmplen = 0;
    ret = (finalize[opid])(ctx, static_cast<unsigned char *>(out) + _outlen, &tmplen);
    if (ret != 1) {
        SSLErrPrint("finalize");
        free(out);
        out = nullptr;
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return -EFAULT;
    }

    _outlen += tmplen;
    if (_outlen < static_cast<int>(outlen)) {
        void *_out = realloc(out, _outlen);
        if (_out == nullptr) {
            ErrPrintCode(errno, "realloc, go ahead");
        } else {
            out = _out;
        }

        outlen = _outlen;
    }

    EVP_CIPHER_CTX_free(ctx);
    ctx = nullptr;
    return 0;
}

// NOTE:
// If this function takes too much time,
// it is possible to move this to a new thread.
int Authenticator::Crypto(int opid, beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize)
{
    if (opid < 0 || opid >= 2) {
        assert((opid >= 0 && opid < 2) && "Invalid operation");
        ErrPrint("Invalid operation");
        return -EINVAL;
    }

    void *_in = nullptr;
    void *in;
    size_t inlen;
    void *out = nullptr;
    size_t outlen = 0;
    int ret = 0;

    if (opid == CRYPTO_OP::DECRYPT && sslConfig.enableBase64 == true) {
        int tmp;
        unsigned char *__in;
        ret = DecodeBase64(__in, tmp, static_cast<const char *>(data), size);
        if (ret < 0) {
            ErrPrint("Failed to decode base64");
            return ret;
        }
        inlen = static_cast<size_t>(tmp);
        in = __in;
        _in = __in;
    } else {
        in = const_cast<void *>(data);
        inlen = static_cast<size_t>(size);
    }

    switch (id) {
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE:
        id = beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY;
        FALLTHROUGH;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY:
        FALLTHROUGH;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY:
        ret = CryptoAsymmetric(opid, id, in, inlen, out, outlen);
        break;
    case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY:
        ret = CryptoSymmetric(opid, id, in, inlen, out, outlen, iv, ivsize);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    free(_in);
    _in = nullptr;

    CryptoOutput *output = nullptr;

    do {
        if (ret < 0) {
            break;
        }

        try {
            output = new CryptoOutput();
        } catch (std::exception &e) {
            ErrPrint("new failed: %s", e.what());
            free(out);
            out = nullptr;
            ret = -ENOMEM;
            break;
        }

        if (opid == CRYPTO_OP::ENCRYPT && sslConfig.enableBase64 == true) {
            char *_output;
            int _outputSize;
            ret = EncodeBase64(_output, _outputSize, static_cast<unsigned char *>(out), outlen);
            free(out);
            out = nullptr;
            if (ret < 0) {
                ErrPrint("EncodeBase64 returns error: %d", ret);
                delete output;
                output = nullptr;
                break;
            } else if (_output == nullptr || _outputSize <= 0) {
                ErrPrint("Invalid encoded data");
                delete output;
                output = nullptr;
                ret = -EINVAL;
                break;
            }
            output->output = _output;
            output->size = _outputSize;
            out = _output;
        } else {
            output->output = out;
            output->size = outlen;
        }
    } while (0);

    // TODO:
    // ret value should be used to send output result
    // the output should be data and status
    DbgPrint("return status: %d", ret);

    ret = asyncCtx.outputProducer->Send(OUTPUT::CRYPTO_OUTPUT, static_cast<void *>(output));
    if (ret < 0) {
        delete output;
        output = nullptr;
        free(out);
        out = nullptr;
    }

    return ret;
}

int Authenticator::Encrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize)
{
    if (activated == false) {
        ErrPrint("Not yet activated");
        return -EILSEQ;
    }

    if (id == beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY) {
        if (iv == nullptr || ivsize <= 0) {
            ErrPrint("Initial vector must be provided");
            return -EINVAL;
        }
    } else if (id == beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY) {
        ErrPrint("Invalid operation");
        return -EINVAL;
    }

    int ret;
    if (asyncCtx.eventLoop != nullptr) {
        CryptoInput *input;

        try {
            input = new CryptoInput();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            return -ENOMEM;
        }

        input->op = CRYPTO_OP::ENCRYPT;
        input->id = id;
        input->data = data;
        input->size = size;
        input->iv = iv;
        input->ivsize = ivsize;

        ret = command->Send(COMMAND::CRYPTO, input);
        if (ret < 0) {
            delete input;
            input = nullptr;
        }
    } else {
        ret = Crypto(CRYPTO_OP::ENCRYPT, id, data, size, iv, ivsize);
    }
    return ret;
}

int Authenticator::Decrypt(beyond_authenticator_key_id id, const void *data, int size, const void *iv, int ivsize)
{
    if (activated == false) {
        ErrPrint("Not yet activated");
        return -EILSEQ;
    }

    if (id == beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY) {
        if (iv == nullptr || ivsize <= 0) {
            ErrPrint("Initial vector must be provided");
            return -EINVAL;
        }
    } else if (id == beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY) {
        ErrPrint("Invalid operation");
        return -EINVAL;
    }

    int ret;
    if (asyncCtx.eventLoop != nullptr) {
        CryptoInput *input;

        try {
            input = new CryptoInput();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            return -ENOMEM;
        }

        input->op = CRYPTO_OP::DECRYPT;
        input->id = id;
        input->data = data;
        input->size = size;
        input->iv = iv;
        input->ivsize = ivsize;

        ret = command->Send(COMMAND::CRYPTO, input);
        if (ret < 0) {
            delete input;
            input = nullptr;
        }
    } else {
        ret = Crypto(CRYPTO_OP::DECRYPT, id, data, size, iv, ivsize);
    }

    return ret;
}

int Authenticator::GetResult(void *&data, int &size)
{
    if (activated == false) {
        ErrPrint("Not yet activated");
        return -EILSEQ;
    }

    int cmdId = -1;
    void *_data = nullptr;
    int ret = outputConsumer->Recv(cmdId, _data);
    if (cmdId != OUTPUT::CRYPTO_OUTPUT) {
        ErrPrint("data: %p", _data);
        return -EFAULT;
    }

    if (_data == nullptr) {
        ret = -EINVAL;
    } else {
        CryptoOutput *output = static_cast<CryptoOutput *>(_data);
        data = output->output;
        size = output->size;
        delete output;
        output = nullptr;
    }
    return ret;
}

int Authenticator::GetPrivateKey(void *&key, int &size)
{
    // TODO:
    // Validate to access member variable, if the enableAsync is true
    if (sslContext.keypair == nullptr) {
        ErrPrint("Private key is not prepared");
        return -EINVAL;
    }

    BIO *result = BIO_new(BIO_s_mem());
    if (result == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    // TODO:
    // If it is not activated,
    // return -EILSEQ;

    if (PEM_write_bio_PrivateKey(
            result,             // BIO to write private key to
            sslContext.keypair, // EVP_PKEY structure
            nullptr,            // default cipher for encrypting the key on disk
            nullptr,            // passphrase required for decrypting the key on disk
            0,                  // length of the passphrase string
            nullptr,            // callback for requesting a password
            nullptr             // data to pass to the callback
            ) != 1) {
        SSLErrPrint("PEM_write_bio_PrivateKey");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    char buffer[DEFAULT_BIO_BUFSZ] = { '\0' };
    int sz = BIO_read(result, static_cast<void *>(buffer), sizeof(buffer));
    if (sz <= 0) {
        SSLErrPrint("BIO_read");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    BIO_free(result);
    result = nullptr;

    void *_key = calloc(1, sz + 1);
    if (_key == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "calloc");
        return ret;
    }
    memcpy(_key, buffer, sz);

    key = _key;
    size = sz;
    DumpToFile("private.key", key, size);
    return 0;
}

int Authenticator::GetPublicKey(void *&key, int &size)
{
    // TODO:
    // Validate to access member variable, if the enableAsync is true
    if (sslContext.x509 == nullptr) {
        ErrPrint("Certificate is not prepared");
        return -EINVAL;
    }

    BIO *result = BIO_new(BIO_s_mem());
    if (result == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    // TODO:
    // If it is not activated,
    // return -EILSEQ;

    EVP_PKEY *pubkey = X509_get_pubkey(sslContext.x509);
    if (pubkey == nullptr) {
        SSLErrPrint("X509_get_pubkey");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    RSA *rsa = EVP_PKEY_get1_RSA(pubkey);
    if (rsa == nullptr) {
        SSLErrPrint("EVP_PKEY_get1_RSA");
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    if (PEM_write_bio_RSAPublicKey(result, rsa) != 1) {
        SSLErrPrint("PEM_write_bio_RSAPublicKey");
        RSA_free(rsa);
        rsa = nullptr;

        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    RSA_free(rsa);
    rsa = nullptr;

    EVP_PKEY_free(pubkey);
    pubkey = nullptr;

    char buffer[DEFAULT_BIO_BUFSZ] = { '\0' };
    int sz = BIO_read(result, static_cast<void *>(buffer), sizeof(buffer));
    if (sz <= 0) {
        SSLErrPrint("BIO_read");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    BIO_free(result);
    result = nullptr;

    void *_key = calloc(1, sz + 1);
    if (_key == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "calloc");
        return ret;
    }
    memcpy(_key, buffer, sz);

    key = _key;
    size = sz;
    DumpToFile("public.key", key, size);
    return 0;
}

int Authenticator::GetCertificate(void *&key, int &size)
{
    // TODO:
    // Validate to access member variable, if the enableAsync is true
    if (sslContext.x509 == nullptr) {
        ErrPrint("Certificate is not prepared");
        return -EINVAL;
    }

    BIO *result = BIO_new(BIO_s_mem());
    if (result == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    // TODO:
    // If it is not activated,
    // return -EILSEQ;

    if (PEM_write_bio_X509(result, sslContext.x509) != 1) {
        SSLErrPrint("PEM_write_bio_X509");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    char buffer[DEFAULT_BIO_BUFSZ] = { '\0' };
    int sz = BIO_read(result, static_cast<void *>(buffer), sizeof(buffer));
    if (sz <= 0) {
        SSLErrPrint("BIO_read");
        BIO_free(result);
        result = nullptr;
        return -EFAULT;
    }

    BIO_free(result);
    result = nullptr;

    void *_key = calloc(1, sz + 1);
    if (_key == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "calloc");
        return ret;
    }
    memcpy(_key, buffer, sz);

    key = _key;
    size = sz;
    DumpToFile("certificate.key", key, size);
    return 0;
}

int Authenticator::GetSecretKey(void *&key, int &size)
{
    if (sslContext.secretKey.empty() == true) {
        ErrPrint("SecretKey is not prepared");
        return -EINVAL;
    }

    char *output = nullptr;
    int outputSize = 0;
    if (sslConfig.enableBase64 == true) {
        int ret = EncodeBase64(output, outputSize, reinterpret_cast<const unsigned char *>(sslContext.secretKey.c_str()), sslContext.secretKey.size());
        if (ret < 0) {
            return ret;
        } else if (output == nullptr || outputSize <= 0) {
            return -EINVAL;
        }
    } else {
        outputSize = sslContext.secretKey.size();
        output = static_cast<char *>(calloc(1, outputSize));
        if (output == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "calloc");
            return ret;
        }

        memcpy(output, sslContext.secretKey.c_str(), outputSize);
    }

    key = output;
    size = outputSize;
    DumpToFile("secret.key", key, size);
    return 0;
}

int Authenticator::GetKey(beyond_authenticator_key_id id, void *&key, int &size)
{
    int ret;

    if (asyncCtx.eventLoop != nullptr) {
        ret = command->Send(COMMAND::GETKEY, reinterpret_cast<void *>(id));
        if (ret < 0) {
            ErrPrint("Failed to send a command");
        } else {
            int cmdId = -1;
            void *data = nullptr;

            ret = command->Recv(cmdId, data);
            if (ret < 0 || cmdId < 0 || data == nullptr) {
                ErrPrint("Failed to receive");
            } else {
                KeyInfo *info = static_cast<KeyInfo *>(data);
                key = info->key;
                size = info->size;
                ret = info->status;
                delete info;
                info = nullptr;
            }
        }
    } else {
        switch (id) {
        case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY:
            return GetPrivateKey(key, size);
        case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY:
            return GetPublicKey(key, size);
        case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE:
            return GetCertificate(key, size);
        case beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY:
            return GetSecretKey(key, size);
            break;
        default:
            ret = -EINVAL;
            break;
        }
    }

    return ret;
}

int Authenticator::GenerateSignature(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize)
{
    if (asyncCtx.eventLoop != nullptr) {
        GenerateSignData *_data;

        try {
            _data = new GenerateSignData();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            return -ENOMEM;
        }

        _data->data = const_cast<unsigned char *>(data);
        _data->size = dataSize;
        _data->status = 0;

        int ret = command->Send(COMMAND::GENERATE_SIGN, reinterpret_cast<void *>(_data));
        if (ret < 0) {
            ErrPrint("Failed to send a command");
            delete _data;
            _data = nullptr;
        } else {
            int cmdId = -1;
            void *recvData = nullptr;

            ret = command->Recv(cmdId, recvData);
            if (ret < 0) {
                ErrPrint("Failed to receive");
            } else if (cmdId < 0 || recvData == nullptr) {
                ErrPrint("Invalid command and data");
                ret = -EINVAL;
            } else {
                _data = static_cast<GenerateSignData *>(recvData);
                ret = _data->status;
                if (ret == 0) {
                    encoded = _data->data;
                    encodedSize = _data->size;
                }
                delete _data;
                _data = nullptr;
            }
        }

        return ret;
    }

    return GenerateSign(data, dataSize, encoded, encodedSize);
}

int Authenticator::VerifySignature(unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic)
{
    if (asyncCtx.eventLoop != nullptr) {
        VerifySignData *data;

        try {
            data = new VerifySignData();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            return -ENOMEM;
        }

        data->data = original;
        data->size = originalSize;
        data->signedData = signedData;
        data->signedDataSize = signedDataSize;
        data->authentic = false;
        data->status = 0;

        int ret = command->Send(COMMAND::VERIFY_SIGN, reinterpret_cast<void *>(data));
        if (ret < 0) {
            ErrPrint("Failed to send a command");
            delete data;
            data = nullptr;
        } else {
            int cmdId = -1;
            void *_data = nullptr;

            ret = command->Recv(cmdId, _data);
            if (ret < 0) {
                ErrPrint("Failed to receive");
            } else if (cmdId < 0 || data == nullptr) {
                ErrPrint("Invalid cmdId or cmdData");
                ret = -EINVAL;
            } else {
                data = static_cast<VerifySignData *>(_data);
                ret = data->status;
                if (ret == 0) {
                    authentic = data->authentic;
                }
                delete data;
                data = nullptr;
            }
        }

        return ret;
    }

    return VerifySign(signedData, signedDataSize, original, originalSize, authentic);
}

int Authenticator::GenerateSign(const unsigned char *data, int dataSize, unsigned char *&encoded, int &encodedSize)
{
    if (sslContext.keypair == nullptr) {
        ErrPrint("keypair is not prepared");
        return -EINVAL;
    }

    RSA *rsa = EVP_PKEY_get1_RSA(sslContext.keypair);
    if (rsa == nullptr) {
        SSLErrPrint("EVP_PKEY_get1_RSA");
        return -EFAULT;
    }

    EVP_PKEY *pKey = EVP_PKEY_new();
    if (pKey == nullptr) {
        SSLErrPrint("EVP_PKEY_new");
        RSA_free(rsa);
        rsa = nullptr;
        return -EFAULT;
    }

    if (EVP_PKEY_assign_RSA(pKey, rsa) != 1) {
        SSLErrPrint("EVP_PKEY_assign_RSA");
        RSA_free(rsa);
        rsa = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }
    rsa = nullptr;

    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    if (ctx == NULL) {
        SSLErrPrint("EVP_MD_CTX_create");
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    const EVP_MD *md = EVP_get_digestbyname("SHA256");
    if (md == NULL) {
        SSLErrPrint("EVP_get_digestbyname");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    if (EVP_DigestSignInit(ctx, NULL, md, NULL, pKey) <= 0) {
        SSLErrPrint("EVP_DigestSignInit");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    if (EVP_DigestSignUpdate(ctx, data, dataSize) <= 0) {
        SSLErrPrint("EVP_DigestSignUpdate");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    if (EVP_DigestSignFinal(ctx, NULL, (size_t *)&encodedSize) <= 0) {
        SSLErrPrint("EVP_DigestSignFinal");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    unsigned char *_encoded;
    _encoded = static_cast<unsigned char *>(calloc(1, encodedSize));
    if (_encoded == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "calloc");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return ret;
    }

    if (EVP_DigestSignFinal(ctx, _encoded, (size_t *)&encodedSize) <= 0) {
        SSLErrPrint("EVP_DigestSignFinal");
        free(_encoded);
        _encoded = nullptr;
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pKey);
        pKey = nullptr;
        return -EFAULT;
    }

    encoded = _encoded;

    EVP_MD_CTX_destroy(ctx);
    ctx = nullptr;
    EVP_PKEY_free(pKey);
    pKey = nullptr;
    return 0;
}

int Authenticator::VerifySign(const unsigned char *signedData, int signedDataSize, const unsigned char *original, int originalSize, bool &authentic)
{
    // TODO:
    // If it is not activated,
    // return -EILSEQ;
    if (sslContext.x509 == nullptr) {
        ErrPrint("Certificate is not prepared");
        return -EINVAL;
    }

    EVP_PKEY *pubkey = X509_get_pubkey(sslContext.x509);
    if (pubkey == nullptr) {
        SSLErrPrint("X509_get_pubkey");
        return -EFAULT;
    }

    RSA *rsa = EVP_PKEY_get1_RSA(pubkey);
    if (rsa == nullptr) {
        SSLErrPrint("EVP_PKEY_get1_RSA");
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    EVP_PKEY *pKey = EVP_PKEY_new();
    if (pKey == nullptr) {
        SSLErrPrint("EVP_PKEY_new");
        RSA_free(rsa);
        rsa = nullptr;

        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    if (EVP_PKEY_assign_RSA(pKey, rsa) != 1) {
        SSLErrPrint("EVP_PKEY_assign_RSA");
        RSA_free(rsa);
        rsa = nullptr;

        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }
    rsa = nullptr;

    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    if (ctx == NULL) {
        SSLErrPrint("EVP_MD_CTX_create");
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    const EVP_MD *md = EVP_get_digestbyname("SHA256");
    if (md == NULL) {
        SSLErrPrint("EVP_get_digestbyname");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    if (EVP_DigestVerifyInit(ctx, NULL, md, NULL, pKey) <= 0) {
        SSLErrPrint("EVP_DigestVerifyInit");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    if (EVP_DigestVerifyUpdate(ctx, original, originalSize) <= 0) {
        SSLErrPrint("EVP_DigestVerifyUpdate");
        EVP_MD_CTX_destroy(ctx);
        ctx = nullptr;
        EVP_PKEY_free(pubkey);
        pubkey = nullptr;
        return -EFAULT;
    }

    int ret = EVP_DigestVerifyFinal(ctx, signedData, signedDataSize);
    EVP_MD_CTX_destroy(ctx);
    ctx = nullptr;
    EVP_PKEY_free(pubkey);
    pubkey = nullptr;

    authentic = (ret == 1);

    return (ret >= 0) ? 0 : -EFAULT;
}

Authenticator::Authenticator(void)
    : sslConfig{
        .bits = DEFAULT_BITS,
        .serial = DEFAULT_SERIAL,
        .days = DEFAULT_DAYS,
        .isCA = true,
        .enableBase64 = true,
        .secretKeyBits = DEFAULT_SECRET_KEY_BITS,
    }
    , sslContext{
        .x509 = nullptr,
        .keypair = nullptr,
    }
    , asyncCtx{
        .eventLoop = nullptr,
        .handlerObject = nullptr,
    }
    , eventObject(nullptr)
    , activated(false)
    , authenticator(nullptr)
{
}

Authenticator::~Authenticator(void)
{
}

/* Add extension using V3 code: we can set the config file as NULL
 * because we wont reference any other sections.
 */
int Authenticator::AddExtension(X509 *cert, int nid, char *value, X509 *issuer)
{
    X509_EXTENSION *ex;
    X509V3_CTX ctx;

    if (issuer == nullptr) {
        issuer = cert;
    }

    /* This sets the 'context' of the extensions. */
    /* No configuration database */
    X509V3_set_ctx_nodb(&ctx);

    /* Issuer and subject certs: both the target since it is self signed,
	 * no request and no CRL
	 */
    X509V3_set_ctx(&ctx, issuer, cert, NULL, NULL, 0);
    ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
    if (ex == nullptr) {
        ErrPrint("Failed to call X509V3_EXT_conf_nid (%s)", value);
        return -EFAULT;
    }

    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
    ex = nullptr;
    return 0;
}

int Authenticator::LoadPrivateKey(const char *keyString, int keyLength, EVP_PKEY *&keypair)
{
    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    if (BIO_write(bio, keyString, keyLength) <= 0) {
        SSLErrPrint("BIO_write");
        BIO_free(bio);
        bio = nullptr;
        return -EFAULT;
    }

    EVP_PKEY *_keypair = PEM_read_bio_PrivateKey(bio, nullptr, 0, 0);
    if (_keypair == nullptr) {
        SSLErrPrint("BIO_read_bio_PrivateKey");
        BIO_free(bio);
        bio = nullptr;
        return -EFAULT;
    }

    BIO_free(bio);
    bio = nullptr;
    keypair = _keypair;
    return 0;
}

int Authenticator::LoadSecretKey(void)
{
    if (sslConfig.enableBase64 == true) {
        unsigned char *output = nullptr;
        int outputLength = 0;

        int ret = DecodeBase64(output, outputLength, sslConfig.secretKey.c_str(), BITS_TO_BYTES(sslConfig.secretKeyBits));
        if (ret < 0) {
            ErrPrint("Invalid encoding");
            return ret;
        }

        sslContext.secretKey = std::string(reinterpret_cast<char *>(output), outputLength);
        free(output);
    } else {
        sslContext.secretKey = sslConfig.secretKey;
    }

    return 0;
}

int Authenticator::LoadCertificate(const char *certificateString, int certificateStringLength, X509 *&x509)
{
    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    if (BIO_write(bio, certificateString, certificateStringLength) <= 0) {
        SSLErrPrint("BIO_write");
        BIO_free(bio);
        bio = nullptr;
        return -EFAULT;
    }

    X509 *_x509 = PEM_read_bio_X509(bio, nullptr, 0, 0);
    if (_x509 == nullptr) {
        SSLErrPrint("BIO_read_bio_PrivateKey");
        BIO_free(bio);
        bio = nullptr;
        return -EFAULT;
    }

    BIO_free(bio);
    bio = nullptr;
    x509 = _x509;
    return 0;
}

int Authenticator::LoadCertificate(void)
{
    int ret = -ENOENT;

    if (sslConfig.privateKey.empty() == false) {
        EVP_PKEY *pkey = nullptr;
        ret = LoadPrivateKey(sslConfig.privateKey.c_str(), sslConfig.privateKey.size(), pkey);
        if (ret < 0) {
            return ret;
        } else if (pkey == nullptr) {
            ErrPrint("Unable to load a private key");
            return -EFAULT;
        }

        DbgPrint("Private key is loaded");
        EVP_PKEY_free(sslContext.keypair);
        sslContext.keypair = pkey;
    }

    if (sslConfig.certificate.empty() == false) {
        X509 *x509 = nullptr;
        ret = LoadCertificate(sslConfig.certificate.c_str(), sslConfig.certificate.size(), x509);
        if (ret < 0) {
            return ret;
        }

        DbgPrint("Certificate key is loaded");
        X509_free(sslContext.x509);
        sslContext.x509 = x509;
    }

    return ret;
}

int Authenticator::GenerateSecretKey(void)
{
    int keyLength = BITS_TO_BYTES(sslConfig.secretKeyBits);
    unsigned char *buffer;

    buffer = static_cast<unsigned char *>(malloc(keyLength));
    if (buffer == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    // For a secret key to generate use SSL
    if (!RAND_bytes(buffer, keyLength)) {
        SSLErrPrint("RAND Failed");
        return -EFAULT;
    }

    sslContext.secretKey = std::string(reinterpret_cast<char *>(buffer), keyLength);
    free(buffer);
    return 0;
}

int Authenticator::GenerateKey(void)
{
    int bits = sslConfig.bits;

    RSA *rsa = nullptr;
    int ret = 0;
    EVP_PKEY *keypair = nullptr;

#ifndef NDEBUG
#ifndef OPENSSL_NO_CRYPTO_MDEBUG
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#ifndef OPENSSL_NO_STDIO
    FILE *bio_err = stderr;
#else  // OPENSSL_NO_STDIO
    BIO *bio_err = BIO_new(BIO_s_mem());
    if (bio_err == nullptr) {
        ErrPrint("Failed to create a bio");
        return -EFAULT;
    }
#endif // OPENSSL_NO_STDIO
#endif // OPENSSL_NO_CRYPTO_MDEBUG
#endif // NDEBUG

    do {
        keypair = EVP_PKEY_new();
        if (keypair == nullptr) {
            SSLErrPrint("EVP_PKEY_new");
            ret = -EFAULT;
            break;
        }

        rsa = RSA_new();
        if (rsa == nullptr) {
            ret = -EFAULT;
            break;
        }

        BIGNUM *bn = BN_new();
        if (bn == nullptr) {
            ret = -EFAULT;
            break;
        }

        BN_set_word(bn, DEFAULT_PRIME);

        if (RSA_generate_key_ex(rsa, bits, bn, nullptr) == 0) {
            SSLErrPrint("RSA_generate_key_ex");

            BN_clear_free(bn);
            bn = nullptr;

            ret = -EFAULT;
            break;
        }

        BN_clear_free(bn);
        bn = nullptr;

        if (EVP_PKEY_assign_RSA(keypair, rsa) == 0) {
            SSLErrPrint("EVP_PKEY_assign_RSA");
            ret = -EFAULT;
            break;
        }

        rsa = nullptr;
    } while (0);

    if (rsa != nullptr) {
        RSA_free(rsa);
        rsa = nullptr;
    }

    if (ret < 0) {
        ErrPrint("Clean up keypair");
        EVP_PKEY_free(keypair);
        keypair = nullptr;
    } else {
        EVP_PKEY_free(sslContext.keypair);
        sslContext.keypair = keypair;
    }

#ifndef OPENSSL_NO_ENGINE
    ENGINE_cleanup();
#endif

    CRYPTO_cleanup_all_ex_data();

#ifndef NDEBUG

#ifndef OPENSSL_NO_CRYPTO_MDEBUG
#ifndef OPENSSL_NO_STDIO
    CRYPTO_mem_leaks_fp(bio_err);
#else  // OPENSSL_NO_STDIO
    CRYPTO_mem_leaks(bio_err);

    char buffer[DEFAULT_BIO_BUFSZ];
    int sz = BIO_read(bio_err, buffer, sizeof(buffer));
    if (sz > 0) {
        ErrPrint("CRYPTO_mem_leaks: %s", buffer);
    } else {
        SSLErrPrint("BIO_read");
    }

    BIO_free(bio_err);
#endif // OPENSSL_NO_STDIO
    bio_err = nullptr;
#endif // OPENSSL_NO_CRYPTO_MDEBUG
#endif // NDEBUG

    return ret;
}

int Authenticator::GenerateCertificate(void)
{
    int serial = sslConfig.serial;
    int days = sslConfig.days;
    int ret = 0;

#ifndef NDEBUG
#ifndef OPENSSL_NO_CRYPTO_MDEBUG
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#ifndef OPENSSL_NO_STDIO
    FILE *bio_err = stderr;
#else  // OPENSSL_NO_STDIO
    BIO *bio_err = BIO_new(BIO_s_mem());
    if (bio_err == nullptr) {
        ErrPrint("Failed to create a bio");
        return -EFAULT;
    }
#endif // OPENSSL_NO_STDIO
#endif // OPENSSL_NO_CRYPTO_MDEBUG
#endif // NDEBUG

    X509 *x509;
    do {
        x509 = X509_new();
        if (x509 == nullptr) {
            SSLErrPrint("X509_new");
            ret = -EFAULT;
            break;
        }

        X509_set_version(x509, 2);
        X509_set_pubkey(x509, sslContext.keypair);
        ASN1_INTEGER_set(X509_get_serialNumber(x509), serial);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), static_cast<long>(DAYS_TO_SECONDS(days)));

        X509_NAME *name = X509_get_subject_name(x509);
        if (name == nullptr) {
            SSLErrPrint("X509_get_subject_name");
            ret = -EFAULT;
            break;
        }

        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("KR"), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("BeyonD"), -1, -1, 0);
        if (sslConfig.isCA == true) {
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("beyond.net"), -1, -1, 0);
            AddExtension(x509, NID_basic_constraints, const_cast<char *>("critical,CA:TRUE,pathlen:0"));
            AddExtension(x509, NID_key_usage, const_cast<char *>("critical,keyCertSign,cRLSign,keyEncipherment"));
            AddExtension(x509, NID_netscape_cert_type, const_cast<char *>("sslCA,emailCA,objCA"));
        } else {
            X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Inference"), -1, -1, 0);
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("edge.beyond.net"), -1, -1, 0);
            AddExtension(x509, NID_basic_constraints, const_cast<char *>("critical,CA:FALSE"));
            AddExtension(x509, NID_key_usage, const_cast<char *>("critical,keyCertSign,cRLSign,keyEncipherment,nonRepudiation,digitalSignature"));
            AddExtension(x509, NID_ext_key_usage, const_cast<char *>("serverAuth,clientAuth"));
        }

        if (sslConfig.alternativeName.empty() == false) {
            DbgPrint("Add entry of subjectAltName for the IP Address based authentication (%s)", sslConfig.alternativeName.c_str());
            std::string altName = std::string("IP:") + sslConfig.alternativeName;
            AddExtension(x509, NID_subject_alt_name, const_cast<char *>(altName.c_str()));
        }

        X509_set_subject_name(x509, name);

        AddExtension(x509, NID_subject_key_identifier, const_cast<char *>("hash"));

        if (authenticator == nullptr) {
            X509_set_issuer_name(x509, name);
            if (X509_sign(x509, sslContext.keypair, EVP_sha256()) == 0) {
                SSLErrPrint("EVP_PKEY_assign_RSA");
                ret = -EFAULT;
                break;
            }
        } else {
            void *key = nullptr;
            int keyLength = 0;

            ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, key, keyLength);
            if (ret < 0) {
                ErrPrint("Failed to get the private key from the authenticator");
                break;
            }

            EVP_PKEY *pkey_ca = nullptr;
            ret = LoadPrivateKey(static_cast<char *>(key), keyLength, pkey_ca);
            if (ret < 0) {
                ErrPrint("Failed to load the private key");
                break;
            } else if (pkey_ca == nullptr) {
                ErrPrint("Private key is invalid");
                ret = -EINVAL;
                break;
            }

            free(key);
            key = nullptr;
            keyLength = 0;

            ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, key, keyLength);
            if (ret < 0) {
                ErrPrint("Failed to get the certificate from the authenticator");
                EVP_PKEY_free(pkey_ca);
                pkey_ca = nullptr;
                break;
            }

            X509 *x509_ca = nullptr;
            ret = LoadCertificate(static_cast<char *>(key), keyLength, x509_ca);
            free(key);
            key = nullptr;
            keyLength = 0;
            if (ret < 0) {
                ErrPrint("Failed to load the certificate");
                EVP_PKEY_free(pkey_ca);
                pkey_ca = nullptr;
                break;
            } else if (x509_ca == nullptr) {
                ErrPrint("Certiicate is invalid");
                EVP_PKEY_free(pkey_ca);
                pkey_ca = nullptr;
                ret = -EINVAL;
                break;
            }

            X509_set_issuer_name(x509, X509_get_subject_name(x509_ca));
            AddExtension(x509, NID_authority_key_identifier, const_cast<char *>("keyid:always,issuer:always"), x509_ca);

            X509_free(x509_ca);
            x509_ca = nullptr;

            ret = X509_sign(x509, pkey_ca, EVP_sha256());
            EVP_PKEY_free(pkey_ca);
            pkey_ca = nullptr;
            if (ret == 0) {
                SSLErrPrint("X509_sign");
                ret = -EFAULT;
                break;
            }

            ret = 0;
        }
    } while (0);

    if (ret < 0) {
        ErrPrint("Clean up x509");
        X509_free(x509);
        x509 = nullptr;
    } else {
        X509_free(sslContext.x509);
        sslContext.x509 = x509;
    }

#ifndef OPENSSL_NO_ENGINE
    ENGINE_cleanup();
#endif

    CRYPTO_cleanup_all_ex_data();

#ifndef NDEBUG

#ifndef OPENSSL_NO_CRYPTO_MDEBUG
#ifndef OPENSSL_NO_STDIO
    CRYPTO_mem_leaks_fp(bio_err);
#else  // OPENSSL_NO_STDIO
    CRYPTO_mem_leaks(bio_err);

    char buffer[DEFAULT_BIO_BUFSZ];
    int sz = BIO_read(bio_err, buffer, sizeof(buffer));
    if (sz > 0) {
        ErrPrint("CRYPTO_mem_leaks: %s", buffer);
    } else {
        SSLErrPrint("BIO_read");
    }

    BIO_free(bio_err);
#endif // OPENSSL_NO_STDIO
    bio_err = nullptr;
#endif // OPENSSL_NO_CRYPTO_MDEBUG
#endif // NDEBUG

    return ret;
}

int Authenticator::EncodeBase64(char *&output, int &outputLength, const unsigned char *input, int length)
{
    BIO *b64f = BIO_new(BIO_f_base64());
    if (b64f == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    BIO *buff = BIO_new(BIO_s_mem());
    if (buff == nullptr) {
        SSLErrPrint("BIO_new");
        BIO_free_all(b64f);
        return -EFAULT;
    }

    buff = BIO_push(b64f, buff);
    assert(buff != nullptr && !!"BIO_push() failed");

    BIO_set_flags(buff, BIO_FLAGS_BASE64_NO_NL);
    int ret = BIO_set_close(buff, BIO_CLOSE);
    assert(ret == 1 && !!"BIO_set_close() returns unpredicted value");

    ret = BIO_write(buff, input, length);
    if (ret == -2) {
        ErrPrint("Method is not implemented");
        BIO_free_all(buff);
        return -ENOTSUP;
    } else if (ret <= 0) {
        // NOTE:
        // Nomally, this not an error.
        // However, we do not have any case to get into this situation.
        // So I just deal it as an error case.
        ErrPrint("No data was written");
        BIO_free_all(buff);
        return -EFAULT;
    }

    ret = BIO_flush(buff);
    if (ret <= 0) {
        ErrPrint("Failed to do flush");
        BIO_free_all(buff);
        return -EFAULT;
    }

    BUF_MEM *ptr = nullptr;
    BIO_get_mem_ptr(buff, &ptr);
    if (ptr == nullptr) {
        ErrPrint("Failed to get BUF_MEM");
        BIO_free_all(buff);
        return -EFAULT;
    } else if (ptr->length <= 0 || ptr->data == nullptr) {
        ErrPrint("Invalid buffer gotten");
        BIO_free_all(buff);
        return -EINVAL;
    }

    char *_output = static_cast<char *>(calloc(1, ptr->length + 1));
    if (_output == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "calloc");
        BIO_free_all(buff);
        return ret;
    }
    memcpy(_output, ptr->data, ptr->length);

    output = _output;
    outputLength = ptr->length;
    BIO_free_all(buff);
    return 0;
}

int Authenticator::DecodeBase64(unsigned char *&output, int &outputLength, const char *input, int length)
{
    BIO *b64f = BIO_new(BIO_f_base64());
    if (b64f == nullptr) {
        SSLErrPrint("BIO_new");
        return -EFAULT;
    }

    BIO *buff = BIO_new_mem_buf(static_cast<const void *>(input), length);
    if (buff == nullptr) {
        SSLErrPrint("BIO_new_mem_buf");
        BIO_free(b64f);
        b64f = nullptr;
        return -EFAULT;
    }

    buff = BIO_push(b64f, buff);
    if (buff == nullptr) {
        assert(!"BIO_push() failed");
        BIO_free(b64f);
        b64f = nullptr;
        BIO_free(buff);
        buff = nullptr;
        return -EFAULT;
    }

    BIO_set_flags(buff, BIO_FLAGS_BASE64_NO_NL);
    int ret = BIO_set_close(buff, BIO_CLOSE);
    if (ret != 1) {
        assert(!"BIO_set_close() returns unexpected value");
        ErrPrint("BIO_set_close() returns unexpected value");
        // NOTE:
        // Go ahead in case of the release mode.
    }

    // NOTE:
    // We will allocate enough memory for decoding encoded data
    // But we can sure that the decoded data must smaller than encoded data
    // Therefore, we are going to allocate input size buffer first
    // and then reallocate it with its decoded size
    int _outputLength = length;
    unsigned char *_output = static_cast<unsigned char *>(calloc(1, _outputLength));
    if (_output == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "calloc");
        BIO_free_all(buff);
        return ret;
    }

    _outputLength = BIO_read(buff, _output, _outputLength);
    BIO_free_all(buff);
    if (_outputLength == -2) {
        ErrPrint("Method is not implemented");
        free(_output);
        _output = nullptr;
        return -ENOTSUP;
    } else if (_outputLength <= 0) {
        SSLErrPrint("BIO_read");
        free(_output);
        _output = nullptr;
        return -EFAULT;
    }

    // NOTE:
    // Decoded data is not a string,
    // it does not necessary to be ended with '\0'
    void *__output = static_cast<void *>(realloc(static_cast<void *>(_output), _outputLength));
    if (__output == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "realloc");
        free(static_cast<void *>(_output));
        _output = nullptr;
        return ret;
    }

    output = static_cast<unsigned char *>(__output);
    outputLength = _outputLength;
    return 0;
}
