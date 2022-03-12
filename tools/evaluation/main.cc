/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//include logging
#include "task_runner.h"

namespace beyond {
namespace evaluation {

int Main(int argc, char *argv[])
{
    auto task_runner = beyond::evaluation::CreateTaskRunner();
    if (task_runner == nullptr) {
        // TODO: use platform log
        printf("Fail to create TaskRunner\n");
        return 1;
    }
    if (task_runner->Run(&argc, argv) < 0) {
        printf("Fail to Run Task\n");
        return 1;
    }
    return 0;
}
} // namespace evaluation
} // namespace beyond

int main(int argc, char *argv[])
{
    return beyond::evaluation::Main(argc, argv);
}
