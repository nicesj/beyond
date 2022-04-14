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
#include "beyond-peer_jni.h"
#include "JNIHelper.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <beyond/platform/beyond_platform.h>

InferenceNativeInterface::CallbackJNIContext InferenceNativeInterface::callbackInfo = {
    .jvm = nullptr,
    .callbackClass = nullptr,
    .onReceivedOutputsMethodID = nullptr,
    .byteBufferArray = nullptr,
    .getByteBufferArrayMethodID = nullptr,
    .organizeTensorSetMethodID = nullptr,
};

InferenceNativeInterface::InferenceNativeInterface(void)
    : inference(nullptr)
    , looper(nullptr)
    , outputCallbackJobject(nullptr)
{
}

InferenceNativeInterface::~InferenceNativeInterface(void)
{
    if (looper != nullptr) {
        if (inference != nullptr && inference->GetHandle() >= 0) {
            ALooper_removeFd(looper, inference->GetHandle());
        }
        ALooper_release(looper);
        looper = nullptr;
    }

    if (outputCallbackJobject != nullptr) {
        JNIEnv *env = nullptr;
        bool attached = false;
        int JNIStatus = callbackInfo.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
        if (JNIStatus == JNI_EDETACHED) {
            if (callbackInfo.jvm->AttachCurrentThread(&env, nullptr) != 0) {
                ErrPrint("Failed to attach current thread");
            } else {
                attached = true;
            }
        } else if (JNIStatus == JNI_EVERSION) {
            ErrPrint("GetEnv: Unsupported version");
        }

        if (env == nullptr) {
            ErrPrint("Failed to get the environment object");
        } else {
            DbgPrint("Global reference of a jobject is deleted.");
            env->DeleteGlobalRef(outputCallbackJobject);
            outputCallbackJobject = nullptr;
        }

        if (attached == true) {
            callbackInfo.jvm->DetachCurrentThread();
        }
    }

    if (inference != nullptr) {
        inference->Destroy();
        inference = nullptr;
    }
}

void *InferenceNativeInterface::GetBeyonDInstance(void)
{
    return static_cast<void *>(inference);
}

jlong InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_create(JNIEnv *env, jobject thiz, jstring inference_mode)
{
    if (env == nullptr) {
        ErrPrint("JNIEnv is nullptr.");
        return 0L;
    }

    const char *mode = env->GetStringUTFChars(inference_mode, nullptr);
    if (mode == nullptr) {
        ErrPrint("Fail to get StringUTFChars.");
        return 0L;
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
        return 0L;
    }

    handle->inference = beyond::Inference::Create(&option);
    env->ReleaseStringUTFChars(inference_mode, mode);
    if (handle->inference == nullptr) {
        ErrPrint("\tFail to beyond::Inference::Create().");
        delete handle;
        handle = nullptr;
        return 0L;
    }

    if (handle->inference->GetHandle() >= 0) {
        if (handle->AttachEventLoop(env) < 0) {
            ErrPrint("Fail to connect BeyonD event loop to Android looper.");
            delete handle;
            handle = nullptr;
            return 0l;
        }

        if (env->GetJavaVM(&callbackInfo.jvm) != JNI_OK) {
            ErrPrint("Unable to get javaVM.");
            env->ThrowNew(env->FindClass("java/lang/InternalError"), "Fail to get javaVM.");
            delete handle;
            handle = nullptr;
            return 0L;
        }

        try {
            jclass callbackClass = JNIHelper::FindClass(env,
                                                        "com/samsung/android/beyond/inference/TensorOutputCallback");
            callbackInfo.callbackClass = static_cast<jclass>(env->NewGlobalRef(callbackClass));

            callbackInfo.onReceivedOutputsMethodID = JNIHelper::GetMethodID(env,
                                                                            callbackInfo.callbackClass,
                                                                            "onReceivedOutputs",
                                                                            "()V");

            callbackInfo.getByteBufferArrayMethodID = JNIHelper::GetMethodID(env,
                                                                             callbackInfo.callbackClass,
                                                                             "getByteBufferArray",
                                                                             "()[Ljava/nio/ByteBuffer;");

            callbackInfo.organizeTensorSetMethodID = JNIHelper::GetMethodID(env,
                                                                            callbackInfo.callbackClass,
                                                                            "organizeTensorSet",
                                                                            "(J)V");
        } catch (std::exception &e) {
            delete handle;
            handle = nullptr;
            return 0L;
        }
    }

    return reinterpret_cast<long>(handle);
}

int InferenceNativeInterface::AttachEventLoop(JNIEnv *env)
{
    int ret = 0;
    this->looper = ALooper_forThread();
    if (this->looper == nullptr) {
        ErrPrint("Synchronous execution. Or there is no Android looper available.");
        return 0;
    }

    ALooper_acquire(this->looper);

    ret = ALooper_addFd(this->looper,
                        this->inference->GetHandle(),
                        ALOOPER_POLL_CALLBACK,
                        ALOOPER_EVENT_INPUT,
                        static_cast<ALooper_callbackFunc>(OutputCallbackHandler),
                        static_cast<void *>(this));
    if (ret < 0) {
        ErrPrint("Fail to add a fd to Android looper.");
        return -EFAULT;
    }

    return ret;
}

int InferenceNativeInterface::OutputCallbackHandler(int fd, int events, void *data)
{
    InferenceNativeInterface *handle_ = reinterpret_cast<InferenceNativeInterface *>(data);
    assert(handle_ != nullptr && "An inference handle is nullptr");
    assert(handle_->inference != nullptr && "inference is nullptr");

    int ret = 1;
    int status = 0;
    beyond_event_info event = {
        .type = beyond_event_type::BEYOND_EVENT_TYPE_NONE,
        .data = nullptr,
    };
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    status = handle_->inference->FetchEventData(evtData);
    if (status < 0 || evtData == nullptr || (evtData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        if (ret < 0) {
            ErrPrintCode(-ret, "Fail to fetch event data.");
        }
        event.type = beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    } else if ((evtData->type & beyond_event_type::BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) == BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
        DbgPrint("This is an inference event, type = 0x%x", evtData->type);
        event.type = evtData->type;
        event.data = evtData->data;

        status = handle_->InvokeOutputCallback(handle_, event.type, event.data);
        if (status < 0) {
            ErrPrintCode(-status, "Failed to invoke an output callback");
        }
    } else {
        DbgPrint("Not an inference event, type = 0x%x...", evtData->type);
    }

    return ret;
}

int InferenceNativeInterface::InvokeOutputCallback(InferenceNativeInterface *inference_handle_, int eventType, void *eventData)
{
    beyond_tensor *tensors;
    int num_tensors = 0;
    int ret = inference_handle_->inference->GetOutput(tensors, num_tensors);
    if (ret < 0) {
        ErrPrint("Fail to GetOutput, ret = %d\n", ret);
        return -EFAULT;
    }

    if (outputCallbackJobject == nullptr) {
        DbgPrint("Callback object is null.");
        return -EINVAL;
    }

    JNIEnv *env = nullptr;
    int JNIStatus = callbackInfo.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (callbackInfo.jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            ErrPrint("Fail to attach the current thread.");
            return -EFAULT;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
        return -EFAULT;
    }

    jobjectArray byteBufferArray = static_cast<jobjectArray>(env->CallObjectMethod(outputCallbackJobject, callbackInfo.getByteBufferArrayMethodID));
    if (byteBufferArray == nullptr) {
        ErrPrint("Fail to get byteBufferArray.");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        DetachAttachedThread(JNIStatus);
        return -EFAULT;
    }

    for (int i = 0; i < num_tensors; i++) {
        jobject byte_buffer = env->NewDirectByteBuffer(tensors[i].data, tensors[i].size);
        if (byte_buffer == nullptr) {
            ErrPrint("Fail to NewDirectByteBuffer().");
            if (env->ExceptionCheck() == true) {
                JNIPrintException(env);
            }
            inference_handle_->inference->FreeTensor(tensors, num_tensors);
            tensors = nullptr;
            for (int j = 0; j < i; j++) {
                env->DeleteLocalRef(env->GetObjectArrayElement(byteBufferArray, i));
                if (env->ExceptionCheck() == true) {
                    ErrPrint("Fail to DeleteLocalRef().");
                    JNIPrintException(env);
                }
            }
            DetachAttachedThread(JNIStatus);
            return -EFAULT;
        }

        env->SetObjectArrayElement(byteBufferArray, i, byte_buffer);
        if (env->ExceptionCheck() == true) {
            ErrPrint("Fail to set the %d-th bytebuffer to the given array.", i);
            JNIPrintException(env);
            DetachAttachedThread(JNIStatus);
            return -EFAULT;
        }
    }

    env->CallVoidMethod(outputCallbackJobject, callbackInfo.organizeTensorSetMethodID, reinterpret_cast<long>(tensors));
    if (env->ExceptionCheck() == true) {
        ErrPrint("Fail to call a void method.");
        JNIPrintException(env);
        DetachAttachedThread(JNIStatus);
        return -EFAULT;
    }

    env->CallVoidMethod(outputCallbackJobject, callbackInfo.onReceivedOutputsMethodID);
    if (env->ExceptionCheck() == true) {
        ErrPrint("Fail to call a void method.");
        JNIPrintException(env);
        DetachAttachedThread(JNIStatus);
        return -EFAULT;
    }

    return 0;
}

void InferenceNativeInterface::DetachAttachedThread(int JNIStatus)
{
    if (JNIStatus == JNI_EDETACHED) {
        if (callbackInfo.jvm->DetachCurrentThread() != JNI_OK) {
            ErrPrint("Fail to attach the current thread.");
        }
    }
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

jboolean InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_removePeer(JNIEnv *env, jobject thiz, jlong inference_handle, jlong peer_handle) // TODO: Device a better idea to transfer peer_handle.
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    PeerNativeInterface *peer_handle_ = reinterpret_cast<PeerNativeInterface *>(peer_handle);
    int ret = inference_handle_->inference->RemovePeer(static_cast<beyond::InferenceInterface::PeerInterface *>(peer_handle_->GetBeyonDInstance()));
    if (ret < 0) {
        ErrPrint("Fail to remove the given peer to the inference handler, ret = %d\n", ret);
        return false;
    }

    return true;
}

void InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_destroy(JNIEnv *env, jclass klass, jlong inference_handle)
{
    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    if (inference_handle_ == nullptr) {
        ErrPrint("inference_handle_ == nullptr");
        return;
    }

    inference_handle_->inference->Destroy();
    inference_handle_ = nullptr;
}

jint InferenceNativeInterface::Java_com_samsung_android_beyond_InferenceHandler_setOutputCallback(JNIEnv *env, jobject thiz, jlong inference_handle, jobject callback_object)
{
    if (callback_object == nullptr) {
        ErrPrint("The given callback object is null.");
        return -EINVAL;
    }

    InferenceNativeInterface *inference_handle_ = reinterpret_cast<InferenceNativeInterface *>(inference_handle);
    inference_handle_->outputCallbackJobject = env->NewGlobalRef(callback_object);

    return 0;
}

int InferenceNativeInterface::RegisterInferenceNatives(JNIEnv *env)
{
    jclass klass = env->FindClass("com/samsung/android/beyond/inference/InferenceHandler");
    if (klass == nullptr) {
        ErrPrint("Unable to find InferenceHandler class.");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        return -EFAULT;
    }

    static JNINativeMethod inference_jni_methods[] = {
        { "create", "(Ljava/lang/String;)J", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_create) },
        { "addPeer", "(JJ)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_addPeer) },
        { "loadModel", "(JLjava/lang/String;)Z", (void *)Java_com_samsung_android_beyond_InferenceHandler_loadModel },
        { "prepare", "(J)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_prepare) },
        { "run", "(JJI)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_run) },
        { "removePeer", "(JJ)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_removePeer) },
        { "destroy", "(J)V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_destroy) },
        { "setCallback", "(JLcom/samsung/android/beyond/inference/TensorOutputCallback;)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferenceHandler_setOutputCallback) },
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
