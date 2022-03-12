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

package com.samsung.android.beyond;

public class EventType {
    public static final int NONE = 0x00000000;
    public static final int READ = 0x00000010;
    public static final int WRITE = 0x00000020;
    public static final int ERROR = 0x00000040;
    public static final int EVENT_MASK = 0x000000FF;

    public static final int INFERENCE_UNKNOWN = 0x01000000;
    public static final int INFERENCE_READY = 0x01000100;
    public static final int INFERENCE_SUCCESS = 0x01000200;
    public static final int INFERENCE_STOPPED = 0x01000400;  // All inference process is stopped
    public static final int INFERENCE_CANCELED = 0x01000800; // A single inference is canceled
    public static final int INFERENCE_ERROR = 0x01001000;
    public static final int INFERENCE_MASK = 0x0100FF00;
    // ...

    public static final int PEER_UNKNOWN = 0x02000000;
    public static final int PEER_CONNECTED = 0x02000100;
    public static final int PEER_DISCONNECTED = 0x02000200;
    public static final int PEER_INFO_UPDATED = 0x02000400; // When a peer information is changed, this is going to be called.
    public static final int PEER_ERROR = 0x02000800;
    public static final int PEER_MASK = 0x0200FF00; // When a peer information is changed, this is going to be called.
    // ...

    public static final int DISCOVERY_UNKNOWN = 0x04000000;
    public static final int DISCOVERY_DISCOVERED = 0x04000100;
    // ...
    public static final int DISCOVERY_ERROR = 0x04000200;
    public static final int DISCOVERY_MASK = 0x0400FF00;

    public static final int AUTHENTICATOR_UNKNOWN = 0x08000000;
    public static final int AUTHENTICATOR_COMPLETED = 0x80000100; // When the encryption and decryption is completed
    // ...
    public static final int AUTHENTICATOR_ERROR = 0x08000200;
    public static final int AUTHENTICATOR_MASK = 0x0800FF00;
}
