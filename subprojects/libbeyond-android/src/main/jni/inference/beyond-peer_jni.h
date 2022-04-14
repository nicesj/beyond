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

#include <android/looper.h>
#include <jni.h>

class PeerNativeInterface : public NativeInterface {
public:
    static int RegisterPeerNatives(JNIEnv *env);
    void *GetBeyonDInstance(void) override;

private:
    struct Info {
        jclass klass;
        jmethodID constructor;
        jfieldID host;
        jfieldID port;
        jfieldID uuid;
    };

    struct EventListnerJNICache {
        jclass eventListenerClass;
        jmethodID onEventMethodId;
    };

private:
    PeerNativeInterface(void);
    virtual ~PeerNativeInterface(void);

    static void Java_com_samsung_android_beyond_InferencePeer_initialize(JNIEnv *env, jclass klass);
    static jlong Java_com_samsung_android_beyond_InferencePeer_create(JNIEnv *env, jobject thiz, jobjectArray args);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_set_info(JNIEnv *env, jobject thiz, jlong handle, jobject info);
    static jobject Java_com_samsung_android_beyond_InferencePeer_get_info(JNIEnv *env, jobject thiz, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_activate(JNIEnv *env, jobject thiz, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_prepare(JNIEnv *env, jobject thiz, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_deactivate(JNIEnv *env, jobject thiz, jlong handle);
    static void Java_com_samsung_android_beyond_InferencePeer_destroy(JNIEnv *env, jclass klass, jlong handle);
    static jboolean Java_com_samsung_android_beyond_InferencePeer_configure(JNIEnv *env, jobject thiz, jlong handle, jchar type, jobject obj);
    static jint Java_com_samsung_android_beyond_InferencePeer_setEventListener(JNIEnv *env, jobject thiz, jlong handle, jobject event_listener_object);

    static int PeerEventHandler(int fd, int events, void *data);
    static void InitCallbackInfo(JNIEnv *env);
    static void ReleaseArguments(JNIEnv *env, int argc, const char **argv, jstring *strs);

    int AttachEventLoop(JNIEnv *env);
    int InvokeEventListener();
    void DetachAttachedThread(int JNIStatus);

private:
    static Info infoObject;
    static EventListnerJNICache eventListenerJNICache;

    beyond::InferenceInterface::PeerInterface *peer;
    ALooper *looper;
    jobject eventListenerObject;
    JavaVM *jvm;
};

#endif // __BEYOND_ANDROID_PEER_JNI_H__
