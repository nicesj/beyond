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

import com.samsung.android.beyond.ConfigType;
import com.samsung.android.beyond.authenticator.Authenticator;
import com.samsung.android.beyond.inference.InferenceModuleFactory;
import com.samsung.android.beyond.inference.Peer;
import com.samsung.android.beyond.module.authenticator.SSL.SSLModule;
import com.samsung.android.beyond.module.peer.NN.NNModule;

import org.junit.BeforeClass;
import org.junit.Test;

import java.util.UUID;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;
import static com.samsung.android.beyond.TestUtils.*;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class InferencePeerUnitTest {

    private static Context context;

    private Peer inferencePeer = null;

    @BeforeClass
    public static void initialize() {
        context = getInstrumentation().getContext();
    }

    @Test
    public void testCreatePeerServer() {
        inferencePeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        assertNotNull(inferencePeer);
    }

    @Test
    public void testPeerServerSetIpPort() {
        inferencePeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);

        assertEquals(true, inferencePeer.setIpPort());
    }

    @Test
    public void testPeerServerActivateControlChannel() {
        inferencePeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        inferencePeer.setIpPort();

        assertEquals(true, inferencePeer.activateControlChannel());
    }

    @Test
    public void testPeerServerConfigureWithAuthenticator() {
        String[] args = {
                SSLModule.NAME
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);

        int ret = auth.activate();
        assertEquals(ret, 0);
        ret = auth.prepare();
        assertEquals(ret, 0);

        inferencePeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        assertNotNull(inferencePeer);

        boolean status = inferencePeer.configure(ConfigType.AUTHENTICATOR, auth);
        assertTrue(status);

        // TODO:
        // GC the inferencePeer and destroy its native instance
        inferencePeer = null;
        auth.close();
        auth = null;
    }

    @Test
    public void testPeerServerDeactivateControlChannel() {
        inferencePeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        inferencePeer.setIpPort();
        inferencePeer.activateControlChannel();

        assertEquals(true, inferencePeer.deactivateControlChannel());
    }

    @Test
    public void testCreatePeerClient() {
        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);

        assertNotNull(inferencePeer);
    }

    @Test
    public void testPeerClientSetIpPort() {
        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);

        assertEquals(true, inferencePeer.setIpPort(TEST_SERVER_IP, TEST_SERVER_PORT));
    }

    @Test
    public void testPeerClientActivateControlChannel() {
        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);
        inferencePeer.setIpPort(TEST_SERVER_IP, TEST_SERVER_PORT);

        assertEquals(true, inferencePeer.activateControlChannel());
    }

    @Test
    public void testPeerClientDeactivateControlChannel() {
        Peer serverPeer = InferenceModuleFactory.createPeerServer(context, NNModule.NAME);
        serverPeer.setIpPort();
        serverPeer.activateControlChannel();

        inferencePeer = InferenceModuleFactory.createPeerClient(context, NNModule.NAME);
        inferencePeer.setIpPort();
        inferencePeer.activateControlChannel();

        assertEquals(true, inferencePeer.deactivateControlChannel());
    }
}
