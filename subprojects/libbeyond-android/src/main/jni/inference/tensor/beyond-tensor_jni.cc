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

#include <beyond/platform/beyond_platform.h>
#include <inference/beyond-inference_jni.h>

#define GET_BEYOND_INFERENCE(obj) (static_cast<beyond::Inference *>((obj)->GetBeyonDInstance()))

#define JAVA_UTIL_ARRAYLIST_CLASS "java/util/ArrayList"
#define JAVA_LANG_INTEGER_CLASS "java/lang/Integer"
#define JAVA_CLASS_CONSTRUCTOR "<init>"

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

    ret = GetTensorsInfo(env, input_tensors_info, input_num_tensors,
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

    ret = GetTensorsInfo(env, output_tensors_info, output_num_tensors,
                         datatype_values,
                         dimensions_list);
    if (ret < 0) {
        ErrPrint("Fail to fill output tensors information, ret = %d\n", ret);
        return false;
    }

    return true;
}

int TensorJNI::GetTensorsInfo(JNIEnv *env, const beyond_tensor_info *tensors_info, int num_tensors, jobject datatype_values, jobject dimensions_list)
{
    jclass list_class = env->FindClass(JAVA_UTIL_ARRAYLIST_CLASS);
    if (list_class == nullptr) {
        ErrPrint("Fail to find %s class.", JAVA_UTIL_ARRAYLIST_CLASS);
        return -EFAULT;
    }
    jmethodID list_constructor_id = env->GetMethodID(list_class, JAVA_CLASS_CONSTRUCTOR, "()V");
    if (list_constructor_id == nullptr) {
        ErrPrint("Fail to find an <init> method.");
        return -EFAULT;
    }
    jmethodID add_method_id = env->GetMethodID(list_class, "add", "(Ljava/lang/Object;)Z");
    if (add_method_id == nullptr) {
        ErrPrint("Fail to find an add method.");
        return -EFAULT;
    }

    jclass integer_class = env->FindClass(JAVA_LANG_INTEGER_CLASS);
    if (integer_class == nullptr) {
        ErrPrint("Fail to find %s class.", JAVA_LANG_INTEGER_CLASS);
        return -EFAULT;
    }
    jmethodID integer_constructor_id = env->GetMethodID(integer_class, JAVA_CLASS_CONSTRUCTOR, "(I)V");
    if (integer_constructor_id == nullptr) {
        ErrPrint("Fail to find an <init> method.");
        return -EFAULT;
    }

    jboolean result;
    for (int i = 0; i < num_tensors; i++) {
        int type_value = tensors_info[i].type;
        jobject datatype_value = env->NewObject(integer_class, integer_constructor_id, type_value);
        if (datatype_value == nullptr) {
            ErrPrint("Fail to create an Integer instance.");
            return -EFAULT;
        }
        result = env->CallBooleanMethod(datatype_values, add_method_id, datatype_value);
        if (result == JNI_FALSE) {
            ErrPrint("Fail to add a data type value.");
            return -EFAULT;
        }

        jobject list = env->NewObject(list_class, list_constructor_id);
        if (list == nullptr) {
            ErrPrint("Fail to create an ArrayList instance.");
            return -EFAULT;
        }
        int num_dimensions = tensors_info[i].dims->size;
        for (int j = 0; j < num_dimensions; j++) {
            int dimension_value = tensors_info[i].dims->data[j];
            jobject dimension_object = env->NewObject(integer_class, integer_constructor_id,
                                                      dimension_value);
            if (dimension_object == nullptr) {
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

jlong TensorJNI::Java_com_samsung_beyond_TensorHandler_allocateTensors(JNIEnv *env, jobject thiz, jlong inference_handle, jobjectArray tensor_info_array, jint num_tensors, jobjectArray buffer_array)
{
    beyond_tensor_info *tensors_info = nullptr;
    int ret = TransformTensorsInfo(env, tensors_info, num_tensors,
                                   tensor_info_array);
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
        if (bytebuffer == nullptr) {
            ErrPrint("Fail to NewDirectByteBuffer().");
            if (env->ExceptionCheck() == true) {
                JNIPrintException(env);
            }
            GET_BEYOND_INFERENCE(inference_handle_)->FreeTensor(tensors, num_tensors);
            tensors = nullptr;
            for (int j = 0; j < i; j++) {
                env->DeleteLocalRef(env->GetObjectArrayElement(buffer_array, i));
                if (env->ExceptionCheck() == true) {
                    JNIPrintException(env);
                }
            }
            return 0;
        }
        env->SetObjectArrayElement(buffer_array, i, bytebuffer);
        env->DeleteLocalRef(bytebuffer);
    }

    return reinterpret_cast<jlong>(tensors);
}

int TensorJNI::TransformTensorsInfo(JNIEnv *env, beyond_tensor_info *&tensors_info, int num_tensors, jobjectArray tensor_info_array)
{
    beyond_tensor_info *_info = static_cast<beyond_tensor_info *>(malloc(sizeof(beyond_tensor_info) * num_tensors));
    if (_info == nullptr) {
        ErrPrint("Fail to allocate beyond_tensor_infos.");
        return -EFAULT;
    }

    try {
        jclass tensor_info_list_class = env->FindClass("com/samsung/android/beyond/inference/tensor/TensorInfo");
        if (tensor_info_list_class == nullptr) {
            ErrPrint("jclass \"com/samsung/android/beyond/inference/tensor/TensorInfo\" is null.");
            JNIHelper::checkEnvException(env);
        }
        jmethodID get_data_type_value_method_id = JNIHelper::GetMethodID(env,
                                                                         tensor_info_list_class,
                                                                         "getDataTypeValue", "()I");
        jmethodID get_rank_method_id = JNIHelper::GetMethodID(env, tensor_info_list_class,
                                                              "getRank", "()I");
        jmethodID get_dimensions_method_id = JNIHelper::GetMethodID(env, tensor_info_list_class,
                                                                    "getDimensions",
                                                                    "()[I");
        jmethodID get_data_byte_size_method_id = JNIHelper::GetMethodID(env, tensor_info_list_class,
                                                                        "getDataByteSize", "()I");

        for (int i = 0; i < num_tensors; i++) {
            jobject tensor_info_object = env->GetObjectArrayElement(tensor_info_array, i);
            CheckNull(env, tensor_info_object, "Fail to get a tensor_info_object.", _info, i);

            jint type = env->CallIntMethod(tensor_info_object, get_data_type_value_method_id);
            JNIHelper::checkEnvException(env);
            _info[i].type = static_cast<beyond_tensor_type>(type);

            jint data_byte_size = env->CallIntMethod(tensor_info_object, get_data_byte_size_method_id);
            JNIHelper::checkEnvException(env);
            _info[i].size = data_byte_size;

            jintArray dimensions = (jintArray)(env->CallObjectMethod(tensor_info_object, get_dimensions_method_id));
            JNIHelper::checkEnvException(env);
            jint *dimension = env->GetIntArrayElements(dimensions, NULL);
            JNIHelper::checkEnvException(env);
            jint rank = env->CallIntMethod(tensor_info_object, get_rank_method_id);
            JNIHelper::checkEnvException(env);
            _info[i].dims = static_cast<beyond_tensor_info::dimensions *>(malloc(sizeof(beyond_tensor_info::dimensions) + sizeof(int) * rank));
            if (_info[i].dims == nullptr) {
                ErrPrint("Fail to allocate dimensions.");
                env->ReleaseIntArrayElements(dimensions, dimension, 0);
                env->DeleteLocalRef(tensor_info_object);
                FreeTensorDimensions(_info, i);
                return -EFAULT;
            }
            _info[i].dims->size = rank;
            for (int j = 0; j < rank; j++) {
                _info[i].dims->data[j] = (int)(dimension[j]);
            }

            env->ReleaseIntArrayElements(dimensions, dimension, 0);
            env->DeleteLocalRef(tensor_info_object);
        }
        env->DeleteLocalRef(tensor_info_list_class);
    } catch (std::exception &e) {
        return -EFAULT;
    }

    tensors_info = _info;

    return 0;
}

void TensorJNI::CheckNull(JNIEnv *env, jobject object, const char *error_message, beyond_tensor_info *_info, int index)
{
    if (object == nullptr) {
        ErrPrint("%s", error_message);
        FreeTensorDimensions(_info, index);
        JNIHelper::checkEnvException(env);
    }
}

void TensorJNI::FreeTensorDimensions(beyond_tensor_info *&info, int &size)
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

jlong TensorJNI::Java_com_samsung_beyond_TensorHandler_getOutput(JNIEnv *env, jobject thiz, jlong inference_handle, jobjectArray buffer_array)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    beyond_tensor *tensors;
    int num_tensors = 0;
    int ret = GET_BEYOND_INFERENCE(inference_handle_)->GetOutput(tensors, num_tensors);
    if (ret < 0) {
        ErrPrint("Fail to GetOutput, ret = %d\n", ret);
        return 0;
    }

    for (int i = 0; i < num_tensors; i++) {
        jobject byte_buffer = env->NewDirectByteBuffer(tensors[i].data, tensors[i].size);
        if (byte_buffer == nullptr) {
            ErrPrint("Fail to NewDirectByteBuffer().");
            if (env->ExceptionCheck() == true) {
                JNIPrintException(env);
            }
            GET_BEYOND_INFERENCE(inference_handle_)->FreeTensor(tensors, num_tensors);
            tensors = nullptr;
            for (int j = 0; j < i; j++) {
                env->DeleteLocalRef(env->GetObjectArrayElement(buffer_array, i));
                if (env->ExceptionCheck() == true) {
                    JNIPrintException(env);
                }
            }
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
        { "allocateTensors", "(J[Lcom/samsung/android/beyond/inference/tensor/TensorInfo;I[Ljava/nio/ByteBuffer;)J", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_allocateTensors) },
        { "getOutput", "(J[Ljava/nio/ByteBuffer;)J", reinterpret_cast<void *>(Java_com_samsung_beyond_TensorHandler_getOutput) },
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
