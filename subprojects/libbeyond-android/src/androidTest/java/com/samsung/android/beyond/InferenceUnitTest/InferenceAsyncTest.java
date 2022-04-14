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

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;
import static com.samsung.android.beyond.TestUtils.MOBILENET_INPUT_TENSOR_DIMENSIONS;
import static com.samsung.android.beyond.TestUtils.MOBILENET_INPUT_TENSOR_NUMBER;
import static com.samsung.android.beyond.TestUtils.MOBILENET_OUTPUT_TENSOR_DIMENSIONS;
import static com.samsung.android.beyond.TestUtils.MOBILENET_OUTPUT_TENSOR_NUMBER;
import static com.samsung.android.beyond.TestUtils.TAG;
import static com.samsung.android.beyond.TestUtils.TEST_ORANGE_IMAGE;
import static com.samsung.android.beyond.TestUtils.TEST_SERVER_IP;
import static com.samsung.android.beyond.TestUtils.TEST_SERVER_PORT;
import static com.samsung.android.beyond.TestUtils.TEST_UINT8_MODEL_NAME;
import static com.samsung.android.beyond.TestUtils.getModelAbsolutePath;
import static com.samsung.android.beyond.TestUtils.getOrangeRGBImage;
import static com.samsung.android.beyond.TestUtils.organizeLabelMap;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.os.Looper;
import android.util.Log;

import com.samsung.android.beyond.EventListener;
import com.samsung.android.beyond.EventObject;
import com.samsung.android.beyond.inference.InferenceHandler;
import com.samsung.android.beyond.inference.InferenceMode;
import com.samsung.android.beyond.inference.InferenceModuleFactory;
import com.samsung.android.beyond.inference.TensorOutputCallback;
import com.samsung.android.beyond.inference.Peer;
import com.samsung.android.beyond.inference.PeerInfo;
import com.samsung.android.beyond.inference.tensor.Tensor;
import com.samsung.android.beyond.inference.tensor.TensorHandler;
import com.samsung.android.beyond.inference.tensor.TensorInfo;
import com.samsung.android.beyond.inference.tensor.TensorSet;
import com.samsung.android.beyond.module.peer.NN.NNModule;

import org.junit.After;
import org.junit.BeforeClass;
import org.junit.Test;

import java.nio.ByteBuffer;

public class InferenceAsyncTest {

    private static Context context;

    private static String modelAbsolutePath = null;

    private Peer serverPeer = null;

    private Peer inferencePeer = null;

    private InferenceHandler inferenceHandler = null;

    private TensorHandler tensorHandler = null;

    @BeforeClass
    public static void initialize() {
        context = getInstrumentation().getContext();
        modelAbsolutePath = getModelAbsolutePath(context, TEST_UINT8_MODEL_NAME);
        getOrangeRGBImage(context);
        organizeLabelMap(context);

        if (Looper.myLooper() == null) {
            Looper.prepare();
            Log.i(TAG, "ALooper is prepared in this test case.");
        }
    }

    @Test
    public void testAsyncSetOutputCallback() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        assertNotNull(serverPeer);
        assertTrue(serverPeer.setInfo());
        assertTrue(serverPeer.activateControlChannel());

        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);
        assertNotNull(inferencePeer);

        try {
            assertTrue(inferencePeer.registerEventListener(new EventListener() {
                @Override
                public void onEvent(EventObject eventObject) {
                    Log.e(TAG, "Peer Event Listener!!");
                }
            }));
        } catch (Exception e) {
            e.printStackTrace();
            assert(false);
        }

        assertTrue(inferencePeer.setInfo(new PeerInfo(TEST_SERVER_IP, TEST_SERVER_PORT)));

        inferenceHandler = InferenceModuleFactory.createHandler(InferenceMode.REMOTE);
        assertNotNull(inferenceHandler);
        assertTrue(inferenceHandler.addInferencePeer(inferencePeer));   // Cover activateControlChannel()
        assertTrue(inferenceHandler.loadModel(modelAbsolutePath));
        assertTrue(inferenceHandler.prepare());

        tensorHandler = new TensorHandler(inferenceHandler);
        TensorInfo[] inputTensorsInfoArray = tensorHandler.getInputTensorsInfo();
        assertNotNull(inputTensorsInfoArray);
        assertEquals(MOBILENET_INPUT_TENSOR_NUMBER, inputTensorsInfoArray.length);
        for (TensorInfo property : inputTensorsInfoArray) {
            assertArrayEquals(MOBILENET_INPUT_TENSOR_DIMENSIONS, property.getDimensions());
        }
        TensorSet inputTensorSet = tensorHandler.allocateTensorSet(inputTensorsInfoArray);
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

        TensorInfo[] outputTensorInfoArray = tensorHandler.getOutputTensorsInfo();
        assertNotNull(outputTensorInfoArray);
        assertEquals(MOBILENET_OUTPUT_TENSOR_NUMBER, outputTensorInfoArray.length);
        for (TensorInfo tensorInfo : outputTensorInfoArray) {
            assertArrayEquals(MOBILENET_OUTPUT_TENSOR_DIMENSIONS, tensorInfo.getDimensions());
        }

        try {
            assertTrue(inferenceHandler.setOutputCallback(new TensorOutputCallback(tensorHandler, outputTensorInfoArray) {
                @Override
                public void onReceivedOutputs() {
                    TensorSet outputTensors = getOutputTensors();
                    if (outputTensors == null) {
                        Log.e(TAG, "Fail to get output results.");
                        return;
                    }
                    assertTrue(TensorHandlerUnitTest.isOrange(outputTensors.getTensors()));

                    Looper.myLooper().quit();
                }
            }));
        } catch (Exception e) {
            assert(false);
        }

        assertTrue(inferenceHandler.run(inputTensorSet));

        Looper.loop();
    }

    @After
    public void destroyInferenceModules() {
        assertTrue(inferencePeer.deactivateControlChannel());
        assertTrue(serverPeer.deactivateControlChannel());

        assertTrue(inferenceHandler.removeInferencePeer(inferencePeer));

        serverPeer.close();
        inferencePeer.close();
        inferenceHandler.close();
    }
}
