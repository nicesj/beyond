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

#ifndef __BEYOND_ANDROID_AUTHENTICATOR_NATIVE_INTERFACE_H__
#define __BEYOND_ANDROID_AUTHENTICATOR_NATIVE_INTERFACE_H__

#include "NativeInterface.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <android/looper.h>
#include <jni.h>

class AuthenticatorNativeInterface : public NativeInterface {
public:
    void *GetBeyonDInstance(void) override;

public:
    static int RegisterNativeInterface(JNIEnv *env);

private:
    AuthenticatorNativeInterface(void);
    virtual ~AuthenticatorNativeInterface(void);

    static long Java_com_samsung_android_beyond_Authenticator_create(JNIEnv *env, jobject thiz, jobjectArray args);
    static int Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong inst, jchar type, jobject obj);
    static int Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong inst, jchar type, jstring jsonConfig);
    static int Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong inst, jchar type, jlong objInst);
    static int Java_com_samsung_android_beyond_Authenticator_prepare(JNIEnv *env, jobject thiz, jlong inst);
    static int Java_com_samsung_android_beyond_Authenticator_activate(JNIEnv *env, jobject thiz, jlong inst);
    static int Java_com_samsung_android_beyond_Authenticator_deactivate(JNIEnv *env, jobject thiz, jlong inst);
    static void Java_com_samsung_android_beyond_Authenticator_initialize(JNIEnv *env, jclass klass);
    static void Java_com_samsung_android_beyond_Authenticator_destroy(JNIEnv *env, jclass klass, jlong inst);
    static int Java_com_samsung_android_beyond_Authenticator_setEventListener(JNIEnv *env, jobject thiz, jlong inst, jobject listener);

private:
    static int Authenticator_eventHandler(int fd, int events, void *data);

private: // JNI Cache
    struct EventObject {
        jclass klass;
        jmethodID constructor;
        jfieldID eventType;
        jfieldID eventData;
    };

private: // JNI Cache
    static EventObject eventObject;

private:
    int InvokeEventListener(JNIEnv *env, int eventType, void *eventData);
    int AttachEventLoop(void);

private:
    beyond::Authenticator *authenticator;

    ALooper *looper;

    JavaVM *jvm;
    jobject listener;
};

#endif // __BEYOND_ANDROID_AUTHENTICATOR_NATIVE_INTERFACE_H__
