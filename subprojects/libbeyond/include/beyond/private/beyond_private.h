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

#ifndef __BEYOND_PRIVATE_BEYOND_PRIVATE_H__
#define __BEYOND_PRIVATE_BEYOND_PRIVATE_H__

#if defined(__has_cpp_attribute) && __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH
#endif

#include <functional>

#include <beyond/common.h>

#include <beyond/private/log_private.h>
#include <beyond/private/event_object_base_interface_private.h>
#include <beyond/private/event_object_interface_private.h>
#include <beyond/private/event_object_private.h>
#include <beyond/private/command_object_interface_private.h>
#include <beyond/private/command_object_private.h>
#include <beyond/private/module_interface_private.h>

#include <beyond/private/timer_private.h>
#include <beyond/private/event_loop_private.h>

#include <beyond/private/inference_interface_private.h>
#include <beyond/private/inference_runtime_private.h>
#include <beyond/private/inference_peer_interface_private.h>
#include <beyond/private/inference_runtime_interface_private.h>
#include <beyond/private/inference_peer_private.h>
#include <beyond/private/inference_runtime_private.h>
#include <beyond/private/inference_private.h>

#include <beyond/private/discovery_interface_private.h>
#include <beyond/private/discovery_private.h>
#include <beyond/private/discovery_runtime_private.h>

#include <beyond/private/authenticator_interface_private.h>
#include <beyond/private/authenticator_private.h>
#include <beyond/private/resourceinfo_collector.h>

#endif // __BEYOND_PRIVATE_BEYOND_PRIVATE_H__
