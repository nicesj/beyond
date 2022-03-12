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

import android.content.Context;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.samsung.android.beyond.ConfigType;
import com.samsung.android.beyond.discovery.Discovery;
import com.samsung.android.beyond.module.discovery.DNSSD.DNSSDModule;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicInteger;

@RunWith(AndroidJUnit4.class)
public class DiscoveryCallbackUnitTest {
    Context context = ApplicationProvider.getApplicationContext();

    @Test
    public void testSetEventListener() {
        Looper.prepare();

        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(args_server)) {
            assertNotNull(server);
            int ret = server.configure(ConfigType.CONTEXT_ANDROID, context);
            assertEquals(0, ret);
            AtomicInteger invoked = new AtomicInteger();
            server.setEventListener(eventObject -> {
                invoked.getAndIncrement();
                Looper.myLooper().quit();
            });
            ret = server.activate();
            assertEquals(0, ret);

            Looper.loop();

            assertEquals(invoked.get(), 1);
            server.deactivate();
        }
    }
}
