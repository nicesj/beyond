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

#include <cstring>
#include <cctype>
#include <gtest/gtest.h>
#include <dlfcn.h>
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/authenticator_ssl_plugin.h"

#define MODULE_FILENAME "libbeyond-" BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME ".so"

#include "authenticator.h"

static beyond_authenticator_ssl_config_ssl sslConfig = {
    .bits = 4096,
    .serial = 1,
    .days = 365,
    .isCA = 1,
    .enableBase64 = -1,
    .passphrase = "",
    .private_key =
        "-----BEGIN PRIVATE KEY-----\n"
        "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\n"
        "BsoK09goBWvBI+6ZiRjYKiWIfdfNiA==\n"
        "-----END PRIVATE KEY-----",
    .certificate =
        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "-----END CERTIFICATE-----",
    .alternative_name = nullptr,
};

static beyond_authenticator_ssl_config_ssl sslConfigDisableBase64 = {
    .bits = 4096,
    .serial = 1,
    .days = 365,
    .isCA = 1,
    .enableBase64 = 0,
    .passphrase = "",
    .private_key =
        "-----BEGIN PRIVATE KEY-----\n"
        "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\n"
        "BsoK09goBWvBI+6ZiRjYKiWIfdfNiA==\n"
        "-----END PRIVATE KEY-----",
    .certificate =
        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "-----END CERTIFICATE-----",
    .alternative_name = nullptr,
};

static beyond_authenticator_ssl_config_ssl sslConfigNoCert = {
    .bits = 4096,
    .serial = 1,
    .days = 365,
    .isCA = 1,
    .enableBase64 = -1,
    .passphrase = "",
    .private_key = nullptr,
    .certificate = nullptr,
    .alternative_name = "127.0.0.1",
};

static beyond_authenticator_ssl_config_ssl sslConfigNoCertEE = {
    .bits = 4096,
    .serial = 2,
    .days = 365,
    .isCA = 0,
    .enableBase64 = -1,
    .passphrase = "",
    .private_key = nullptr,
    .certificate = nullptr,
    .alternative_name = "127.0.0.1",
};

class AuthenticatorTest : public testing::Test {
protected:
    void SetUp() override
    {
        handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
        ASSERT_NE(handle, nullptr);

        entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(handle, beyond::ModuleInterface::EntryPointSymbol));
        ASSERT_NE(entry, nullptr);
    }

    void TearDown() override
    {
        dlclose(handle);
    }

protected:
    void *handle;
    beyond::ModuleInterface::EntryPoint entry;
};

TEST_F(AuthenticatorTest, PositiveCreateAsync)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveCreate)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveConfigure)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveActivateAsync)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveActivate)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveConfigure_GenerateCert)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveActivate_GenerateCert)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetSecretKey)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);
    for (int i = 0; i < keyLength; i++) {
        EXPECT_NE(isprint(static_cast<char *>(key)[i]), 0);
    }
    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveActivate_NoConfigure)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    int ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositivePrepare_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(10, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);
    invoked = 0;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(10, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    eventLoop->RemoveEventHandler(handlerObject);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositivePrepare)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_All)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_All_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_PrivateKey_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_PublicKey_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_Certificate_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_PrivateKey)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
    EXPECT_EQ(ret, 0);
}

TEST_F(AuthenticatorTest, PositiveGetKey_PublicKey)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveGetKey_Certificate)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *key = nullptr;
    int keyLength = 0;
    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, key, keyLength);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(key, nullptr);
    EXPECT_GT(keyLength, 0);

    free(key);
    key = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveEncrypt)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(out, nullptr);
    EXPECT_GT(outlen, 0);

    free(out);
    out = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeEncrypt)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(out);
    out = nullptr;

    free(descout);
    descout = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeDecrypt)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const void *out = static_cast<const void *>("Invalid operation");
    int outlen = sizeof("Invalid operation");
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, out, outlen);
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_SSLConfigDisableBase64)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigDisableBase64),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(outlen, 512);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(out);
    out = nullptr;

    free(descout);
    descout = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeDecrypt_SSLConfigDisableBase64)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigDisableBase64),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const void *out = static_cast<const void *>("Invalid operation");
    int outlen = sizeof("Invalid operation");
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, out, outlen);
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_SSLConfig)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outlen, 512);
    for (int i = 0; i < outlen; i++) {
        EXPECT_NE(isprint(static_cast<char *>(out)[i]), 0);
    }

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(descout);
    descout = nullptr;

    free(out);
    out = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeDecrypt_SSLConfig)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, out, outlen);
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveEncrypt_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(out, nullptr);
    EXPECT_GT(outlen, 0);

    free(out);
    out = nullptr;

    invoked = 0;
    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeEncrypt_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, -EINVAL);

    invoked = 0;
    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    invoked = 0;

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(descout);
    descout = nullptr;

    free(out);
    out = nullptr;

    invoked = 0;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_SSLConfig_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outlen, 512);
    for (int i = 0; i < outlen; i++) {
        EXPECT_NE(isprint(static_cast<char *>(out)[i]), 0);
    }

    invoked = 0;
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(descout);
    descout = nullptr;

    free(out);
    out = nullptr;

    invoked = 0;
    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_SSLConfigDisableBase64_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigDisableBase64),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(outlen, 512);

    invoked = 0;
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(descout);
    descout = nullptr;

    free(out);
    out = nullptr;

    invoked = 0;
    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveVerifySignature_SSLConfig)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const char *data = "Signature verification test";
    int dataSize = strlen(data);
    unsigned char *encoded = nullptr;
    int encodedSize = 0;

    ret = authenticator->GenerateSignature((unsigned char *)data, dataSize, encoded, encodedSize);
    ASSERT_EQ(ret, 0);

    bool authentic = false;
    ret = authenticator->VerifySignature(encoded, encodedSize, (unsigned char *)data, dataSize, authentic);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(authentic, true);

    free(encoded);
    encoded = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveVerifySignature)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const char *data = "Signature verification test";
    int dataSize = strlen(data);
    unsigned char *encoded = nullptr;
    int encodedSize = 0;

    ret = authenticator->GenerateSignature((unsigned char *)data, dataSize, encoded, encodedSize);
    ASSERT_EQ(ret, 0);

    bool authentic = false;
    ret = authenticator->VerifySignature(encoded, encodedSize, (unsigned char *)data, dataSize, authentic);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(authentic, true);

    free(encoded);
    encoded = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveCA)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticatorCA = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticatorCA, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticatorCA->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticatorCA->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticatorCA->Prepare();
    EXPECT_EQ(ret, 0);

    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    options.object = static_cast<void *>(&sslConfigNoCertEE);

    ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    options.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
    options.object = static_cast<void *>(authenticatorCA);

    ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    // Dump keys
    FILE *fp;
    void *tmpKey;
    int tmpKeyLength;

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, tmpKey, tmpKeyLength);
    if (ret == 0) {
        fp = fopen("/tmp/server.key", "w+");
        fwrite(tmpKey, tmpKeyLength, 1, fp);
        fclose(fp);
        free(tmpKey);
        tmpKey = nullptr;
    }

    ret = authenticator->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, tmpKey, tmpKeyLength);
    if (ret == 0) {
        fp = fopen("/tmp/server.crt", "w+");
        fwrite(tmpKey, tmpKeyLength, 1, fp);
        fclose(fp);
        free(tmpKey);
        tmpKey = nullptr;
    }

    ret = authenticatorCA->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, tmpKey, tmpKeyLength);
    if (ret == 0) {
        fp = fopen("/tmp/rootCA.key", "w+");
        fwrite(tmpKey, tmpKeyLength, 1, fp);
        fclose(fp);
        free(tmpKey);
        tmpKey = nullptr;
    }

    ret = authenticatorCA->GetKey(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE, tmpKey, tmpKeyLength);
    if (ret == 0) {
        fp = fopen("/tmp/rootCA.crt", "w+");
        fwrite(tmpKey, tmpKeyLength, 1, fp);
        fclose(fp);
        free(tmpKey);
        tmpKey = nullptr;
    }

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    void *result = nullptr;
    int resultLen = 0;

    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticator->GetResult(result, resultLen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(result), "hello world");
    EXPECT_EQ(resultLen, static_cast<int>(sizeof("hello world")));

    free(out);
    out = nullptr;
    outlen = 0;

    free(result);
    result = nullptr;
    resultLen = 0;

    ret = authenticatorCA->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world CA", sizeof("hello world CA"));
    EXPECT_EQ(ret, 0);

    ret = authenticatorCA->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticatorCA->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticatorCA->GetResult(result, resultLen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(result), "hello world CA");
    EXPECT_EQ(resultLen, static_cast<int>(sizeof("hello world CA")));

    free(out);
    out = nullptr;
    outlen = 0;

    free(result);
    result = nullptr;
    resultLen = 0;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();

    ret = authenticatorCA->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticatorCA->Destroy();
}

TEST_F(AuthenticatorTest, Positive_encrypted_compare)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;

    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };

    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    void *out[2] = {
        nullptr,
    };
    int outlen[2] = {
        0,
    };
    void *result = nullptr;
    int resultLen = 0;

    ret = authenticator->GetResult(out[0], outlen[0]);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, 0);

    ret = authenticator->GetResult(out[1], outlen[1]);
    EXPECT_EQ(ret, 0);

    DbgPrint("Encrypted string: %s (%s) %d",
             static_cast<char *>(out[0]),
             static_cast<char *>(out[1]),
             outlen[0] == outlen[1] ? !!memcmp(out[0], out[1], outlen[0]) : 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out[0], outlen[0]);
    EXPECT_EQ(ret, 0);

    ret = authenticator->GetResult(result, resultLen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(result), "hello world");
    EXPECT_EQ(resultLen, static_cast<int>(sizeof("hello world")));

    free(result);
    result = nullptr;
    resultLen = 0;

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, out[1], outlen[1]);
    EXPECT_EQ(ret, 0);

    ret = authenticator->GetResult(result, resultLen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(result), "hello world");
    EXPECT_EQ(resultLen, static_cast<int>(sizeof("hello world")));

    EXPECT_STRNE(static_cast<char *>(out[0]), static_cast<char *>(out[1]));

    free(result);
    result = nullptr;
    resultLen = 0;

    free(out[0]);
    out[0] = nullptr;
    outlen[0] = 0;

    free(out[1]);
    out[1] = nullptr;
    outlen[1] = 0;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_Symmetric)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    unsigned char *iv = static_cast<unsigned char *>(malloc(128 >> 3));
    ASSERT_NE(iv, nullptr);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, "hello world", sizeof("hello world"), iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, out, outlen, iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(out);
    out = nullptr;

    free(descout);
    descout = nullptr;

    free(iv);
    iv = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_Symmetric_Long)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    unsigned char *iv = static_cast<unsigned char *>(malloc(128 >> 3));
    ASSERT_NE(iv, nullptr);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, "hello world, I'm a long long test larger than default block size of aes256 cbc algorithm", sizeof("hello world, I'm a long long test larger than default block size of aes256 cbc algorithm"), iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, out, outlen, iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world, I'm a long long test larger than default block size of aes256 cbc algorithm");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world, I'm a long long test larger than default block size of aes256 cbc algorithm")));

    free(out);
    out = nullptr;

    free(descout);
    descout = nullptr;

    free(iv);
    iv = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeEncrypt_invalid_iv)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, NegativeDecrypt_invalid_iv)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, "hello world", sizeof("hello world"));
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST_F(AuthenticatorTest, PositiveDecrypt_Symmetric_Async)
{
    char *argv[] = {
        const_cast<char *>(::Authenticator::NAME),
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE),
    };
    int argc = sizeof(argv) / sizeof(char *);

    optind = 0;
    opterr = 0;
    beyond::AuthenticatorInterface *authenticator = reinterpret_cast<beyond::AuthenticatorInterface *>(entry(argc, argv));
    ASSERT_NE(authenticator, nullptr);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    int invoked;
    beyond::EventLoop::HandlerObject *handlerObject = eventLoop->AddEventHandler(
        authenticator,
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *obj, int event, void *data) -> beyond_handler_return {
            beyond::AuthenticatorInterface *auth = static_cast<beyond::AuthenticatorInterface *>(obj);
            if (event & BEYOND_EVENT_TYPE_READ) {
                beyond::EventObjectInterface::EventData *evtData = nullptr;
                if (auth->FetchEventData(evtData) == 0 && evtData != nullptr) {
                    DbgPrint("EventType: 0x%.8X", evtData->type);
                    int *invoked = static_cast<int *>(data);
                    *invoked = evtData->type;
                    auth->DestroyEventData(evtData);
                }
            }
            return beyond_handler_return::BEYOND_HANDLER_RETURN_RENEW;
        },
        &invoked);
    ASSERT_NE(handlerObject, nullptr);

    beyond_config options = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfigNoCert),
    };
    int ret = authenticator->Configure(&options);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE);

    unsigned char *iv = static_cast<unsigned char *>(malloc(128 >> 3));
    ASSERT_NE(iv, nullptr);

    invoked = 0;
    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, "hello world", sizeof("hello world"), iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *out = nullptr;
    int outlen = 0;
    ret = authenticator->GetResult(out, outlen);
    EXPECT_EQ(ret, 0);

    invoked = 0;
    ret = authenticator->Decrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY, out, outlen, iv, 128 >> 3);
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE);

    void *descout = nullptr;
    int descoutlen = 0;
    ret = authenticator->GetResult(descout, descoutlen);
    EXPECT_EQ(ret, 0);

    EXPECT_STREQ(static_cast<char *>(descout), "hello world");
    EXPECT_EQ(descoutlen, static_cast<int>(sizeof("hello world")));

    free(descout);
    descout = nullptr;

    free(out);
    out = nullptr;

    free(iv);
    iv = nullptr;

    invoked = 0;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    ret = eventLoop->Run(1, 1, -1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((invoked & BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE), BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE);

    ret = eventLoop->RemoveEventHandler(handlerObject);
    EXPECT_EQ(ret, 0);
    handlerObject = nullptr;

    eventLoop->Destroy();

    authenticator->Destroy();
}
