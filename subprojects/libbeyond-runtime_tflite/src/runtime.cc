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

#include <cerrno>

#include <getopt.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/kernels/register.h"

#include "runtime.h"

Runtime *Runtime::Create(void)
{
    Runtime *runtime;

    try {
        runtime = new Runtime();
    } catch (std::exception &e) {
        return nullptr;
    }

    runtime->stop = false;

    return runtime;
}

const char *Runtime::GetModuleName(void) const
{
    return Runtime::NAME;
}

const char *Runtime::GetModuleType(void) const
{
    return beyond::ModuleInterface::TYPE_RUNTIME;
}

void Runtime::Destroy(void)
{
    interpreter = nullptr;
    model = nullptr;

    if (inputTensorInfo != nullptr && inputTensorInfoSize > 0) {
        while (inputTensorInfoSize-- > 0) {
            free(inputTensorInfo[inputTensorInfoSize].dims);
            inputTensorInfo[inputTensorInfoSize].dims = nullptr;
        }
        free(inputTensorInfo);
        inputTensorInfo = nullptr;
    }

    if (outputTensorInfo != nullptr && outputTensorInfoSize > 0) {
        while (outputTensorInfoSize-- > 0) {
            free(outputTensorInfo[outputTensorInfoSize].dims);
            outputTensorInfo[outputTensorInfoSize].dims = nullptr;
        }
        free(outputTensorInfo);
        outputTensorInfo = nullptr;
    }

    delete this;
}

int Runtime::Configure(const beyond_config *options)
{
    // TODO:
    // nnapi, edgetpu and several kinds of delegators could be configured using this method
    return 0;
}

int Runtime::LoadModel(const char *model)
{
    if (access(model, R_OK) < 0) {
        ErrPrintCode(errno, "access");
        return -EINVAL;
    }

    this->model = tflite::FlatBufferModel::BuildFromFile(model);
    if (this->model == nullptr) {
        ErrPrint("Failed to load a model: %s", model);
        return -EIO;
    }

    TfLiteStatus status = tflite::InterpreterBuilder(*this->model, resolver)(&interpreter);
    if (status != kTfLiteOk) {
        this->model = nullptr;
        return -EFAULT;
    }

    // interpreter->UseNNAPI(false);

    // NOTE:
    // The tensor buffers should be allocated after loading a model,
    // or if there is a change on the input or the output tensor using Set{In|Out}putTensorInfo() method.
    status = interpreter->AllocateTensors();
    if (status != kTfLiteOk) {
        ErrPrint("Failed to allocate tensors: %d", static_cast<int>(status));
        return -EFAULT;
    }

    return 0;
}

beyond_tensor_type Runtime::ConvertType(int type)
{
    switch (type) {
    case kTfLiteUInt8:
        return BEYOND_TENSOR_TYPE_UINT8;
    case kTfLiteInt8:
        return BEYOND_TENSOR_TYPE_INT8;
    case kTfLiteInt16:
        return BEYOND_TENSOR_TYPE_INT16;
    case kTfLiteInt32:
        return BEYOND_TENSOR_TYPE_INT32;
        //    case kTfLiteInt64:
        //        return BEYOND_TENSOR_TYPE_INT64;
        //    case kTfLiteFloat16:
        //        return BEYOND_TENSOR_TYPE_FLOAT16;
    case kTfLiteFloat32:
        return BEYOND_TENSOR_TYPE_FLOAT32;
        //    case kTfLiteFloat64:
        //        FALLTHROUGH;
    case kTfLiteNoType:
        FALLTHROUGH;
    case kTfLiteBool:
        FALLTHROUGH;
    case kTfLiteString:
        FALLTHROUGH;
    case kTfLiteComplex64:
        FALLTHROUGH;
    default:
        ErrPrint("Unsupported type");
        break;
    }

    return BEYOND_TENSOR_TYPE_UNSUPPORTED;
}

int Runtime::GetInputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (inputTensorInfo != nullptr) {
        info = inputTensorInfo;
        size = inputTensorInfoSize;
        return 0;
    }

    inputTensorInfoSize = interpreter->inputs().size();

    inputTensorInfo = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * inputTensorInfoSize));
    if (inputTensorInfo == nullptr) {
        int eno = -errno;
        ErrPrintCode(errno, "malloc");
        return eno;
    }

    for (int i = 0; i < inputTensorInfoSize; i++) {
        int tensorIdx = interpreter->inputs()[i];
        TfLiteTensor *tensorPtr = interpreter->tensor(tensorIdx);

        inputTensorInfo[i].type = Runtime::ConvertType(tensorPtr->type);
        if (inputTensorInfo[i].type == BEYOND_TENSOR_TYPE_UNSUPPORTED) {
            while (i-- > 0) {
                free(inputTensorInfo[i].dims);
                inputTensorInfo[i].dims = nullptr;
            }
            free(inputTensorInfo);
            inputTensorInfo = nullptr;
            return -ENOTSUP;
        }

        inputTensorInfo[i].name = nullptr;
        inputTensorInfo[i].size = tensorPtr->bytes;
        DbgPrint("Tensor raw data size in bytes: %ld\n", static_cast<long>(tensorPtr->bytes));
        inputTensorInfo[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(*inputTensorInfo->dims) + sizeof(int) * tensorPtr->dims->size));
        if (inputTensorInfo[i].dims == nullptr) {
            int eno = -errno;
            ErrPrintCode(errno, "malloc");

            while (i-- > 0) {
                free(inputTensorInfo[i].dims);
                inputTensorInfo[i].dims = nullptr;
            }
            free(inputTensorInfo);
            inputTensorInfo = nullptr;

            return eno;
        }

        inputTensorInfo[i].dims->size = tensorPtr->dims->size;
        for (int j = 0; j < tensorPtr->dims->size; j++) {
            inputTensorInfo[i].dims->data[j] = tensorPtr->dims->data[j];
        }
    }

    info = inputTensorInfo;
    size = inputTensorInfoSize;
    return 0;
}

int Runtime::GetOutputTensorInfo(const beyond_tensor_info *&info, int &size)
{
    if (outputTensorInfo != nullptr) {
        info = outputTensorInfo;
        size = outputTensorInfoSize;
        return 0;
    }

    outputTensorInfoSize = interpreter->outputs().size();

    outputTensorInfo = static_cast<beyond_tensor_info *>(malloc(sizeof(*outputTensorInfo) * outputTensorInfoSize));
    if (outputTensorInfo == nullptr) {
        int eno = -errno;
        ErrPrintCode(errno, "malloc");
        return eno;
    }

    for (int i = 0; i < outputTensorInfoSize; i++) {
        int tensorIdx = interpreter->outputs()[i];
        TfLiteTensor *tensorPtr = interpreter->tensor(tensorIdx);

        outputTensorInfo[i].name = nullptr;
        outputTensorInfo[i].type = ConvertType(tensorPtr->type);
        if (outputTensorInfo[i].type == BEYOND_TENSOR_TYPE_UNSUPPORTED) {
            while (i-- > 0) {
                free(outputTensorInfo[i].dims);
                outputTensorInfo[i].dims = nullptr;
            }
            free(outputTensorInfo);
            outputTensorInfo = nullptr;
            return -ENOTSUP;
        }

        outputTensorInfo[i].size = tensorPtr->bytes;
        outputTensorInfo[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(*outputTensorInfo->dims) + sizeof(int) * tensorPtr->dims->size));
        if (outputTensorInfo[i].dims == nullptr) {
            int eno = -errno;
            ErrPrintCode(errno, "malloc");

            while (i-- > 0) {
                free(outputTensorInfo[i].dims);
                outputTensorInfo[i].dims = nullptr;
            }
            free(outputTensorInfo);
            outputTensorInfo = nullptr;

            return eno;
        }

        outputTensorInfo[i].dims->size = tensorPtr->dims->size;
        for (int j = 0; j < tensorPtr->dims->size; j++) {
            outputTensorInfo[i].dims->data[j] = tensorPtr->dims->data[j];
        }
    }

    info = outputTensorInfo;
    size = outputTensorInfoSize;
    return 0;
}

int Runtime::SetInputTensorInfo(const beyond_tensor_info *info, int size)
{
    // TODO:
    // implement me
    return -ENOSYS;
}

int Runtime::SetOutputTensorInfo(const beyond_tensor_info *info, int size)
{
    // TODO:
    // implement me
    return -ENOSYS;
}

int Runtime::AllocateTensor(const beyond_tensor_info *info, int size, beyond_tensor *&tensor)
{
    // NOTE:
    // Unfortunately, there is no official way to allocate tensor buffer multiple times
    // There is, however, a way to hack the allocated tensor buffer.
    // But for the first implementation in a general way,
    // we are going to use heap memory even we have to copy tensor to allocated tensor buffer of the tflite.
    beyond_tensor *_tensor = static_cast<beyond_tensor *>(malloc(sizeof(beyond_tensor) * size));
    if (_tensor == nullptr) {
        int ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    for (int i = 0; i < size; i++) {
        _tensor[i].type = info[i].type;
        _tensor[i].size = info[i].size;
        _tensor[i].data = malloc(info[i].size);
        if (_tensor[i].data == nullptr) {
            int ret = -errno;
            ErrPrintCode(errno, "malloc for tensor.data");
            while (--i >= 0) {
                free(_tensor[i].data);
                _tensor[i].data = nullptr;
            }
            free(_tensor);
            _tensor = nullptr;
            return ret;
        }
    }

    tensor = _tensor;
    return 0;
}

void Runtime::FreeTensor(beyond_tensor *&tensor, int size)
{
    for (int i = 0; i < size; i++) {
        free(tensor[i].data);
        tensor[i].data = nullptr;
    }
    free(tensor);
    tensor = nullptr;
    return;
}

int Runtime::Prepare(void)
{
    return 0;
}

/*
bool Runtime::IsCancelled(void *data)
{
    Runtime *tflite = static_cast<Runtime *>(data);
    return tflite->stop;
}
*/

int Runtime::Invoke(const beyond_tensor *input, int size, const void *context)
{
    TfLiteStatus status;
    int inputSize = interpreter->inputs().size();

    if (size != inputSize) {
        DbgPrint("Tensor dimension mismatched: %d %d\n", size, inputSize);
        return -EFAULT;
    }

    // TODO:
    // If the tensorflow-lite provides a public method to change the tensor data ptr,
    // we are able to use our beyond_tensor data ptr directly,
    // otherwise we have to hack the tensorPtr->data.raw ptr
    for (int i = 0; i < inputSize; i++) {
        int tensorIdx = interpreter->inputs()[i];
        TfLiteTensor *tensorPtr = interpreter->tensor(tensorIdx);

        if (tensorPtr->bytes != static_cast<size_t>(input[i].size)) {
            ErrPrint("Tensor size mismatched: %zu\n", tensorPtr->bytes);
            break;
        }

        memcpy(tensorPtr->data.raw, input[i].data, tensorPtr->bytes);
    }

    //    interpreter->SetCancellationFunction(static_cast<void *>(this), IsCancelled);
    status = interpreter->Invoke();
    if (status != kTfLiteOk) {
        if (stop == true) {
            return -ECANCELED;
        }

        return -EFAULT;
    }

    return 0;
}

int Runtime::GetOutput(beyond_tensor *&tensor, int &size)
{
    int ret = 0;

    size = interpreter->outputs().size();
    if (size <= 0) {
        ErrPrint("Invalid output tensor size");
        return -EFAULT;
    }

    tensor = static_cast<beyond_tensor *>(malloc(sizeof(beyond_tensor) * size));
    if (tensor == nullptr) {
        ret = -errno;
        ErrPrintCode(errno, "malloc");
        return ret;
    }

    for (int i = 0; i < size; i++) {
        int tensorIdx = interpreter->outputs()[i];
        TfLiteTensor *tensorPtr = interpreter->tensor(tensorIdx);

        tensor[i].type = ConvertType(tensorPtr->type);
        if (tensor[i].type == BEYOND_TENSOR_TYPE_UNSUPPORTED) {
            ret = -ENOTSUP;

            // Do not use --i
            // Here, the i is unsigned. so if you use --i,
            // the loop will go on forever.
            while (i-- > 0) {
                free(tensor[i].data);
            }

            free(tensor);
            tensor = nullptr;
            break;
        }

        tensor[i].size = tensorPtr->bytes;
        tensor[i].data = malloc(tensor[i].size);
        if (tensor[i].data == nullptr) {
            ret = -errno;
            ErrPrintCode(errno, "malloc %u", i);

            // Do not use --i
            // Here, the i is unsigned. so if you use --i,
            // the loop will go on forever.
            while (i-- > 0) {
                free(tensor[i].data);
            }

            free(tensor);
            tensor = nullptr;
            break;
        }

        memcpy(tensor[i].data, tensorPtr->data.raw, tensor[i].size);
    }

    return ret;
}

int Runtime::Stop(void)
{
    stop = true;
    return 0;
}

int Runtime::GetHandle(void) const
{
    return -ENOTSUP;
}

int Runtime::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Runtime::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Runtime::FetchEventData(EventObjectInterface::EventData *&data)
{
    return -ENOTSUP;
}

int Runtime::DestroyEventData(EventObjectInterface::EventData *&data)
{
    return -ENOTSUP;
}

extern "C" {

API void *_main(int argc, char *argv[])
{
    const option opts[] = {
        {
            .name = "cpu",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'c',
        },
        {
            .name = "gpu",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'g',
        },
        {
            .name = "npu",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'n',
        },
        {
            .name = "dsp",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'd',
        },
    };

    int c;
    int idx;

    while ((c = getopt_long(argc, argv, "-:cgnd", opts, &idx)) != -1) {
        switch (c) {
        case 'c':
            // accelerator CPU
            break;
        case 'g':
            // accelerator GPU
            break;
        case 'n':
            // accelerator NPU
            break;
        case 'd':
            // accelerator DSP
            break;
        default:
            break;
        }
    }

    Runtime *runtime = Runtime::Create();
    if (runtime == nullptr) {
        // TODO:
        // Unable to create a runtime module instance
    }

    return reinterpret_cast<void *>(runtime);
}
}
