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
import android.content.Context;

import androidx.annotation.NonNull;

import com.samsung.android.beyond.EventListener;
import com.samsung.android.beyond.EventObject;
import com.samsung.android.beyond.NativeInstance;
import com.samsung.android.beyond.ConfigType;

public class Discovery extends NativeInstance {
    static {
        initialize();
    }

    public static class Info {
        public String name;
        public String host;
        public int port;
        public String uuid;
    }

    public class DefaultEventObject extends EventObject {
    }

    private EventListener eventListener;
    private static final String TAG = "BEYOND_ANDROID_DISCOVERY";

    public Discovery(@NonNull Context context, @NonNull String[] arguments) {
        if (arguments.length == 0) {
            throw new IllegalArgumentException("There is no argument");
        }

        long nativeInstance = create(arguments);
        if (nativeInstance == 0L) {
            throw new RuntimeException("The native instance of Discovery is not created successfully.");
        }
        registerNativeInstance(nativeInstance, (Long instance) -> destroy(instance));

        if (configure(ConfigType.CONTEXT_ANDROID, context) == false) {
            throw new RuntimeException("Failed to configure the android context");
        }

        eventListener = null;
    }

    public boolean activate() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return activate(instance) == 0 ? true : false;
    }

    public boolean deactivate() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return deactivate(instance) == 0 ? true: false;
    }

    public boolean setItem(@NonNull String key, @NonNull byte[] value) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid now.");
            return false;
        }

        if (value.length == 0) {
            Log.e(TAG, "Arguments are invalid.");
            return false;
        }

        return setItem(instance, key, value) == 0 ? true : false;
    }

    public boolean removeItem(@NonNull String key) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid now.");
            return false;
        }

        return removeItem(instance, key) == 0 ? true : false;
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
            if (obj.getClass().getName().equals("android.app.Application") == true) {
                return configure(instance, type, obj) == 0 ? true : false;
            }

            // TODO: This function can generate a proper exception
            return false;
        }

        return true;
    }

    public boolean setEventListener(@NonNull EventListener eventListener) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        this.eventListener = eventListener;
        if (setEventListener(instance, this.eventListener) < 0) {
            Log.e(TAG, "Fail to set the given event listener.");
            return false;     // TODO: Change as an exception
        }

        return true;
    }

    private static native void initialize();
    private native long create(String[] arguments);
    private static native void destroy(long instance);
    private native int activate(long instance);
    private native int deactivate(long instance);
    private native int setItem(long instance, String key, byte[] value);
    private native int removeItem(long instance, String key);
    private native int configure(long instance, char type, Object obj);
    private native int setEventListener(long instance, EventListener eventListener);
}
