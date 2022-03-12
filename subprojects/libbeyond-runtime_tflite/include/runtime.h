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

#ifndef __BEYOND_RUNTIME_TFLITE_H__
#define __BEYOND_RUNTIME_TFLITE_H__

#include <memory>
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/kernels/register.h"

class Runtime final : public beyond::InferenceInterface::RuntimeInterface {
public:
    static constexpr const char *NAME = "runtime_tflite";
    static Runtime *Create(void);

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

public: // inference interface
    int Configure(const beyond_config *options = nullptr) override;
    int LoadModel(const char *model) override;
    int GetInputTensorInfo(const beyond_tensor_info *&info, int &size) override;
    int GetOutputTensorInfo(const beyond_tensor_info *&info, int &size) override;
    int SetInputTensorInfo(const beyond_tensor_info *info, int size) override;
    int SetOutputTensorInfo(const beyond_tensor_info *info, int size) override;
    int AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor) override;
    void FreeTensor(beyond_tensor *&tensor, int size) override;
    int Prepare(void) override;

    int Invoke(const beyond_tensor *input, int size, const void *context = nullptr) override;
    int GetOutput(beyond_tensor *&tensor, int &size) override;

    int Stop(void) override;

private:
    Runtime(void) = default;
    ~Runtime(void) = default;

    static beyond_tensor_type ConvertType(int type);
    //    static bool IsCancelled(void *data);

    std::unique_ptr<tflite::FlatBufferModel> model;
    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    beyond_tensor_info *inputTensorInfo;
    int inputTensorInfoSize;

    beyond_tensor_info *outputTensorInfo;
    int outputTensorInfoSize;

    bool stop;
};

#endif // __BEYOND_RUNTIME_TFLITE_H__
