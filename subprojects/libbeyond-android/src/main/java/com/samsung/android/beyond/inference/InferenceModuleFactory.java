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

package com.samsung.android.beyond.inference;

import android.content.Context;

public class InferenceModuleFactory {

    public static InferenceHandler createHandler(InferenceMode inferenceMode) {
        return new InferenceHandler(inferenceMode);
    }

    public static Peer createPeerServer(Context context, String peerType) {
        return new Peer(context, peerType, NodeType.SERVER);
    }

    public static Peer createPeerClient(Context context, String peerType) {
        return new Peer(context, peerType, NodeType.CLIENT);
    }
}
