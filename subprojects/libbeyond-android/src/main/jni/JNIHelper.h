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

#ifndef __BEYOND_ANDROID_JNI_HELPER_H__
#define __BEYOND_ANDROID_JNI_HELPER_H__

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <cerrno>
#include <jni.h>
#include <string>
#include <functional>

#define JNI_LONG_TYPE "J"
#define JNI_OBJECT_TYPE "Ljava/lang/Object;"
#define JNI_INT_TYPE "I"
#define BEYOND_NATIVE_INSTANCE_FIELD_NAME "instance"
#define JNIPrintException(env) JNIHelper::PrintException(env, __FUNCTION__, __LINE__)

class JNIHelper {
public:
    template <typename T>
    static T CallObjectMethod(JNIEnv *env, jobject obj, const char *methodName, const char *signature, ...);

    template <typename T>
    static T CallStaticObjectMethod(JNIEnv *env, jclass klass, const char *methodName, const char *signature, ...);

    template <typename T>
    static T *GetNativeInstance(JNIEnv *env, jobject obj);

    template <typename T>
    static int GetArrayElement(JNIEnv *env, jobjectArray argv, int idx, T &out);

public:
    // NOTE:
    // The following static functions is going to be extracted to common implementation later
    // Every BeyonD Java API are able to be implemented using the following functions.
    static void PrintException(JNIEnv *env, const char *funcname = nullptr, int lineno = -1, bool clear = true);
    static int CallVoidMethod(JNIEnv *env, jobject obj, const char *methodName, const char *signature, ...);
    static jclass FindClass(JNIEnv *env, const char *name);
    static jmethodID GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *signature);
    static void checkEnvException(JNIEnv *env);
    static int ExecuteWithEnv(JavaVM *jvm, const std::function<int(JNIEnv *, void *)> &callback, void *data = nullptr);

private:
    JNIHelper(void);
    virtual ~JNIHelper(void);
};

// NOTE:
// We are able to use this method for all other BeyonD C++ components
// If every Java Class has 'instance' field to maintain their Native Instance,
// GetInstanceFieldId and GetNativeInstance can be common
template <typename T>
T *JNIHelper::GetNativeInstance(JNIEnv *env, jobject obj)
{
    jclass klass = env->GetObjectClass(obj);
    if (klass == nullptr) {
        ErrPrint("Unable to get the class");
        return nullptr;
    }

    jfieldID fieldId = env->GetFieldID(klass, BEYOND_NATIVE_INSTANCE_FIELD_NAME, JNI_LONG_TYPE);
    env->DeleteLocalRef(klass);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return nullptr;
    }
    if (fieldId == nullptr) {
        DbgPrint("Cannot find the 'instance' member");
        return nullptr;
    }

    return reinterpret_cast<T *>(env->GetLongField(obj, fieldId));
}

template <typename T>
T JNIHelper::CallObjectMethod(JNIEnv *env, jobject obj, const char *methodName, const char *signature, ...)
{
    jclass klass = env->GetObjectClass(obj);
    if (klass == nullptr) {
        ErrPrint("Unable to get the class");
        return static_cast<T>(nullptr);
    }

    // First get the class object
    jmethodID methodId = env->GetMethodID(klass, methodName, signature);
    env->DeleteLocalRef(klass);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return static_cast<T>(nullptr);
    }
    if (methodId == nullptr) {
        ErrPrint("Invalid object");
        return static_cast<T>(nullptr);
    }

    va_list ap;
    va_start(ap, signature);
    T retObj = static_cast<T>(env->CallObjectMethodV(obj, methodId, ap));
    va_end(ap);

    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return static_cast<T>(nullptr);
    }

    return retObj;
}

template <typename T>
T JNIHelper::CallStaticObjectMethod(JNIEnv *env, jclass klass, const char *methodName, const char *signature, ...)
{
    jmethodID methodId = env->GetStaticMethodID(klass, methodName, signature);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        ErrPrint("Method not found: %s, signature: %s", methodName, signature);
        return static_cast<T>(nullptr);
    }
    if (methodId == nullptr) {
        ErrPrint("Method is not found, methodId is nullptr");
        return static_cast<T>(nullptr);
    }

    va_list ap;
    va_start(ap, signature);
    T retObj = static_cast<T>(env->CallStaticObjectMethodV(klass, methodId, ap));
    va_end(ap);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return static_cast<T>(nullptr);
    }

    return retObj;
}

template <typename T>
int JNIHelper::GetArrayElement(JNIEnv *env, jobjectArray argv, int idx, T &out)
{
    T val = static_cast<T>(env->GetObjectArrayElement(argv, idx));
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (val == nullptr) {
        ErrPrint("Invalid argument");
        return -EINVAL;
    }
    out = val;
    return 0;
}

#endif // __BEYOND_ANDROID_JNI_HELPER_H__
