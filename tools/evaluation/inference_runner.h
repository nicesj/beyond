/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _BEYOND_TOOLS_EVALUATION_INFERENCE_RUNNER_H_
#define _BEYOND_TOOLS_EVALUATION_INFERENCE_RUNNER_H_

#include <string>
#include <vector>

#include "tensordata.h"

namespace beyond {
namespace evaluation {

class InferenceRunner {
public:
    InferenceRunner()
    {
    }
    virtual ~InferenceRunner()
    {
    }

    virtual bool LoadModel(const std::string &file_path) = 0;

    void SetInputs(const std::vector<Tensor *> &inputs_ptrs)
    {
        inputs_ = &inputs_ptrs;
    }
    const std::vector<Tensor *> *GetOutputs() const
    {
        return &outputs_;
    }

    // i guess most runtime feed tensor info by themself after loading a model, but
    // below members may need
    // virtual void SetInputTensorInfo(TensorInfo* info) = 0;
    // virtual void SetOutputTensorInfo(TensorInfo* info) = 0;
    // virtual void GetInputTensorInfo(TensorInfo* info) = 0;
    // virtual void GetOutputTensorInfo(TensorInfo* info) = 0;

    virtual bool Invoke() = 0;

    bool IsValid() const
    {
        return errors_.empty();
    }
    const std::string &GetErrors() const
    {
        return errors_;
    }

private:
    std::string model_path_;
    std::string errors_;
    const std::vector<Tensor *> *inputs_ = nullptr;
    std::vector<Tensor *> outputs_;
};

} // namespace evaluation
} // namespace beyond

#endif // _BEYOND_TOOLS_EVALUATION_INFERENCE_RUNNER_H_
