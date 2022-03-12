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

#include "peer_model.h"

#include <cstdio>
#include <cerrno>

#include <memory>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

Peer::Model::Model(void)
    : inputTensorInfo(nullptr)
    , outputTensorInfo(nullptr)
    , inputTensorInfoSize(0)
    , outputTensorInfoSize(0)
    , modelPath(nullptr)
{
}

Peer::Model::~Model(void)
{
    free(modelPath);
    modelPath = nullptr;

    FreeTensorInfo(inputTensorInfo, inputTensorInfoSize);
    FreeTensorInfo(outputTensorInfo, outputTensorInfoSize);
}

int Peer::Model::SetModelPath(const char *modelPath)
{
    free(this->modelPath);

    this->modelPath = strdup(modelPath);
    if (this->modelPath == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "strdup");
        return ret;
    }

    return 0;
}

const char *Peer::Model::GetModelPath(void)
{
    return modelPath;
}

int Peer::Model::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    info = inputTensorInfo;
    size = inputTensorInfoSize;
    return 0;
}

int Peer::Model::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    info = outputTensorInfo;
    size = outputTensorInfoSize;
    return 0;
}

int Peer::Model::SetInputTensorInfo(beyond_tensor_info *info, int size)
{
    FreeTensorInfo(inputTensorInfo, inputTensorInfoSize);
    inputTensorInfo = info;
    inputTensorInfoSize = size;
    return 0;
}

int Peer::Model::SetOutputTensorInfo(beyond_tensor_info *info, int size)
{
    FreeTensorInfo(outputTensorInfo, outputTensorInfoSize);
    outputTensorInfo = info;
    outputTensorInfoSize = size;
    return 0;
}

void Peer::Model::FreeTensorInfo(beyond_tensor_info *&info, int &size)
{
    if (info == nullptr || size == 0) {
        return;
    }

    for (int i = 0; i < size; i++) {
        free(info[i].name);
        info[i].name = nullptr;

        free(info[i].dims);
        info[i].dims = nullptr;
    }
    free(info);
    info = nullptr;

    size = 0;
}

int Peer::Model::DupTensorInfo(beyond_tensor_info *&dest, const beyond_tensor_info *src, int size)
{
    beyond_tensor_info *_dest;

    _dest = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * size));
    if (_dest == nullptr) {
        ErrPrintCode(errno, "malloc");
        return -errno;
    }

    for (int i = 0; i < size; i++) {
        if (src[i].name != nullptr) {
            _dest[i].name = strdup(src[i].name);
            if (_dest[i].name == nullptr) {
                int ret = -errno;
                ErrPrintCode(errno, "strdup");
                FreeTensorInfo(_dest, i);
                return ret;
            }
        } else {
            _dest[i].name = nullptr;
        }

        _dest[i].type = src[i].type;
        _dest[i].size = src[i].size;

        _dest[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * src[i].dims->size));
        if (_dest[i].dims == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "malloc");
            free(_dest[i].name);
            _dest[i].name = nullptr;
            FreeTensorInfo(_dest, i);
            return ret;
        }

        _dest[i].dims->size = src[i].dims->size;

        for (int j = 0; j < src[i].dims->size; j++) {
            _dest[i].dims->data[j] = src[i].dims->data[j];
        }
    }

    dest = _dest;
    return 0;
}
