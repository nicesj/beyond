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
#include "tensordata_util.h"

#include "tensordata.h"

namespace beyond {
namespace evaluation {
namespace {
// TODO: tensor info can be generated by both cmdline and InferenceRunner
/*
bool TensorInfoGenerate(
	const std::string &layer_names,
    const std::string &layer_types,
    const std::string &layer_shapes,
    const std::vector<TensorInfo> &tensorinfo) {
    return true;
}

bool TensorGenerateFromRaw(
    const std::string &raw_tensor_path,
    const std::vector<TensorInfo> &tensorinfo,
    const std::vector<Tensor> &tensor)
{
    return true;
}

bool TensorDiff(
    const std::vector<TensorInfo> &tensorinfo,
    const std::vector<Tensor> &test_outputs,
    const std::vector<Tensor> &ref_outputs)
{
    return true;
}
*/
} // namespace
} // namespace evaluation
} // namespace beyond
