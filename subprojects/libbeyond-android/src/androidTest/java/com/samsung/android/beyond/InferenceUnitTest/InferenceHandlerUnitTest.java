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

import com.samsung.android.beyond.inference.InferenceHandler;
import com.samsung.android.beyond.inference.InferenceMode;
import com.samsung.android.beyond.inference.InferenceModuleFactory;
import com.samsung.android.beyond.inference.Peer;
import com.samsung.android.beyond.inference.tensor.Tensor;
import com.samsung.android.beyond.inference.tensor.TensorHandler;
import com.samsung.android.beyond.inference.tensor.TensorInfo;
import com.samsung.android.beyond.inference.tensor.TensorSet;
import com.samsung.android.beyond.module.peer.NN.NNModule;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.nio.ByteBuffer;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;
import static com.samsung.android.beyond.TestUtils.MOBILENET_INPUT_TENSOR_DIMENSIONS;
import static com.samsung.android.beyond.TestUtils.MOBILENET_INPUT_TENSOR_NUMBER;
import static com.samsung.android.beyond.TestUtils.TEST_ORANGE_IMAGE;
import static com.samsung.android.beyond.TestUtils.TEST_SERVER_IP;
import static com.samsung.android.beyond.TestUtils.TEST_SERVER_PORT;
import static com.samsung.android.beyond.TestUtils.TEST_UINT8_MODEL_NAME;
import static com.samsung.android.beyond.TestUtils.getModelAbsolutePath;
import static com.samsung.android.beyond.TestUtils.getOrangeRGBImage;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class InferenceHandlerUnitTest {

    private static Context context;

    private static String modelAbsolutePath = null;

    private InferenceHandler inferenceHandler = null;

    private TensorHandler tensorHandler = null;

    private Peer inferencePeer = null;

    @BeforeClass
    public static void initialize() {
        context = getInstrumentation().getContext();
        modelAbsolutePath = getModelAbsolutePath(context, TEST_UINT8_MODEL_NAME);
        getOrangeRGBImage(context);
    }

    @Before
    public void prepareInferenceModules() {
        inferenceHandler = InferenceModuleFactory.createHandler(InferenceMode.REMOTE);
        assertNotNull(inferenceHandler);

        Peer serverPeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        assertNotNull(serverPeer);
        assertTrue(serverPeer.setIpPort());
        assertTrue(serverPeer.activateControlChannel());

        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);
        assertNotNull(inferencePeer);
        assertTrue(inferencePeer.setIpPort(TEST_SERVER_IP, TEST_SERVER_PORT));
    }

    @Test
    public void testAddInferencePeer() {
        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()

        assertTrue(inferencePeer.deactivateControlChannel());
    }

    @Test
    public void testLoadModel() {
        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()

        assertTrue(inferenceHandler.loadModel(modelAbsolutePath));

        assertTrue(inferencePeer.deactivateControlChannel());
    }

    @Test
    public void testPrepare() {
        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()
        assertTrue(inferenceHandler.loadModel(modelAbsolutePath));

        assertTrue(inferenceHandler.prepare());

        assertTrue(inferencePeer.deactivateControlChannel());
    }

    @Test
    public void testRunInference() {
        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()
        assertTrue(inferenceHandler.loadModel(modelAbsolutePath));
        assertTrue(inferenceHandler.prepare());
        tensorHandler = new TensorHandler(inferenceHandler);
        TensorInfo[] inputTensorsInfo = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfo);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsInfo.length);
        for (TensorInfo property : inputTensorsInfo) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, property.getDimensions());
        }
        TensorSet inputTensorSet = tensorHandler.allocateTensorSet(inputTensorsInfo);
        assertNotNull(inputTensorSet);
        Tensor[] inputTensors = inputTensorSet.getTensors();
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensors.length);
        for (Tensor tensor : inputTensors) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, tensor.getTensorInfo().getDimensions());
            assertTrue(tensor.setData(TEST_ORANGE_IMAGE));
            ByteBuffer byteBuffer = (ByteBuffer) tensor.getData();
            assertNotNull(byteBuffer);
            assertArrayEquals(TEST_ORANGE_IMAGE.array(), byteBuffer.array());
        }

        assertTrue(inferenceHandler.run(inputTensorSet));

        assertTrue(inferencePeer.deactivateControlChannel());
    }
}
