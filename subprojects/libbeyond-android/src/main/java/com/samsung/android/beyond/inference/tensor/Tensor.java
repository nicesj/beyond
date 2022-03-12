package com.samsung.android.beyond.inference.tensor;

import android.util.Log;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.HashMap;
import java.util.Map;

import static com.samsung.android.beyond.inference.Option.TAG;

public class Tensor<T> {

    private final TensorInfo tensorInfo;

    private final Class<T> classType;

    private ByteBuffer buffer;

    protected Tensor(Class<T> classType, TensorInfo tensorInfo) {
        this.classType = classType;
        this.tensorInfo = tensorInfo;
    }

    public boolean setData(Buffer dataBuffer) {
        if (dataBuffer == null) {
            Log.e(TAG, "The given dataBuffer is null.");
            return false;
        }

        try {
            switch (getTensorInfo().getDataType()) {
                case UINT8:
                    if (dataBuffer instanceof ByteBuffer == false) {
                        Log.e(TAG, "dataBuffer is not an instance of ByteBuffer.");
                        return false;
                    }
                    ByteBuffer byteBuffer = (ByteBuffer) dataBuffer;
                    if (byteBuffer.hasArray() == false) {
                        Log.e(TAG, "The given buffer is not backed by an accessible array.");
                        return false;
                    }
                    buffer.mark();
                    buffer.put(byteBuffer.array());
                    buffer.reset();
                    break;
                case FLOAT32:
                    if (dataBuffer instanceof FloatBuffer == false) {
                        Log.e(TAG, "dataBuffer is not an instance of FloatBuffer.");
                        return false;
                    }
                    FloatBuffer floatBuffer = (FloatBuffer) dataBuffer;
                    if (floatBuffer.hasArray() == false) {
                        Log.e(TAG, "The given buffer is not backed by an accessible array.");
                        return false;
                    }
                    float[] floatData = floatBuffer.array();
                    buffer.mark();
                    buffer.asFloatBuffer().put(floatData);
                    buffer.reset();
                    break;
                // TODO: Add conversions for the other data types.
                default:
                    Log.e(TAG, "Unsupported data type.");
                    return false;
            }
        } catch (Exception e) {
            Log.e(TAG, "Fail to set data to the buffer.");
            return false;
        }

        return true;
    }

    public Buffer getData() {
        switch (tensorInfo.getDataType()) {
            case UINT8:
                byte[] byteData = new byte[tensorInfo.getDataByteSize()];
                buffer.mark();
                buffer.get(byteData);
                buffer.reset();
                return ByteBuffer.wrap(byteData);
            case FLOAT32:
                float[] floatData = new float[tensorInfo.getDataByteSize() / tensorInfo.getDataType().getByteSize()];
                FloatBuffer floatBuffer = buffer.asFloatBuffer();
                buffer.mark();
                floatBuffer.get(floatData, 0, floatData.length);
                buffer.reset();
                return FloatBuffer.wrap(floatData);
        }
        Log.e(TAG, "Fail to get buffer data from the given tensor.");

        return null;
    }

    public TensorInfo getTensorInfo() {
        return tensorInfo;
    }

    protected boolean setBuffer(ByteBuffer buffer) {
        if (buffer == null) {
            Log.e(TAG, "The given data is null.");
            return false;
        }

        this.buffer = buffer;
        this.buffer.clear();

        return true;
    }

    protected ByteBuffer getBuffer() {
        return buffer;
    }

    public static DataType fromClass(Class<?> c) {
        DataType dataType = typeCodes.get(c);
        if (dataType == null) {
            throw new IllegalArgumentException(
                    c.getName() + " objects cannot be used as elements in a BeyonD Tensor");
        }
        return dataType;
    }

    public static Class<?> fromValue(DataType dataType) {
        for (Class<?> classType : typeCodes.keySet()) {
            if (dataType.equals(fromClass(classType))) {
                return classType;
            }
        }
        return null;
    }

    private static final Map<Class<?>, DataType> typeCodes = new HashMap<>();

    static {
        typeCodes.put(Float.class, DataType.FLOAT32);
        typeCodes.put(UInt8.class, DataType.UINT8);
    }
}
