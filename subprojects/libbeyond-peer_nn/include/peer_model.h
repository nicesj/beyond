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

#ifndef __BEYOND_PEER_NN_PEER_MODEL_H__
#define __BEYOND_PEER_NN_PEER_MODEL_H__

#include "peer.h"

class Peer::Model final {
public:
    Model(void);
    virtual ~Model(void);

    int SetModelPath(const char *modelPath);
    const char *GetModelPath(void);

    int GetInputTensorInfo(const beyond_tensor_info *&info, int &size);
    int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size);
    int SetInputTensorInfo(beyond_tensor_info *info, int size);
    int SetOutputTensorInfo(beyond_tensor_info *info, int size);

    static void FreeTensorInfo(beyond_tensor_info *&info, int &size);
    static int DupTensorInfo(beyond_tensor_info *&dest, const beyond_tensor_info *src, int size);

private:
    beyond_tensor_info *inputTensorInfo;
    beyond_tensor_info *outputTensorInfo;

    int inputTensorInfoSize;
    int outputTensorInfoSize;

    char *modelPath;
};

#endif // __BEYOND_PEER_NN_PEER_MODEL_H__
