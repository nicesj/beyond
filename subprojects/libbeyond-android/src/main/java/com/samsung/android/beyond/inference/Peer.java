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

import androidx.annotation.NonNull;

import android.content.Context;
import android.util.Log;

import java.util.regex.Pattern;

import com.samsung.android.beyond.EventListener;
import com.samsung.android.beyond.NativeInstance;
import com.samsung.android.beyond.ConfigType;

import static com.samsung.android.beyond.inference.Option.TAG;

public class Peer extends NativeInstance {
    static {
        initialize();
    }

    private EventListener eventListener;

    Peer(@NonNull Context context, @NonNull String peerType, @NonNull NodeType nodeType) {
        switch (nodeType) {
            case SERVER:
                String[] serverConfiguration = new String[4];
                serverConfiguration[0] = peerType;
                serverConfiguration[1] = "--server";
                serverConfiguration[2] = "--path";
                String defaultModelStoragePath = context.getFilesDir().getAbsolutePath();
                serverConfiguration[3] = defaultModelStoragePath;
                Log.d(TAG, "Path to store a model in a peer server = " + defaultModelStoragePath);

                instance = create(serverConfiguration);
                break;
            case CLIENT:
                String[] clientConfiguration = new String[1];
                clientConfiguration[0] = peerType;
                Log.i(TAG, "clientConfiguration (" + clientConfiguration[0] + ")");

                instance = create(clientConfiguration);
                break;
        }

        if (instance == 0L) {
            Log.e(TAG, "Fail to create a native peer");
            return;
        }

        registerNativeInstance(instance, (Long instance) -> destroy(instance));

        if (configure(instance, ConfigType.CONTEXT_ANDROID, context) == false) {
            Log.e(TAG, "Unable to set the android context");
        }
    }

    public boolean setIPPort(@NonNull String IP, int port) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        if (validateIPv4(IP) == false || port < 0) {
            Log.e(TAG, "Arguments are invalid.");
            return false;
        }

        return setInfo(instance, new PeerInfo(IP, port));
    }

    private boolean validateIPv4(@NonNull String IP) {
        String regexIPv4 = "^(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)(\\.(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)){3}$";
        Pattern pattern = Pattern.compile(regexIPv4);
        return pattern.matcher(IP).matches();
    }

    public boolean setInfo() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return setInfo(instance, new PeerInfo("0.0.0.0", 3000));
    }

    public boolean setInfo(@NonNull PeerInfo info) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return setInfo(instance, info);
    }

    public PeerInfo getInfo() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return null;
        }

        return getInfo(instance);
    }

    public boolean activateControlChannel() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return activate(instance);
    }

    public boolean configure(@NonNull char type, @NonNull Object obj) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        if (Character.isDefined(type) == false) {
            Log.e(TAG, "Arguments are invalid.");
            return false;
        }

        if (type == ConfigType.CONTEXT_ANDROID) {
            if (obj.getClass().getName().equals("android.app.Application") == false) {
                Log.e(TAG, "Context is not Valid " + obj.getClass().getName());
                return false;
            }
        }

        return configure(instance, type, obj);
    }

    public boolean deactivateControlChannel() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return deactivate(instance);
    }

    public boolean registerEventListener(@NonNull EventListener eventListener) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        this.eventListener = eventListener;

        if (setEventListener(instance, eventListener) < 0) {
            Log.e(TAG, "Fail to set the given event listener.");
            return false;
        }

        return true;
    }

    private static native void initialize();

    private native long create(String[] args);

    private native boolean setInfo(long handle, PeerInfo info);

    private native PeerInfo getInfo(long handle);

    private native boolean activate(long handle);

    private native boolean configure(long handle, char type, Object auth);

    private native boolean deactivate(long handle);

    private static native void destroy(long handle);

    private native int setEventListener(long handle, EventListener eventListener);
}
