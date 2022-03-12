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

#ifndef __BEYOND_ANDROID_PEER_JNI_H__
#define __BEYOND_ANDROID_PEER_JNI_H__

#include "NativeInterface.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include "NativeInterface.h"

#include <jni.h>

class PeerNativeInterface : public NativeInterface {
public:
    static int RegisterPeerNatives(JNIEnv *env);
    void *GetBeyonDInstance(void) override;

private:
    PeerNativeInterface(void);
    virtual ~PeerNativeInterface(void);

    static jlong Java_com_samsung_android_beyond_InferencePeer_create(JNIEnv *env, jobject thiz, jobject context, jobjectArray args);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_set_info(JNIEnv *env, jobject thiz, jlong handle, jstring peer_ip, jint peer_port);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_set_uuid(JNIEnv *env, jobject thiz, jlong handle, jstring peer_uuid);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_activate(JNIEnv *env, jobject thiz, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_prepare(JNIEnv *env, jobject thiz, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_deactivate(JNIEnv *env, jobject thiz, jlong handle);
    static void Java_com_samsung_android_beyond_InferencePeer_destroy(JNIEnv *env, jclass klass, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_configure(JNIEnv *env, jobject thiz, jlong handle, jchar type, jobject obj);

    beyond::InferenceInterface::PeerInterface *peer;
};

#if USE_PEER_NN
#define PEER_TYPE_PEER_NN "peer_nn"

typedef jboolean (*NNStreamerInitialization)(JNIEnv *env, jobject context);

static constexpr const char *NNStreamerNativeLibraryName = "libnnstreamer-native.so";
static constexpr const char *NNStreamerInitFunctionName = "nnstreamer_native_initialize";
#endif

#endif // __BEYOND_ANDROID_PEER_JNI_H__
