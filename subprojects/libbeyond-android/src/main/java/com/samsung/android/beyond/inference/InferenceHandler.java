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

import java.util.*;
import android.util.Log;

import com.samsung.android.beyond.inference.tensor.TensorSet;
import com.samsung.android.beyond.NativeInstance;

import static com.samsung.android.beyond.inference.Option.TAG;

import androidx.annotation.NonNull;

public class InferenceHandler extends NativeInstance {
    private List<Peer> peerList = new ArrayList<Peer>();

    private TensorOutputCallback tensorOutputCallback;

    InferenceHandler(@NonNull InferenceMode inferenceMode) {
        long nativeInstance = create(inferenceMode.toString());
        if (nativeInstance == 0L) {
            throw new RuntimeException("The native instance of InferenceHandler is not created successfully.");
        }

        registerNativeInstance(nativeInstance, (Long instance) -> destroy(instance));
    }

    // TODO:
    // removePeer() should be provided
    public boolean addInferencePeer(@NonNull Peer inferencePeer) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        if (inferencePeer.getNativeInstance() == 0L) {
            Log.e(TAG, "The given peer instance is invalid.");
            return false;
        }

        if (addPeer(instance, inferencePeer.getNativeInstance()) == true) {
            // NOTE:
            // Hold the peer instance until release them from the this.
            peerList.add(inferencePeer);
            return true;
        }

        return false;
    }

    public boolean loadModel(@NonNull String modelPath) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        Log.i(TAG, "Model path in a client : " + modelPath);
        return loadModel(instance, modelPath);
    }

    public boolean prepare() {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return prepare(instance);
    }

    public boolean run(@NonNull TensorSet inputTensors) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        if (inputTensors.getTensorsInstance() == 0L) {
            Log.e(TAG, "The given tensor set is invalid.");
            return false;
        }

        return run(instance, inputTensors.getTensorsInstance(), inputTensors.getTensors().length);
    }

    public boolean removeInferencePeer(@NonNull Peer inferencePeer) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        return removePeer(instance, inferencePeer.getNativeInstance());
    }

    public boolean setOutputCallback(@NonNull TensorOutputCallback tensorOutputCallback) {
        if (instance == 0L) {
            Log.e(TAG, "Instance is invalid.");
            return false;
        }

        this.tensorOutputCallback = tensorOutputCallback;

        if (setCallback(instance, tensorOutputCallback) < 0) {
            Log.e(TAG, "Fail to set the given output callback.");
            return false;
        }

        return true;
    }

    private native long create(String inferenceMode);

    private native boolean addPeer(long inferenceHandle, long peerHandle);

    private native boolean loadModel(long handle, String modelPath);

    private native boolean prepare(long inferenceHandle);

    private native boolean run(long inferenceHandle, long tensorsInstance, int numberOfTensors);

    private native boolean removePeer(long inferenceHandle, long peerHandle);

    private static native void destroy(long inferenceHandle);

    private native int setCallback(long instance, TensorOutputCallback tensorOutputCallback);
}
