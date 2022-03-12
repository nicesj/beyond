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

#ifndef __BEYOND_PRIVATE_EVENT_OBJECT_H__
#define __BEYOND_PRIVATE_EVENT_OBJECT_H__

#include <vector>
#include <functional>

#include <beyond/common.h>
#include <beyond/private/event_object_interface_private.h>

namespace beyond {
class API EventObject : public EventObjectInterface {
public:
    explicit EventObject(int handle = -1);
    virtual ~EventObject(void) = default;

    int GetHandle(void) const override;

    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;

    int FetchEventData(EventObjectInterface::EventData *&info) override;
    int DestroyEventData(EventObjectInterface::EventData *&info) override;

    virtual int FetchEventData(EventObjectInterface::EventData *&data, const std::function<beyond_handler_return(EventObjectInterface *iface, int type, EventObjectInterface::EventData *&d)> &eventFetcher);

private:
    struct HandlerData {
        beyond_event_handler_t handler;
        int type;
        void *data;
        bool in_use;
        bool is_deleted;
    };

    std::vector<HandlerData *> handlers;
    int handle;
};
} // namespace beyond

#endif // __BEYOND_PRIVATE_EVENT_OBJECT_H__
