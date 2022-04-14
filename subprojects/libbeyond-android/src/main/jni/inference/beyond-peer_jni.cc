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

#include "beyond-peer_jni.h"
#include "JNIHelper.h"

#include <dlfcn.h>

#include <android/looper.h>
#include <exception>

PeerNativeInterface::Info PeerNativeInterface::infoObject = {
    .klass = nullptr,
    .constructor = nullptr,
    .host = nullptr,
    .port = nullptr,
    .uuid = nullptr,
};

PeerNativeInterface::EventListnerJNICache PeerNativeInterface::eventListenerJNICache = {
    .eventListenerClass = nullptr,
    .onEventMethodId = nullptr,
};

PeerNativeInterface::PeerNativeInterface(void)
    : peer(nullptr)
    , looper(nullptr)
    , eventListenerObject(nullptr)
    , jvm(nullptr)
{
}

PeerNativeInterface::~PeerNativeInterface(void)
{
    if (looper != nullptr) {
        if (peer != nullptr && peer->GetHandle() >= 0) {
            ALooper_removeFd(looper, peer->GetHandle());
        }
        ALooper_release(looper);
        looper = nullptr;
    }

    if (eventListenerObject != nullptr) {
        JNIEnv *env = nullptr;
        if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) == JNI_OK) {
            env->DeleteGlobalRef(eventListenerObject);
            eventListenerObject = nullptr;
        }
    }

    if (peer != nullptr) {
        peer->Destroy();
        peer = nullptr;
    }
}

void *PeerNativeInterface::GetBeyonDInstance(void)
{
    return peer;
}

jlong PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_create(JNIEnv *env, jobject thiz, jobjectArray args)
{
    int argc = env->GetArrayLength(args);
    const char **argv = nullptr;
    jstring *strs = nullptr;
    if (env->ExceptionCheck() == true) {
        ErrPrint("There is an exception!");
        return 0l;
    }

    if (argc <= 0) {
        ErrPrint("argc <= 0");
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
            JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
            while (--i >= 0) {
                env->ReleaseStringUTFChars(strs[i], argv[i]);
            }
            delete[] strs;
            delete[] argv;
            return 0l;
        }

        argv[i] = env->GetStringUTFChars(strs[i], 0);
    }

    DbgPrint("\tStart BeyonD peer creation, type: %s", argv[0]);

    PeerNativeInterface *peer_handle;
    try {
        peer_handle = new PeerNativeInterface();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        ReleaseArguments(env, argc, argv, strs);
        return 0l;
    }

    if (env->GetJavaVM(&peer_handle->jvm) != 0) {
        ErrPrint("Unable to get javaVM");
        delete peer_handle;
        peer_handle = nullptr;
        ReleaseArguments(env, argc, argv, strs);
        return 0l;
    }

    beyond_argument arg = {
        .argc = argc,
        .argv = const_cast<char **>(argv),
    };
    peer_handle->peer = beyond::Inference::Peer::Create(&arg);
    if (peer_handle->peer == nullptr) {
        ErrPrint("Fail to create a peer instance, type: %s", argv[0]);
        delete peer_handle;
        peer_handle = nullptr;
        ReleaseArguments(env, argc, argv, strs);
        return 0l;
    }

    ReleaseArguments(env, argc, argv, strs);

    if (argc == 1 && peer_handle->peer->GetHandle() >= 0) {
        if (peer_handle->AttachEventLoop(env) < 0) {
            ErrPrint("Fail to connect BeyonD event loop to Android looper.");
            delete peer_handle;
            peer_handle = nullptr;
            return 0l;
        }
    }

    return reinterpret_cast<jlong>(peer_handle);
}

int PeerNativeInterface::AttachEventLoop(JNIEnv *env)
{
    int ret = 0;

    this->looper = ALooper_forThread();
    if (this->looper == nullptr) {
        ErrPrint("Synchronous execution. Or there is no Android looper available.");
        return 0;
    }

    ALooper_acquire(this->looper);

    ret = ALooper_addFd(this->looper,
                        this->peer->GetHandle(),
                        ALOOPER_POLL_CALLBACK,
                        ALOOPER_EVENT_INPUT,
                        static_cast<ALooper_callbackFunc>(PeerEventHandler),
                        static_cast<void *>(this));
    if (ret < 0) {
        ErrPrint("Fail to add a fd to Android looper.");
        return -EFAULT;
    }

    return ret;
}

int PeerNativeInterface::PeerEventHandler(int fd, int events, void *data)
{
    PeerNativeInterface *peer_handle_ = reinterpret_cast<PeerNativeInterface *>(data);
    assert(peer_handle_ != nullptr && "A peer handle is nullptr");
    assert(peer_handle_->peer != nullptr && "peer is nullptr");

    int ret = 1;
    int status = 0;
    beyond_event_info event = {
        .type = beyond_event_type::BEYOND_EVENT_TYPE_NONE,
        .data = nullptr,
    };
    beyond::EventObjectInterface::EventData *evtData = nullptr;
    if ((events & (ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR)) != 0) {
        ErrPrint("Error events occur.");
        ret = 0;
        event.type |= beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
    } else if ((events & ALOOPER_EVENT_INPUT) == ALOOPER_EVENT_INPUT) {
        status = peer_handle_->peer->FetchEventData(evtData);
        if (status < 0 || evtData == nullptr || (evtData->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
            if (status < 0) {
                ErrPrintCode(-status, "Fail to fetch an event data.");
            }

            event.type |= beyond_event_type::BEYOND_EVENT_TYPE_ERROR;
        } else {
            event.type |= evtData->type;
            event.data = evtData->data;

            status = peer_handle_->InvokeEventListener();
            if (status < 0) {
                ErrPrintCode(-status, "Fail to invoke an output callback.");
            }
        }
    } else {
        ErrPrint("Unknown event type.");
    }

    return ret;
}

int PeerNativeInterface::InvokeEventListener()
{
    JNIEnv *env = nullptr;
    int JNIStatus = jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (JNIStatus == JNI_EDETACHED) {
        if (jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            ErrPrint("Fail to attach the current thread.");
            return -EFAULT;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        ErrPrint("GetEnv: Unsupported version");
        return -EFAULT;
    }

    if (eventListenerObject == nullptr) {
        ErrPrint("Callback object is null.");
        DetachAttachedThread(JNIStatus);
        return -EINVAL;
    }
    env->CallVoidMethod(eventListenerObject, eventListenerJNICache.onEventMethodId, nullptr);
    if (env->ExceptionCheck() == true) {
        ErrPrint("Fail to call a void method.");
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        DetachAttachedThread(JNIStatus);
        return -EFAULT;
    }

    return 0;
}

void PeerNativeInterface::DetachAttachedThread(int JNIStatus)
{
    if (JNIStatus == JNI_EDETACHED) {
        if (jvm->DetachCurrentThread() != JNI_OK) {
            ErrPrint("Fail to attach the current thread.");
        }
    }
}

void PeerNativeInterface::ReleaseArguments(JNIEnv *env, int argc, const char **argv, jstring *strs)
{
    for (int i = 0; i < argc; i++) {
        env->ReleaseStringUTFChars(strs[i], argv[i]);
    }

    delete[] argv;
    delete[] strs;
}

jobject PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_get_info(JNIEnv *env, jobject thiz, jlong handle)
{
    if (env == nullptr) {
        ErrPrint("JNIEnv is nullptr");
        return nullptr;
    }

    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return nullptr;
    }

    const beyond_peer_info *info = nullptr;
    if (peer_handle->peer->GetInfo(info) < 0) {
        ErrPrint("Failed to get the peer info");
        return nullptr;
    }

    jobject object = env->NewObject(infoObject.klass, infoObject.constructor);
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        return nullptr;
    }
    if (object == nullptr) {
        ErrPrint("Failed to construct a new info object");
        return nullptr;
    }

    if (info->host != nullptr) {
        jstring host = env->NewStringUTF(info->host);
        if (env->ExceptionCheck() == true) {
            JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
            return nullptr;
        }

        if (host == nullptr) {
            ErrPrint("host is not valid");
            return nullptr;
        }

        env->SetObjectField(object, infoObject.host, host);
    }

    jstring uuid = env->NewStringUTF(info->uuid);
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        return nullptr;
    }

    if (uuid == nullptr) {
        ErrPrint("uuid is not valid");
        return nullptr;
    }

    env->SetObjectField(object, infoObject.uuid, static_cast<jobject>(uuid));

    // TODO:
    // Port array should be changed to a single value
    // No more need to keep the multiple port information
    env->SetIntField(object, infoObject.port, static_cast<const int>(info->port[0]));
    return object;
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_set_info(JNIEnv *env, jobject thiz, jlong handle, jobject info)
{
    if (env == nullptr) {
        ErrPrint("JNIEnv is nullptr.");
        return false;
    }

    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return false;
    }

    beyond_peer_info peerInfo = {
        .name = nullptr,
        .host = nullptr,
        .port = { 0, 0 },
        .uuid = {
            0,
        },
        .free_memory = 0llu,
        .free_storage = 0llu,
        .count_of_runtimes = 0,
        .runtimes = nullptr,
    };

    jobject host = env->GetObjectField(info, infoObject.host);
    jobject uuid = env->GetObjectField(info, infoObject.uuid);
    jint port = env->GetIntField(info, infoObject.port);

    if (host != nullptr) {
        peerInfo.host = const_cast<char *>(env->GetStringUTFChars(static_cast<jstring>(host), nullptr));
    }

    if (uuid != nullptr) {
        const char *_uuid = env->GetStringUTFChars(static_cast<jstring>(uuid), nullptr);
        if (_uuid != nullptr) {
            snprintf(peerInfo.uuid, BEYOND_UUID_LEN, "%s", _uuid);
            env->ReleaseStringUTFChars(static_cast<jstring>(uuid), _uuid);
        }
    }

    peerInfo.port[0] = static_cast<unsigned short>(port);
    peerInfo.port[1] = 0;
    peerInfo.free_memory = 0llu;
    peerInfo.free_storage = 0llu;
    peerInfo.count_of_runtimes = 0;
    peerInfo.runtimes = nullptr;

    int ret = peer_handle->peer->SetInfo(&peerInfo);

    if (peerInfo.host != nullptr) {
        env->ReleaseStringUTFChars(static_cast<jstring>(host), peerInfo.host);
    }

    if (ret < 0) {
        ErrPrint("Fail to set peer info, ret = %d\n", ret);
        return false;
    }

    return true;
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_activate(JNIEnv *env, jobject thiz, jlong handle)
{
    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return false;
    }

    int ret = peer_handle->peer->Activate();
    if (ret < 0) {
        ErrPrint("Fail to activate a peer, ret = %d\n", ret);
        return false;
    }

    return true;
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_deactivate(JNIEnv *env, jobject thiz, jlong handle)
{
    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return false;
    }

    int ret = peer_handle->peer->Deactivate();
    if (ret < 0) {
        ErrPrint("Fail to deactivate a peer, ret = %d\n", ret);
        return false;
    }

    return true;
}

void PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_destroy(JNIEnv *env, jclass klass, jlong handle)
{
    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return;
    }

    peer_handle->peer->Destroy();
    peer_handle = nullptr;
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_configure(JNIEnv *env, jobject thiz, jlong handle, jchar type, jobject obj)
{
    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle->peer == nullptr) {
        ErrPrint("peer == nullptr");
        return false;
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
            .javaVM = peer_handle->jvm,
            .applicationContext = obj,
        };

        config.object = static_cast<void *>(&ctx);
        ret = peer_handle->peer->Configure(&config);
    } else {
        ret = peer_handle->peer->Configure(&config);
    }

    return ret == 0;
}

void PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_initialize(JNIEnv *env, jclass klass)
{
    jclass infoKlass = env->FindClass("com/samsung/android/beyond/inference/PeerInfo");
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

    infoObject.constructor = env->GetMethodID(infoObject.klass, "<init>", "()V");
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

    infoObject.host = env->GetFieldID(infoObject.klass, "host", "Ljava/lang/String;");
    if (env->ExceptionCheck() == true) {
        JNIHelper::PrintException(env, __FUNCTION__, __LINE__);
        env->DeleteGlobalRef(infoObject.klass);
        infoObject.klass = nullptr;
        return;
    }
    if (infoObject.host == nullptr) {
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

    InitCallbackInfo(env);

    DbgPrint("Successfully initialized");
}

void PeerNativeInterface::InitCallbackInfo(JNIEnv *env)
{
    try {
        jclass eventListenerClass = JNIHelper::FindClass(env,
                                                         "com/samsung/android/beyond/EventListener");

        eventListenerJNICache.eventListenerClass = static_cast<jclass>(env->NewGlobalRef(eventListenerClass));
        env->DeleteLocalRef(eventListenerClass);

        eventListenerJNICache.onEventMethodId = JNIHelper::GetMethodID(env,
                                                                       eventListenerJNICache.eventListenerClass,
                                                                       "onEvent",
                                                                       "(Lcom/samsung/android/beyond/EventObject;)V");
    } catch (std::exception &e) {
        ErrPrint("An exception occurs.");
        return;
    }
}

jint PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_setEventListener(JNIEnv *env, jobject thiz, jlong handle, jobject event_listener_object)
{
    if (event_listener_object == nullptr) {
        ErrPrint("The given event listener object is null.");
        return -EINVAL;
    }

    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return false;
    }
    peer_handle->eventListenerObject = env->NewGlobalRef(event_listener_object);
    if (env->ExceptionCheck() == true) {
        JNIPrintException(env);
        return -EFAULT;
    }

    return 0;
}

int PeerNativeInterface::RegisterPeerNatives(JNIEnv *env)
{
    jclass klass = env->FindClass("com/samsung/android/beyond/inference/Peer");
    if (klass == nullptr) {
        ErrPrint("Unable to find Peer class.");
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
        }
        return -EFAULT;
    }

    static JNINativeMethod peer_jni_methods[] = {
        { "initialize", "()V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_initialize) },
        { "create", "([Ljava/lang/String;)J", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_create) },
        { "setInfo", "(JLcom/samsung/android/beyond/inference/PeerInfo;)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_set_info) },
        { "getInfo", "(J)Lcom/samsung/android/beyond/inference/PeerInfo;", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_get_info) },
        { "activate", "(J)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_activate) },
        { "configure", "(JCLjava/lang/Object;)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_configure) },
        { "deactivate", "(J)Z", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_deactivate) },
        { "destroy", "(J)V", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_destroy) },
        { "setEventListener", "(JLcom/samsung/android/beyond/EventListener;)I", reinterpret_cast<void *>(Java_com_samsung_android_beyond_InferencePeer_setEventListener) },
    };

    if (env->RegisterNatives(klass, peer_jni_methods, sizeof(peer_jni_methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        ErrPrint("Failed to register peer jni methods for BeyonD Java APIs.");
        return -EFAULT;
    }

    return 0;
}
