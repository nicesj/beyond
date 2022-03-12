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

#ifndef __BEYOND_PRIVATE_COMMAND_OBJECT_INTERFACE_H__
#define __BEYOND_PRIVATE_COMMAND_OBJECT_INTERFACE_H__

namespace beyond {

class CommandObjectInterface : public EventObjectBaseInterface {
public:
    CommandObjectInterface(void) = default;
    virtual ~CommandObjectInterface(void) = default;

    virtual int Send(int id, void *data = nullptr) = 0;
    virtual int Recv(int &id) = 0;
    virtual int Recv(int &id, void *&data) = 0;
};

} // namespace beyond

#endif // __BEYOND_PRIVATE_COMMAND_OBJECT_INTERFACE_H__
