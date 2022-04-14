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
import com.samsung.android.beyond.inference.PeerInfo;
import com.samsung.android.beyond.module.authenticator.SSL.SSLModule;

import org.junit.BeforeClass;
import org.junit.Test;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;
import static com.samsung.android.beyond.TestUtils.*;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class InferencePeerUnitTest {

    private static Context context;

    private Peer serverPeer = null;

    private Peer inferencePeer = null;

    @BeforeClass
    public static void initialize() {
        context = getInstrumentation().getContext();
    }

    @Test
    public void testCreatePeerServer() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");
        assertNotNull(serverPeer);

        serverPeer.close();
    }

    @Test
    public void testPeerServerSetInfo() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");

        assertEquals(true, serverPeer.setInfo());

        serverPeer.close();
    }

    @Test
    public void testPeerServerGetInfo() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");
        serverPeer.setInfo();

        PeerInfo info = serverPeer.getInfo();
        assertNotNull(info);
        assertEquals(info.getHost(), "0.0.0.0");
        assertEquals(info.getPort(), 3000);

        serverPeer.close();
    }

    @Test
    public void testPeerServerActivateAndDeactivateControlChannel() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");
        serverPeer.setInfo();

        assertEquals(true, serverPeer.activateControlChannel());

        assertEquals(true, serverPeer.deactivateControlChannel());

        serverPeer.close();
    }

    @Test
    public void testPeerServerConfigureWithAuthenticator() {
        String[] args = {
                SSLModule.NAME
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);

        assertTrue(auth.activate());
        assertTrue(auth.prepare());

        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");
        assertNotNull(serverPeer);

        boolean status = serverPeer.configure(ConfigType.AUTHENTICATOR, auth);
        assertTrue(status);

        // TODO:
        // GC the inferencePeer and destroy its native instance
        auth.close();
        auth = null;

        serverPeer.close();
    }

    @Test
    public void testCreatePeerClient() {
        inferencePeer = InferenceModuleFactory.createPeerClient(context, "dummyModule");

        assertNotNull(inferencePeer);

        inferencePeer.close();
    }

    @Test
    public void testPeerClientSetInfo() {
        inferencePeer = InferenceModuleFactory.createPeerClient(context, "dummyModule");
        PeerInfo info = new PeerInfo(TEST_SERVER_IP, TEST_SERVER_PORT);
        assertEquals(true, inferencePeer.setInfo(info));

        inferencePeer.close();
    }

    @Test
    public void testPeerClientActivateAndDeactivateControlChannel() {
        serverPeer = InferenceModuleFactory.createPeerServer(context, "dummyModule");
        serverPeer.setInfo();
        serverPeer.activateControlChannel();

        inferencePeer = InferenceModuleFactory.createPeerClient(context, "dummyModule");
        inferencePeer.setInfo(new PeerInfo(TEST_SERVER_IP, TEST_SERVER_PORT));

        assertEquals(true, inferencePeer.activateControlChannel());

        assertTrue(inferencePeer.deactivateControlChannel());
        assertTrue(serverPeer.deactivateControlChannel());

        inferencePeer.close();
        serverPeer.close();
    }
}
