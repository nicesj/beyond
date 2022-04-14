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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.samsung.android.beyond.discovery.Discovery;
import com.samsung.android.beyond.module.discovery.DNSSD.DNSSDModule;

import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicInteger;

@RunWith(AndroidJUnit4.class)
public class DiscoveryCallbackUnitTest {
    Context context = ApplicationProvider.getApplicationContext();

    @BeforeClass
    public static void prepareLoop() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
    }

    @Test
    public void testSetEventListener() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(context, args_server)) {
            assertNotNull(server);
            AtomicInteger invoked = new AtomicInteger();
            server.setEventListener(eventObject -> {
                invoked.getAndIncrement();
                assertEquals(1, invoked.get());
                Looper.myLooper().quit();
            });
            assertTrue(server.activate());

            Looper.loop();

            server.deactivate();
        }
    }
}
