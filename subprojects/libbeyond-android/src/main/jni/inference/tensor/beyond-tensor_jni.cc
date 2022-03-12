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

#include "beyond-tensor_jni.h"
#include "JNIHelper.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <map>
#include <utility>

#include <android/looper.h>

#include <beyond/platform/beyond_platform.h>
#include <inference/beyond-inference_jni.h>

#define GET_BEYOND_INFERENCE(obj) (static_cast<beyond::Inference *>((obj)->GetBeyonDInstance()))

TensorJNI::TensorJNI(void)
{
}

TensorJNI::~TensorJNI(void)
{
}

jboolean TensorJNI::Java_com_samsung_beyond_TensorHandler_getInputTensorsInfo(JNIEnv *env, jobject thiz, jlong inference_handle, jobject datatype_values, jobject dimensions_list)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    const beyond_tensor_info *input_tensors_info;
    int input_num_tensors = 0;
    int ret = GET_BEYOND_INFERENCE(inference_handle_)->GetInputTensorInfo(input_tensors_info, input_num_tensors);
    if (ret < 0) {
        ErrPrint("Fail to get input tensors information from an inference handler, ret = %d\n", ret);
        return false;
    }

    ret = getTensorsInfo(env, input_tensors_info, input_num_tensors,
                         datatype_values, dimensions_list);
    if (ret < 0) {
        ErrPrint("Fail to fill input tensors information, ret = %d\n", ret);
        return false;
    }

    return true;
}

jboolean TensorJNI::Java_com_samsung_beyond_TensorHandler_getOutputTensorsInfo(JNIEnv *env, jobject thiz, jlong inference_handle, jobject datatype_values, jobject dimensions_list)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    const beyond_tensor_info *output_tensors_info;
    int output_num_tensors = 0;
    int ret = GET_BEYOND_INFERENCE(inference_handle_)->GetOutputTensorInfo(output_tensors_info, output_num_tensors);
    if (ret < 0) {
        ErrPrint("Fail to get output tensors information from an inference handler, ret = %d\n", ret);
        return false;
    }

    ret = getTensorsInfo(env, output_tensors_info, output_num_tensors,
                         datatype_values,
                         dimensions_list);
    if (ret < 0) {
        ErrPrint("Fail to fill output tensors information, ret = %d\n", ret);
        return false;
    }

    return true;
}

int TensorJNI::getTensorsInfo(JNIEnv *env, const beyond_tensor_info *tensors_info, int num_tensors, jobject datatype_values, jobject dimensions_list)
{
    jclass list_class = env->FindClass("java/util/ArrayList");
    if (list_class == NULL) {
        ErrPrint("Fail to find java/util/List class.");
        return -EFAULT;
    }
    jmethodID list_constructor_id = env->GetMethodID(list_class, "<init>", "()V");
    if (list_constructor_id == NULL) {
        ErrPrint("Fail to find an <init> method.");
        return -EFAULT;
    }
    jmethodID add_method_id = env->GetMethodID(list_class, "add", "(Ljava/lang/Object;)Z");
    if (add_method_id == NULL) {
        ErrPrint("Fail to find an add method.");
        return -EFAULT;
    }

    jclass integer_class = env->FindClass("java/lang/Integer");
    if (integer_class == NULL) {
        ErrPrint("Fail to find java/util/Integer class.");
        return -EFAULT;
    }
    jmethodID integer_constructor_id = env->GetMethodID(integer_class, "<init>", "(I)V");
    if (integer_constructor_id == NULL) {
        ErrPrint("Fail to find an <init> method.");
        return -EFAULT;
    }

    jboolean result;
    for (int i = 0; i < num_tensors; i++) {
        int type_value = tensors_info[i].type;
        jobject datatype_value = env->NewObject(integer_class, integer_constructor_id, type_value);
        if (datatype_value == NULL) {
            ErrPrint("Fail to create an Integer instance.");
            return -EFAULT;
        }
        result = env->CallBooleanMethod(datatype_values, add_method_id, datatype_value);
        if (result == JNI_FALSE) {
            ErrPrint("Fail to add a data type value.");
            return -EFAULT;
        }

        jobject list = env->NewObject(list_class, list_constructor_id);
        if (list == NULL) {
            ErrPrint("Fail to create an ArrayList instance.");
            return -EFAULT;
        }
        int num_dimensions = tensors_info[i].dims->size;
        for (int j = 0; j < num_dimensions; j++) {
            int dimension_value = tensors_info[i].dims->data[j];
            jobject dimension_object = env->NewObject(integer_class, integer_constructor_id,
                                                      dimension_value);
            if (dimension_object == NULL) {
                ErrPrint("Fail to create an Integer instance.");
                return -EFAULT;
            }
            result = env->CallBooleanMethod(list, add_method_id, dimension_object);
            if (result == JNI_FALSE) {
                ErrPrint("Fail to add a dimension value.");
                return -EFAULT;
            }
        }
        result = env->CallBooleanMethod(dimensions_list, add_method_id, list);
        if (result == JNI_FALSE) {
            ErrPrint("Fail to add a dimension list.");
            return -EFAULT;
        }
    }

    return 0;
}

jlong TensorJNI::Java_com_samsung_beyond_TensorHandler_allocateTensors(JNIEnv *env, jobject thiz, jlong inference_handle, jobject datatype_values, jintArray data_sizes, jobject dimensions_list, jint num_tensors, jobjectArray buffer_array)
{
    beyond_tensor_info *tensors_info = nullptr;
    int ret = transformTensorsInfo(env, tensors_info, num_tensors,
                                   datatype_values, data_sizes,
                                   dimensions_list);
    if (ret < 0) {
        ErrPrint("Fail to transform the information of tensors, ret = %d\n", ret);
        return 0;
    }

    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    beyond_tensor *tensors;
    ret = GET_BEYOND_INFERENCE(inference_handle_)->AllocateTensor(tensors_info, num_tensors, tensors);
    if (ret < 0) {
        ErrPrint("Fail to allocate the buffers of tensors, ret = %d\n", ret);
        return 0;
    }
    for (int i = 0; i < num_tensors; i++) {
        jobject bytebuffer = env->NewDirectByteBuffer(tensors[i].data, tensors_info[i].size);
        if (bytebuffer == NULL) {
            ErrPrint("Fail to NewDirectByteBuffer().");
            GET_BEYOND_INFERENCE(inference_handle_)->FreeTensor(tensors, num_tensors);
            tensors = nullptr;
            return 0;
        }
        env->SetObjectArrayElement(buffer_array, i, bytebuffer);
    }

    return reinterpret_cast<jlong>(tensors);
}

int TensorJNI::transformTensorsInfo(JNIEnv *env, beyond_tensor_info *&tensors_info, int num_tensors, jobject datatype_values, jintArray data_sizes, jobject dimensions_list)
{
    jclass list_class = env->FindClass("java/util/ArrayList");
    if (list_class == NULL) {
        ErrPrint("Fail to find java/util/List class.");
        return -EFAULT;
    }
    jmethodID get_method_id = env->GetMethodID(list_class, "get", "(I)Ljava/lang/Object;");
    if (get_method_id == NULL) {
        ErrPrint("Fail to find a get method.");
        return -EFAULT;
    }
    jmethodID size_method_id = env->GetMethodID(list_class, "size", "()I");
    if (size_method_id == NULL) {
        ErrPrint("Fail to find a size method.");
        return -EFAULT;
    }

    jclass integer_class = env->FindClass("java/lang/Integer");
    if (integer_class == NULL) {
        ErrPrint("Fail to find java/lang/Integer class.");
        return -EFAULT;
    }
    jmethodID intValue_method_id = env->GetMethodID(integer_class, "intValue", "()I");
    if (intValue_method_id == NULL) {
        ErrPrint("Fail to find an intValue method.");
        return -EFAULT;
    }

    beyond_tensor_info *_info = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * num_tensors));
    if (_info == nullptr) {
        ErrPrint("Fail to allocate beyond_tensor_infos.");
        return -EFAULT;
    }
    jint *datasize_values = env->GetIntArrayElements(data_sizes, 0);
    for (int i = 0; i < num_tensors; i++) {
        jobject integer_object = env->CallObjectMethod(datatype_values, get_method_id, i);
        if (integer_object == nullptr) {
            ErrPrint("Fail to get an integerObject.");
            freeTensorDimensions(_info, i);
            return -EFAULT;
        }
        jint type = env->CallIntMethod(integer_object, intValue_method_id);
        _info[i].type = static_cast<beyond_tensor_type>(type);

        _info[i].size = datasize_values[i];

        jobject dimension_list_object = env->CallObjectMethod(dimensions_list, get_method_id, i);
        if (dimension_list_object == nullptr) {
            ErrPrint("Fail to get a dimension_list_object.");
            freeTensorDimensions(_info, i);
            return -EFAULT;
        }
        jobject dimension_object = env->CallObjectMethod(dimension_list_object, get_method_id, i);
        if (dimension_object == nullptr) {
            ErrPrint("Fail to get a dimension_object.");
            freeTensorDimensions(_info, i);
            return -EFAULT;
        }
        jint dimension = env->CallIntMethod(dimension_object, intValue_method_id);
        jint rank = env->CallIntMethod(dimension_list_object, size_method_id);
        _info[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * rank));
        if (_info[i].dims == nullptr) {
            ErrPrint("Fail to allocate dimensions.");
            freeTensorDimensions(_info, i);
            return -EFAULT;
        }
        _info[i].dims->size = rank;
        for (int j = 0; j < rank; j++) {
            _info[i].dims->data[j] = dimension;
        }
    }
    tensors_info = _info;

    return 0;
}

void TensorJNI::freeTensorDimensions(beyond_tensor_info *&info, int &size)
{
    if (info == nullptr || size == 0) {
        return;
    }

    for (int i = 0; i < size; i++) {
        free(info[i].dims);
        info[i].dims = nullptr;
    }
    free(info);
    info = nullptr;

    size = 0;
}

jlong TensorJNI::Java_com_samsung_beyond_TensorHandler_getOutput(JNIEnv *env, jobject thiz, jlong inference_handle, jobjectArray buffer_array, jint num_tensors)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    beyond_tensor *tensors;
    int ret = GET_BEYOND_INFERENCE(inference_handle_)->GetOutput(tensors, num_tensors);
    if (ret < 0) {
        ErrPrint("Fail to GetOutput, ret = %d\n", ret);
        return 0;
    }

    for (int i = 0; i < num_tensors; i++) {
        jobject byte_buffer = env->NewDirectByteBuffer(tensors[i].data, tensors[i].size);
        if (byte_buffer == NULL) {
            ErrPrint("Fail to NewDirectByteBuffer().");
            return 0;
        }
        env->SetObjectArrayElement(buffer_array, i, byte_buffer);
    }

    return reinterpret_cast<jlong>(tensors);
}

void TensorJNI::Java_com_samsung_beyond_TensorHandler_freeTensors(JNIEnv *env, jobject thiz, jlong inference_handle, jlong tensors_instance, jint num_tensors)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    beyond_tensor *tensor = reinterpret_cast<beyond_tensor *>(tensors_instance);
    GET_BEYOND_INFERENCE(inference_handle_)->FreeTensor(tensor, num_tensors);
}

int TensorJNI::RegisterTensorNatives(JNIEnv *env)
{
    jclass klass = env->FindClass("com/samsung/android/beyond/inference/tensor/TensorHandler");
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (klass == nullptr) {
        ErrPrint("Unable to find TensorContainer class");
        return -EFAULT;
    }

    static JNINativeMethod tensor_jni_methods[] = {
        { "getInputTensorsInfo", "(JLjava/util/List;Ljava/util/List;)Z", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_getInputTensorsInfo) },
        { "getOutputTensorsInfo", "(JLjava/util/List;Ljava/util/List;)Z", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_getOutputTensorsInfo) },
        { "allocateTensors", "(JLjava/util/List;[ILjava/util/List;I[Ljava/nio/ByteBuffer;)J", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_allocateTensors) },
        { "getOutput", "(J[Ljava/nio/ByteBuffer;I)J", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_getOutput) },
        { "freeTensors", "(JJI)V", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_freeTensors) },
    };

    if (env->RegisterNatives(klass, tensor_jni_methods,
                             sizeof(tensor_jni_methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        ErrPrint("Failed to register tensor JNI methods for BeyonD Java APIs.");
        env->DeleteLocalRef(klass);
        return -EFAULT;
    }
    env->DeleteLocalRef(klass);

    return 0;
}
