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
#include <cstdbool>
#include <cerrno>
#include <exception>

#include <pthread.h>

#include <jni.h>

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "inference/beyond-inference_jni.h"
#include "inference/beyond-peer_jni.h"
#include "inference/tensor/beyond-tensor_jni.h"
#include "authenticator/beyond-authenticator_jni.h"
#include "discovery/beyond-discovery_jni.h"

static JavaVM *java_vm;

/**
 * @brief Load native methods.
 */
jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = nullptr;
    java_vm = vm;

    InfoPrint("Initializing, JNI...");

    int JNIStatus;
    bool attached = false;

    JNIStatus = vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != 0) {
            ErrPrint("Failed to attach current thread");
            return JNI_ERR;
        }

        attached = true;
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("Unsupported version");
        return JNI_ERR;
    }

    if (env == nullptr) {
        ErrPrint("On initializing, failed to get JNIEnv.");
        return JNI_ERR;
    }

    if (InferenceNativeInterface::RegisterInferenceNatives(env) < 0) {
        if (attached == true) {
            vm->DetachCurrentThread();
        }
        return JNI_ERR;
    }

    if (TensorJNI::RegisterTensorNatives(env) < 0) {
        if (attached == true) {
            vm->DetachCurrentThread();
        }
        return JNI_ERR;
    }

    if (PeerNativeInterface::RegisterPeerNatives(env) < 0) {
        if (attached == true) {
            vm->DetachCurrentThread();
        }
        return JNI_ERR;
    }

    if (AuthenticatorNativeInterface::RegisterNativeInterface(env) < 0) {
        if (attached == true) {
            vm->DetachCurrentThread();
        }
        return JNI_ERR;
    }

    if (DiscoveryNativeInterface::RegisterNativeInterface(env) < 0) {
        if (attached == true) {
            vm->DetachCurrentThread();
        }
        return JNI_ERR;
    }

    if (attached == true) {
        vm->DetachCurrentThread();
    }

    return JNI_VERSION_1_4;
}
