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

#ifndef __BEYOND_AUTHENTICATOR_SSL_AUTHENTICATOR_EVENT_OBJECT_H__
#define __BEYOND_AUTHENTICATOR_SSL_AUTHENTICATOR_EVENT_OBJECT_H__

class Authenticator::EventObject final : public beyond::EventObject {
public:
    static EventObject *Create(void);
    void Destroy(void);
    int FetchEventData(EventObjectInterface::EventData *&data) override;
    int PublishEventData(int type, void *data = nullptr);

private:
    explicit EventObject(int fd);
    virtual ~EventObject(void) = default;

    int publishHandle;
};

#endif // __BEYOND_AUTHENTICATOR_SSL_AUTHENTICATOR_EVENT_OBJECT_H__
