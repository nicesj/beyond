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
#include "NativeInterface.h"

#include <dlfcn.h>

#include <exception>

static void release_arguments(JNIEnv *env, int argc, const char **argv, jstring *strs);

PeerNativeInterface::PeerNativeInterface(void)
    : peer(nullptr)
{
}

PeerNativeInterface::~PeerNativeInterface(void)
{
}

void *PeerNativeInterface::GetBeyonDInstance(void)
{
    return peer;
}

jlong PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_create(JNIEnv *env, jobject thiz, jobject context, jobjectArray args)
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

#if USE_PEER_NN
    if (strncmp(argv[0], PEER_TYPE_PEER_NN, strlen(PEER_TYPE_PEER_NN)) == 0) {
        DbgPrint("Load module: %s", NNStreamerNativeLibraryName);
        void *dlHandle = dlopen(NNStreamerNativeLibraryName, RTLD_LAZY | RTLD_LOCAL);
        if (dlHandle == nullptr) {
            ErrPrint("dlopen: %s", dlerror());
            release_arguments(env, argc, argv, strs);
            return 0l;
        }

        DbgPrint("\tDLSYM %s", NNStreamerInitFunctionName);
        NNStreamerInitialization nnstreamer_native_initialize = reinterpret_cast<NNStreamerInitialization>(dlsym(dlHandle, NNStreamerInitFunctionName));
        if (nnstreamer_native_initialize == nullptr) {
            DbgPrint("Unable to find an entry function: %s", NNStreamerInitFunctionName);
            release_arguments(env, argc, argv, strs);
            return 0l;
        }

        jboolean isInitialized = nnstreamer_native_initialize(env, context);
        if (isInitialized != JNI_TRUE) {
            ErrPrint("Fail to initialize NNSTREAMER.");
            release_arguments(env, argc, argv, strs);
            return 0l;
        }
    }
#endif

    DbgPrint("\tStart BeyonD peer creation, type: %s", argv[0]);

    PeerNativeInterface *peer_handle;
    try {
        peer_handle = new PeerNativeInterface();
    } catch (std::exception &e) {
        ErrPrint("new: %s", e.what());
        release_arguments(env, argc, argv, strs);
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
        release_arguments(env, argc, argv, strs);
        return 0l;
    }

    release_arguments(env, argc, argv, strs);

    return reinterpret_cast<jlong>(peer_handle);
}

static void release_arguments(JNIEnv *env, int argc, const char **argv, jstring *strs)
{
    for (int i = 0; i < argc; i++) {
        env->ReleaseStringUTFChars(strs[i], argv[i]);
    }

    delete[] argv;
    delete[] strs;
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_set_info(JNIEnv *env, jobject thiz, jlong handle, jstring peer_ip, jint peer_port)
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

    const struct beyond_peer_info *info;
    int ret = peer_handle->peer->GetInfo(info);
    if (ret < 0) {
        ErrPrint("Fail to get peer info, ret = %d\n", ret);
        return false;
    }

    const char *ip = env->GetStringUTFChars(peer_ip, nullptr);
    if (ip == nullptr) {
        ErrPrint("Fail to get StringUTFChars of a peer IP.");
        return false;
    }
    DbgPrint("Peer IP = %s, ports[0] = %d, ports[1] = %d\n", ip, peer_port, peer_port + 1);
    struct beyond_peer_info new_info = {
        .host = const_cast<char *>(ip),
        .port = { static_cast<unsigned short>(peer_port), static_cast<unsigned short>(peer_port + 1) },
        .free_memory = 0llu,
        .free_storage = 0llu,
    };
    if (info == nullptr) {
        new_info.uuid[0] = '\0';
    } else {
        snprintf(new_info.uuid, BEYOND_UUID_LEN, "%s", info->uuid);
    }
    ret = peer_handle->peer->SetInfo(&new_info);
    if (ret < 0) {
        ErrPrint("Fail to set peer info, ret = %d\n", ret);
        env->ReleaseStringUTFChars(peer_ip, ip);
        return false;
    }

    env->ReleaseStringUTFChars(peer_ip, ip);

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
}

jboolean PeerNativeInterface::Java_com_samsung_android_beyond_InferencePeer_configure(JNIEnv *env, jobject thiz, jlong handle, jchar type, jobject obj)
{
    PeerNativeInterface *peer_handle = reinterpret_cast<PeerNativeInterface *>(handle);
    if (peer_handle == nullptr) {
        ErrPrint("peer_handle == nullptr");
        return false;
    }

    NativeInterface *nativeInterface = nullptr;

    if (obj != nullptr) {
        nativeInterface = static_cast<NativeInterface *>(JNIHelper::GetNativeInstance<void>(env, obj));
    }

    beyond_config _config = {
        .type = static_cast<char>(type),
        .object = nativeInterface == nullptr ? obj : nativeInterface->GetBeyonDInstance(),
    };

    int ret = peer_handle->peer->Configure(&_config);
    if (ret < 0) {
        ErrPrint("Fail to configure a peer, ret = %d", ret);
        return false;
    }

    return true;
}

int PeerNativeInterface::RegisterPeerNatives(JNIEnv *env)
{
    jclass klass = env->FindClass("com/samsung/android/beyond/inference/Peer");
    if (klass == nullptr) {
        return -EFAULT;
    }

    static JNINativeMethod peer_jni_methods[] = {
        { "create", "(Landroid/content/Context;[Ljava/lang/String;)J", (void *)Java_com_samsung_android_beyond_InferencePeer_create },
        { "setInfo", "(JLjava/lang/String;I)Z", (void *)Java_com_samsung_android_beyond_InferencePeer_set_info },
        { "activate", "(J)Z", (void *)Java_com_samsung_android_beyond_InferencePeer_activate },
        { "configure", "(JCLjava/lang/Object;)Z", (void *)Java_com_samsung_android_beyond_InferencePeer_configure },
        { "deactivate", "(J)Z", (void *)Java_com_samsung_android_beyond_InferencePeer_deactivate },
        { "destroy", "(J)V", (void *)Java_com_samsung_android_beyond_InferencePeer_destroy },
    };

    if (env->RegisterNatives(klass, peer_jni_methods,
                             sizeof(peer_jni_methods) / sizeof(JNINativeMethod)) != JNI_OK) {
        ErrPrint("Failed to register peer jni methods for BeyonD Java APIs.");
        return -EFAULT;
    }

    return 0;
}
