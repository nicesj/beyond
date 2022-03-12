package com.samsung.android.beyond.inference.tensor;

public enum DataType {

    UINT8(0x10, 1),

    FLOAT32(0x22, 4);
    // TODO: Add more tensor types.

    private final int typeValue;

    private final int byteSize;

    DataType(int typeValue, int byteSize) {
        this.typeValue = typeValue;
        this.byteSize = byteSize;
    }

    public int getTypeValue() {
        return typeValue;
    }

    public int getByteSize() {
        return byteSize;
    }

    public static DataType fromValue(int value) {
        for (DataType dataType : DataType.values()) {
            if (value == dataType.getTypeValue()) {
                return dataType;
            }
        }
        return null;
    }
}
