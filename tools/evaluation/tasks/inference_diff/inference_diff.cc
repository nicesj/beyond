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
#include <string>
#include <fstream>
#include <vector>

#include "task_runner.h"
#include "task_options.h"
#include "tensordata.h"
#include "tensordata_util.h"
#include "tflite_runner.h"
#include "remote_peer_runner.h"

namespace beyond {
namespace evaluation {

// command line options
constexpr char ModelFileOpt[] = "model_file";
constexpr char InputFileOpt[] = "input";
// TODO: generate reference tensor data with given raw file
// TfliteRunner only supports tensorflow-lite CPU, so it will enable
// to support comparision with various nn backend of beyond
// constexpr char OutputExpectationFileOpt[] = "expect_output";
constexpr char EdgeIPOpt[] = "edge_ip";

// TODO: tensor info of outputs compare to the  file by given --expect_output
// e.g. layer properties
//	--output_layer: bondbox,heatmap  : name separated by comma
//	--output_type:  int64,float32 : data type separated by comma
//	--output_shape: {1,56,56},{1,56,56,21} : shape separated by comma and brace

class InferenceOutputDiff : public TaskRunner {
public:
    InferenceOutputDiff()
        : edge_ip_("127.0.0.1")
    {
    }

    ~InferenceOutputDiff() override
    {
    }

protected:
    std::vector<Option> GetOptions() final;

    int RunImpl() final;

private:
    bool CheckCmdline();
    bool Run();

    std::string model_file_path_;
    std::string input_tensor_path_;
    std::string edge_ip_;

    std::vector<const TensorInfo *> inputs_info_;
    std::vector<const TensorInfo *> outputs_info_;

    std::unique_ptr<InferenceRunner> beyond_runner_;
    std::unique_ptr<InferenceRunner> reference_runner_;
};

std::vector<Option> InferenceOutputDiff::GetOptions()
{
    std::vector<Option> opt_list = {
        Option::CreateOption(ModelFileOpt, &model_file_path_,
                             "Path to tflite model file"),
        Option::CreateOption(InputFileOpt, &input_tensor_path_,
                             "Path to input raw tensor file"),
        Option::CreateOption(EdgeIPOpt, &edge_ip_,
                             "Edge IP address (default: 127.0.0.1)"),
    };
    return opt_list;
}

int InferenceOutputDiff::RunImpl()
{
    if (CheckCmdline() == false)
        return -1;

    if (Run() == false)
        return -1;

    return 1;
}

bool InferenceOutputDiff::CheckCmdline()
{
    bool result = true;

    std::ifstream model_check(model_file_path_);
    if (model_check.good() == false) {
        printf("Incorrect Model file\n");
        result = false;
    }
    std::ifstream input_check(input_tensor_path_);
    if (input_check.good() == false) {
        printf("Incorrect Input file\n");
        result = false;
    }

    printf("Model File : %s \n", model_file_path_.c_str());
    printf("Input File : %s \n", input_tensor_path_.c_str());
    printf("Edge IP Address : %s \n", edge_ip_.c_str());

    return result;
}

bool InferenceOutputDiff::Run()
{
    /*
    std::vector<const Tensor *> inputs;
    if (beyond::evaluation::TensorGenerateFromRaw(input_tensor_path_, &inputs_info_, &inputs) == false) {
        printf("failed GenerateTensorFromRaw \n");
        return false;
    }

    beyond_runner_.reset(new RemotePeerRunner()); // TODO: configure framework, accel, edge_ip
    if (beyond_runner_->LoadModel() != true) {
        printf("failed beyond LoadModel\n");
        return false;
    }

    beyond_runner_->SetInputs(&inputs);
    if (beyond_runner_->Invoke() != false) {
        printf("failed beyond Invoke\n");
        return false;
    }
    std::vector<const Tensor *> test_outputs;
    test_outputs = beyond_runner_->GetOutputs();

    reference_runner_.reset(new TfliteRunner());
    if (reference_runner_->LoadModel() != true) {
        printf("failed tensorflow-lite LoadModel\n");
        return false;
    }
    reference_runner_->SetInputs(&inputs);
    if (reference_runner_->Invoke() != false) {
        printf("failed tensorflow-lite Invoke\n");
        return false;
    }
    std::vector<const Tensor *> expected_outputs;
    expected_outputs = reference_runner_->GetOutputs();

    if (beyond::evaluation::TensorDiff(outputs_info_, &test_outputs, &expected_outputs) == false) {
        return false;
    }
    */

    return true;
}

std::unique_ptr<TaskRunner> CreateTaskRunner()
{
    return std::unique_ptr<TaskRunner>(new InferenceOutputDiff());
}

} // namespace evaluation
} // namespace beyond
