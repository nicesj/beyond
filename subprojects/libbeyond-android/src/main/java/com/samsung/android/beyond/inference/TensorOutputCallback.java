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

import com.samsung.android.beyond.inference.tensor.TensorHandler;
import com.samsung.android.beyond.inference.tensor.TensorInfo;
import com.samsung.android.beyond.inference.tensor.TensorSet;

import java.nio.ByteBuffer;

public class TensorOutputCallback extends OutputCallback {

    private TensorHandler tensorHandler;
    private TensorInfo[] tensorInfoArray;
    private ByteBuffer[] byteBufferArray;
    private TensorSet tensorSet;

    public TensorOutputCallback() {}

    public TensorOutputCallback(@NonNull TensorHandler tensorHandler, @NonNull TensorInfo[] tensorInfoArray) {
        this.tensorHandler = tensorHandler;
        this.tensorInfoArray = tensorInfoArray;
        byteBufferArray = new ByteBuffer[tensorInfoArray.length];
    }

    ByteBuffer[] getByteBufferArray() {
        return byteBufferArray;
    }

    public TensorSet getOutputTensors() {
        return tensorSet;
    }

    public void organizeTensorSet(@NonNull long instance) {
        tensorSet = tensorHandler.createTensorSet(instance, tensorInfoArray, byteBufferArray);
    }

    @Override
    public void onReceivedOutputs() {
        // Example code
        // TensorSet outputTensors = getOutputTensors();
        // if (outputTensors == null) {
        //     Log.e(TAG, "Fail to get output results.");
        //     return;
        // }
        // userDefinedDecodingFunction(outputTensors.getTensors());
    }
}
