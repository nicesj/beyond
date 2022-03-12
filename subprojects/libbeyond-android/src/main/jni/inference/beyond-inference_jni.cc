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

#include "beyond-inference_jni.h"
#include "JNIHelper.h"
#include "beyond-peer_jni.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <map>
#include <utility>

#include <android/looper.h>

#include <beyond/platform/beyond_platform.h>

InferenceNativeInterface::InferenceNativeInterface(void)
    : inference(nullptr)
{
}

InferenceNativeInterface::~InferenceNativeInterface(void)
{
}

void *InferenceNativeInterface::GetBeyonDInstance(void)
{
    return static_cast<void *>(inference);
}

jlong InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_create(JNIEnv *env, jobject thiz, jstring inference_mode)
{
    if (env == nullptr) {
        ErrPrint("JNIEnv is nullptr.");
        return 0l;
    }

    const char *mode = env->GetStringUTFChars(inference_mode, nullptr);
    if (mode == nullptr) {
        ErrPrint("Fail to get StringUTFChars.");
        return 0l;
    }

    struct beyond_argument option = {
        .argc = 1,
        .argv = (char **)&mode,
    };

    InferenceNativeInterface *handle;
    try {
        handle = new InferenceNativeInterface();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        env->ReleaseStringUTFChars(inference_mode, mode);
        return 0l;
    }

    handle->inference = beyond::Inference::Create(&option);
    if (handle->inference == nullptr) {
        ErrPrint("\tFail to beyond::Inference::Create().");
        env->ReleaseStringUTFChars(inference_mode, mode);
        delete handle;
        handle = nullptr;
        return 0l;
    }

    env->ReleaseStringUTFChars(inference_mode, mode);

    return reinterpret_cast<long>(handle);
}

jboolean InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_addPeer(JNIEnv *env, jobject thiz, jlong inference_handle, jlong peer_handle) // TODO: Device a better idea to transfer peer_handle.
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    PeerNativeInterface *peer_handle_ = reinterpret_cast<PeerNativeInterface *>(peer_handle);
    int ret = inference_handle_->inference->AddPeer(static_cast<beyond::InferenceInterface::PeerInterface *>(peer_handle_->GetBeyonDInstance()));
    if (ret < 0) {
        ErrPrint("Fail to add a peer to an inference handler, ret = %d\n", ret);
        return false;
    }

    return true;
}

jboolean InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_loadModel(JNIEnv *env, jobject thiz, jlong inference_handle, jstring model_path)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    const char *model = env->GetStringUTFChars(model_path, nullptr);
    if (model == nullptr) {
        ErrPrint("Fail to convert a model path.");
        return false;
    }

    int ret = inference_handle_->inference->LoadModel(model);
    if (ret < 0) {
        ErrPrint("Fail to load the given model (%s), ret = %d\n", model, ret);
        env->ReleaseStringUTFChars(model_path, model);
        return false;
    }

    env->ReleaseStringUTFChars(model_path, model);

    return true;
}

jboolean InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_prepare(JNIEnv *env, jobject thiz, jlong inference_handle)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    if (inference_handle_ == nullptr) {
        ErrPrint("inference_handle_ == nullptr");
        return false;
    }

    int ret = inference_handle_->inference->Prepare();
    if (ret < 0) {
        ErrPrint("Fail to prepare a peer to an inference handler, ret = %d\n", ret);
        return false;
    }

    return true;
}

jboolean InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_run(JNIEnv *env, jobject thiz, jlong inference_handle, jlong tensors_instance, jint num_tensors)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    beyond_tensor *tensors = reinterpret_cast<beyond_tensor *>(tensors_instance);
    int ret = inference_handle_->inference->Invoke(tensors, num_tensors, nullptr);
    if (ret < 0) {
        ErrPrint("Fail to Invoke inference, ret = %d\n", ret);
        return false;
    }

    return true;
}

void InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_destroy(JNIEnv *env, jclass klass, jlong inference_handle)
{
    return;
}

int InferenceNativeInterface::RegisterInferenceNatives(JNIEnv *env)
{
    jclass klass = env->FindClass("com/samsung/android/beyond/inference/InferenceHandler");
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (klass == nullptr) {
        ErrPrint("Unable to find InferenceHandler class");
        return -EFAULT;
    }

    static JNINativeMethod inference_jni_methods[] = {
        { "nativeCreateInference", "(Ljava/lang/String;)J", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_create) },
        { "addPeer", "(JJ)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_addPeer) },
        { "loadModel", "(JLjava/lang/String;)Z", (void *)Java_com_samsung_android_beyond_InferenceHandler_loadModel },
        { "prepare", "(J)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_prepare) },
        { "run", "(JJI)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_run) },
        { "destroy", "(J)V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_destroy) },
    };

    if (env->RegisterNatives(klass, inference_jni_methods,
                             sizeof(inference_jni_methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        ErrPrint("Failed to register inference JNI methods for BeyonD Java APIs.");
        env->DeleteLocalRef(klass);
        return -EFAULT;
    }
    env->DeleteLocalRef(klass);

    return 0;
}
