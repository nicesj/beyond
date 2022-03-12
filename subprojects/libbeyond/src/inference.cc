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
#include <cerrno>

#include <exception>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"
#include "beyond/common.h"
#include "beyond/private/module_interface_private.h"
#include "beyond/private/inference_interface_private.h"
#include "beyond/private/inference_private.h"

#include "inference_impl.h"

namespace beyond {

Inference *Inference::Create(const beyond_argument *arg)
{
    return static_cast<Inference *>(Inference::impl::Create(arg));
}

const char *Inference::TensorTypeToString(beyond_tensor_type type)
{
    switch (type) {
    case BEYOND_TENSOR_TYPE_INT8:
        return "int8";
    case BEYOND_TENSOR_TYPE_INT16:
        return "int16";
    case BEYOND_TENSOR_TYPE_INT32:
        return "int32";
    case BEYOND_TENSOR_TYPE_INT64:
        return "int64";
    case BEYOND_TENSOR_TYPE_UINT8:
        return "uint8";
    case BEYOND_TENSOR_TYPE_UINT16:
        return "uint16";
    case BEYOND_TENSOR_TYPE_UINT32:
        return "uint32";
    case BEYOND_TENSOR_TYPE_UINT64:
        return "uint64";
    case BEYOND_TENSOR_TYPE_FLOAT16:
        return "float16";
    case BEYOND_TENSOR_TYPE_FLOAT32:
        return "float32";
    default:
        ErrPrint("Unable to determine the type!");
        return "error";
    }

    // NOTE:
    // Cannot reach to here!
}

int Inference::TensorTypeToSize(beyond_tensor_type type)
{
    switch (type) {
    case BEYOND_TENSOR_TYPE_INT8:
    case BEYOND_TENSOR_TYPE_UINT8:
        return 1;
    case BEYOND_TENSOR_TYPE_INT16:
    case BEYOND_TENSOR_TYPE_UINT16:
    case BEYOND_TENSOR_TYPE_FLOAT16:
        return 2;
    case BEYOND_TENSOR_TYPE_INT32:
    case BEYOND_TENSOR_TYPE_UINT32:
    case BEYOND_TENSOR_TYPE_FLOAT32:
        return 4;
    case BEYOND_TENSOR_TYPE_INT64:
    case BEYOND_TENSOR_TYPE_UINT64:
        return 8;
    default:
        ErrPrint("Unable to determine the type!");
        return 0;
    }

    // NOTE:
    // Cannot reach to here!
}

beyond_tensor_type Inference::TensorTypeStringToType(const char *str)
{
    if (strcmp(str, "int8") == 0) {
        return BEYOND_TENSOR_TYPE_INT8;
    } else if (strcmp(str, "int16") == 0) {
        return BEYOND_TENSOR_TYPE_INT16;
    } else if (strcmp(str, "int32") == 0) {
        return BEYOND_TENSOR_TYPE_INT32;
    } else if (strcmp(str, "int64") == 0) {
        return BEYOND_TENSOR_TYPE_INT64;
    } else if (strcmp(str, "uint8") == 0) {
        return BEYOND_TENSOR_TYPE_UINT8;
    } else if (strcmp(str, "uint16") == 0) {
        return BEYOND_TENSOR_TYPE_UINT16;
    } else if (strcmp(str, "uint32") == 0) {
        return BEYOND_TENSOR_TYPE_UINT32;
    } else if (strcmp(str, "float16") == 0) {
        return BEYOND_TENSOR_TYPE_FLOAT16;
    } else if (strcmp(str, "float32") == 0) {
        return BEYOND_TENSOR_TYPE_FLOAT32;
    }

    return BEYOND_TENSOR_TYPE_UNSUPPORTED;
}

} // namespace beyond
