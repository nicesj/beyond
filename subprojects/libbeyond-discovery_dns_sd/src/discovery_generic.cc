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
#ifndef ANDROID
#include "discovery.h"
#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

const char *Discovery::GetModuleName(void) const
{
    return Discovery::NAME;
}

const char *Discovery::GetModuleType(void) const
{
    return beyond::ModuleInterface::TYPE_DISCOVERY;
}

int Discovery::AddHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Discovery::RemoveHandler(beyond_event_handler_t handler, int type, void *data)
{
    return -ENOTSUP;
}

int Discovery::Configure(const beyond_config *options)
{
    return -ENOSYS;
}

#endif // !ANDROID
