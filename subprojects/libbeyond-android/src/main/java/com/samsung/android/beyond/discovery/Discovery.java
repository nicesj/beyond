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

package com.samsung.android.beyond.discovery;

import android.util.Log;

import com.samsung.android.beyond.EventListener;
import com.samsung.android.beyond.EventObject;
import com.samsung.android.beyond.NativeInstance;
import com.samsung.android.beyond.ConfigType;

public class Discovery extends NativeInstance {
    static {
        initialize();
    }

    public class DefaultEventObject extends EventObject {
    }

    private static final String TAG = "BEYOND_ANDROID_DISCOVERY";
    private EventListener eventListener;

    public Discovery(String[] arguments) {
        if (arguments == null || arguments.length == 0) {
            throw new IllegalArgumentException("There is no argument");
        }
        registerNativeInstance(create(arguments), (Long instance) -> destroy(instance));
        eventListener = null;
    }

    public int activate() {
        if (instance == 0) {
            throw new IllegalStateException();
        }

        return activate(instance);
    }

    public int deactivate() {
        if (instance == 0) {
            throw new IllegalStateException();
        }

        return deactivate(instance);
    }

    public int setItem(String key, byte[] value) {
        if (instance == 0) {
            throw new IllegalStateException();
        }

        return setItem(instance, key, value);
    }

    public int removeItem(String key) {
        if (instance == 0) {
            throw new IllegalStateException();
        }

        return removeItem(instance, key);
    }

    public int configure(char type, Object obj) {
        if (type == ConfigType.CONTEXT_ANDROID) {
            if (obj.getClass().getName().equals("android.app.Application") == true) {
                return this.configure(instance, type, obj);
            }

            // TODO:
            // This function should generate a proper exception
            return -22;
        }

        return -22;
    }

    public void setEventListener(EventListener eventListener) {
        this.eventListener = eventListener;
        if (setEventListener(instance, this.eventListener != null) < 0) {
            // TODO: throw an exception
        }
    }

    private static native void initialize();
    private native long create(String[] arguments);
    private static native void destroy(long instance);
    private native int activate(long instance);
    private native int deactivate(long instance);
    private native int setItem(long instance, String key, byte[] value);
    private native int removeItem(long instance, String key);
    private native int configure(long instance, char type, Object obj);
    private native int setEventListener(long instance, boolean flag);
}
