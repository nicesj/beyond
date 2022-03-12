package com.samsung.android.beyond.inference.tensor;

import android.util.Log;

import com.samsung.android.beyond.inference.InferenceHandler;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

import static com.samsung.android.beyond.inference.Option.TAG;

public class TensorHandler {

    private final InferenceHandler inferenceHandler;

    public TensorHandler(InferenceHandler inferenceHandler) {
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

    private boolean organizeTensorsInfo(TensorInfo[] tensorInfoArray, List<Integer> dataTypeValues, List<List<Integer>> dimensionsList) {
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

    public TensorSet allocateTensorSet(TensorInfo[] tensorInfoArray) {
        List<Integer> dataTypeValues = new ArrayList<>();
        List<List<Integer>> dimensionsList = new ArrayList<>();
        int[] dataSizes = new int[tensorInfoArray.length];
        if (transformTensorsInfo(tensorInfoArray, dataTypeValues, dataSizes, dimensionsList) == false) {
            Log.e(TAG, "Fail to organize the information of tensors.");
            return null;
        }

        ByteBuffer[] bufferArray = new ByteBuffer[tensorInfoArray.length];
        long tensorsInstance = allocateTensors(inferenceHandler.getNativeInstance(), dataTypeValues, dataSizes, dimensionsList, tensorInfoArray.length, bufferArray);
        if (tensorsInstance == 0) {
            Log.e(TAG, "Fail to allocate the buffers of tensors.");
            return null;
        }

        return organizeTensorSet(tensorsInstance, tensorInfoArray, bufferArray);
    }

    private boolean transformTensorsInfo(TensorInfo[] tensorInfoArray, List<Integer> dataTypeValues, int[] dataSizes, List<List<Integer>> dimensionsList) {
        if (tensorInfoArray.length == 0) {
            Log.e(TAG, "The given tensorInfoArray is empty.");
            return false;
        }

        for (int i = 0; i < tensorInfoArray.length; i++) {
            TensorInfo tensorInfo = tensorInfoArray[i];
            dataTypeValues.add(tensorInfo.getDataType().getTypeValue());
            dataSizes[i] = tensorInfo.getDataByteSize();
            List<Integer> dimensions = new ArrayList<>();
            for (int j = 0; j < tensorInfo.getRank(); j++) {
                dimensions.add(tensorInfo.getDimensions()[j]);
            }
            dimensionsList.add(dimensions);
        }

        return true;
    }

    private TensorSet organizeTensorSet(long tensorsInstance, TensorInfo[] tensorInfoArray, ByteBuffer[] bufferArray) {
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

    public TensorSet getOutput(TensorInfo[] tensorInfoArray) {
        ByteBuffer[] bufferArray = new ByteBuffer[tensorInfoArray.length];
        long tensorsInstance = getOutput(inferenceHandler.getNativeInstance(), bufferArray, tensorInfoArray.length);

        return organizeTensorSet(tensorsInstance, tensorInfoArray, bufferArray);
    }

    public int freeTensors(TensorSet tensorSet) {
        freeTensors(inferenceHandler.getNativeInstance(), tensorSet.getTensorsInstance(), tensorSet.getTensors().length);
        return 0;
    }

    private native boolean getInputTensorsInfo(long inferenceHandle, List<Integer> dataTypeValue, List<List<Integer>> dimensions);

    private native boolean getOutputTensorsInfo(long inferenceHandle, List<Integer> dataTypeValue, List<List<Integer>> dimensions);

    private native long allocateTensors(long inferenceHandle, List<Integer> dataTypeValue, int[] dataSizes, List<List<Integer>> dimensions, int tensorsNumber, ByteBuffer[] bufferArray);

    private native long getOutput(long inferenceHandle, ByteBuffer[] bufferArray, int numberOfTensors);

    private native void freeTensors(long inferenceHandle, long tensorInstance, int numberOfTensors);
}
