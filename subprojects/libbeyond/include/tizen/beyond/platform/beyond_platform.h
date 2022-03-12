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

#define LOG_RED "\033[31m"
#define LOG_GREEN "\033[32m"
#define LOG_BROWN "\033[33m"
#define LOG_BLUE "\033[34m"
#define LOG_END "\033[0m"

#include <dlog/dlog.h>

#ifdef DLOG_STDOUT
#include <stdio.h>
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PLATFORM_LOGD(fmt, arg...) printf("[%s]%s(%d):" fmt "\n", LOG_TAG, __FILENAME__, __LINE__, ##arg)
#define PLATFORM_LOGI(fmt, arg...) printf("[%s]%s(%d):" fmt "\n", LOG_TAG, __FILENAME__, __LINE__, ##arg)
#define PLATFORM_LOGE(fmt, arg...) printf(LOG_RED "[%s]%s(%d):" fmt LOG_END "\n", LOG_TAG, __FILENAME__, __LINE__, ##arg)
#else
#define PLATFORM_LOGD(fmt, ...) LOGD(fmt, ##__VA_ARGS__)
#define PLATFORM_LOGI(fmt, ...) LOGI(fmt, ##__VA_ARGS__)
#define PLATFORM_LOGE(fmt, ...) LOGE(fmt, ##__VA_ARGS__)
#endif /* DLOG_STDOUT */

#endif // __BEYOND_PLATFORM_H__
