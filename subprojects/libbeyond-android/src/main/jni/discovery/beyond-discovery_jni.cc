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
#define BEYOND_DISCOVERY_INFO_CLASS_NAME "com/samsung/android/beyond/discovery/Discovery$Info"
#define BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_NAME "<init>"
#define BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_SIGNATURE "()V"
#define BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_TYPE_FIELD_NAME "eventType"
#define BEYOND_DISCOVERY_EVENT_OBJECT_EVENT_DATA_FIELD_NAME "eventData"

DiscoveryNativeInterface::EventObject DiscoveryNativeInterface::eventObject = {
    .klass = nullptr,
    .constructor = nullptr,
    .eventType = nullptr,
    .eventData = nullptr,
};

DiscoveryNativeInterface::Info DiscoveryNativeInterface::infoObject = {
    .klass = nullptr,
    .constructor = nullptr,
    .name = nullptr,
    .host = nullptr,
    .port = nullptr,
    .uuid = nullptr,
};

DiscoveryNativeInterface::DiscoveryNativeInterface(void)
    : discovery(nullptr)
    , looper(nullptr)
    , jvm(nullptr)
    , listener(nullptr)
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

    if (listener != nullptr) {
        JNIHelper::ExecuteWithEnv(jvm, [&](JNIEnv *env, void *data) -> int {
            // NOTE:
            // this can be happens when the user calls "close" method directly
            DbgPrint("Event listener is not cleared, forcibly delete the reference");
            env->DeleteGlobalRef(listener);
            listener = nullptr;
            return 0;
        });
    }

    jvm = nullptr;
}

void DiscoveryNativeInterface::initializeInfo(JNIEnv *env, jclass klass)
{
    jclass infoKlass = env->FindClass(BEYOND_DISCOVERY_INFO_CLASS_NAME);
    if (infoKlass == nullptr) {
        ErrPrint("Unable to get the class");
        return;
    }

    infoObject.klass = static_cast<jclass>(env->NewGlobalRef(infoKlass));
    env->DeleteLocalRef(infoKlass);
    if (infoObject.klass == nullptr) {
        ErrPrint("Unable to get the Info class");
        return;
    }

    infoObject.constructor = env->GetMethodID(infoObject.klass,
                                              BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_NAME,
                                              BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_SIGNATURE);
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.constructor == nullptr) {
        ErrPrint("Unable to get the info constructor");
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }

    infoObject.name = env->GetFieldID(infoObject.klass, "name", "Ljava/lang/String;");
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.name == nullptr) {
        ErrPrint("Unable to get the name field from info");
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }

    infoObject.host = env->GetFieldID(infoObject.klass, "host", "Ljava/lang/String;");
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.name == nullptr) {
        ErrPrint("Unable to get the host field from info");
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }

    infoObject.port = env->GetFieldID(infoObject.klass, "port", "I");
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.port == nullptr) {
        ErrPrint("Unable to get the port field from info");
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }

    infoObject.uuid = env->GetFieldID(infoObject.klass, "uuid", "Ljava/lang/String;");
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.uuid == nullptr) {
        ErrPrint("Unable to get the uuid field from info");
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
}

void DiscoveryNativeInterface::initializeEventObject(JNIEnv *env, jclass klass)
{
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
                                               BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_NAME,
                                               BEYOND_DISCOVERY_DEFAULT_CONSTRUCTOR_SIGNATURE);
    if (eventObject.constructor == nullptr) {
        ErrPrint("Unable to find the event constructor metohd");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        env->DeleteGlobalRef(eventObject.klass);
        eventObject.klass = nullptr;
        return;
    }
}

void DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_initialize(JNIEnv *env, jclass klass)
{
    initializeInfo(env, klass);
    initializeEventObject(env, klass);
    DbgPrint("Discovery JNI cache initialized");
}

jobject DiscoveryNativeInterface::NewEventObject(JNIEnv *env, void *eventData)
{
    beyond_peer_info *info = static_cast<beyond_peer_info *>(eventData);
    jstring host = nullptr;
    jstring name = nullptr;

    jobject object = env->NewObject(infoObject.klass, infoObject.constructor);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return nullptr;
    }
    if (object == nullptr) {
        ErrPrint("Failed to construct a new info object");
        return nullptr;
    }

    if (info->host == nullptr) {
        DbgPrint("There is no host information");
    } else {
        host = env->NewStringUTF(info->host);
        if (env->ExceptionCheck() == true) {
            JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
            env->DeleteLocalRef(object);
            return nullptr;
        }

        if (host == nullptr) {
            ErrPrint("host is not valid");
            env->DeleteLocalRef(object);
            return nullptr;
        }

        env->SetObjectField(object, infoObject.host, host);
    }

    if (info->name == nullptr) {
        DbgPrint("There is no service name");
    } else {
        name = env->NewStringUTF(info->name);
        if (env->ExceptionCheck() == true) {
            JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
            env->DeleteLocalRef(object);
            env->DeleteLocalRef(host);
            return nullptr;
        }

        if (name == nullptr) {
            ErrPrint("host is not valid");
            env->DeleteLocalRef(object);
            env->DeleteLocalRef(host);
            return nullptr;
        }

        env->SetObjectField(object, infoObject.name, name);
    }

    jstring uuid = env->NewStringUTF(info->uuid);
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteLocalRef(object);
        env->DeleteLocalRef(host);
        env->DeleteLocalRef(name);
        return nullptr;
    }

    if (uuid == nullptr) {
        ErrPrint("uuid is not valid");
        env->DeleteLocalRef(object);
        env->DeleteLocalRef(host);
        env->DeleteLocalRef(name);
        return nullptr;
    }

    env->SetObjectField(object, infoObject.uuid, static_cast<jobject>(uuid));

    // TODO:
    // Port array should be changed to a single value
    // No more need to keep the multiple port information
    env->SetIntField(object, infoObject.port, static_cast<const int>(info->port[0]));

    return object;
}

int DiscoveryNativeInterface::InvokeEventListener(JNIEnv *env, int eventType, void *eventData)
{
    jobject object = env->NewObject(eventObject.klass, eventObject.constructor);
    if (object == nullptr) {
        ErrPrint("Failed to construct a new event object");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        return -EFAULT;
    }

    env->SetIntField(object, eventObject.eventType, eventType);
    jobject evtObj = nullptr;
    if (eventData != nullptr) {
        evtObj = NewEventObject(env, eventData);
        if (evtObj == nullptr) {
            env->DeleteLocalRef(object);
            return -EFAULT;
        }

        env->SetObjectField(object, eventObject.eventData, evtObj);
    }

    int ret = JNIHelper::CallVoidMethod(env, listener,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_NAME,
                                        BEYOND_EVENT_LISTENER_ON_EVENT_METHOD_SIGNATURE,
                                        object);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(evtObj);
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

    if (inst->listener != nullptr) {
        JNIHelper::ExecuteWithEnv(inst->jvm, [inst, &event](JNIEnv *env, void *data) -> int {
            // NOTE:
            // Now, publish the event object to the event listener
            int status = inst->InvokeEventListener(env, event.type, event.data);
            if (status < 0) {
                ErrPrint("Failed to invoke the event listener");
            }
            return status;
        });
    } else {
        DbgPrint("EventListener is not registered");
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
            int ret = inst->AttachEventLoop();
            if (ret < 0) {
                delete inst;
                inst = nullptr;
                break;
            }
        }
    } while (0);

    for (int i = 0; i < argc; i++) {
        env->ReleaseStringUTFChars(strs[i], argv[i]);
    }
    delete[] strs;
    delete[] argv;
    return reinterpret_cast<jlong>(inst);
}

int DiscoveryNativeInterface::AttachEventLoop(void)
{
    looper = ALooper_forThread();
    if (looper == nullptr) {
        ErrPrint("There is no android looper found");
        return -ENOTSUP;
    }

    ALooper_acquire(looper);

    int ret = ALooper_addFd(looper,
                            discovery->GetHandle(),
                            ALOOPER_POLL_CALLBACK,
                            ALOOPER_EVENT_INPUT,
                            static_cast<ALooper_callbackFunc>(Discovery_eventHandler),
                            static_cast<void *>(this));
    if (ret < 0) {
        ErrPrint("Failed to add event handler");
    } // otherwise, the ret is 1 if succeed

    return ret;
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

jint DiscoveryNativeInterface::Java_com_samsung_android_beyond_discovery_Discovery_setEventListener(JNIEnv *env, jobject _thiz, jlong instance, jobject listener)
{
    auto inst = reinterpret_cast<DiscoveryNativeInterface *>(instance);
    if (inst == nullptr || inst->discovery == nullptr) {
        ErrPrint("Discovery object is nullptr");
        return -EFAULT;
    }

    if (inst->listener != nullptr) {
        env->DeleteGlobalRef(inst->listener);
        inst->listener = nullptr;
    }

    if (listener != nullptr) {
        inst->listener = env->NewGlobalRef(listener);
        if (inst->listener == nullptr) {
            ErrPrint("Failed to get a global reference");
            return -EFAULT;
        }
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
        { "setEventListener", "(J" BEYOND_EVENT_LISTENER_CLASS_TYPE ")I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_discovery_Discovery_setEventListener) },
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
