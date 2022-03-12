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
#ifndef _BEYOND_TOOLS_EVALUATION_TENSORDATA_H_
#define _BEYOND_TOOLS_EVALUATION_TENSORDATA_H_

namespace beyond {
namespace evaluation {

typedef enum {
    TENSOR_TYPE_NOTYPE = 0,
    TENSOR_TYPE_INT8 = 1,
    TENSOR_TYPE_INT16 = 2,
    TENSOR_TYPE_INT32 = 3,
    TENSOR_TYPE_INT64 = 4,
    TENSOR_TYPE_UINT8 = 5,
    TENSOR_TYPE_UINT16 = 6,
    TENSOR_TYPE_UINT32 = 7,
    TENSOR_TYPE_UINT64 = 8,
    TENSOR_TYPE_FLOAT16 = 9,
    TENSOR_TYPE_FLOAT32 = 10
} TensorDataType;

typedef struct {
    TensorDataType dtype;
    void *data;
    int size; //bytes
} Tensor;

typedef struct {
    TensorDataType dtype;
    struct dimensions {
        int size;
        int data[1];
    } * dims;

    int size; //bytes
    const char *name;
} TensorInfo;

} // namespace evaluation
} // namespace beyond

#endif // _BEYOND_TOOLS_EVALUATION_TENSORDATA_H_
