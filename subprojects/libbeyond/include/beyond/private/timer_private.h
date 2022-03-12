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

#ifndef __BEYOND_PRIVATE_TIMER_H__
#define __BEYOND_PRIVATE_TIMER_H__

namespace beyond {

class API Timer : public EventObject {
public:
    struct EventData : public EventObjectInterface::EventData {
        uint64_t sequenceId;
    };

public:
    static Timer *Create(void);

    virtual void Destroy(void) = 0;
    virtual int SetTimer(double timer) = 0;
    virtual double GetTimer(void) const = 0;

protected:
    explicit Timer(int fd);
    virtual ~Timer(void) = default;

private:
    class impl;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_TIMER_H__
