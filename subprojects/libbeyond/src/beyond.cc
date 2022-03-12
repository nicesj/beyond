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

#include <cstdio>
#include <cassert>

#include <sys/time.h>
#include <libgen.h>

#include "beyond/platform/beyond_platform.h"
#include "beyond/private/log_private.h"

#if !defined(REVISION)
#define REVISION "not defined"
#endif

__thread __beyond__tls__ __beyond;

extern "C" {
static void beyond_init(void) __attribute__((constructor));
static void beyond_fini(void) __attribute__((destructor));
}

void beyond_init(void)
{
    DbgPrint("Initialize BeyonD - " REVISION);
}

void beyond_fini(void)
{
    DbgPrint("Finalize BeyonD");
}
