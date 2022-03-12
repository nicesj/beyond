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

#ifndef __BEYOND_ANDROID_INFERENCE_JNI_H__
#define __BEYOND_ANDROID_INFERENCE_JNI_H__

#include "NativeInterface.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <jni.h>

class InferenceNativeInterface : public NativeInterface {
public:
    void *GetBeyonDInstance(void) override;

public:
    static int RegisterInferenceNatives(JNIEnv *env);

private:
    InferenceNativeInterface(void);
    virtual ~InferenceNativeInterface(void);

    static jlong Java_com_samsung_android_beyond_InferenceHandler_create(JNIEnv *env, jobject thiz, jstring inference_mode);
    static void Java_com_samsung_android_beyond_InferenceHandler_destroy(JNIEnv *env, jclass klass, jlong inference_handle);
    static jboolean Java_com_samsung_android_beyond_InferenceHandler_addPeer(JNIEnv *env, jobject thiz, jlong inference_handle, jlong peer_handle);
    static jboolean Java_com_samsung_android_beyond_InferenceHandler_loadModel(JNIEnv *env, jobject thiz, jlong inference_handle, jstring model_path);
    static jboolean Java_com_samsung_android_beyond_InferenceHandler_prepare(JNIEnv *env, jobject thiz, jlong inference_handle);
    static jboolean Java_com_samsung_android_beyond_InferenceHandler_run(JNIEnv *env, jobject thiz, jlong inference_handle, jlong tensors_instance, jint num_tensors);

private:
    beyond::Inference *inference;
};

#endif // __BEYOND_ANDROID_INFERENCE_JNI_H__
