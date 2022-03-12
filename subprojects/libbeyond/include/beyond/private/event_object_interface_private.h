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

#ifndef __BEYOND_PRIVATE_EVENT_OBJECT_INTERFACE_H__
#define __BEYOND_PRIVATE_EVENT_OBJECT_INTERFACE_H__

#include <beyond/common.h>
#include <beyond/private/event_object_base_interface_private.h>

namespace beyond {

class EventObjectInterface : public EventObjectBaseInterface {
public:
    struct EventData {
        int type;
        void *data;
    };

    EventObjectInterface(void) = default;
    virtual ~EventObjectInterface(void) = default;

    virtual int AddHandler(beyond_event_handler_t handler, int type, void *data) = 0;
    virtual int RemoveHandler(beyond_event_handler_t handler, int type, void *data) = 0;
    virtual int FetchEventData(EventData *&info) = 0;
    virtual int DestroyEventData(EventData *&data) = 0;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_EVENT_OBJECT_INTERFACE_H__
