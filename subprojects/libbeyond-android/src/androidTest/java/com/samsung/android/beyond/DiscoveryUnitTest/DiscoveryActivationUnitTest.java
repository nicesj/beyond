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
import com.samsung.android.beyond.ConfigType;

import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

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
        try (Discovery server = new Discovery(args_server)) {
            assertNotNull(server);
            int ret = server.configure(ConfigType.CONTEXT_ANDROID, context);
            assertEquals(0, ret);
            ret = server.activate();
            assertEquals(0, ret);
            server.deactivate();
        }
    }

    @Test
    public void testActivateClient() {
        String[] args_client = { DNSSDModule.NAME };
        try (Discovery client = new Discovery(args_client)) {
            assertNotNull(client);
            int ret = client.configure(ConfigType.CONTEXT_ANDROID, context);
            assertEquals(0, ret);
            ret = client.activate();
            assertEquals(0, ret);
            client.deactivate();
        }
    }

    @Test(expected = IllegalStateException.class)
    public void testActivateAfterClosed() {
        String[] args_client = { DNSSDModule.NAME };
        Discovery client = new Discovery(args_client);
        assertNotNull(client);
        client.close();
        client.activate();
    }

    @Test(expected = IllegalStateException.class)
    public void testDeactivateAfterClosed() {
        String[] args_client = { DNSSDModule.NAME };
        Discovery client = new Discovery(args_client);
        assertNotNull(client);
        client.close();
        client.deactivate();
    }
}
