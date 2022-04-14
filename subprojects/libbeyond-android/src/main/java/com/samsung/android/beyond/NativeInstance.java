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

import android.util.Log;

import androidx.annotation.NonNull;

import java.util.function.Consumer;

public abstract class NativeInstance implements AutoCloseable {
    static {
        System.loadLibrary("beyond-android");
    }

    private static final String TAG = "BEYOND_ANDROID_NATIVE";

    protected static class Destructor implements NativeResourceManager.Destroyable {
        private long nativeInstance;
        private Consumer<Long> function;

        public Destructor(@NonNull long instance, @NonNull Consumer<Long> function) {
            nativeInstance = instance;
            this.function = function;
        }

        public void destroy() {
            if (nativeInstance == 0) {
                Log.d(TAG, "Already destroyed");
                return;
            }

            try {
                Log.d(TAG, "Invoke destroy");
                function.accept(nativeInstance);
                nativeInstance = 0;
            } catch (Exception e) {
                Log.e(TAG, "Exception " + e.toString());
            }
        }
    }

    protected long instance;
    private Destructor destructor;

    protected void registerNativeInstance(@NonNull long nativeInstance, @NonNull Consumer<Long> function) {
        instance = nativeInstance;
        destructor = new Destructor(instance, function);
        NativeResourceManager.register(this, destructor);
    }

    public long getNativeInstance() {
        return instance;
    }

    // NOTE:
    // The programmer must call this destructor explicitly,
    // in order to clean up native instance properly.
    @Override
    public void close() {
        if (destructor == null) {
            Log.e(TAG, "Already closed");
            // TODO: A proper exception would be thrown
            return;
        }

        destructor.destroy();
        destructor = null;
        instance = 0;
    }
}
