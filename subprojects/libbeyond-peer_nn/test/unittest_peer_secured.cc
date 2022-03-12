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
#include <cstdlib>
#include <exception>
#include <gtest/gtest.h>

#include <unistd.h>
#include <dlfcn.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "beyond/plugin/peer_nn_plugin.h"
#include "beyond/plugin/authenticator_ssl_plugin.h"

#include "peer.h"
#include "peer_model.h"
#include "unittest.h"

#define I420Size(w, h) ((w) * (h) + (w) * (h) / 2)

static struct beyond_input_config s_image_config = {
    .input_type = BEYOND_INPUT_TYPE_IMAGE,
    .config = {
        .image = {
            .format = "I420",
            .width = 224,
            .height = 224,
            .convert_format = "RGB",
            .convert_width = 224,
            .convert_height = 224,
            .transform_mode = "typecast",
            .transform_option = "uint8" },
    },
};

static beyond_config s_input_config = {
    .type = BEYOND_CONFIG_TYPE_INPUT,
    .object = &s_image_config,
};

static struct beyond_input_config s_pose_image_config = {
    .input_type = BEYOND_INPUT_TYPE_IMAGE,
    .config = {
        .image = {
            .format = "I420",
            .width = 320,
            .height = 240,
            .convert_format = "RGB",
            .convert_width = 257,
            .convert_height = 257,
            .transform_mode = "arithmetic",
            .transform_option = "typecast:float32,div:127.5,add:-1" },
    },
};

static beyond_config s_pose_input_config = {
    .type = BEYOND_CONFIG_TYPE_INPUT,
    .object = &s_pose_image_config,
};

static struct beyond_peer_info s_valid_info = {
    .name = const_cast<char *>("name"),
    .host = const_cast<char *>("127.0.0.1"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
    .uuid = "ec0e0cec-d797-4ba5-b698-f2420c74b787",
};

static struct beyond_peer_info s_invalid_info_uuid = {
    .name = const_cast<char *>("name"),
    .host = const_cast<char *>("127.0.0.1"),
    .port = { 50000 },
    .free_memory = 0llu,
    .free_storage = 0llu,
    .uuid = "11111111-0000-0000-b698-f2420c74b787",
};

static int LoadImageData(const char *filename, void *data, long size)
{
    int ret;
    FILE *fp = fopen(filename, "r");
    if (fp == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "fopen");
        return ret;
    }

    if (fseek(fp, 0, SEEK_END) < 0) {
        ret = -errno;
        ErrPrintCode(errno, "fseek");
        fclose(fp);
        return ret;
    }

    long sz = ftell(fp);
    if (sz < 0) {
        ret = -errno;
        ErrPrintCode(errno, "ftell");
        fclose(fp);
        return ret;
    }

    if (sz > size) {
        ErrPrint("Test data will be truncated: %ld <> %ld", sz, size);
    }

    if (fseek(fp, 0, SEEK_SET) < 0) {
        ret = -errno;
        ErrPrintCode(errno, "fseek");
        fclose(fp);
        return ret;
    }

    if (fread(data, 1, size, fp) != static_cast<size_t>(size)) {
        ret = -errno;
        ErrPrintCode(errno, "fread");
        fclose(fp);
        return ret;
    }

    if (fclose(fp) < 0) {
        ErrPrintCode(errno, "fclose");
    }

    return 0;
}

TEST_F(PeerTest, PositiveLoadModelDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveGetInputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    // FIXME:
    // The pipeline must be prepared for getting the tensor shape of the model
    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveGetOutputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    // FIXME:
    // The pipeline must be prepared for getting the tensor shape of the model
    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 2;
    dims->data[0] = 1;
    dims->data[1] = 1001;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };

    const beyond_tensor_info *outputTensorInfo;
    int size;
    ret = peer->GetOutputTensorInfo(outputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(outputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(outputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(outputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(outputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(outputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSetInputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSetInputTensorInfoDevice_withPrepare_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    ASSERT_EQ(ret, 0);

    const beyond_tensor_info *inputTensorInfo;
    int size;
    ret = peer->GetInputTensorInfo(inputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(inputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(inputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(inputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(inputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(inputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(inputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(inputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->SetInputTensorInfo(nullptr, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_Size_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo;
    ret = peer->SetInputTensorInfo(&tensorInfo, -1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_TensorSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = 0,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_DimensionsSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 0;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetInputTensorInfoDevice_DimensionsNull_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = nullptr,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSetOutputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    ASSERT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    ASSERT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    ASSERT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    const beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    ASSERT_EQ(ret, 0);

    const beyond_tensor_info *outputTensorInfo;
    int size;
    ret = peer->GetOutputTensorInfo(outputTensorInfo, size);
    ASSERT_EQ(ret, 0);

    EXPECT_EQ(size, 1);
    EXPECT_EQ(outputTensorInfo->type, tensorInfo.type);
    EXPECT_EQ(outputTensorInfo->size, tensorInfo.size);
    EXPECT_EQ(outputTensorInfo->dims->size, tensorInfo.dims->size);
    EXPECT_EQ(outputTensorInfo->dims->data[0], tensorInfo.dims->data[0]);
    EXPECT_EQ(outputTensorInfo->dims->data[1], tensorInfo.dims->data[1]);
    EXPECT_EQ(outputTensorInfo->dims->data[2], tensorInfo.dims->data[2]);
    EXPECT_EQ(outputTensorInfo->dims->data[3], tensorInfo.dims->data[3]);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->SetOutputTensorInfo(nullptr, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_Size_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo;
    ret = peer->SetOutputTensorInfo(&tensorInfo, -1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_TensorSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 2;
    dims->data[2] = 3;
    dims->data[3] = 4;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 0,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_DimensionsSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 0;
    dims->data[0] = 1;
    dims->data[1] = 2;
    dims->data[2] = 3;
    dims->data[3] = 4;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSetOutputTensorInfoDevice_DimensionsNull_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_INT8,
        .size = 1004,
        .name = nullptr,
        .dims = nullptr,
    };
    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
    };
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE;
    config.object = static_cast<void *>(&options);
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_BeforeActivate_ImageInput_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
        .server = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
            .framework = const_cast<char *>("tensorflow-lite"),
            .accel = nullptr,
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_BeforeActivate_VideoInput_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_VIDEO,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
        .server = {
            .input_type = BEYOND_INPUT_TYPE_VIDEO,
            .preprocessing = const_cast<char *>(PREPROCESSING),
            .postprocessing = const_cast<char *>("postprocessing"),
            .framework = const_cast<char *>("tensorflow-lite"),
            .accel = nullptr,
        },
    };
    beyond_config config = {
        .type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE,
        .object = static_cast<void *>(&options),
    };
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_ImageConfig_BeforeActivate_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "I420",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_VideoConfig_BeforeActivate_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_VIDEO,
        .config = {
            .video = {
                .frame = {
                    .format = "NV21",
                    .width = 224,
                    .height = 224,
                    .convert_format = "RGB",
                    .convert_width = 224,
                    .convert_height = 224,
                    .transform_mode = "typecast",
                    .transform_option = "uint8" },
                .fps = 30,
                .duration = -1, // Live video - no duration
            },
        },
    };

    struct beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_INPUT,
        .object = &input_config,
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveConfigureDevice_ImageConfig_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "I420",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    config.type = BEYOND_CONFIG_TYPE_INPUT;
    config.object = &input_config;

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositivePrepareWithConfigureDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>("video/x-raw,format=RGB,width=640,height=480,framerate=0/1 ! videoconvert ! videoscale ! video/x-raw,format=I420,width=224,height=224 ! jpegenc"),
        },
        .server = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>("jpegdec ! videoconvert ! video/x-raw,format=RGB,width=224,height=224 ! tensor_converter ! tensor_transform mode=typecast option=uint8"),
        },
    };
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE;
    config.object = static_cast<void *>(&options);
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativePrepareWithConfigureDevice_InvalidPreprocessing_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_plugin_peer_nn_config options = {
        .client = {
            .input_type = BEYOND_INPUT_TYPE_IMAGE,
            .preprocessing = const_cast<char *>("I'm not a valid pipe description"),
            .postprocessing = const_cast<char *>("postprocessing"),
        },
    };
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_PIPELINE;
    config.object = static_cast<void *>(&options);
    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, -EFAULT);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositivePrepareWithoutConfigureDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int ret;
    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveInvokeDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    // NOTE:
    // The tensorInfo should not have dims in the case of the input type is configured.
    // Reference Issue: https://github.sec.samsung.net/BeyonD/beyond/issues/731
    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = I420Size(s_image_config.config.image.width, s_image_config.config.image.height),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    ret = LoadImageData("orange.yuv", tensor->data, tensor->size);
    ASSERT_EQ(ret, 0);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 1;
    dims->data[2] = 1;
    dims->data[3] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Invoke(tensor, 1, nullptr);
    EXPECT_EQ(ret, 0);

    beyond_tensor *output;
    int outputSize;
    ret = peer->GetOutput(output, outputSize);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outputSize, 0);
    EXPECT_NE(output, nullptr);

    uint8_t *scores = static_cast<uint8_t *>(output[0].data);
    uint8_t maxScore = 0;
    int index = -1;
    for (int i = 0; i < output[0].size; i++) {
        if (scores[i] > 0 && scores[i] > maxScore) {
            index = i;
            maxScore = scores[i];
        }
    }

    EXPECT_EQ(index, 951);

    peer->FreeTensor(output, outputSize);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveInvokeDevice_Posenet_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_pose_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_POSENET_FILENAME);
    EXPECT_EQ(ret, 0);

    // NOTE:
    // The tensorInfo should not have dims in the case of the input type is configured.
    // Reference Issue: https://github.sec.samsung.net/BeyonD/beyond/issues/731
    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 257;
    dims->data[2] = 257;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_FLOAT32,
        .size = I420Size(s_pose_image_config.config.image.width, s_pose_image_config.config.image.height),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    ret = LoadImageData("pose.yuv", tensor->data, tensor->size);
    ASSERT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Invoke(tensor, 1, nullptr);
    EXPECT_EQ(ret, 0);

    beyond_tensor *output;
    int outputSize;
    ret = peer->GetOutput(output, outputSize);
    EXPECT_EQ(ret, 0);

    ASSERT_EQ(outputSize, 4);
    ASSERT_NE(output, nullptr);

    // TODO:
    // Validate output result

    peer->FreeTensor(output, outputSize);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveInvokeDevice_ImageConfig_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    struct beyond_input_config input_config = {
        .input_type = BEYOND_INPUT_TYPE_IMAGE,
        .config = {
            .image = {
                .format = "I420",
                .width = 224,
                .height = 224,
                .convert_format = "RGB",
                .convert_width = 224,
                .convert_height = 224,
                .transform_mode = "typecast",
                .transform_option = "uint8" },
        },
    };

    config.type = BEYOND_CONFIG_TYPE_INPUT;
    config.object = &input_config;

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    ret = LoadImageData("orange.yuv", tensor->data, tensor->size);
    ASSERT_EQ(ret, 0);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 1;
    dims->data[2] = 1;
    dims->data[3] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Invoke(tensor, 1, nullptr);
    EXPECT_EQ(ret, 0);

    beyond_tensor *output;
    int outputSize;
    ret = peer->GetOutput(output, outputSize);
    EXPECT_EQ(ret, 0);

    EXPECT_GT(outputSize, 0);
    EXPECT_NE(output, nullptr);

    uint8_t *scores = static_cast<uint8_t *>(output[0].data);
    uint8_t maxScore = 0;
    int index = -1;
    for (int i = 0; i < output[0].size; i++) {
        if (scores[i] > 0 && scores[i] > maxScore) {
            index = i;
            maxScore = scores[i];
        }
    }

    EXPECT_EQ(index, 951);

    peer->FreeTensor(output, outputSize);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveInvokeDevice_Async_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor_info::dimensions *dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * 4));
    ASSERT_NE(dims, nullptr);

    dims->size = 4;
    dims->data[0] = 1;
    dims->data[1] = 224;
    dims->data[2] = 224;
    dims->data[3] = 3;

    beyond_tensor_info tensorInfo = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = dims->data[0] * dims->data[1] * dims->data[2] * dims->data[3] * static_cast<int>(sizeof(unsigned char)),
        .name = nullptr,
        .dims = dims,
    };
    ret = peer->SetInputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    beyond_tensor *tensor;
    ret = peer->AllocateTensor(&tensorInfo, 1, tensor);
    ASSERT_EQ(ret, 0);

    ret = LoadImageData("orange.yuv", tensor->data, tensor->size);
    ASSERT_EQ(ret, 0);

    dims->size = 2;
    dims->data[0] = 1;
    dims->data[1] = 1001;

    tensorInfo.size = dims->data[0] * dims->data[1] * static_cast<int>(sizeof(unsigned char));

    ret = peer->SetOutputTensorInfo(&tensorInfo, 1);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    ASSERT_EQ(ret, 0);

    ret = peer->Invoke(tensor, 1, reinterpret_cast<void *>(0x1004beef));
    EXPECT_EQ(ret, 0);

    beyond::EventLoop *eventLoop = beyond::EventLoop::Create(false, false);
    ASSERT_NE(eventLoop, nullptr);

    eventLoop->AddEventHandler(
        static_cast<beyond::EventObjectBaseInterface *>(peer),
        BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR,
        [](beyond::EventObjectBaseInterface *object, int type, void *cbData)
            -> beyond_handler_return {
            beyond::InferenceInterface::PeerInterface *peer = static_cast<beyond::InferenceInterface::PeerInterface *>(object);

            EXPECT_EQ((type & BEYOND_EVENT_TYPE_READ), BEYOND_EVENT_TYPE_READ);

            int ret;
            beyond_handler_return handler_ret = BEYOND_HANDLER_RETURN_RENEW;

            beyond::EventObjectInterface::EventData *evtData = nullptr;
            ret = peer->FetchEventData(evtData);
            EXPECT_EQ(ret, 0);

            EXPECT_TRUE(((evtData->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) == BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) || ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED));

            if ((evtData->type & BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) == BEYOND_EVENT_TYPE_PEER_INFO_UPDATED) {
                EXPECT_EQ(evtData->data, nullptr);
            } else if ((evtData->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) == BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
                EXPECT_EQ(evtData->data, reinterpret_cast<void *>(0x1004beef));

                beyond_tensor *output;
                int outputSize;
                ret = peer->GetOutput(output, outputSize);
                EXPECT_EQ(ret, 0);

                EXPECT_GT(outputSize, 0);
                EXPECT_NE(output, nullptr);

                uint8_t *scores = static_cast<uint8_t *>(output[0].data);
                uint8_t maxScore = 0;
                int index = -1;
                for (int i = 0; i < output[0].size; i++) {
                    if (scores[i] > 0 && scores[i] > maxScore) {
                        index = i;
                        maxScore = scores[i];
                    }
                }

                EXPECT_EQ(index, 951);

                peer->FreeTensor(output, outputSize);
                handler_ret = BEYOND_HANDLER_RETURN_CANCEL;
            }

            ret = peer->DestroyEventData(evtData);
            EXPECT_EQ(ret, 0);
            return handler_ret;
        },
        [](beyond::EventObjectBaseInterface *eventObject, void *cbData)
            -> void {
            return;
        },
        tensor);

    ret = eventLoop->Run(10, 2, -1);
    ASSERT_EQ(ret, 0);

    eventLoop->Destroy();

    peer->FreeTensor(tensor, 1);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    free(dims);
    dims = nullptr;

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensor_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    ret = peer->Invoke(nullptr, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    beyond_tensor tensor;
    ret = peer->Invoke(&tensor, 0, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensorSize_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    char buffer[4096];
    beyond_tensor tensor = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = -1,
        .data = static_cast<char *>(buffer),
    };
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeInvokeDevice_InvalidTensorData_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Prepare();
    EXPECT_EQ(ret, 0);

    char buffer[4096];
    beyond_tensor tensor = {
        .type = BEYOND_TENSOR_TYPE_UINT8,
        .size = sizeof(buffer),
        .data = nullptr,
    };
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeInvokeWithoutPrepareDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    beyond_tensor tensor;
    ret = peer->Invoke(&tensor, 1, nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeConfigureDevice_secured)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    int argc = 1;
    char *argv[2];
    argv[0] = const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME);

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(argc, argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->LoadModel(MODEL_FILENAME);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(nullptr);
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSecuredGRPC_ConfiguredCA)
{
    beyond_authenticator_ssl_config_ssl rootSSLConfig = {
        .bits = -1,
        .serial = -1,
        .days = -1,
        .isCA = -1,
        .enableBase64 = -1,
        .passphrase = "",
        .private_key =
            "-----BEGIN PRIVATE KEY-----\n"
            "MIIJQwIBADANBgkqhkiG9w0BAQEFAASCCS0wggkpAgEAAoICAQC46ZLQSghvjvFh\n"
            "M/qZt6LfEYl4dtThHZ+QhVvLfLvVOquuVxH0134as8984axepNYJ/XyH4iq9K+/G\n"
            "lqjoFJ6wBHutLfntXB3KLma6qdQvlVcRr6uzIinQVEIJSzp64e9c72MDZMdv67/s\n"
            "pIEovk7jgC6hgkZIQVd9rE6spBMY1ONXr+oYGU0U9r3TbEIyUsa7oTd3jXIgJoUC\n"
            "0V6vtRlmqNkvQeu6v9Puj+ZHc+uuSnj1Mcx+edHDBHnMoP9C5pSu0pxvu1+ONqCZ\n"
            "GBEJdwGASxYIDVdTatIyx3eGJhHukOtCkJpfw6Sh56xRJiVFKrA67ZqrM+M5RY/M\n"
            "7nszk52ZU1pvdJNeizsZ6tezmtnGoZ1XEfrTNJn4LDZFKJtXOiKp6KKNmLhSxcBt\n"
            "zTFGwI6pktzI2FCIi3UePv4nuosOUxJR6q4Zk/SBSFuEffON4EKhNCgCX+PUqP2p\n"
            "SbT+CeRVmOdV0n6gd3OkGiG/nclVeStSdbi9XYO/jEBkiBl2fKQ1T66HY/jp8W84\n"
            "+Me39I9i4GNxzX8B4CPayEX2ZHgiVlmPRJRuI7jlXDKrxmwNEDcPgTSjxOmA71ny\n"
            "8hZMIpC33eTThuAzZPZBjJvh83Rzt54LU0paK1qo4MRhNhtd5lMOUv4LCD7Mg4R1\n"
            "dDbFen+fgRy7td8XKMuFvL5kFzJMNwIDAQABAoICAEcll9eMpLJHzZgY59M9VO1/\n"
            "UeWH02DKhRqWNTuWQq9IY8Ywujf6sgqUJMFoE2pXAgPWBJRD8S3YOemvDk49oNEY\n"
            "6H05s3AggVXJhL1Nmta0H0wuy2GhQ3Vk9gOdbmLZi7+2W+JyZEor6yyiHxAOKUxf\n"
            "hZGfDmu+uGsiYSML/k0PnGmgxfF/yqjGR0OR4+Z48v4+iZj2U3MLXyI2bLgudheJ\n"
            "4AbO1mSEaobf1zqm34ewH9o3zvba6Fqg3jxdtdmH3q1lW8uhzKJrYl+FYwjBQVKb\n"
            "kV6Hw5HVCAuBs/tpqnygReTWvo8aN74T5blTdAOo6SXDRj+ZN1RR74JqP/0YTwqH\n"
            "tGh89nNsMxX8dhi0I7atELG2seJbrdXxOoqQ4U/sGDO1eGA4ywUVihXvVb2CFIPM\n"
            "RPHFF2RNei13KCXMO97Ty55JQB10HN2xSn3LwOH8H9ub0mtxx4F/iobfEW4mNXT9\n"
            "Zn1Dm0tGwWk6nAJb4Eni+gopxK+444SCJKaFEUKFacnyh5pUi3TpQ/BLXCNuIlp8\n"
            "/voLQGNtQhgBVkW6Thl4BZMojxPkI81zvKAh01RAOZSBHoMseHqy6mrWHex6J5hh\n"
            "kxX/p9GnWK2aSw7NGAjx677mi/9Bijdy10RUSS/y/uPXGwV9aQvgqBPsHirasJLZ\n"
            "ZyjRChO2zeQB3htIG6wBAoIBAQDpFUm7i8zU0dpVct0WpZu0jRQTF7a1GXvGdVg2\n"
            "uJmHWZgFBTbtjvkElhjzPoy4ADcCgiuEP0J8P/Unxi/7QPY5gbuPxJoCaAN71EGB\n"
            "G2hIjLVQsUGGmSqmGKwdqnw0pEzOPeDSHmUcSVxuA9QHiP3HSf4VYXVl4pX01OF9\n"
            "xgIufwSTl9oUcTXC4JXGTQ43w5bspXfUSvDrS1gFVIrPsUWwwLDkCh+6SlcpdxMX\n"
            "iR0b+w+BVR+xWY7df1dhE9AsQT9LW1Vt95nJezNrOmYX/m33ywW5BZbGmxpfx24j\n"
            "Rg/sJKMhvbnKqnbe+KOWpxcDHJwh7aezr7wHQ8/fp6JJvoC9AoIBAQDLF9NagJSc\n"
            "B8OmKiVbTl9il78FrEc+XOVXSrbmZQHJHoN0tc8Fr96fOXvnJkBJ/3GU0D0QZp6z\n"
            "SLRJB5ez5uVCt7ahbQD80Iizbhfg6yzFLJWEIjZpBJ7k4lMqfq1zbT2N9OG7QOSY\n"
            "HMxCzghCFRNwN0md6oPEwQbFyKmaANwI0oRDtRWiUGEg/C9jFXozpBMbHjrYH3NB\n"
            "IodGXyfW2YD4cxHjcQCScz/9We7KTZiwhTX7dOn5ndejq4YekMCXPmOv++72foQD\n"
            "AqO0TEZVOBKkplfvRlQmRsZpcqSpb+a01SvxlnF61TcBlQg9NxUy4o0ER774kmTe\n"
            "Q91Wkemy55IDAoIBAQDToHCByD0CPkdurgvvNA5bsHw5mZ5ab/jiCEk/5fv/2Gke\n"
            "pc1phBa1A1NEB9bcedV4gZfhS06iYa/FnTyTdDgbnp8dufPbm3UOSXnwL2JP/PHj\n"
            "gg1smEUQ6fXcOZ7sbQEPgT9PiClltXYmrXMmJEvHVndMEmD1UPW2hlL2T4JLlSgi\n"
            "mg134hJeDmvu0KfIGd4+nz5dkm2MNayFqm0ehmYwRcRWSJrmGflpvKOpCuVTMnCx\n"
            "jQhpjlcY5TYA/mxUwikl0peOcPFA7ouRIETyJCDUi5F2nIx5ZvpbXEez3zk1v04e\n"
            "pJS8XmnvqPFfJ/bM7H0WkSFjFHw0XG2xNBM1wbJBAoIBAF55KNISug6S9goX1OTS\n"
            "YkCkwjFLYKC76dtfYBFwrxH1ZcUmxbSpiO+cd+yguIszjoxhCebVNcHEckj+hS0k\n"
            "nUUZ3JTe9fSktNJyxhzUiTD3el3K5HCZu6hRN8quvtTQ3i9o2JCYsT2pN8NjwCet\n"
            "UiuDLHWPH9ioyhO7Mln3SGO6OdidJgEpTuVfKlP3K69WBaU5vLnId363JyIvJYMm\n"
            "Dn6EWK/qYw+9GOkrqo7k5cBHV0MvsZ9yM1tpcKxLPaudVBYLJa/4TkRwN+KpEJaX\n"
            "zORWlNUza/WaOrXWpI5FBZbdCuIIz6UKBdpwjzKaqvvOszZogYdz4gQaoZ2hpoqY\n"
            "ei0CggEBAN/W5UO+T9yIl+2Xl4EwAUj1KRiIiio4V4rTkVCz8nQAa7nlFtnpX2Jt\n"
            "ns9/H8iVFX1ApM5Db996zLT2mth9XMNFEqEadzhSV4r08m8VspC11sflNj+eXZE+\n"
            "WEpfk+3MNflB5LegvMWlD+PQjFduRAVrrviNykCzR9O0b3nTrx6O0EGFfpdP9bfV\n"
            "RcNo78aRQh82nvEpuT2YhxZ28250pRLkdi+FttiQfHzOi/S2182s7buefMArKofO\n"
            "DxSfbqhTaO8oXPYBam/6jk8C8ZvCBs456UgPjjPFjRG0usxu/QVjuOANVxbroNFA\n"
            "DRMLOtoSnbQfw0zw+klnV3AzPyVhiK4=\n"
            "-----END PRIVATE KEY-----",
        .certificate =
            "-----BEGIN CERTIFICATE-----\n"
            "MIIE8TCCAtmgAwIBAgIBATANBgkqhkiG9w0BAQQFADAAMB4XDTIxMDUxNzExMDkx\n"
            "N1oXDTIyMDUxNzExMDkxN1owIzELMAkGA1UEBhMCS1IxFDASBgNVBAMMC3Jvb3Qu\n"
            "QmV5b25EMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAuOmS0EoIb47x\n"
            "YTP6mbei3xGJeHbU4R2fkIVby3y71TqrrlcR9Nd+GrPPfOGsXqTWCf18h+IqvSvv\n"
            "xpao6BSesAR7rS357Vwdyi5muqnUL5VXEa+rsyIp0FRCCUs6euHvXO9jA2THb+u/\n"
            "7KSBKL5O44AuoYJGSEFXfaxOrKQTGNTjV6/qGBlNFPa902xCMlLGu6E3d41yICaF\n"
            "AtFer7UZZqjZL0Hrur/T7o/mR3Prrkp49THMfnnRwwR5zKD/QuaUrtKcb7tfjjag\n"
            "mRgRCXcBgEsWCA1XU2rSMsd3hiYR7pDrQpCaX8OkoeesUSYlRSqwOu2aqzPjOUWP\n"
            "zO57M5OdmVNab3STXos7GerXs5rZxqGdVxH60zSZ+Cw2RSibVzoiqeiijZi4UsXA\n"
            "bc0xRsCOqZLcyNhQiIt1Hj7+J7qLDlMSUequGZP0gUhbhH3zjeBCoTQoAl/j1Kj9\n"
            "qUm0/gnkVZjnVdJ+oHdzpBohv53JVXkrUnW4vV2Dv4xAZIgZdnykNU+uh2P46fFv\n"
            "OPjHt/SPYuBjcc1/AeAj2shF9mR4IlZZj0SUbiO45Vwyq8ZsDRA3D4E0o8TpgO9Z\n"
            "8vIWTCKQt93k04bgM2T2QYyb4fN0c7eeC1NKWitaqODEYTYbXeZTDlL+Cwg+zIOE\n"
            "dXQ2xXp/n4Ecu7XfFyjLhby+ZBcyTDcCAwEAAaNTMFEwDwYDVR0TAQH/BAUwAwEB\n"
            "/zAOBgNVHQ8BAf8EBAMCASYwDwYDVR0RBAgwBocEfwAAATAdBgNVHQ4EFgQUxwb9\n"
            "P+Mu9Jm7kUDh8osoyacFjEswDQYJKoZIhvcNAQEEBQADggIBAJYfKw4kzRxJhYnb\n"
            "pi25HO60W/Nimnw+lAGR+g0H8xQBJwP2CBbtY/KLRKy/jAPzqY+hzr35aMUppJju\n"
            "gvTre/eLZ9t58lASbc93PJ0+Pjp93BvFh3/ozzvgwcvVUYi54P9aniPIFyijNt1V\n"
            "JSb2s5OteXtMKx0L4h/z59/eubgak9ZDZWUi5i7BG5P+tHrUYUV3mbQkHl1YMaIJ\n"
            "p0Muu0wczndj5PZd8AQvy+3fANdM+4VD0z565Bb1PdxjlLXqzYu73SZFkRjHhXNw\n"
            "vFmluB6VP2Q8JBkH8FMInP2CoPlp9SlT4O4qti7grf+PCqc3cPbOeLl5dFGu7NFi\n"
            "hJQAlhN2b3XRI0a6NULMmiApJDbAbbuzWrFKsYb+iorUPCNdCew1ikLXVhfb2WvZ\n"
            "TvGYsJtb6pselB8pKnVs/FHLUwWM5xM2A/1QDPExkcTVvWlBzU67Zn+9nJxj98To\n"
            "zfUDRKNirugokYzHo01SbT4LwjgLIt48xY9uQoklrVoErp/IwszxqpxL6pvyzM1S\n"
            "wl99uGLWON7ktZXzV1MVJuyUjSrHpaBY1aoNlfO9l8akjykPiSSXGqmjNIYVtpPZ\n"
            "NmY6ygsY7UOkefHLuS13kXFGn74FxZYYjVR2HVl5oWjqR2+nePP3Xsn27FfM5QSx\n"
            "c9wlVRzUVDFshld//s+DBfezRK4g\n"
            "-----END CERTIFICATE-----",
        .alternative_name = "127.0.0.1",
    };

    // NOTE:
    // server certificate is already signed using the above rootCA's private key
    beyond_authenticator_ssl_config_ssl serverSSLConfig = {
        .bits = -1,
        .serial = -1,
        .days = -1,
        .isCA = -1,
        .enableBase64 = -1,
        .passphrase = "",
        .private_key =
            "-----BEGIN PRIVATE KEY-----\n"
            "MIIJQgIBADANBgkqhkiG9w0BAQEFAASCCSwwggkoAgEAAoICAQC9tSlRNV8CQF0Y\n"
            "apGj3uDXkB+qqoQGrwv4Yiv3pm/GVAtKBC6FGmon8il0sejRHf7/GyPdQaSc2xgl\n"
            "B7Y5q/eXlUrgOlLpJmYd7Xdh4jhrzLRyz1zuokG6nXpE9t0sLfSCnnZ3Sf/kr0AR\n"
            "xqEHM7ZnjRTbbAr50mbaZ3HkDycNDy+hCIlcBGANKY/XMO0l/XVIg/qVvypgYTNF\n"
            "iXSK7gWH0/D5S4lAuzt9TeaNgk8TCSP1CuMTg7xWrAsobii74K7UBWwqFdiozYuR\n"
            "ThgbLH9o1yzrkseIw8QeKFIFH4B1xAQcNZfOc9wjLHruibF2/4IwJbxQSIG+OHH/\n"
            "pxLx57jWqByRjWxZ5/SQL7He5hBuyImkkonGhQYGda4iRmmhdUF70s60ZtWJZonn\n"
            "7lyAu96owu0Th2vL7uMe0Xawww3ZwK/LanaF62n5xGP5vVbZsQAHcipYrAFQ04P/\n"
            "VqHDP0vu+nTx7C1RwCUeWROMpChVYG+rhyIrbYZCFa6ODxcKmNK1HVen2nUixt4j\n"
            "beXCmlXd39gPdIzhSqSYMt4kireLH4KRgqT96m737MlU9+ADBcz6RcheM80LP+t6\n"
            "L6IoDsIuyObGftR97lGObEn0Dpa+oDER1LIfmALwHKyUJAU7KxO5vRDVvXzKZLtm\n"
            "OvbU+obD8p9whtEr6nx20UVFC5r+OwIDAQABAoICACM/kvK82O9hKCsOOgtZsSs8\n"
            "YzXhwvA+/BllnEfCjAgsu4BAMKiYlNrhOuSs3dZlHWknEM3ekYh3iQ/wU+J5WmK2\n"
            "4ZeyHo+li9nJsqHkV5loCqs+bkUErvPOqNZBjCzWSRUv/lEB1eMW0O+8mVTuPdkV\n"
            "tKdkdtGeT3ALQnUef6IEjVP9cxA+2932N/zC4X1qj60uoJPMVkJcLRuhg58AxNRN\n"
            "A/w+Fb2KTG5m2Ay5BppB24V/RVvt4UO1aclUVos6HyHT9BoJvxz2PBV+jioWZYIE\n"
            "YRtQRXTa8wircznydXrsNdtL9e3tRxzd+eVyfh/fL8Bkco30Ou8uK7hVwcZiyMv/\n"
            "nngAooQCOgylE2/lXsgbn2M7ritEARok5oXhrZeG4U2QoS4vA92u3Mr5gXbZw/Gt\n"
            "e9Lp+CKrpjWqpt3pIR2dyOuvGpx4LiEIDcfBZJE3FWMM4DskNBCR943R7msW+vnf\n"
            "fKHC8s8EIcJ07HkycCEwkstmV2aSuMe7Q6qxUmD+KBVj/q8eC2um8BamoWD9M4sv\n"
            "WeqlObRBFcZo0o+uBGtcufuidMDh9vYpJYoXv81ltSoDTIqe5WVHPoCWgmV1kA07\n"
            "56eBwp00PNNSWvyrxlffrxubsqVtfDlTlFYExTtdqkodtK+dimEeQbxwQcXw5OtR\n"
            "FFWdHVYwRHiiU/bjybb5AoIBAQDpgFlqzYnfNRmtGr2/TTNbMo1MdoOd/Ye/0llN\n"
            "cGs8e1IJxITxJ1iwokqslBhfJCiUdZvADWzNcNLx2PyrEc/YFmD1Ry/ED8OzrsEt\n"
            "b3lrmFgiHDFqpR09/wOs3Dotzw6uNffvpp7tQLX0g9raBcq9CKfaTXrKnCL+3k7L\n"
            "5m/n+O+Z9iDNqmhDOFePjmpgANS9ZVAab+7zdX6vK2+AGu3gK/4YWjmnHeykvaMM\n"
            "Iw6xUBh5BHVjnwn/WHObnBRD4YE7nKpXSQhZi8vsY8s8BBqoUj4ODmZ67IdrEzoY\n"
            "uUcQW3EP3ZJIDBJxiScopMmIKuJDWGmbrjxcK/jEokzV2SV/AoIBAQDP/JOlsHrK\n"
            "VCRZQcaWXFr5eH/30XsSq3FnEgeTVlHF+ECTb5WbhW68Hhwln512TA5qb3h/d9ac\n"
            "OYbo4EWzY0SmTT0hkbv5QcG4A8nvJ0C+XC6qfclQeLa58zvAuVdsjyCehTW+SQ8j\n"
            "WlX2S76tvybIvXEDnnOqmgNr7Z4T2BGbqFr8ntQ2AdY8gqzYGL7UgWL4FZKRy9mU\n"
            "YcwdhpcCEwJYWTz6vA3EIRZhIfl/uQ/hepir2iy652JYWzEGHgCewpkTgRO8iK9b\n"
            "Kw8av/BLyuKTK2NDfcCjHjZpCK2hWvYVLU3JCge6GjGupH45vA+NoQXIF5rkQ04g\n"
            "dEFGgEFwa51FAoIBAQCNnxgcrB80Lxusnyx4y9UbOhTzTGpVt8DO+kDJtCaGX3GJ\n"
            "lRTgwvGK6FQMSiJiCidGq/JUUJAuJoD6yJWvGDWMpT4XZh61dq5G2/Y0nYjyVksW\n"
            "HS1ntk9/G53aCRSMVipcRUVkqBV6ZqY1cIebdqnZb1eHEzkni/25wZHfH5u+AYEp\n"
            "S1voAbQNGS4aVtFz+u0NFla4Qi2Woiu9CMYu16ZxMZ2Cna6cCo1N+erbYKP3rVG6\n"
            "jJa6XmqM4dP0jHzKEwrz4fh6ykPzM9PyQzCv7PlSH9edZOSJJ86WhenVtwJADIYN\n"
            "jmC7q/6/t+T1RMUq/n+PQx+CmfHoIY9Xi+y4Q2T3AoIBAH99J1Ps9ZeINC+yLfSE\n"
            "8A7zWh2h/nrXNFAlsRcTVlSvc4XsZBxMkjAllMNLL84PmNaNNaOM5bQlXxjoQFFR\n"
            "jAcUWWB2YG7Na91MFT/PI9SL1N8U842sMPWSrxHXiks1AJ2qseLODcVx3jd2/o6q\n"
            "GS/7T4cUXXo5pddGdBtd2o07iWpIQXRJc/TrdN+Ra4f/N5cyQgG0ns5hlCiVE4Nl\n"
            "+44ERWi8VQPf9EPd+33bBm0EJQlSVxDKPHJEk6xYP7ERP5vBB0QN1M9heYTAGp+a\n"
            "4X+snNAGCUrzfg6sDyJVC3q3pnKQ/2OIIuQWWHkzWaVLCqw4K+23g/BI0qpQe8xZ\n"
            "arECggEAZ378wyaNgJ5Pu8tYiWPynEodZUR6r3tTEsZybaeM16iTitJEPphWIWi9\n"
            "CU0IwN3e8FpTHzIm5tUvjKcgs1R+pPLAYrNQQW62qF6Edg2rw/CEo6FQ8chxi2Yq\n"
            "OEcUhI7yk4I80jeDPgp/jH7yeB/oqtzaUEEeKHNmqsGyaCrQC7fjriVgHV4hA2o5\n"
            "4Pai2AjhOShcdGkjhRCVje/BKpeRo/yjr4ehHOMfK4qjbEE6NTtlzU+fsvrg7n8Q\n"
            "TXnXN3lJAZyy1KLm1+AO2BBg1JLuUyWBSN2OCk8Y54yWW/YtBFG44zFlSzAI2vcw\n"
            "d7YB05okwQKvcHBr9kEsZnTE7B8LcA==\n"
            "-----END PRIVATE KEY-----",
        .certificate =
            "-----BEGIN CERTIFICATE-----\n"
            "MIIFETCCAvmgAwIBAgIBAjANBgkqhkiG9w0BAQQFADAjMQswCQYDVQQGEwJLUjEU\n"
            "MBIGA1UEAwwLcm9vdC5CZXlvbkQwHhcNMjEwNTE3MTEwOTE3WhcNMjIwNTE3MTEw\n"
            "OTE3WjAjMQswCQYDVQQGEwJLUjEUMBIGA1UEAwwLZWRnZS5CZXlvbkQwggIiMA0G\n"
            "CSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC9tSlRNV8CQF0YapGj3uDXkB+qqoQG\n"
            "rwv4Yiv3pm/GVAtKBC6FGmon8il0sejRHf7/GyPdQaSc2xglB7Y5q/eXlUrgOlLp\n"
            "JmYd7Xdh4jhrzLRyz1zuokG6nXpE9t0sLfSCnnZ3Sf/kr0ARxqEHM7ZnjRTbbAr5\n"
            "0mbaZ3HkDycNDy+hCIlcBGANKY/XMO0l/XVIg/qVvypgYTNFiXSK7gWH0/D5S4lA\n"
            "uzt9TeaNgk8TCSP1CuMTg7xWrAsobii74K7UBWwqFdiozYuRThgbLH9o1yzrkseI\n"
            "w8QeKFIFH4B1xAQcNZfOc9wjLHruibF2/4IwJbxQSIG+OHH/pxLx57jWqByRjWxZ\n"
            "5/SQL7He5hBuyImkkonGhQYGda4iRmmhdUF70s60ZtWJZonn7lyAu96owu0Th2vL\n"
            "7uMe0Xawww3ZwK/LanaF62n5xGP5vVbZsQAHcipYrAFQ04P/VqHDP0vu+nTx7C1R\n"
            "wCUeWROMpChVYG+rhyIrbYZCFa6ODxcKmNK1HVen2nUixt4jbeXCmlXd39gPdIzh\n"
            "SqSYMt4kireLH4KRgqT96m737MlU9+ADBcz6RcheM80LP+t6L6IoDsIuyObGftR9\n"
            "7lGObEn0Dpa+oDER1LIfmALwHKyUJAU7KxO5vRDVvXzKZLtmOvbU+obD8p9whtEr\n"
            "6nx20UVFC5r+OwIDAQABo1AwTjAMBgNVHRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIB\n"
            "JjAPBgNVHREECDAGhwR/AAABMB0GA1UdDgQWBBRg//UhGlXd4116KAWXjgtBZxA0\n"
            "GzANBgkqhkiG9w0BAQQFAAOCAgEAKvvppFj9ZwsTlaqyVnyy/+L3JtBuRXoVlnyJ\n"
            "0IgkeTpIof+wMKkqaSsw4emHWysT1DOPUyZIVKhCVp9OIu70fx98MykYRGbQTe1x\n"
            "C4/ViQP8RJPxwgeyfYPZFg3TlB+HrlQ5PCen3BwF3cnvuH3K6ePZoyZHmXah9S4W\n"
            "pJmq7ZSqYs8hSYQt0I3Xjg0ch2l3BEplUVvdrszDWvsDcL3roWjc9N70gyAZi0od\n"
            "VZxpk1jdjcXWtjWNHvDtF9YO4jRgKCSJE/4wUA7Qpq3jIfNz/4bYHRiV81UKCCAs\n"
            "q7ZMFwsCOfZZ6b/JuWogrTM1FD2X7rzA9f3NiaFkCtg4m6yDnEglNKvXx7RTnMuT\n"
            "/NYTlGRw7LfLMDixSQvYqhCnnkmxX2l7MIlB8KshUaNZUNgleW6xZnkadme2Uz6W\n"
            "ifx7PqHXEnENO0b1rZm12dy1RHhtuHNUO5MOBR2vK9wh1ZO+4/mG6eBchSNZvCNH\n"
            "vRxFJ+qAk+kMuB4gFhOmRlvrn27M/UFIW90PARjA3fab01H5f/91QXIQophWj9L6\n"
            "pr3vXIUTCK8aHO9Hrin/se9oHdtdVeLFtqNK4MQrYJM+RGa+7soLbzTwSfVHBOHR\n"
            "VlR74dWh8u371YiFU16maFmE09QTvGGbQ0n9kHGmPXUB3p7v+xsa224oGaQK531a\n"
            "e7xZt1s=\n"
            "-----END CERTIFICATE-----",
        .alternative_name = "127.0.0.1",
    };

    char *auth_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME),
    };

    beyond_argument args = {
        .argc = sizeof(auth_argv) / sizeof(char *),
        .argv = auth_argv,
    };

    beyond::AuthenticatorInterface *authCA;
    beyond::AuthenticatorInterface *auth;

    auth = beyond::Authenticator::Create(&args);
    ASSERT_NE(auth, nullptr);

    authCA = beyond::Authenticator::Create(&args);
    ASSERT_NE(auth, nullptr);

    beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = static_cast<void *>(&rootSSLConfig),
    };

    int ret = authCA->Configure(&config);
    ASSERT_EQ(ret, 0);

    ret = authCA->Activate();
    ASSERT_EQ(ret, 0);

    ret = authCA->Prepare();
    ASSERT_EQ(ret, 0);

    DbgPrint("Authenticator CA module is prepared");

    config.object = static_cast<void *>(&serverSSLConfig);
    ret = auth->Configure(&config);
    ASSERT_EQ(ret, 0);

    ret = auth->Activate();
    ASSERT_EQ(ret, 0);

    ret = auth->Prepare();
    ASSERT_EQ(ret, 0);

    DbgPrint("Authenticator module is prepared");

    StartGrpcServer(auth, authCA);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    args.argc = sizeof(peer_argv) / sizeof(char *);
    args.argv = peer_argv;

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();

    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSecuredGRPC_CA)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };

    beyond_argument args;
    args.argc = sizeof(peer_argv) / sizeof(char *);
    args.argv = peer_argv;

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();

    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, NegativeSecuredGRPC_CA_InvalidUUID)
{
    beyond::AuthenticatorInterface *authCA = nullptr;
    beyond::AuthenticatorInterface *auth = nullptr;

    StartGrpcServer(auth, authCA);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };

    beyond_argument args;
    args.argc = sizeof(peer_argv) / sizeof(char *);
    args.argv = peer_argv;

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_invalid_info_uuid);
    EXPECT_EQ(ret, 0);

    beyond_config config;
    config.type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR;
    config.object = static_cast<void *>(authCA);

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, -EINVAL);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, -EILSEQ);

    peer->Destroy();

    StopGrpcServer();

    auth->Deactivate();
    authCA->Deactivate();
    auth->Destroy();
    authCA->Destroy();
}

TEST_F(PeerTest, PositiveSecuredGRPC)
{
    beyond::AuthenticatorInterface *auth = nullptr;
    StartGrpcServer(auth);

    char *peer_argv[] = {
        const_cast<char *>(BEYOND_PLUGIN_PEER_NN_NAME),
    };
    beyond_argument args = {
        .argc = sizeof(peer_argv) / sizeof(char *),
        .argv = peer_argv,
    };

    optind = 0;
    opterr = 0;
    beyond::InferenceInterface::PeerInterface *peer = reinterpret_cast<beyond::InferenceInterface::PeerInterface *>(entry(args.argc, args.argv));
    ASSERT_NE(peer, nullptr);

    int ret = peer->SetInfo(&s_valid_info);
    EXPECT_EQ(ret, 0);

    beyond_config config = {
        .type = BEYOND_CONFIG_TYPE_AUTHENTICATOR,
        .object = static_cast<void *>(auth),
    };

    ret = peer->Configure(&config);
    EXPECT_EQ(ret, 0);

    ret = peer->Configure(&s_input_config);
    EXPECT_EQ(ret, 0);

    ret = peer->Activate();
    EXPECT_EQ(ret, 0);

    ret = peer->Deactivate();
    EXPECT_EQ(ret, 0);

    peer->Destroy();

    StopGrpcServer();
    auth->Deactivate();
    auth->Destroy();
}
