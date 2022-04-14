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

package com.samsung.android.beyond.AuthenticatorUnitTest;

import android.content.Context;
import android.os.Looper;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.beyond.ConfigType;
import com.samsung.android.beyond.authenticator.Authenticator;
import com.samsung.android.beyond.module.authenticator.SSL.SSLModule;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicInteger;

import static android.os.Looper.myLooper;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class AuthenticatorUnitTest {

    @Test
    public void useAppContext() {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();

        assertEquals("com.samsung.android.beyond.test", appContext.getPackageName());
    }

    @Test
    public void testPositiveCreate()
    {
        String[] args = {
            SSLModule.NAME
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);
        auth.close();
    }

    @Test
    public void testPositiveConfigure()
    {
        String[] args = {
            SSLModule.NAME
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);

        assertTrue(auth.configure(ConfigType.JSON, "{\"hello\":\"world\"}"));

        auth.close();
    }

    @Test
    public void testPositivePrepare()
    {
        String[] args = {
            SSLModule.NAME,
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);
        auth.activate();

        auth.prepare();

        auth.deactivate();
        auth.close();
    }

    @Test
    public void testPositiveDestructor()
    {
        String[] args = {
            SSLModule.NAME,
        };

        for (int i = 0; i < 10; i++) {

            for (int j = 0; j < 100; j++) {
                Object abc = new String[80000];
                Authenticator auth;
                auth = new Authenticator(args);
                assertNotNull(auth);
            }

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {

            }
        }
    }

    @Test
    public void testPositivePrepareCA()
    {
        String[] args = {
            SSLModule.NAME,
        };

        Authenticator auth;
        auth = new Authenticator(args);
        assertNotNull(auth);
        assertTrue(auth.configure(ConfigType.JSON, "{\"ssl\": {\"bits\":-1,\"days\":365,\"serial\": 1,\"enable_base64\": 1,\"is_ca\": 1}}"));
        assertTrue(auth.activate());
        assertTrue(auth.prepare());

        Authenticator authEE;
        authEE = new Authenticator(args);
        assertNotNull(authEE);
        assertTrue(authEE.configure(ConfigType.AUTHENTICATOR, auth));
        assertTrue(authEE.configure(ConfigType.JSON, "{\"ssl\": {\"bits\":-1,\"days\":365,\"serial\": 1,\"enable_base64\": 1,\"is_ca\": 0}}"));
        assertTrue(authEE.activate());
        assertTrue(authEE.prepare());

        assertTrue(authEE.deactivate());
        authEE.close();

        assertTrue(auth.deactivate());
        auth.close();
    }

    @Test
    public void testPositivePrepareAsync()
    {
        Looper.prepare();

        String[] args = {
            SSLModule.NAME,
            SSLModule.ARGUMENT_ASYNC_MODE,
        };
        Authenticator auth = new Authenticator(args);
        assertNotNull(auth);
        AtomicInteger invoked = new AtomicInteger();
        auth.setEventListener(eventObject -> {
            assertEquals((eventObject.eventType & SSLModule.EVENT_TYPE_PREPARE_DONE), SSLModule.EVENT_TYPE_PREPARE_DONE);
            invoked.getAndIncrement();
            myLooper().quit();
        });
        auth.activate();
        auth.prepare();

        Looper.loop();
        assertEquals(invoked.get(), 1);

        auth.deactivate();
        auth.close();
    }
}
