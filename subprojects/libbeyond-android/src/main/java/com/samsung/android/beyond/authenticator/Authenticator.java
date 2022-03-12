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

package com.samsung.android.beyond.authenticator;

import android.util.Log;

import com.samsung.android.beyond.EventListener;
import com.samsung.android.beyond.EventObject;
import com.samsung.android.beyond.NativeInstance;
import com.samsung.android.beyond.ConfigType;

public class Authenticator extends NativeInstance {
    static {
        initialize();
    }

    public class DefaultEventObject extends EventObject {
    }

    private EventListener eventListener;
    private static final String TAG = "BEYOND_ANDROID_AUTH";

    public Authenticator(String[] arguments) {
        registerNativeInstance(create(arguments), (Long instance) -> destroy(instance));
        Log.d(TAG, "Authenticator created");
        eventListener = null;
    }

    public void setEventListener(EventListener eventListener) {
        this.eventListener = eventListener;
        if (this.setEventListener(instance, eventListener != null) < 0) {
            // TODO: throw an exception
        }
    }

    public int configure(char type, String jsonString) {
        return this.configure(instance, type, jsonString);
    }

    public int configure(char type, Object obj) {
        if (type == ConfigType.CONTEXT_ANDROID) {
            if (obj.getClass().getName().equals("android.app.Application") == true) {
                return this.configure(instance, type, obj);
            }

            // TODO:
            // The error code should be declared or the function should generate a proper exception
            return -22;
        }

        if (type == ConfigType.JSON) {
            // TODO:
            // The error code should be declared or the function should generate a proper exception
            return -22;
        }

        try {
            NativeInstance nativeInstance = (NativeInstance)obj;
            return this.configure(instance, type, nativeInstance.getNativeInstance());
        } catch (ClassCastException e) {
            return this.configure(instance, type, obj);
        }
    }

    public int deactivate() {
        return this.deactivate(instance);
    }

    public int activate() {
        return this.activate(instance);
    }

    public int prepare() {
        return this.prepare(instance);
    }

    private static native void initialize();

    private native long create(String[] arguments);
    private static native void destroy(long instance);
    private native int configure(long instance, char type, String jsonString);

    // NOTE:
    // Authenticator object can be used for configuring the instance
    // it could be applied in recursive manner
    private native int configure(long instance, char type, Object obj);
    private native int configure(long instance, char type, long inst);

    private native int deactivate(long instance);
    private native int activate(long instance);
    private native int prepare(long instance);
    private native int setEventListener(long instance, boolean flag);
}
