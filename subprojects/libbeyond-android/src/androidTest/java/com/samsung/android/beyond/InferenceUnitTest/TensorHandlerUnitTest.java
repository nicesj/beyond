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

package com.samsung.android.beyond.InferenceUnitTest;

import android.content.Context;
import android.util.Log;

import com.samsung.android.beyond.inference.InferenceHandler;
import com.samsung.android.beyond.inference.InferenceMode;
import com.samsung.android.beyond.inference.InferenceModuleFactory;
import com.samsung.android.beyond.inference.Peer;
import com.samsung.android.beyond.inference.PeerInfo;
import com.samsung.android.beyond.inference.tensor.Tensor;
import com.samsung.android.beyond.inference.tensor.TensorHandler;
import com.samsung.android.beyond.inference.tensor.TensorInfo;
import com.samsung.android.beyond.inference.tensor.TensorSet;
import com.samsung.android.beyond.module.peer.NN.NNModule;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;
import static com.samsung.android.beyond.TestUtils.*;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class TensorHandlerUnitTest {

    private static Context context;

    private InferenceHandler inferenceHandler = null;

    private TensorHandler tensorHandler = null;

    private Peer serverPeer = null;

    private Peer inferencePeer = null;

    private TensorSet inputTensorSet = null;

    private static String uint8ModelAbsolutePath = null;

    private static String float32ModelAbsolutePath = null;

    @BeforeClass
    public static void initialize() {
        generateTestData();

        context = getInstrumentation().getContext();
        getOrangeRGBImage(context);
        organizeLabelMap(context);
        uint8ModelAbsolutePath = getModelAbsolutePath(context, TEST_UINT8_MODEL_NAME);
        float32ModelAbsolutePath = getModelAbsolutePath(context, TEST_FLOAT32_MODEL_NAME);
    }

    @Before
    public void prepareInferenceModules() {
        inferenceHandler = InferenceModuleFactory.createHandler(InferenceMode.REMOTE);
        assertNotNull(inferenceHandler);

        serverPeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        assertNotNull(serverPeer);
        assertTrue(serverPeer.setInfo());
        assertTrue(serverPeer.activateControlChannel());

        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);
        assertNotNull(inferencePeer);

        assertTrue(inferencePeer.setInfo(new PeerInfo(TEST_SERVER_IP, TEST_SERVER_PORT)));

        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()

        tensorHandler = new TensorHandler(inferenceHandler);
    }

    @Test
    public void testGetTensorsInfo() {
        assertTrue(inferenceHandler.loadModel(uint8ModelAbsolutePath));
        assertTrue(inferenceHandler.prepare());

        TensorInfo[] inputTensorsInfo = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfo);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsInfo.length);
        for (TensorInfo tensorInfo : inputTensorsInfo) {
            assertArrayEquals(tensorInfo.getDimensions(), MOBILENET_INPUT_TENSOR_DIMENSIONS);
        }

        TensorInfo[] outputTensorsInfo = tensorHandler.getOutputTensorsInfo();
        assertNotNull(outputTensorsInfo);
        assertEquals(MOBILENET_OUTPUT_TENSOR_NUMBER, outputTensorsInfo.length);
        for (TensorInfo tensorInfo : outputTensorsInfo) {
            assertArrayEquals(tensorInfo.getDimensions(), MOBILENET_OUTPUT_TENSOR_DIMENSIONS);
        }
    }

    @Test
    public void testAllocateTensors() {
        assertTrue(inferenceHandler.loadModel(uint8ModelAbsolutePath));
        assertTrue(inferenceHandler.prepare());
        TensorInfo[] inputTensorsInfo = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfo);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsInfo.length);
        for (TensorInfo tensorInfo : inputTensorsInfo) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }
        TensorInfo[] outputTensorsInfo = tensorHandler.getOutputTensorsInfo();
        assertNotNull(outputTensorsInfo);
        assertEquals(MOBILENET_OUTPUT_TENSOR_NUMBER, outputTensorsInfo.length);
        for (TensorInfo tensorInfo : outputTensorsInfo) {
            assertArrayEquals(MOBILENET_OUTPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }

        TensorSet inputTensorSet = tensorHandler.allocateTensorSet(inputTensorsInfo);
        assertNotNull(inputTensorSet);
        Tensor[] inputTensors = inputTensorSet.getTensors();
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensors.length);
        for (Tensor tensor : inputTensors) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());
        }

        TensorSet outputTensorSet = tensorHandler.allocateTensorSet(outputTensorsInfo);
        assertNotNull(outputTensorSet);
        Tensor[] outputTensors = outputTensorSet.getTensors();
        assertEquals(MOBILENET_OUTPUT_TENSOR_NUMBER, outputTensors.length);
        for (Tensor tensor : outputTensors) {
            assertArrayEquals(MOBILENET_OUTPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());
        }
    }

    @Test
    public void testSetDataWithGeneratedUint8Data() {
        assertTrue(inferenceHandler.loadModel(uint8ModelAbsolutePath));
        assertTrue(inferenceHandler.prepare());
        TensorInfo[] inputTensorsInfo = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfo);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsInfo.length);
        for (TensorInfo tensorInfo : inputTensorsInfo) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }
        inputTensorSet = tensorHandler.allocateTensorSet(inputTensorsInfo);
        assertNotNull(inputTensorSet);
        Tensor[] inputTensors = inputTensorSet.getTensors();
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensors.length);

        for (Tensor tensor : inputTensors) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());

            assertTrue(tensor.setData(TEST_DATA_UINT8));
            ByteBuffer byteBuffer = (ByteBuffer) tensor.getData();
            assertNotNull(byteBuffer);
            assertArrayEquals(TEST_DATA_UINT8.array(), byteBuffer.array());
        }
    }

    @Test
    public void testSetDataWithGeneratedFloat32Data() {
        assertTrue(inferenceHandler.loadModel(float32ModelAbsolutePath));
        assertTrue(inferenceHandler.prepare());
        TensorInfo[] inputTensorsInfo = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfo);
        assertEquals(POSENET_INPUT_TENSOR_NUMBER, inputTensorsInfo.length);
        for (TensorInfo tensorInfo : inputTensorsInfo) {
            assertArrayEquals(POSENET_INPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }
        TensorSet inputTensorSet = tensorHandler.allocateTensorSet(inputTensorsInfo);
        assertNotNull(inputTensorSet);
        Tensor[] inputTensors = inputTensorSet.getTensors();
        assertEquals(POSENET_INPUT_TENSOR_NUMBER, inputTensors.length);

        for (Tensor tensor : inputTensors) {
            assertArrayEquals(POSENET_INPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());

            assertTrue(tensor.setData(TEST_DATA_FLOAT32));
            FloatBuffer floatBuffer = (FloatBuffer) tensor.getData();
            assertNotNull(floatBuffer);
            assertArrayEquals(TEST_DATA_FLOAT32.array(), floatBuffer.array(), 0);
        }
    }

    @Test
    public void testGetOutput() {
        assertTrue(inferenceHandler.loadModel(uint8ModelAbsolutePath));
        assertTrue(inferenceHandler.prepare());
        tensorHandler = new TensorHandler(inferenceHandler);
        TensorInfo[] inputTensorsProperties = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsProperties);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsProperties.length);
        for (TensorInfo tensorInfo : inputTensorsProperties) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }
        TensorInfo[] outputTensorsProperties = tensorHandler.getOutputTensorsInfo();
        assertNotNull(outputTensorsProperties);
        assertEquals(MOBILENET_OUTPUT_TENSOR_NUMBER, outputTensorsProperties.length);
        for (TensorInfo tensorInfo : outputTensorsProperties) {
            assertArrayEquals(MOBILENET_OUTPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }
        TensorSet inputTensors = tensorHandler.allocateTensorSet(inputTensorsProperties);
        assertNotNull(inputTensors);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensors.getTensors().length);
        for (Tensor tensor : inputTensors.getTensors()) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());
            assertTrue(tensor.setData(TEST_ORANGE_IMAGE));
            ByteBuffer byteBuffer = (ByteBuffer) tensor.getData();
            assertNotNull(byteBuffer);
            assertArrayEquals(TEST_ORANGE_IMAGE.array(), byteBuffer.array());
        }
        assertTrue(inferenceHandler.run(inputTensors));

        TensorSet outputTensors = tensorHandler.getOutput(outputTensorsProperties);
        assertNotNull(outputTensors);
        assertTrue(isOrange(outputTensors.getTensors()));
    }

    public static boolean isOrange(Tensor[] tensors) {
        for (Tensor tensor : tensors) {
            int dataSize = tensor.getTensorInfo().getDataByteSize();
            int maxIndex = -1;
            int maxScore = Integer.MIN_VALUE;
            ByteBuffer byteBuffer = (ByteBuffer) tensor.getData();
            Log.i(TAG, "Result array size = " + byteBuffer.capacity());
            for (int i = 0; i < dataSize; i++) {
                if (byteBuffer.get(i) > maxScore) {
                    maxIndex = i;
                    maxScore = byteBuffer.get(i);
                }
            }
            Log.i(TAG, "Max index = " + maxIndex);
            String label = labelMap.get(new Integer(maxIndex));
            if (label.equals("orange") == false) {
                return false;
            }
        }
        return true;
    }

    @After
    public void destroyInferenceModules() {
        assertTrue(inferencePeer.deactivateControlChannel());
        assertTrue(serverPeer.deactivateControlChannel());

        assertTrue(inferenceHandler.removeInferencePeer(inferencePeer));

        serverPeer.close();
        inferencePeer.close();
        inferenceHandler.close();
        Log.i(TAG, "Inference modules are destroyed!!");
    }
}
