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

#include "beyond-discovery_jni.h"
#include "JNIHelper.h"

#include <cstdio>
#include <cerrno>

#define BEYOND_EVENT_LISTENER_CLASS_TYPE "Lcom/samsung/android/beyond/EventListener;"
#define BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_NAME "onEvent"
#define BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_SIGNATURE "(Lcom/samsung/android/beyond/EventObject;)V"

#define BEYOND_DISCOVERY_EVENT_OBJECT_CLASS_NAME "com/samsung/android/beyond/discovery/Discovery$DefaultEventObject"
#define BEYOND_DISCOVERY_EVENT_OBJECT_CONSTRUCTOR_NAME "<init>"
#define BEYOND_DISCOVERY_EVENT_OBJECT_CONSTRUCTOR_SIGANTURE "()V"
#define BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_TYPE_FIELD_NAME "eventType"
#define BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_DATA_FIELD_NAME "eventData"

#define BEYOND_DISCOVERY_EVENT_LISTENER_FIELD_NAME "eventListener"

DiscoveryNativeInterface::EventObject DiscoveryNativeInterface::eventObject = {
    .klass = nullptr,
    .constructor = nullptr,
    .eventType = nullptr,
    .eventData = nullptr,
};
jfieldID DiscoveryNativeInterface::eventListener = nullptr;

DiscoveryNativeInterface::DiscoveryNativeInterface(void)
    : discovery(nullptr)
    , looper(nullptr)
    , jvm(nullptr)
    , thiz(nullptr)
{
}

DiscoveryNativeInterface::~DiscoveryNativeInterface(void)
{
    if (looper != nullptr) {
        if (discovery != nullptr && discovery->GetHandle() >= 0) {
            ALooper_removeFd(looper, discovery->GetHandle());
        }
        ALooper_release(looper);
        looper = nullptr;
    }

    if (discovery != nullptr) {
        discovery->Destroy();
        discovery = nullptr;
    }
}

void DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_initialize(JNIEnv *env, jclass klass)
{
    eventListener = env->GetFieldID(klass,
                                    BEYOND_DISCOVERY_EVENT_LISTENER_FIELD_NAME,
                                    BEYOND_EVENT_LISTENER_CLASS_TYPE);
    if (eventListener == nullptr) {
        ErrPrint("Unable to find the field id of an eventListener");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
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
    jclass eventObjectClass = env->FindClass(BEYOND_DISCOVERY_EVENT_OBJECT_CLASS_NAME);
    if (eventObjectClass == nullptr) {
        ErrPrint("Unable to find the discovery event object class");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
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
                                            BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_TYPE_FIELD_NAME,
                                            JNI_INT_TYPE);
    if (eventObject.eventType == nullptr) {
        ErrPrint("Unable to find the event type field");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    eventObject.eventData = env->GetFieldID(eventObject.klass,
                                            BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_DATA_FIELD_NAME,
                                            JNI_OBJECT_TYPE);
    if (eventObject.eventData == nullptr) {
        ErrPrint("Unable to find the event data field");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    eventObject.constructor = env->GetMethodID(eventObject.klass,
                                               BEYOND_DISCOVERY_EVENT_OBJECT_CONSTRUCTOR_NAME,
                                               BEYOND_DISCOVERY_EVENT_OBJECT_CONSTRUCTOR_SIGANTURE);
    if (eventObject.constructor == nullptr) {
        ErrPrint("Unable to find the event constructor metohd");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }

    DbgPrint("Discovery JNI cache initialized");
}

int DiscoveryNativeInterface::InvokeEventListener(JNIEnv *env, jobject thiz, int eventType, void *eventData)
{
    jobject eventListenerObject = env->GetObjectField(thiz, eventListener);
    if (eventListenerObject == nullptr) {
        DbgPrint("No listener registered");
        return 0;
    }

    jobject object = env->NewObject(eventObject.klass, eventObject.constructor);
    if (object == nullptr) {
        ErrPrint("Failed to construct a new event object");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        return -EFAULT;
    }

    env->SetIntField(object, eventObject.eventType, eventType);
    if (eventData != nullptr) {
        // TODO: convert eventData to JAVA object
        // env->SetObjectField(object, eventObject.eventData, static_cast<jobject>(eventData));
    }
    int ret = JNIHelper::CallVoidMethod(env, eventListenerObject,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_NAME,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_SIGNATURE,
                                        object);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(eventListenerObject);
    return ret;
}

int DiscoveryNativeInterface::Discovery_eventHandler(int fd, int events, void *data)
{
    auto inst = static_cast<DiscoveryNativeInterface *>(data);
    assert(inst != nullptr && "inst is nullptr");
    if (inst->discovery == nullptr || inst->looper == nullptr) {
        assert(inst->discovery != nullptr && "discovery is nullptr");
        assert(inst->looper != nullptr && "looper is nullptr");
        ErrPrint("Invalid callbackdata");
        return 0;
    }

    if (inst->thiz == nullptr) {
        DbgPrint("Event listener is not ready yet");
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
        status = inst->discovery->FetchEventData(evtData);
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
    int JNIStatus = inst->jvm->GetEnv((void **)(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_OK) {
        // NOTE:
        // Now, publish the event object to the event listener
        status = inst->InvokeEventListener(env, inst->thiz, event.type, event.data);
        if (status < 0) {
            ErrPrint("Failed to invoke the event listener");
        }
    } else if (JNIStatus == JNI_EDETACHED) {
        if (inst->jvm->AttachCurrentThread(&env, nullptr) == 0) {
            // NOTE:
            // Now, publish the event object to the event listener
            status = inst->InvokeEventListener(env, inst->thiz, event.type, event.data);
            if (status < 0) {
                ErrPrint("Failed to invoke the event listener");
            }

            inst->jvm->DetachCurrentThread();
        } else {
            ErrPrint("Failed to attach current thread");
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
    }

    // NOTE:
    // event.data must be preserved even if the discovery destroy the eventData
    // because we are not able to know when the EventObject in the managed code
    // is going to be destroyed
    inst->discovery->DestroyEventData(evtData);
    return ret;
}

jlong DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_create(JNIEnv *env, jobject thiz, jobjectArray args)
{
    DiscoveryNativeInterface *inst = nullptr;
    int argc = env->GetArrayLength(args);
    const char **argv = nullptr;
    jstring *strs = nullptr;

    try {
        argv = new const char *[argc];
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "new failed");
        return 0;
    }

    try {
        strs = new jstring[argc];
    } catch (std::exception &e) {
        ErrPrint("new failed: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "new failed");
        delete[] argv;
        return 0;
    }

    for (int i = 0; i < argc; i++) {
        strs[i] = static_cast<jstring>(env->GetObjectArrayElement(args, i));
        if (env->ExceptionCheck() == true) {
            JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
            while (--i >= 0) {
                env->ReleaseStringUTFChars(strs[i], argv[i]);
            }
            delete[] strs;
            delete[] argv;
            return 0;
        }
        argv[i] = env->GetStringUTFChars(strs[i], 0);
    }

    do {
        try {
            inst = new DiscoveryNativeInterface();
        } catch (std::exception &e) {
            env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "new failed");
            break;
        }

        if (env->GetJavaVM(&inst->jvm) != 0) {
            ErrPrint("Unable to get javaVM");
            env->ThrowNew(env->FindClass("java/lang/InternalError"), "jvm get failed");
            delete inst;
            inst = nullptr;
            break;
        }

        beyond_argument arg = {
            .argc = argc,
            .argv = const_cast<char **>(argv),
        };
        inst->discovery = beyond::Discovery::Create(&arg);
        if (inst->discovery == nullptr) {
            ErrPrint("beyond::Discovery::Create failed");
            env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "beyond::Discovery::Create failed");
            delete inst;
            inst = nullptr;
            break;
        }

        if (inst->discovery->GetHandle() >= 0) {
            inst->looper = ALooper_forThread();
            if (inst->looper == nullptr) {
                ErrPrint("There is no android looper found");
                delete inst;
                inst = nullptr;
                break;
            }

            ALooper_acquire(inst->looper);

            int ret = ALooper_addFd(inst->looper,
                                    inst->discovery->GetHandle(),
                                    ALOOPER_POLL_CALLBACK,
                                    ALOOPER_EVENT_INPUT,
                                    static_cast<ALooper_callbackFunc>(Discovery_eventHandler),
                                    static_cast<void *>(inst));
            if (ret < 0) {
                ErrPrint("Failed to add event handler");
                delete inst;
                inst = nullptr;
                break;
            } // otherwise, the ret is 1 if succeed
        }
    } while (0);

    for (int i = 0; i < argc; i++) {
        env->ReleaseStringUTFChars(strs[i], argv[i]);
    }
    delete[] strs;
    delete[] argv;
    return reinterpret_cast<jlong>(inst);
}

void DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_destroy(JNIEnv *env, jclass klass, jlong instance)
{
    if (instance == 0) {
        // NOTE:
        // destroy() method always returns success
        // later, we have to change return type of destroy() method to "void"
        return;
    }

    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
        return;
    }

    delete inst;
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_activate(JNIEnv *env, jobject thiz, jlong instance)
{
    if (instance == 0) {
        ErrPrint("discovery object is nullptr");
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid instance");
        return -EINVAL;
    }

    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
        return -EFAULT;
    }

    return inst->discovery->Activate();
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_deactivate(JNIEnv *env, jobject thiz, jlong instance)
{
    if (instance == 0) {
        ErrPrint("discovery object is nullptr");
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid instance");
        return -EINVAL;
    }

    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
        return -EFAULT;
    }

    return inst->discovery->Deactivate();
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_setItem(JNIEnv *env, jobject thiz, jlong instance, jstring key, jbyteArray value)
{
    if (instance == 0) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid instance");
        ErrPrint("discovery object is nullptr");
        return -EINVAL;
    }

    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
        return -EFAULT;
    }

    const char *key_cstr = env->GetStringUTFChars(key, 0);
    uint8_t valueSize = env->GetArrayLength(value);
    jbyte *valueBuf = env->GetByteArrayElements(value, 0);

    int ret = inst->discovery->SetItem(key_cstr, valueBuf, valueSize);

    env->ReleaseByteArrayElements(value, valueBuf, JNI_ABORT);
    env->ReleaseStringUTFChars(key, key_cstr);

    return ret;
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_removeItem(JNIEnv *env, jobject thiz, jlong instance, jstring key)
{
    if (instance == 0) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid instance");
        ErrPrint("discovery object is nullptr");
        return -EINVAL;
    }

    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
        return -EFAULT;
    }

    const char *key_cstr = env->GetStringUTFChars(key, 0);
    int ret = inst->discovery->RemoveItem(key_cstr);
    env->ReleaseStringUTFChars(key, key_cstr);

    // TODO:
    // ret would be changed to the exception

    return ret;
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_configure(JNIEnv *env, jobject thiz, jlong instance, jchar type, jobject obj)
{
    if (instance == 0) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid instance");
        ErrPrint("discovery object is nullptr");
        return -EINVAL;
    }

    DiscoveryNativeInterface *inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst->discovery == nullptr) {
        ErrPrint("invalid discovery object");
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
        ret = inst->discovery->Configure(&config);
    } else {
        ret = inst->discovery->Configure(&config);
    }

    return ret;
}

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_setEventListener(JNIEnv *env, jobject _thiz, jlong instance, jboolean flag)
{
    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst == nullptr || inst->discovery == nullptr) {
        ErrPrint("Discovery object is nullptr");
        return -EFAULT;
    }

    if (flag == JNI_TRUE) {
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

void *DiscoveryNativeInterface::GetBeyonDInstance(void)
{
    return static_cast<void *>(discovery);
}

int DiscoveryNativeInterface::RegisterNativeInterface(JNIEnv *env)
{
    JNINativeMethod discovery_methods[] = {
        { "initialize", "()V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_initialize) },
        { "create", "([Ljava/lang/String;)J", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_create) },
        { "destroy", "(J)V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_destroy) },
        { "activate", "(J)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_activate) },
        { "deactivate", "(J)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_deactivate) },
        { "setItem", "(JLjava/lang/String;[B)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_setItem) },
        { "removeItem", "(JLjava/lang/String;)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_removeItem) },
        { "configure", "(JCLjava/lang/Object;)I", reinterpret_cast<void *>((int (*)(JNIEnv *, jobject, jlong, jchar, jobject))Java_com_samsung_android_beyond_discovery_Discovery_configure) },
        { "setEventListener", "(JZ)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_setEventListener) },
    };

    jclass klass = env->FindClass("com/samsung/android/beyond/discovery/Discovery");
    if (klass == nullptr) {
        ErrPrint("Unable to find the Discovery class");
        return -EFAULT;
    }

    if (env->RegisterNatives(klass, discovery_methods, sizeof(discovery_methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        env->DeleteLocalRef(klass);
        ErrPrint("Failed to register discovery methods for BeyonD Java APIs");
        return -EFAULT;
    }
    env->DeleteLocalRef(klass);

    return 0;
}
