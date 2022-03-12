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

#ifndef __BEYOND_PRIVATE_LOG_H__
#define __BEYOND_PRIVATE_LOG_H__

#include <stdio.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#if defined(SYS_gettid)
#if defined(__APPLE__)
#include <sys/types.h>
#define GETTID() ((unsigned long)getpid())
#else
#define GETTID() syscall(SYS_gettid)
#endif // __APPLE__
#else // SYS_gettid
#define GETTID() 0lu
#endif // SYS_gettid

#if !defined(PLATFORM_LOGD)
#define PLATFORM_LOGD(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#endif
#if !defined(PLATFORM_LOGI)
#define PLATFORM_LOGI(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#endif
#if !defined(PLATFORM_LOGE)
#define PLATFORM_LOGE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#define BEYOND_ERRMSG_LEN 80

// Increase this if you need more space for profiling
#define BEYOND_PROFILE_ID_MAX 8

struct __beyond__tls__ {
    struct timeval last_timestamp;
    struct timeval profile_timestamp[BEYOND_PROFILE_ID_MAX];
    char profile_idx; // Max to 255
    char initialized; // Max to 255, but we only use 0 or 1
};

extern __thread struct __beyond__tls__ __beyond;

#if (_POSIX_C_SOURCE >= 200112L) && !defined(_GNU_SOURCE) || defined(__APPLE__)
#define BEYOND_STRERROR_R(errno, buf, buflen)        \
    do {                                             \
        int ret = strerror_r(errno, buf, buflen);    \
        if (ret != 0) {                              \
            assert(ret == 0 && "strerror_r failed"); \
        }                                            \
    } while (0)
#else
#define BEYOND_STRERROR_R(errno, buf, buflen)                 \
    do {                                                      \
        const char *errstr = strerror_r(errno, buf, buflen);  \
        if (errstr == nullptr) {                              \
            assert(errstr != nullptr && "strerror_r failed"); \
        }                                                     \
    } while (0)
#endif

#define InitBeyond()                                                \
    do {                                                            \
        if (__beyond.initialized == 0) {                            \
            __beyond.initialized = 1;                               \
            if (gettimeofday(&__beyond.last_timestamp, NULL) < 0) { \
                assert(!"gettimeofday failed");                     \
            }                                                       \
        }                                                           \
    } while (0)

#define PROFILE_MARK()                                                                       \
    do {                                                                                     \
        if (__beyond.profile_idx < BEYOND_PROFILE_ID_MAX) {                                  \
            if (gettimeofday(__beyond.profile_timestamp + __beyond.profile_idx, NULL) < 0) { \
                assert(!"gettimeofday failed");                                              \
            }                                                                                \
            ++__beyond.profile_idx;                                                          \
        } else {                                                                             \
            ErrPrint("Unable to mark a profile point: %d\n", __beyond.profile_idx);          \
        }                                                                                    \
    } while (0)

#define PROFILE_ESTIMATE(tres)                                                                    \
    do {                                                                                          \
        struct timeval tv;                                                                        \
        struct timeval res;                                                                       \
        if (gettimeofday(&tv, NULL) < 0) {                                                        \
            assert(!"gettimeofday failed");                                                       \
        }                                                                                         \
        --__beyond.profile_idx;                                                                   \
        timersub(&tv, __beyond.profile_timestamp + __beyond.profile_idx, &res);                   \
        (tres) = static_cast<double>(res.tv_sec) + static_cast<double>(res.tv_usec) / 1000000.0f; \
    } while (0)

#define PROFILE_RESET(count)             \
    do {                                 \
        __beyond.profile_idx -= (count); \
    } while (0)

#if defined(_LOG_WITH_TIMESTAMP)
#if defined(NDEBUG)
#define DbgPrint(fmt, ...)
#else
#define DbgPrint(fmt, ...)                                                                \
    do {                                                                                  \
        struct timeval tv;                                                                \
        struct timeval res;                                                               \
        InitBeyond();                                                                     \
        if (gettimeofday(&tv, NULL) < 0) {                                                \
            assert(!"gettimeofday failed");                                               \
        }                                                                                 \
        timersub(&tv, &__beyond_last_timestamp, &res);                                    \
        PLATFORM_LOGD("[%lu] %lu.%.6lu(%lu.%.6lu) " fmt, GETTID(), tv.tv_sec, tv.tv_usec, \
                      res.tv_sec, res.tv_usec, ##__VA_ARGS__);                            \
        __beyond_last_timestamp.tv_sec = tv.tv_sec;                                       \
        __beyond_last_timestamp.tv_usec = tv.tv_usec;                                     \
    } while (0)
#endif

#define InfoPrint(fmt, ...)                                                               \
    do {                                                                                  \
        struct timeval tv;                                                                \
        struct timeval res;                                                               \
        InitBeyond();                                                                     \
        if (gettimeofday(&tv, NULL) < 0) {                                                \
            assert(!"gettimeofday failed");                                               \
        }                                                                                 \
        timersub(&tv, &__beyond_last_timestamp, &res);                                    \
        PLATFORM_LOGI("[%lu] %lu.%.6lu(%lu.%.6lu) " fmt, GETTID(), tv.tv_sec, tv.tv_usec, \
                      res.tv_sec, res.tv_usec, ##__VA_ARGS__);                            \
        __beyond_last_timestamp.tv_sec = tv.tv_sec;                                       \
        __beyond_last_timestamp.tv_usec = tv.tv_usec;                                     \
    } while (0)

#define ErrPrint(fmt, ...)                                                                                  \
    do {                                                                                                    \
        struct timeval tv;                                                                                  \
        struct timeval res;                                                                                 \
        InitBeyond();                                                                                       \
        if (gettimeofday(&tv, NULL) < 0) {                                                                  \
            assert(!"gettimeofday failed");                                                                 \
        }                                                                                                   \
        timersub(&tv, &__beyond_last_timestamp, &res);                                                      \
        PLATFORM_LOGE("[%lu] %lu.%.6lu(%lu.%.6lu) \033[31m" fmt "\033[0m", GETTID(), tv.tv_sec, tv.tv_usec, \
                      res.tv_sec, res.tv_usec, ##__VA_ARGS__);                                              \
        __beyond_last_timestamp.tv_sec = tv.tv_sec;                                                         \
        __beyond_last_timestamp.tv_usec = tv.tv_usec;                                                       \
    } while (0)

#define ErrPrintCode(_beyond_errno, fmt, ...)                                                                       \
    do {                                                                                                            \
        struct timeval tv;                                                                                          \
        struct timeval res;                                                                                         \
        char errMsg[BEYOND_ERRMSG_LEN] = { '\0' };                                                                  \
        int _errno = (_beyond_errno);                                                                               \
                                                                                                                    \
        BEYOND_STRERROR_R(_errno, errMsg, sizeof(errMsg));                                                          \
                                                                                                                    \
        InitBeyond();                                                                                               \
        if (gettimeofday(&tv, NULL) < 0) {                                                                          \
            assert(!"gettimeofday failed");                                                                         \
        }                                                                                                           \
        timersub(&tv, &__beyond_last_timestamp, &res);                                                              \
        PLATFORM_LOGE("[%lu] %lu.%.6lu(%lu.%.6lu) (%d:%s) \033[31m" fmt "\033[0m", GETTID(), tv.tv_sec, tv.tv_usec, \
                      res.tv_sec, res.tv_usec, _errno, errMsg, ##__VA_ARGS__);                                      \
        __beyond_last_timestamp.tv_sec = tv.tv_sec;                                                                 \
        __beyond_last_timestamp.tv_usec = tv.tv_usec;                                                               \
    } while (0)

#else // _LOG_WITH_TIMESTAMP
#if defined(NDEBUG)
#define DbgPrint(fmt, ...)
#else
#define DbgPrint(fmt, ...) PLATFORM_LOGD("[%lu] " fmt, GETTID(), ##__VA_ARGS__)
#endif

#define InfoPrint(fmt, ...) PLATFORM_LOGI("[%lu] " fmt, GETTID(), ##__VA_ARGS__)
#define ErrPrint(fmt, ...) PLATFORM_LOGE("[%lu] \033[31m" fmt "\033[0m", GETTID(), ##__VA_ARGS__)
#define ErrPrintCode(_beyond_errno, fmt, ...)                                                           \
    do {                                                                                                \
        char errMsg[BEYOND_ERRMSG_LEN] = { '\0' };                                                      \
        int _errno = (_beyond_errno);                                                                   \
        BEYOND_STRERROR_R(_errno, errMsg, sizeof(errMsg));                                              \
        PLATFORM_LOGE("[%lu] (%d:%s) \033[31m" fmt "\033[0m", GETTID(), _errno, errMsg, ##__VA_ARGS__); \
    } while (0)

#endif // _LOG_WITH_TIMESTAMP

#endif // __BEYOND_PRIVATE_LOG_H__
