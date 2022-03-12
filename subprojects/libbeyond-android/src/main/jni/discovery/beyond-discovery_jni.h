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

#ifndef __BEYOND_ANDROID_DISCOVERY_JNI_H__
#define __BEYOND_ANDROID_DISCOVERY_JNI_H__

#include "NativeInterface.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <android/looper.h>
#include <jni.h>

class DiscoveryNativeInterface : public NativeInterface {
public:
    void *GetBeyonDInstance(void) override;

public:
    static int RegisterNativeInterface(JNIEnv *env);

private:
    DiscoveryNativeInterface(void);
    virtual ~DiscoveryNativeInterface(void);

private:
    static void Java_com_samsung_android_beyond_discovery_Discovery_initialize(JNIEnv *env, jclass klass);
    static jlong Java_com_samsung_android_beyond_discovery_Discovery_create(JNIEnv *env, jobject thiz, jobjectArray args);
    static void Java_com_samsung_android_beyond_discovery_Discovery_destroy(JNIEnv *env, jclass klass, jlong instance);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_configure_object(JNIEnv *env, jobject thiz, jlong instance, jchar type, jobject obj);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_configure(JNIEnv *env, jobject thiz, jlong instance, jchar type, jstring jsonConfig);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_activate(JNIEnv *env, jobject thiz, jlong instance);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_deactivate(JNIEnv *env, jobject thiz, jlong instance);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_setItem(JNIEnv *env, jobject thiz, jlong instance, jstring key, jbyteArray value);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_removeItem(JNIEnv *env, jobject thiz, jlong instance, jstring key);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_configure(JNIEnv *env, jobject thiz, jlong inst, jchar type, jobject obj);
    static jint Java_com_samsung_android_beyond_discovery_Discovery_setEventListener(JNIEnv *env, jobject thiz, jlong instance, jboolean flag);

private:
    static int Discovery_eventHandler(int fd, int events, void *data);

private: // JNI Cache
    struct EventObject {
        jclass klass;
        jmethodID constructor;
        jfieldID eventType;
        jfieldID eventData;
    };

private: // JNI Cache
    static EventObject eventObject;
    static jfieldID eventListener;

private:
    int InvokeEventListener(JNIEnv *env, jobject thiz, int eventType, void *eventData);

private:
    beyond::Discovery *discovery;
    ALooper *looper;
    JavaVM *jvm;
    jobject thiz;
};

#endif // __BEYOND_ANDROID_DISCOVERY_JNI_H__
