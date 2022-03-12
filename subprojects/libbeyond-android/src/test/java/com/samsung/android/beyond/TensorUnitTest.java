package com.samsung.android.beyond;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.samsung.android.beyond.inference.tensor.DataType;
import com.samsung.android.beyond.inference.tensor.Tensor;
import com.samsung.android.beyond.inference.tensor.TensorInfo;
import com.samsung.android.beyond.inference.tensor.UInt8;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.mockito.Mock;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.Random;

import static org.junit.Assert.assertTrue;

public class TensorUnitTest {

    private static TestTensor uintTensor;
    private static TestTensor floatTensor;

    @Mock
    private static TensorInfo uintTensorInfo;
    @Mock
    private static TensorInfo floatTensorInfo;

    private static int[] dimensions;

    private static byte[] byteData;
    private static float[] floatData;

    private static ByteBuffer uintTestBuffer;
    private static ByteBuffer floatTestBuffer;

    private static ByteBuffer uintDataBuffer;
    private static FloatBuffer floatDataBuffer;
    private IntBuffer unsupportedTypeBuffer;

    @BeforeClass
    public static void prepareTests() {
        initializeDimensions();
        initializeTestData();
        initializeBuffers();
        initializeMocks();
        initializeStubs();
    }

    private static void initializeDimensions() {
        int rank = 4;
        dimensions = new int[rank];
        for (int i = 0; i < rank; i++) {
            dimensions[i] = i + 1;
        }
    }

    private static void initializeTestData() {
        int dimensionSize = calculateDataSize(dimensions);
        byteData = new byte[dimensionSize];
        for (int i = 0; i < dimensionSize; i++) {
            byteData[i] = (byte) (new Random().nextInt(256));
        }

        floatData = new float[dimensionSize];
        for (int i = 0; i < dimensionSize; i++) {
            floatData[i] = new Random().nextFloat();
        }
    }

    private static int calculateDataSize(int[] dimensions) {
        int dataSize = 1;
        for (int dimension: dimensions) {
            dataSize *= dimension;
        }
        return dataSize;
    }

    private static void initializeBuffers() {
        uintTestBuffer = ByteBuffer.wrap(new byte[byteData.length]);
        floatTestBuffer = ByteBuffer.wrap(new byte[floatData.length * DataType.FLOAT32.getByteSize()]);

        uintDataBuffer = ByteBuffer.wrap(byteData);
        floatDataBuffer = FloatBuffer.wrap(floatData);
    }

    private static void initializeMocks() {
        uintTensorInfo = mock(TensorInfo.class);
        floatTensorInfo = mock(TensorInfo.class);
    }

    private static void initializeStubs() {
        when(uintTensorInfo.getDataType()).thenReturn(DataType.UINT8);
        when(uintTensorInfo.getDimensions()).thenReturn(dimensions);
        when(uintTensorInfo.getDataByteSize()).thenReturn(calculateDataSize(dimensions));

        when(floatTensorInfo.getDataType()).thenReturn(DataType.FLOAT32);
        when(floatTensorInfo.getDimensions()).thenReturn(dimensions);
        when(floatTensorInfo.getDataByteSize()).thenReturn(calculateDataSize(dimensions) * DataType.FLOAT32.getByteSize());
    }

    @Before
    public void createTestTensors() {
        uintTensor = new TestTensor(UInt8.class, uintTensorInfo, uintTestBuffer);
        floatTensor = new TestTensor(Float.class, floatTensorInfo, floatTestBuffer);
    }

    @Test
    public void testSetDataWithCorrectBuffer() {
        assertTrue(uintTensor.setData(uintDataBuffer));

        assertTrue(floatTensor.setData(floatDataBuffer));
    }

    @Test
    public void testSetDataWithWrongBuffer() {
        assertFalse(uintTensor.setData(null));

        assertFalse(uintTensor.setData(floatDataBuffer));
        assertFalse(uintTensor.setData(unsupportedTypeBuffer));

        assertFalse(floatTensor.setData(uintDataBuffer));
        assertFalse(floatTensor.setData(unsupportedTypeBuffer));
    }

    @Test
    public void testGetDataAndChangeIntoCorrectTypeBuffer() {
        assertTrue(uintTensor.setData(uintDataBuffer));
        Buffer buffer = uintTensor.getData();
        ByteBuffer byteBuffer = (ByteBuffer) buffer;

        assertArrayEquals(uintDataBuffer.array(), byteBuffer.array());

        assertTrue(floatTensor.setData(floatDataBuffer));
        buffer = floatTensor.getData();
        FloatBuffer floatBuffer = (FloatBuffer) buffer;

        assertArrayEquals(floatDataBuffer.array(), floatBuffer.array(), 0);
    }

    @Test(expected = ClassCastException.class)
    public void testGetDataAndChangeIntoWrongTypeBuffer() {
        assertTrue(uintTensor.setData(uintDataBuffer));
        Buffer buffer = uintTensor.getData();
        FloatBuffer floatBuffer = (FloatBuffer) buffer;

        assertTrue(floatTensor.setData(floatDataBuffer));
        buffer = floatTensor.getData();
        ByteBuffer byteBuffer = (ByteBuffer) buffer;
    }

    private class TestTensor extends Tensor {
         TestTensor(Class classType, TensorInfo tensorInfo, ByteBuffer buffer) {
            super(classType, tensorInfo);
            super.setBuffer(buffer);
        }
    }
}
