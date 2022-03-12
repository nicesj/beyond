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
