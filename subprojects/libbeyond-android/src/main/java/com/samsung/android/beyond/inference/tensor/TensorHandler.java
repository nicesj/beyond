package com.samsung.android.beyond.inference.tensor;

import android.util.Log;

import com.samsung.android.beyond.inference.InferenceHandler;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

import static com.samsung.android.beyond.inference.Option.TAG;

import androidx.annotation.NonNull;

public class TensorHandler {

    private final InferenceHandler inferenceHandler;

    public TensorHandler(@NonNull InferenceHandler inferenceHandler) {
        this.inferenceHandler = inferenceHandler;
    }

    public TensorInfo[] getInputTensorsInfo() {
        List<Integer> inputDataTypeValues = new ArrayList<>();
        List<List<Integer>> inputDimensionsList = new ArrayList<>();
        if (getInputTensorsInfo(inferenceHandler.getNativeInstance(), inputDataTypeValues, inputDimensionsList) == false
                || inputDataTypeValues.size() == 0) {
            Log.e(TAG, "Fail to get the information of input tensors.");
            return null;
        }
        TensorInfo[] tensorInfoArray = new TensorInfo[inputDataTypeValues.size()];
        if (organizeTensorsInfo(tensorInfoArray, inputDataTypeValues, inputDimensionsList) == false) {
            Log.e(TAG, "Fail to organize the information of input tensors.");
            return null;
        }

        return tensorInfoArray;
    }

    public TensorInfo[] getOutputTensorsInfo() {
        List<Integer> outputDataTypeValues = new ArrayList<>();
        List<List<Integer>> outputDimensionsList = new ArrayList<>();
        if (getOutputTensorsInfo(inferenceHandler.getNativeInstance(), outputDataTypeValues, outputDimensionsList) == false
                || outputDataTypeValues.size() == 0) {
            Log.e(TAG, "Fail to get the information of output tensors.");
            return null;
        }

        TensorInfo[] tensorInfoArray = new TensorInfo[outputDataTypeValues.size()];
        if (organizeTensorsInfo(tensorInfoArray, outputDataTypeValues, outputDimensionsList) == false) {
            Log.e(TAG, "Fail to organize the information of output tensors.");
            return null;
        }

        return tensorInfoArray;
    }

    private boolean organizeTensorsInfo(@NonNull TensorInfo[] tensorInfoArray, @NonNull List<Integer> dataTypeValues, @NonNull List<List<Integer>> dimensionsList) {
        if (dataTypeValues.size() != dimensionsList.size()) {
            Log.e(TAG, "The number of tensors is abnormal.");
            return false;
        }
        for (int i = 0; i < dataTypeValues.size(); i++) {
            StringBuilder dimensionsLog = new StringBuilder("DataType = ");
            DataType dataType = DataType.fromValue(dataTypeValues.get(i));
            if (dataType == null) {
                Log.e(TAG, "Fail to find a data type from the given value (" + dataTypeValues.get(i) + ")");
                return false;
            }
            dimensionsLog.append(dataType.toString()).append(", Dimensions = { ");
            List<Integer> dimensions = dimensionsList.get(i);
            int[] intDimensions = new int[dimensions.size()];
            for (int j = 0; j < dimensions.size(); j++) {
                intDimensions[j] = dimensions.get(j);
                dimensionsLog.append(intDimensions[j]).append(" ");
            }
            dimensionsLog.append("}");
            Log.i(TAG, dimensionsLog.toString());
            tensorInfoArray[i] = new TensorInfo(dataType, intDimensions);
        }

        return true;
    }

    public TensorSet allocateTensorSet(@NonNull TensorInfo[] tensorInfoArray) {
        ByteBuffer[] bufferArray = new ByteBuffer[tensorInfoArray.length];
        long tensorsInstance = allocateTensors(inferenceHandler.getNativeInstance(), tensorInfoArray, tensorInfoArray.length, bufferArray);
        if (tensorsInstance == 0L) {
            throw new RuntimeException("Fail to allocate the buffers of tensors.");
        }

        return organizeTensorSet(tensorsInstance, tensorInfoArray, bufferArray);
    }

    private TensorSet organizeTensorSet(@NonNull long tensorsInstance, @NonNull TensorInfo[] tensorInfoArray, @NonNull ByteBuffer[] bufferArray) {
        Tensor[] tensorArray = new Tensor[tensorInfoArray.length];
        for (int i = 0; i < tensorInfoArray.length; i++) {
            TensorInfo tensorInfo = tensorInfoArray[i];
            DataType dataType = tensorInfo.getDataType();
            Tensor tensor = new Tensor(Tensor.fromValue(dataType), tensorInfo);
            if (tensor.setBuffer(bufferArray[i]) == false) {
                Log.e(TAG, "Fail to setBuffer.");
                freeTensors(inferenceHandler.getNativeInstance(), tensorsInstance, tensorInfoArray.length);
                return null;
            }
            tensorArray[i] = tensor;
        }

        TensorSet tensorSet = new TensorSet(tensorsInstance, this);
        tensorSet.setTensors(tensorArray);

        return tensorSet;
    }

    public TensorSet getOutput(@NonNull TensorInfo[] tensorInfoArray) {
        ByteBuffer[] bufferArray = new ByteBuffer[tensorInfoArray.length];
        long tensorsInstance = getOutput(inferenceHandler.getNativeInstance(), bufferArray);
        if (tensorsInstance == 0L) {
            throw new RuntimeException("Fail to get output tensors.");
        }

        return organizeTensorSet(tensorsInstance, tensorInfoArray, bufferArray);
    }

    public void freeTensors(@NonNull TensorSet tensorSet) {
        freeTensors(inferenceHandler.getNativeInstance(), tensorSet.getTensorsInstance(), tensorSet.getTensors().length);
    }

    public TensorSet createTensorSet(@NonNull long tensorsInstance, @NonNull TensorInfo[] tensorInfoArray, @NonNull ByteBuffer[] bufferArray) {
        if (tensorInfoArray.length == 0 || bufferArray.length == 0) {
            throw new IllegalArgumentException("Arguments are invalid.");
        }

        return organizeTensorSet(tensorsInstance, tensorInfoArray, bufferArray);
    }

    private native boolean getInputTensorsInfo(long inferenceHandle, List<Integer> dataTypeValue, List<List<Integer>> dimensions);

    private native boolean getOutputTensorsInfo(long inferenceHandle, List<Integer> dataTypeValue, List<List<Integer>> dimensions);

    private native long allocateTensors(long inferenceHandle, TensorInfo[] tensorInfoArray, int numberOfTensors, ByteBuffer[] bufferArray);

    private native long getOutput(long inferenceHandle, ByteBuffer[] bufferArray);

    private native void freeTensors(long inferenceHandle, long tensorInstance, int numberOfTensors);
}
