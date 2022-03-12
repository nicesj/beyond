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
import android.util.Log;

import com.samsung.android.beyond.NativeInstance;
import static com.samsung.android.beyond.inference.Option.TAG;

public class Peer extends NativeInstance {
    public Peer(Context context, String peerType, NodeType nodeType) {
        switch (nodeType) {
            case SERVER:
                String[] serverConfiguration = new String[4];
                serverConfiguration[0] = peerType;
                serverConfiguration[1] = "--server";
                serverConfiguration[2] = "--path";
                String defaultModelStoragePath = context.getFilesDir().getAbsolutePath();
                serverConfiguration[3] = defaultModelStoragePath;
                Log.d(TAG, "Path to store a model in a peer server = " + defaultModelStoragePath);

                instance = create(context, serverConfiguration);
                if (instance == 0) {
                    Log.e(TAG, "Fail to create a native peer server" );
                }
                break;
            case CLIENT:
                String[] clientConfiguration = new String[1];
                clientConfiguration[0] = peerType;
                Log.i(TAG, "clientConfiguration (" + clientConfiguration[0] + ")");

                instance = create(context, clientConfiguration);
                if (instance == 0) {
                    Log.e(TAG, "Fail to create a native peer client");
                }
                break;
        }

        registerNativeInstance(instance, (Long instance) -> destroy(instance));
    }

    public boolean setIpPort() {
        return setInfo(instance, peerIP, peerPort);
    }

    public boolean setIpPort(String peerIP, int peerPort) {
        /** TODO: Check validation of arguments. **/
        this.peerIP = peerIP;
        this.peerPort = peerPort;
        return setIpPort();
    }

    public boolean activateControlChannel() {
        return activate(instance);
    }

    public boolean configure(char type, Object obj) {
        return configure(instance, type, obj);
    }

    public boolean deactivateControlChannel() {
        return deactivate(instance);
    }

    private native long create(Context context, String[] args);

    private native boolean setInfo(long handle, String peerIP, int peerPort);

    private native boolean activate(long handle);

    private native boolean configure(long handle, char type, Object auth);

    private native boolean deactivate(long handle);

    private static native void destroy(long handle);

    private String peerIP = "0.0.0.0";

    private int peerPort = 3000;
}
