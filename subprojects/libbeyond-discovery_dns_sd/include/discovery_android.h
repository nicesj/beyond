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

#ifndef __BEYOND_DISCOVERY_DNS_SD_H__
#define __BEYOND_DISCOVERY_DNS_SD_H__

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>
#include <beyond/plugin/discovery_dns_sd_plugin.h>
#include <stdarg.h>

#define JNIPrintException(env) Discovery::PrintException(env, __FUNCTION__, __LINE__)

class API Discovery : public beyond::DiscoveryInterface::RuntimeInterface {
public:
    Discovery(void);
    virtual ~Discovery(void);

public:
    static constexpr const char *NAME = "discovery_dns_sd";

    // module interface
    const char *GetModuleName(void) const override;
    const char *GetModuleType(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int Configure(const beyond_config *options = nullptr) override;

    static void PrintException(JNIEnv *env, const char *funcname, int lineno, bool clear = true);

    template <typename T>
    static T CallObjectMethod(JNIEnv *env, jobject obj, const char *methodName, const char *signature, ...)
    {
        jclass klass = env->GetObjectClass(obj);
        if (klass == nullptr) {
            ErrPrint("Unable to get the class");
            return static_cast<T>(nullptr);
        }

        // First get the class object
        jmethodID methodId = env->GetMethodID(klass, methodName, signature);
        env->DeleteLocalRef(klass);
        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
            return static_cast<T>(nullptr);
        }
        if (methodId == nullptr) {
            ErrPrint("Invalid object");
            return static_cast<T>(nullptr);
        }

        va_list ap;
        va_start(ap, signature);
        T retObj = static_cast<T>(env->CallObjectMethodV(obj, methodId, ap));
        va_end(ap);

        if (env->ExceptionCheck() == true) {
            JNIPrintException(env);
            return static_cast<T>(nullptr);
        }

        return retObj;
    }

private:
    JavaVM *jvm;
    jobject nsdManager;
};

#endif // __BEYOND_DISCOVERY_DNS_SD_H__
