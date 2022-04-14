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

#include "JNIHelper.h"

#include <cstdio>

void JNIHelper::PrintException(JNIEnv *env, const char *funcname, int lineno, bool clear)
{
    static bool invoked = false;

    if (invoked == true) {
        ErrPrint("Nested exceptions detected");
        return;
    }

    invoked = true;
    jthrowable e = env->ExceptionOccurred();
    if (clear == true) {
        env->ExceptionClear();
    }

    jstring message = JNIHelper::CallObjectMethod<jstring>(env, e, "getMessage", "()Ljava/lang/String;");
    const char *mstr = env->GetStringUTFChars(message, nullptr);

    ErrPrint("Exception: %s (%s:%d)", mstr, funcname, lineno);

    env->ReleaseStringUTFChars(message, mstr);
    env->DeleteLocalRef(message);
    invoked = false;
}

int JNIHelper::CallVoidMethod(JNIEnv *env, jobject obj, const char *methodName, const char *signature, ...)
{
    jclass klass = env->GetObjectClass(obj);
    if (klass == nullptr) {
        ErrPrint("Unable to get the class");
        return -EFAULT;
    }

    // First get the class object
    jmethodID methodId = env->GetMethodID(klass, methodName, signature);
    env->DeleteLocalRef(klass);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (methodId == nullptr) {
        ErrPrint("Invalid object");
        return -ENOENT;
    }

    va_list ap;
    va_start(ap, signature);
    env->CallVoidMethodV(obj, methodId, ap);
    va_end(ap);

    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }

    return 0;
}

jclass JNIHelper::FindClass(JNIEnv *env, const char *name)
{
    jclass ret = env->FindClass(name);
    if (ret == nullptr) {
        ErrPrint("jclass (%s) is null.", name);
        JNIHelper::checkEnvException(env);
    }

    return ret;
}

jmethodID JNIHelper::GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *signature)
{
    jmethodID ret = env->GetMethodID(clazz, name, signature);
    if (ret == nullptr) {
        ErrPrint("jmethod (%s, %s) is null.", name, signature);
        JNIHelper::checkEnvException(env);
    }

    return ret;
}

void JNIHelper::checkEnvException(JNIEnv *env)
{
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        throw std::exception();
    }
}

int JNIHelper::ExecuteWithEnv(JavaVM *jvm, const std::function<int(JNIEnv *, void *)> &callback, void *data)
{
    if (jvm == nullptr) {
        ErrPrint("Invalid arguments, jvm(%p)", jvm);
        return -EINVAL;
    }

    JNIEnv *env = nullptr;

    int JNIStatus = jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (jvm->AttachCurrentThread(&env, nullptr) != 0) {
            ErrPrint("Failed to attach current thread");
            return -EFAULT;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
        return -ENOTSUP;
    }

    if (env == nullptr) {
        ErrPrint("Failed to get the environment object");
        return -EFAULT;
    }

    int ret = callback(env, data);

    if (JNIStatus == JNI_EDETACHED) {
        jvm->DetachCurrentThread();
    }

    return ret;
}
