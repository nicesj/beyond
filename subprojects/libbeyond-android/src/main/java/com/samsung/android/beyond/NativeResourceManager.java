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

import java.lang.ref.*;
import java.lang.Thread;
import java.util.HashMap;

public class NativeResourceManager {
    private static final String TAG = "BEYOND_ANDROID_RESOURCE";

    public interface Destroyable {
        void destroy();
    }

    private static NativeResourceManager instance = new NativeResourceManager();
    private ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
    private HashMap<PhantomReference<Object>, Destroyable> referenceMap = new HashMap<PhantomReference<Object>, Destroyable>();
    private Manager manager = new Manager();

    private class Manager extends Thread {
        public Manager() {
            setDaemon(true);
        }

        public void run() {
            Log.d(TAG, "Started");
            try {
                Reference<? extends Object> ref;
                while ((ref = queue.remove()) != null) {
                    Destroyable destructor = referenceMap.remove(ref);
                    Log.d(TAG, "Reference " + ref + " released " + destructor);
                    if (destructor == null) {
                        Log.e(TAG, "Invalid destructor");
                    } else {
                        destructor.destroy();
                    }
                }
            } catch (InterruptedException e) {
                Log.d(TAG, "Interrupted " + e);
            }
        }
    }

    private NativeResourceManager() {
        manager.start();
    }

    public static void register(Object object, Destroyable info) {
        instance.referenceMap.put(new PhantomReference<>(object, instance.queue), info);
    }
}
