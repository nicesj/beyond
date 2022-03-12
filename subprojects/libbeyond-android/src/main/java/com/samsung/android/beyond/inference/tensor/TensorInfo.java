package com.samsung.android.beyond.inference.tensor;

import android.util.Log;

import static com.samsung.android.beyond.inference.Option.TAG;

public class TensorInfo {

    private final DataType dataType;

    private final int rank;

    private final int[] dimensions;

    private int dataByteSize;

    TensorInfo(DataType dataType, int[] dimensions) {
        this.dataType = dataType;
        this.rank = dimensions.length;
        this.dimensions = dimensions;
        this.dataByteSize = calculateDataByteSize(dataType, dimensions);
    }

    private int calculateDataByteSize(DataType dataType, int[] dimensions) {
        int dataSize = dataType.getByteSize();
        for (int dimension : dimensions) {
            dataSize *= dimension;
        }
        Log.d(TAG, "dataSize = " + dataSize);

        return dataSize;
    }

    public DataType getDataType() {
        return dataType;
    }

    public int getRank() {
        return rank;
    }

    public int[] getDimensions() {
        return dimensions;
    }

    public int getDataByteSize() {
        return dataByteSize;
    }
}
