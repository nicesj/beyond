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

#include "unittest.h"

#include <beyond/private/beyond_private.h>
#include <exception>
#include <gtest/gtest.h>
#include <dlfcn.h>

#include "beyond/plugin/peer_nn_plugin.h"
#include "beyond/plugin/authenticator_ssl_plugin.h"

static struct beyond_peer_info s_valid_edge_info = {
    .name = const_cast<char *>("edge"),
    .host = const_cast<char *>("0.0.0.0"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
    .uuid = "ec0e0cec-d797-4ba5-b698-f2420c74b787",
};

static GrpcContext s_grpcCtx;

void PeerTest::SetUp()
{
    handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
    ErrPrint("dlerror: %s", dlerror());
    ASSERT_NE(handle, nullptr);
    entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(handle, beyond::ModuleInterface::EntryPointSymbol));
    ASSERT_NE(entry, nullptr);
}

void PeerTest::TearDown()
{
    dlclose(handle);
}

void StopGrpcServer(void)
{
    ASSERT_NE(s_grpcCtx.peer, nullptr);
    int ret = s_grpcCtx.peer->Deactivate();
    ASSERT_EQ(ret, 0);

    s_grpcCtx.peer->Destroy();
    s_grpcCtx.peer = nullptr;

    dlclose(s_grpcCtx.handle);
    s_grpcCtx.handle = nullptr;
}

void StartGrpcServer(void)
{
    s_grpcCtx.handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
    ASSERT_NE(s_grpcCtx.handle, nullptr);

    beyond::ModuleInterface::EntryPoint entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(s_grpcCtx.handle, beyond::ModuleInterface::EntryPointSymbol));
    ASSERT_NE(entry, nullptr);

    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    s_grpcCtx.peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(s_grpcCtx.peer, nullptr);

    int ret = s_grpcCtx.peer->SetInfo(&s_valid_edge_info);
    ASSERT_EQ(ret, 0);

    ret = s_grpcCtx.peer->Activate();
    ASSERT_EQ(ret, 0);
}

void StartGrpcServer(beyond::AuthenticatorInterface *&auth, beyond::AuthenticatorInterface *&authCA)
{
    char *auth_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME),
    };

    beyond_argument args = {
        .argc = sizeof(auth_argv) / sizeof(char *),
        .argv = auth_argv,
    };

    beyond_authenticator_ssl_config_ssl ssl = {
        .bits = -1,
        .serial = 1,
        .days = -1,
        .isCA = -1,
        .enableBase64 = -1,
        .passphrase = nullptr,
        .private_key = nullptr,
        .certificate = nullptr,
        .alternative_name = "127.0.0.1",
    };

    beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&ssl),
    };

    if (authCA == nullptr) {
        authCA = beyond::Authenticator::Create(&args);
        ASSERT_NE(authCA, nullptr);

        int ret = authCA->Configure(&config);
        ASSERT_EQ(ret, 0);

        ret = authCA->Activate();
        ASSERT_EQ(ret, 0);

        ret = authCA->Prepare();
        ASSERT_EQ(ret, 0);

        DbgPrint("Authenticator CA module is prepared");
    }

    if (auth == nullptr) {
        auth = beyond::Authenticator::Create(&args);
        ASSERT_NE(auth, nullptr);

        ssl.isCA = 0;
        ssl.serial = 2;
        int ret = auth->Configure(&config);
        ASSERT_EQ(ret, 0);

        config.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
        config.object = static_cast<void *>(authCA);
        ret = auth->Configure(&config);
        ASSERT_EQ(ret, 0);

        ret = auth->Activate();
        ASSERT_EQ(ret, 0);

        ret = auth->Prepare();
        ASSERT_EQ(ret, 0);

        DbgPrint("Authenticator module is prepared");
    }

    s_grpcCtx.handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
    ASSERT_NE(s_grpcCtx.handle, nullptr);

    beyond::ModuleInterface::EntryPoint entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(s_grpcCtx.handle, beyond::ModuleInterface::EntryPointSymbol));
    ASSERT_NE(entry, nullptr);

    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    s_grpcCtx.peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(s_grpcCtx.peer, nullptr);

    int ret = s_grpcCtx.peer->SetInfo(&s_valid_edge_info);
    ASSERT_EQ(ret, 0);

    beyond_config options = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR,
        .object = static_cast<void *>(authCA),
    };

    ret = s_grpcCtx.peer->Configure(&options);
    ASSERT_EQ(ret, 0);

    options.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
    options.object = static_cast<void *>(auth);
    ret = s_grpcCtx.peer->Configure(&options);
    ASSERT_EQ(ret, 0);

    ret = s_grpcCtx.peer->Activate();
    ASSERT_EQ(ret, 0);
}

void StartGrpcServer(beyond::AuthenticatorInterface *&auth)
{
    char *auth_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME),
    };

    beyond_argument args = {
        .argc = sizeof(auth_argv) / sizeof(char *),
        .argv = auth_argv,
    };

    auth = beyond::Authenticator::Create(&args);
    ASSERT_NE(auth, nullptr);

    beyond_authenticator_ssl_config_ssl ssl = {
        .bits = -1,
        .serial = 1,
        .days = -1,
        .isCA = -1,
        .enableBase64 = -1,
        .passphrase = nullptr,
        .private_key = nullptr,
        .certificate = nullptr,
        .alternative_name = "127.0.0.1",
    };

    beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&ssl),
    };

    int ret = auth->Configure(&config);
    ASSERT_EQ(ret, 0);

    ret = auth->Activate();
    ASSERT_EQ(ret, 0);

    ret = auth->Prepare();
    ASSERT_EQ(ret, 0);

    DbgPrint("Authenticator module is prepared");

    s_grpcCtx.handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
    ASSERT_NE(s_grpcCtx.handle, nullptr);

    beyond::ModuleInterface::EntryPoint entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(s_grpcCtx.handle, beyond::ModuleInterface::EntryPointSymbol));
    ASSERT_NE(entry, nullptr);

    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    s_grpcCtx.peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(s_grpcCtx.peer, nullptr);

    ret = s_grpcCtx.peer->SetInfo(&s_valid_edge_info);
    ASSERT_EQ(ret, 0);

    const beyond_config options = {
        .type = BEYOND_CONFIG_TYPE_AUTHENTICATOR,
        .object = static_cast<void *>(auth),
    };

    ret = s_grpcCtx.peer->Configure(&options);
    ASSERT_EQ(ret, 0);

    ret = s_grpcCtx.peer->Activate();
    ASSERT_EQ(ret, 0);
}

void StartGrpcServer(beyond_peer_info *info)
{
    s_grpcCtx.handle = dlopen(MODULE_FILENAME, RTLD_LAZY);
    ASSERT_NE(s_grpcCtx.handle, nullptr);

    beyond::ModuleInterface::EntryPoint entry = reinterpret_cast<beyond::ModuleInterface::EntryPoint>(dlsym(s_grpcCtx.handle, beyond::ModuleInterface::EntryPointSymbol));
    ASSERT_NE(entry, nullptr);

    int argc = 4;
    char *argv[4];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);
    argv[1] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER);
    argv[2] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_ARGUMENT_STORAGE_PATH);
    argv[3] = const_cast<char *>("/tmp/");

    optind = 0;
    opterr = 0;
    s_grpcCtx.peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(s_grpcCtx.peer, nullptr);

    int ret = s_grpcCtx.peer->SetInfo(info);
    ASSERT_EQ(ret, 0);

    ret = s_grpcCtx.peer->Activate();
    ASSERT_EQ(ret, 0);
}

const char *GetModelFilename(bool posenet)
{
    static char dirpath[FILENAME_MAXSIZE];

    const char *basedir = getenv("TEST_BASEDIR");
    if (basedir == nullptr) {
        basedir = "";
    }

    if (posenet == true) {
        snprintf(dirpath, sizeof(dirpath), "%ssubprojects/libbeyond-peer_nn/test/posenet-model-mobilenet_v1_100.tflite", basedir);
    } else {
        snprintf(dirpath, sizeof(dirpath), "%ssubprojects/libbeyond-peer_nn/test/mobilenet_v1_1.0_224_quant.tflite", basedir);
    }
    return dirpath;
}

int main(int argc, char *argv[])
{
    int result = -1;

    try {
        testing::InitGoogleTest(&argc, argv);
    } catch (std::exception &e) {
        ErrPrint("catch 'testing::internal::<unnamed>::ClassUniqueToAlwaysTrue': %s", e.what());
    }

    setenv("GST_PLUGIN_PATH", "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/", 1);

    try {
        result = RUN_ALL_TESTS();
    } catch (std::exception &e) {
        ErrPrint("catch `testing::internal::GoogleTestFailureException`: %s", e.what());
    }

    return result;
}
