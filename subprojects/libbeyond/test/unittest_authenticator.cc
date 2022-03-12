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

#include <gtest/gtest.h>
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <beyond/plugin/authenticator_ssl_plugin.h>

static const char *decrypted_string = "my test data";

static const char *argv[] = {
    BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME,
};

const beyond_argument arg = {
    .argc = sizeof(argv) / sizeof(char *),
    .argv = const_cast<char **>(argv)
};

static beyond_authenticator_ssl_config_ssl sslConfig = {
    .bits = -1,
    .serial = -1,
    .days = -1,
    .isCA = -1,
    .enableBase64 = -1,
    .passphrase = nullptr,
    .private_key =
        "-----BEGIN PRIVATE KEY-----\n"
        "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\n"
        "cv7Wls7EjAuzvO6rlEwRcrgd748z\n"
        "-----END PRIVATE KEY-----",
    .certificate =
        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "-----END CERTIFICATE-----",
    .alternative_name = "127.0.0.1",
};

TEST(Authenticator, PositiveCreate)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveDestroy)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveConfigure_null)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveActivate)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveDeactivate)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveGetHandle)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    int handle = authenticator->GetHandle();
    EXPECT_EQ(handle, -ENOTSUP);

    authenticator->Destroy();
}

TEST(Authenticator, PositivePrepare)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, NegativeEncrypt_Private)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY, "my test data", sizeof("my test data"));
    EXPECT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveGetResult)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY, "my test data", sizeof("my test data"));
    EXPECT_EQ(ret, 0);

    void *data = nullptr;
    int size = 0;
    ret = authenticator->GetResult(data, size);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(data, nullptr);
    EXPECT_GT(size, 0);

    free(data);
    data = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, NegativeDecrypt_SSLConfig_PublicKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const void *data = static_cast<const void *>("invalid operation");
    int size = sizeof("invalid operation");
    ret = authenticator->Decrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY,
        data, size);
    ASSERT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveDecrypt_SSLConfig_PrivateKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&sslConfig),
    };
    int ret = authenticator->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY,
        decrypted_string,
        strlen(decrypted_string));
    ASSERT_EQ(ret, 0);

    void *data = nullptr;
    int size = 0;
    ret = authenticator->GetResult(data, size);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(size, 0);

    ret = authenticator->Decrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY,
        data, size);
    ASSERT_EQ(ret, 0);

    void *ddata = nullptr;
    int dsize = 0;
    ret = authenticator->GetResult(ddata, dsize);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(ddata, nullptr);
    EXPECT_GT(dsize, 0);

    EXPECT_STREQ(decrypted_string, static_cast<char *>(ddata));

    free(data);
    data = nullptr;

    free(ddata);
    ddata = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, NegativeDecrypt_PublicKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    int ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    const void *data = static_cast<const void *>("invalid operation");
    int size = sizeof("invalid operation");
    ret = authenticator->Decrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY,
        data, size);
    ASSERT_EQ(ret, -EINVAL);

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveDecrypt_PrivateKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    int ret = authenticator->Activate();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Prepare();
    EXPECT_EQ(ret, 0);

    ret = authenticator->Encrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY,
        decrypted_string,
        strlen(decrypted_string));
    ASSERT_EQ(ret, 0);

    void *data = nullptr;
    int size = 0;
    ret = authenticator->GetResult(data, size);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(size, 0);

    ret = authenticator->Decrypt(
        beyond_authenticator_key_id::BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY,
        data, size);
    ASSERT_EQ(ret, 0);

    void *ddata = nullptr;
    int dsize = 0;
    ret = authenticator->GetResult(ddata, dsize);
    EXPECT_EQ(ret, 0);

    EXPECT_NE(ddata, nullptr);
    EXPECT_GT(dsize, 0);

    EXPECT_STREQ(decrypted_string, static_cast<char *>(ddata));

    free(data);
    data = nullptr;

    free(ddata);
    ddata = nullptr;

    ret = authenticator->Deactivate();
    EXPECT_EQ(ret, 0);

    authenticator->Destroy();
}

TEST(Authenticator, PositiveGetKey_PrivateKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
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
}

TEST(Authenticator, PositiveGetKey_PublicKey)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
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

TEST(Authenticator, PositiveGetKey_Certificate)
{
    auto authenticator = beyond::Authenticator::Create(&arg);
    ASSERT_NE(authenticator, nullptr);

    auto ret = authenticator->Configure(nullptr);
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
