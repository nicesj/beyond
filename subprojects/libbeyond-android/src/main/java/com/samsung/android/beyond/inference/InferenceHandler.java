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

import android.util.Log;

import com.samsung.android.beyond.inference.tensor.TensorSet;
import com.samsung.android.beyond.NativeInstance;

import static com.samsung.android.beyond.inference.Option.TAG;

public class InferenceHandler extends NativeInstance {
    public InferenceHandler(InferenceMode inferenceMode) {
        registerNativeInstance(nativeCreateInference(inferenceMode.toString()), (Long instance) -> destroy(instance));
    }

    public boolean addInferencePeer(Peer inferencePeer) {
        return addPeer(instance, inferencePeer.getNativeInstance());
    }

    public boolean loadModel(String modelPath) {
        Log.i(TAG, "Model path in a client : " + modelPath);
        return loadModel(instance, modelPath);
    }

    public boolean prepare() {
        return prepare(instance);
    }

    public boolean run(TensorSet inputTensors) {
        return run(instance, inputTensors.getTensorsInstance(), inputTensors.getTensors().length);
    }

    private native long nativeCreateInference(String inferenceMode);

    private native boolean addPeer(long inferenceHandle, long peerHandle);

    private native boolean loadModel(long handle, String modelPath);

    private native boolean prepare(long inferenceHandle);

    private native boolean run(long inferenceHandle, long tensorsInstance, int numberOfTensors);

    private static native void destroy(long inferenceHandle);
}
