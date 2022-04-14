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

package com.samsung.android.beyond.DiscoveryUnitTest;

import android.content.Context;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;

import com.samsung.android.beyond.discovery.Discovery;
import com.samsung.android.beyond.module.discovery.DNSSD.DNSSDModule;

import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class DiscoveryActivationUnitTest {
    Context context = ApplicationProvider.getApplicationContext();

    @BeforeClass
    public static void prepareLoop() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
    }

    @Test
    public void testActivateServer() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(context, args_server)) {
            assertNotNull(server);

            assertTrue(server.activate());

            server.deactivate();
        }
    }

    @Test
    public void testActivateClient() {
        String[] args_client = { DNSSDModule.NAME };
        try (Discovery client = new Discovery(context, args_client)) {
            assertNotNull(client);

            assertTrue(client.activate());

            client.deactivate();
        }
    }

    @Test
    public void testActivateAfterClosed() {
        String[] args_client = { DNSSDModule.NAME };
        Discovery client = new Discovery(context, args_client);
        assertNotNull(client);
        client.close();

        assertFalse(client.activate());
    }

    @Test
    public void testDeactivateAfterClosed() {
        String[] args_client = { DNSSDModule.NAME };
        Discovery client = new Discovery(context, args_client);
        assertNotNull(client);
        client.close();

        assertFalse(client.deactivate());
    }
}
