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

#include "beyond-authenticator_jni.h"
#include "JNIHelper.h"

#include <beyond/platform/beyond_platform.h>

#include <cstdio>
#include <cerrno>

#define APPLICATION_CONTEXT_CLASS_NAME "android.app.Application"

#define BEYOND_EVENT_LISTENER_CLASS_TYPE "Lcom/samsung/android/beyond/EventListener;"
#define BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_NAME "onEvent"
#define BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_SIGNATURE "(Lcom/samsung/android/beyond/EventObject;)V"

#define BEYOND_AUTHENTICATOR_EVENT_OBJECT_CLASS_NAME "com/samsung/android/beyond/authenticator/Authenticator$DefaultEventObject"
#define BEYOND_AUTHENTICATOR_EVENT_OBJECT_CONSTRUCTOR_NAME "<init>"
#define BEYOND_AUTHENTICATOR_EVENT_OBJECT_CONSTRUCTOR_SIGANTURE "()V"
#define BEYOND_AUTHENTICATOR_EVENT_OBJECT_EVENT_TYPE_FIELD_NAME "eventType"
#define BEYOND_AUTHENTICATOR_EVENT_OBJECT_EVENT_DATA_FIELD_NAME "eventData"

#define BEYOND_AUTHENTICATOR_EVENT_LISTENER_FIELD_NAME "eventListener"

AuthenticatorNativeInterface::EventObject AuthenticatorNativeInterface::eventObject = {
    .klass = nullptr,
    .constructor = nullptr,
    .eventType = nullptr,
    .eventData = nullptr,
};
jfieldID AuthenticatorNativeInterface::eventListener = nullptr;

AuthenticatorNativeInterface::AuthenticatorNativeInterface(void)
    : authenticator(nullptr)
    , looper(nullptr)
    , jvm(nullptr)
    , thiz(nullptr)
{
}

void *AuthenticatorNativeInterface::GetBeyonDInstance(void)
{
    return static_cast<void *>(authenticator);
}

AuthenticatorNativeInterface::~AuthenticatorNativeInterface(void)
{
    if (authenticator != nullptr) {
        if (authenticator->GetHandle() >= 0) {
            if (looper != nullptr) {
                int _status;
                _status = ALooper_removeFd(looper, authenticator->GetHandle());
                DbgPrint("ALooper_removeFd: %d", _status);
            } else {
                ErrPrint("No looper found");
            }
        } else {
            DbgPrint("There was no FD");
        }

        authenticator->Destroy();
        authenticator = nullptr;
    }

    // NOTE:
    // Even there is no authenticator, we have to check the looper
    if (looper != nullptr) {
        ALooper_release(looper);
        looper = nullptr;
    }

    // TODO:
    // In the destructor, we are doing too much works
    // It should be narrowed and simplified
    // otherwise, the destructor code should be separated
    if (thiz != nullptr) {
        JNIEnv *env = nullptr;
        bool attached = false;
        int JNIStatus = jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
        if (JNIStatus == JNI_EDETACHED) {
            if (jvm->AttachCurrentThread(&env, nullptr) != 0) {
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
            // NOTE:
            // this can be happens when the user calls "close" method directly
            DbgPrint("Event listener is not cleared, forcibly delete the reference");
            env->DeleteGlobalRef(thiz);
            thiz = nullptr;
        }

        if (attached == true) {
            jvm->DetachCurrentThread();
        }
    }

    jvm = nullptr;
}

void AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_initialize(JNIEnv *env, jclass klass)
{
    eventListener = env->GetFieldID(klass,
                                    BEYOND_AUTHENTICATOR_EVENT_LISTENER_FIELD_NAME,
                                    BEYOND_EVENT_LISTENER_CLASS_TYPE);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return;
    }
    if (eventListener == nullptr) {
        ErrPrint("Unable to find the field id of an eventListener");
        return;
    }

    // NOTE:
    // Holds the event object class
    // And cache the field ids of it.
    // If we do not hold the event object class,
    // the event object class can be unloaded by a classLoader,
    // and then it could be loaded again when it is required later.
    // in that case we may need to resolve field ids again.
    //
    // The following initialization can be deferred when a
    jclass eventObjectClass = env->FindClass(BEYOND_AUTHENTICATOR_EVENT_OBJECT_CLASS_NAME);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return;
    }
    if (eventObjectClass == nullptr) {
        ErrPrint("Unable to find the authenticator event object class");
        return;
    }

    eventObject.klass = static_cast<jclass>(env->NewGlobalRef(eventObjectClass));
    env->DeleteLocalRef(eventObjectClass);
    eventObjectClass = nullptr;
    if (eventObject.klass == nullptr) {
        ErrPrint("Unable to allocate global reference for the event object class");
        return;
    }

    eventObject.eventType = env->GetFieldID(eventObject.klass,
                                            BEYOND_AUTHENTICATOR_EVENT_OBJECT_EVENT_TYPE_FIELD_NAME,
                                            JNI_INT_TYPE);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }
    if (eventObject.eventType == nullptr) {
        ErrPrint("Unable to find the event type field");
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    eventObject.eventData = env->GetFieldID(eventObject.klass,
                                            BEYOND_AUTHENTICATOR_EVENT_OBJECT_EVENT_DATA_FIELD_NAME,
                                            JNI_OBJECT_TYPE);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    eventObject.constructor = env->GetMethodID(eventObject.klass,
                                               BEYOND_AUTHENTICATOR_EVENT_OBJECT_CONSTRUCTOR_NAME,
                                               BEYOND_AUTHENTICATOR_EVENT_OBJECT_CONSTRUCTOR_SIGANTURE);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    DbgPrint("Authenticator JNI cache initialized");
}

int AuthenticatorNativeInterface::InvokeEventListener(JNIEnv *env, jobject thiz, int eventType, void *eventData)
{
    jobject eventListenerObject = env->GetObjectField(thiz, eventListener);
    if (eventListenerObject == nullptr) {
        DbgPrint("No listener registered");
        return 0;
    }

    jobject object = env->NewObject(eventObject.klass, eventObject.constructor);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (object == nullptr) {
        ErrPrint("Failed to construct a new event object");
        return -EFAULT;
    }

    env->SetIntField(object, eventObject.eventType, eventType);
    if (eventData != nullptr) {
        env->SetObjectField(object, eventObject.eventData, static_cast<jobject>(eventData));
    }

    int ret = JNIHelper::CallVoidMethod(env, eventListenerObject,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_NAME,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_SIGNATURE,
                                        object);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(eventListenerObject);
    return ret;
}

int AuthenticatorNativeInterface::Authenticator_eventHandler(int fd, int events, void *data)
{
    AuthenticatorNativeInterface *inst = static_cast<AuthenticatorNativeInterface *>(data);
    assert(inst != nullptr && "inst is nullptr");
    if (inst->authenticator == nullptr || inst->looper == nullptr) {
        assert(inst->authenticator != nullptr && "authenticator is nullptr");
        assert(inst->looper != nullptr && "looper is nullptr");
        ErrPrint("Invalid callbackdata");
        return 0;
    }

    int ret = 1;
    int status;
    beyond_event_info event = {
        .type = beyond_event_type::BEYOND_EVENT_TYPE_NONE,
        .data = nullptr,
    };

    // Now, check the exceptional cases
    if ((events & (ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR)) != 0) {
        ErrPrint("Event file descriptor is closed");
        ret = 0;
        // NOTE:
        // Do not cut off the handler from here.
        // Go ahead and check all available events even if it is error
        event.type |= beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    }

    beyond::EventObjectInterface::EventData *evtData = nullptr;

    // NOTE:
    // If there is an event consumerable,
    // Consuming it as soon as possible we can do
    if ((events & ALOOPER_EVENT_INPUT) == ALOOPER_EVENT_INPUT) {
        status = inst->authenticator->FetchEventData(evtData);
        if (status < 0 || evtData == nullptr || (evtData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
            if (status < 0) {
                ErrPrintCode(-status, "Fetch event data failed");
            }

            event.type |= beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
        } else {
            event.type |= evtData->type;
            event.data = evtData->data;
        }
    }

    JNIEnv *env = nullptr;
    bool attached = false;
    int JNIStatus;

    JNIStatus = inst->jvm->GetEnv((void **)(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (inst->jvm->AttachCurrentThread(&env, nullptr) != 0) {
            ErrPrint("Failed to attach current thread");
        } else {
            attached = true;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
    }

    if (env == nullptr) {
        ErrPrint("Failed to getEnv, event cannot be delivered to the managed code");
    } else {
        // NOTE:
        // Now, publish the event object to the event listener
        if (inst->thiz != nullptr) {
            status = inst->InvokeEventListener(env, inst->thiz, event.type, event.data);
            if (status < 0) {
                ErrPrint("Failed to invoke the event listener");
            }
        } else {
            DbgPrint("Event listener is not ready yet");
        }

        if (attached == true) {
            inst->jvm->DetachCurrentThread();
        }
    }

    // NOTE:
    // event.data must be preserved even if the authenticator destroy the eventData
    // because we are not able to know when the EventObject in the managed code
    // is going to be destroyed
    inst->authenticator->DestroyEventData(evtData);
    return ret;
}

void AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_destroy(JNIEnv *env, jclass klass, jlong _inst)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return;
    }

    delete inst;
    inst = nullptr;
}

long AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_create(JNIEnv *env, jobject thiz, jobjectArray args)
{
    int argc = env->GetArrayLength(args);
    const char **argv = nullptr;
    jstring *strs = nullptr;

    if (argc <= 0) {
        ErrPrint("Invalid arguments");
        return 0l;
    }

    try {
        argv = new const char *[argc];
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        return 0l;
    }

    try {
        strs = new jstring[argc];
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        delete[] argv;
        return 0l;
    }

    for (int i = 0; i < argc; i++) {
        strs[i] = static_cast<jstring>(env->GetObjectArrayElement(args, i));
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
            while (--i >= 0) {
                env->ReleaseStringUTFChars(strs[i], argv[i]);
            }
            delete[] strs;
            delete[] argv;
            return 0l;
        }
        argv[i] = env->GetStringUTFChars(strs[i], 0);
    }

    AuthenticatorNativeInterface *inst = nullptr;

    do {
        try {
            inst = new AuthenticatorNativeInterface();
        } catch (std::exception &e) {
            ErrPrint("new: %s", e.what());
            break;
        }

        if (env->GetJavaVM(&inst->jvm) != 0) {
            ErrPrint("Unable to get javaVM");
            delete inst;
            inst = nullptr;
            break;
        }

        beyond_argument arg = {
            .argc = argc,
            .argv = const_cast<char **>(argv),
        };
        inst->authenticator = beyond::Authenticator::Create(&arg);
        if (inst->authenticator == nullptr) {
            ErrPrint("Failed to create an authenticator module");
            delete inst;
            inst = nullptr;
            break;
        }

        if (inst->authenticator->GetHandle() >= 0) {
            inst->looper = ALooper_forThread();
            if (inst->looper == nullptr) {
                ErrPrint("There is no android looper found");
                delete inst;
                inst = nullptr;
                break;
            }

            ALooper_acquire(inst->looper);

            int ret = ALooper_addFd(inst->looper,
                                    inst->authenticator->GetHandle(),
                                    ALOOPER_POLL_CALLBACK,
                                    ALOOPER_EVENT_INPUT,
                                    static_cast<ALooper_callbackFunc>(Authenticator_eventHandler),
                                    static_cast<void *>(inst));
            if (ret < 0) {
                ErrPrint("Failed to add event handler");
                delete inst;
                inst = nullptr;
                break;
            } // otherwise, the ret is 1 if succeed
        } else {
            DbgPrint("Authenticator module does not support asynchronous mode");
        }
    } while (0);

    for (int i = 0; i < argc; i++) {
        env->ReleaseStringUTFChars(strs[i], argv[i]);
    }
    delete[] strs;
    delete[] argv;
    return reinterpret_cast<long>(inst);
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong _inst, jchar type, jlong objInst)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }

    int ret = -EINVAL;
    if (objInst != 0) {
        NativeInterface *nativeInterface = reinterpret_cast<NativeInterface *>(objInst);

        beyond_config conf = {
            .type = static_cast<char>(type),
            .object = nativeInterface->GetBeyonDInstance(),
        };

        ret = inst->authenticator->Configure(&conf);
    }
    return ret;
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong _inst, jchar type, jobject obj)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }

    beyond_config config = {
        .type = static_cast<char>(type),
        .object = obj,
    };

    int ret;
    if (type == BEYOND_CONFIG_TYPE_CONTEXT_ANDROID) {
        // NOTE:
        // Configure the module for the java context
        // Every module is going to get the java context configuration after creating it.
        //
        // The "obj" parameter is validated from the java implementation,
        // therefore it does not necessary to validate the object in the JNI
        beyond_config_context_android ctx = {
            .javaVM = inst->jvm,
            .applicationContext = obj,
        };

        config.object = static_cast<void *>(&ctx);
        ret = inst->authenticator->Configure(&config);
    } else {
        ret = inst->authenticator->Configure(&config);
    }

    return ret;
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_configure(JNIEnv *env, jobject thiz, jlong _inst, jchar type, jstring jsonConfig)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }
    const char *_jsonConfig = env->GetStringUTFChars(jsonConfig, 0);
    beyond_config conf = {
        .type = static_cast<char>(type),
        .object = static_cast<void *>(const_cast<char *>(_jsonConfig)),
    };
    DbgPrint("Json Configuration String: %s", _jsonConfig);
    int ret = inst->authenticator->Configure(&conf);
    env->ReleaseStringUTFChars(jsonConfig, _jsonConfig);
    return ret;
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_prepare(JNIEnv *env, jobject thiz, jlong _inst)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }
    return inst->authenticator->Prepare();
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_activate(JNIEnv *env, jobject thiz, jlong _inst)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }
    return inst->authenticator->Activate();
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_deactivate(JNIEnv *env, jobject thiz, jlong _inst)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }
    return inst->authenticator->Deactivate();
}

int AuthenticatorNativeInterface::Java_com_samsung_android_beyond_Authenticator_setEventListener(JNIEnv *env, jobject _thiz, jlong _inst, jboolean flag)
{
    AuthenticatorNativeInterface *inst = reinterpret_cast<AuthenticatorNativeInterface *>(_inst);
    if (inst == nullptr || inst->authenticator == nullptr) {
        ErrPrint("Authenticator object is nullptr");
        return -EFAULT;
    }

    if (flag == true) {
        inst->thiz = env->NewGlobalRef(_thiz);
        if (inst->thiz == nullptr) {
            ErrPrint("Failed to get a global reference");
            return -EFAULT;
        }
    } else if (inst->thiz == _thiz) {
        env->DeleteGlobalRef(inst->thiz);
        inst->thiz = nullptr;
    } else {
        ErrPrint("This object corrupted");
        return -EFAULT;
    }

    return 0;
}

int AuthenticatorNativeInterface::RegisterNativeInterface(JNIEnv *env)
{
    JNINativeMethod methods[] = {
        { "initialize", "()V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_initialize) },
        { "create", "([Ljava/lang/String;)J", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_create) },
        { "destroy", "(J)V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_destroy) },
        { "configure", "(JCLjava/lang/String;)I", reinterpret_cast<void *>((int (*)(JNIEnv *, jobject, jlong, jchar, jstring))Java_com_samsung_android_beyond_Authenticator_configure) },
        { "configure", "(JCLjava/lang/Object;)I", reinterpret_cast<void *>((int (*)(JNIEnv *, jobject, jlong, jchar, jobject))Java_com_samsung_android_beyond_Authenticator_configure) },
        { "configure", "(JCJ)I", reinterpret_cast<void *>((int (*)(JNIEnv *, jobject, jlong, jchar, jlong))Java_com_samsung_android_beyond_Authenticator_configure) },
        { "prepare", "(J)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_prepare) },
        { "activate", "(J)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_activate) },
        { "deactivate", "(J)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_deactivate) },
        { "setEventListener", "(JZ)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_Authenticator_setEventListener) },
    };

    jclass klass = env->FindClass("com/samsung/android/beyond/authenticator/Authenticator");
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }
    if (klass == nullptr) {
        ErrPrint("Unable to find the Authenticator class");
        return -EFAULT;
    }

    if (env->RegisterNatives(klass, methods, sizeof(methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        env->DeleteLocalRef(klass);
        ErrPrint("Failed to register authenticator methods for BeyonD Java APIs");
        return -EFAULT;
    }
    env->DeleteLocalRef(klass);

    return 0;
}
