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

#ifndef __BEYOND_PRIVATE_INFERENCE_RUNTIME_H__
#define __BEYOND_PRIVATE_INFERENCE_RUNTIME_H__

#include <beyond/common.h>
#include <beyond/private/inference_private.h>
#include <beyond/private/inference_runtime_interface_private.h>

namespace beyond {

class Inference::Runtime : public InferenceInterface::RuntimeInterface {
public:
    static Runtime *Create(const beyond_argument *arg);

protected:
    Runtime(void) = default;
    virtual ~Runtime(void) = default;

private:
    class impl;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_INFERENCE_RUNTIME_H__
