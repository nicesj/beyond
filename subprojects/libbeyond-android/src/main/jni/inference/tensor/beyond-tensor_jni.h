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

#ifndef __BEYOND_ANDROID_TENSOR_JNI_H__
#define __BEYOND_ANDROID_TENSOR_JNI_H__

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <jni.h>

class TensorJNI {
public:
    static int RegisterTensorNatives(JNIEnv *env);

private:
    TensorJNI(void);
    virtual ~TensorJNI(void);

    static jboolean Java_com_samsung_beyond_TensorHandler_getInputTensorsInfo(JNIEnv *env, jobject thiz, jlong inference_handle, jobject datatype_values, jobject dimensions_list);
    static jboolean Java_com_samsung_beyond_TensorHandler_getOutputTensorsInfo(JNIEnv *env, jobject thiz, jlong inference_handle, jobject datatype_values, jobject dimensions_list);
    static int GetTensorsInfo(JNIEnv *env, const beyond_tensor_info *tensors_info, int num_tensors, jobject datatype_values, jobject dimensions_list);
    static jlong Java_com_samsung_beyond_TensorHandler_allocateTensors(JNIEnv *env, jobject thiz, jlong inference_handle, jobjectArray tensor_info_array, jint num_tensors, jobjectArray buffer_array);
    static int TransformTensorsInfo(JNIEnv *env, beyond_tensor_info *&tensors_info, int num_tensors, jobjectArray tensor_info_array);
    static void FreeTensorDimensions(beyond_tensor_info *&info, int &size);
    static jlong Java_com_samsung_beyond_TensorHandler_getOutput(JNIEnv *env, jobject thiz, jlong inference_handle, jobjectArray byte_array);
    static void Java_com_samsung_beyond_TensorHandler_freeTensors(JNIEnv *env, jobject thiz, jlong inference_handle, jlong tensors_instance, jint num_tensors);

    static void CheckNull(JNIEnv *env, jobject object, const char *error_message, beyond_tensor_info *_info, int index);
};

#endif // __BEYOND_ANDROID_TENSOR_JNI_H__
