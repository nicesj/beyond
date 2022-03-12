package com.samsung.android.beyond.inference.tensor;

import android.util.Log;

import com.samsung.android.beyond.NativeInstance;

import static com.samsung.android.beyond.inference.Option.TAG;

public class TensorSet extends NativeInstance {

    private final long tensorsInstance;

    private Tensor[] tensors;

    TensorSet(long tensorsInstance, TensorHandler tensorHandler) {
        this.tensorsInstance = tensorsInstance;
        registerNativeInstance(tensorsInstance, (Long instance) -> tensorHandler.freeTensors(this));
        Log.i(TAG, "Phantom Reference is set to a tensor set.");
    }

    public long getTensorsInstance() {
        return tensorsInstance;
    }

    public Tensor[] getTensors() {
        return tensors;
    }

    void setTensors(Tensor[] tensors) {
        this.tensors = tensors;
    }
}
