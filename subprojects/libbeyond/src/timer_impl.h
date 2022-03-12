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

#ifndef __BEYOND_INTERNAL_TIMER_IMPL_H__
#define __BEYOND_INTERNAL_TIMER_IMPL_H__

#include <cstdint>

#include "beyond/private/timer_private.h"
#include "beyond/private/event_object_private.h"

namespace beyond {

class Timer::impl final : public Timer {
public:
    static impl *Create(void);

    void Destroy(void) override;
    int SetTimer(double timer) override;
    double GetTimer(void) const override;

public:
    int GetHandle(void) const override;
    int AddHandler(beyond_event_handler_t handler, int type, void *data) override;
    int RemoveHandler(beyond_event_handler_t handler, int type, void *data) override;
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int DestroyEventData(EventObjectInterface::EventData *&data) override;

private:
    explicit impl(int fd);
    ~impl(void) = default;

    double registeredTime;
};

} // namespace beyond

#endif // __BEYOND_INTERNAL_TIMER_IMPL_H__
