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

#ifndef __BEYOND_PLATFORM_H__
#define __BEYOND_PLATFORM_H__

#ifndef LOG_TAG
#define LOG_TAG "BEYOND"
#endif

#include <stdio.h>
#include <libgen.h>

#include <jni.h>
#include <android/log.h>

#define PLATFORM_LOGD(fmt, ...)                     \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, \
                        "[%s:%s:%d] " fmt, basename((char *)(__FILE__)), __func__, __LINE__, ##__VA_ARGS__)

#define PLATFORM_LOGI(fmt, ...)                    \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, \
                        "[%s:%s:%d] " fmt, basename((char *)(__FILE__)), __func__, __LINE__, ##__VA_ARGS__)

#define PLATFORM_LOGE(fmt, ...)                     \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                        "[%s:%s:%d] " fmt, basename((char *)(__FILE__)), __func__, __LINE__, ##__VA_ARGS__)

#define BEYOND_CONFIG_TYPE_CONTEXT_ANDROID ((char)(0x0b))
struct beyond_config_context_android {
    JavaVM *javaVM;
    jobject applicationContext;
};

#endif // __BEYOND_PLATFORM_H__
