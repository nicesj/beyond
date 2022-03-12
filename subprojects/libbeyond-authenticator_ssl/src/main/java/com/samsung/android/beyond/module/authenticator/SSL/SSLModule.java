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

package com.samsung.android.beyond.module.authenticator.SSL;

public class SSLModule {
    public static final String NAME = "authenticator_ssl";
    public static final String ARGUMENT_ASYNC_MODE = "--async";
    public static final char CONFIG_SSL = 0xfe;
    public static final char CONFIG_SECRET_KEY = 0xfd;

    public static final int EVENT_TYPE_PREPARE_DONE = 0x08010100;
    public static final int EVENT_TYPE_PREPARE_ERROR = 0x08010200;
    public static final int EVENT_TYPE_DEACTIVATE_DONE = 0x08020100;
    public static final int EVENT_TYPE_DEACTIVATE_ERROR = 0x08020200;
    public static final int EVENT_TYPE_CRYPTO_DONE = 0x08040100;
    public static final int EVENT_TYPE_CRYPTO_ERROR = 0x08040200;
}
