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
#ifdef ANDROID
#include "discovery.h"
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

Discovery::Discovery(void)
    : jvm(nullptr)
    , nsdManager(nullptr)
{
}

Discovery::~Discovery(void)
{
    if (jvm == nullptr || nsdManager == nullptr) {
        return;
    }

    JNIEnv *env = nullptr;
    bool attached = false;
    int JNIStatus;

    JNIStatus = jvm->GetEnv((void **)(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (jvm->AttachCurrentThread(&env, nullptr) != 0) {
            ErrPrint("Failed to attach current thread");
            env = nullptr;
        } else {
            attached = true;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
    }

    if (env != nullptr) {
        env->DeleteGlobalRef(nsdManager);
        nsdManager = nullptr;

        if (attached == true) {
            jvm->DetachCurrentThread();
        }
    }
}

const char *Discovery::GetModuleName(void) const
{
    return Discovery::NAME;
}

const char *Discovery::GetModuleType(void) const
{
    return beyond::ModuleInterface::TYPE_DISCOVERY;
}

int Discovery::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Discovery::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Discovery::Configure(const beyond_config *options)
{
    int ret = -EINVAL;

    if (options == nullptr) {
        ErrPrint("options is nullptr");
        return ret;
    }

    if (options->type == BEYOND_CONFIG_TYPE_CONTEXT_ANDROID) {
        beyond_config_context_android *android = static_cast<beyond_config_context_android *>(options->object);
        if (android == nullptr) {
            ErrPrint("Android Context object is not valid");
            return ret;
        }

        if (android->applicationContext == nullptr) {
            ErrPrint("Application context is not valid");
            return ret;
        }

        JNIEnv *env = nullptr;
        bool attached = false;
        int JNIStatus;

        JNIStatus = android->javaVM->GetEnv((void **)(&env), JNI_VERSION_1_4);
        if (JNIStatus == JNI_EDETACHED) {
            if (android->javaVM->AttachCurrentThread(&env, nullptr) != 0) {
                ErrPrint("Failed to attach current thread");
                return -EFAULT;
            } else {
                attached = true;
            }
        } else if (JNIStatus == JNI_EVERSION) {
            ErrPrint("GetEnv: Unsupported version");
        }

        if (env == nullptr) {
            ErrPrint("Failed to getEnv, event cannot be delivered to the managed code");
            ret = -EFAULT;
        } else {
            do {
                jclass contextKlass = env->FindClass("android/content/Context");
                if (contextKlass == nullptr) {
                    ErrPrint("Unable to get the Context class");
                    ret = -EFAULT;
                    break;
                }

                jfieldID fieldId = env->GetStaticFieldID(contextKlass, "NSD_SERVICE", "Ljava/lang/String;");
                if (env->ExceptionCheck() == true) {
                    env->DeleteLocalRef(contextKlass);
                    JNIPrintException(env);
                    ret = -EFAULT;
                    break;
                }
                if (fieldId == nullptr) {
                    ErrPrint("Cannot find the 'NSD_SERVICE' member");
                    env->DeleteLocalRef(contextKlass);
                    ret = -EFAULT;
                    break;
                }

                jobject serviceName = env->GetStaticObjectField(contextKlass, fieldId);
                env->DeleteLocalRef(contextKlass);
                if (serviceName == nullptr) {
                    ErrPrint("Failed to get the 'NSD_SERVICE' value");
                    ret = -EFAULT;
                    break;
                }

                jclass klass = env->GetObjectClass(android->applicationContext);
                if (klass == nullptr) {
                    ErrPrint("Unable to get the class");
                    ret = -EFAULT;
                    break;
                }
                // First get the class object
                jmethodID methodId = env->GetMethodID(klass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
                env->DeleteLocalRef(klass);
                if (env->ExceptionCheck() == true) {
                    JNIPrintException(env);
                    env->DeleteLocalRef(serviceName);
                    ret = -EFAULT;
                    break;
                }
                if (methodId == nullptr) {
                    ErrPrint("Invalid object");
                    env->DeleteLocalRef(serviceName);
                    ret = -EFAULT;
                    break;
                }

                jobject _nsdManager = env->CallObjectMethod(android->applicationContext, methodId, serviceName);
                env->DeleteLocalRef(serviceName);
                if (env->ExceptionCheck() == true) {
                    JNIPrintException(env);
                    ret = -EFAULT;
                    break;
                }

                nsdManager = env->NewGlobalRef(_nsdManager);
                env->DeleteLocalRef(_nsdManager);

                jvm = android->javaVM;

                ret = 0;
            } while (0);

            if (attached == true) {
                android->javaVM->DetachCurrentThread();
            }
        }
    }

    return ret;
}

void Discovery::PrintException(JNIEnv *env, const char *funcname, int lineno, bool clear)
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

    jstring message = Discovery::CallObjectMethod<jstring>(env, e, "getMessage", "()Ljava/lang/String;");
    const char *mstr = env->GetStringUTFChars(message, nullptr);

    ErrPrint("Exception: %s (%s:%d)", mstr, funcname, lineno);

    env->ReleaseStringUTFChars(message, mstr);
    env->DeleteLocalRef(message);
    invoked = false;
}

#endif // ANDROID
