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

#ifndef __BEYOND_INTERNAL_INFERENCE_IMPL_EVENT_OBJECT_H__
#define __BEYOND_INTERNAL_INFERENCE_IMPL_EVENT_OBJECT_H__

#include "beyond/private/event_object_base_interface_private.h"
#include "beyond/private/event_object_interface_private.h"
#include "beyond/private/event_object_private.h"

#include "inference_impl.h"

namespace beyond {

class Inference::impl::EventObject final : public beyond::EventObject {
public:
    static EventObject *Create(void);
    void Destroy(void);
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int PublishEventData(EventObjectInterface::EventData *eventData);

private:
    explicit EventObject(int fd);
    virtual ~EventObject(void) = default;

    int publishHandle;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_INFERENCE_IMPL_EVENT_OBJECT_H__
