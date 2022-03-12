package com.samsung.android.beyond.DiscoveryUnitTest;

import android.content.Context;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;

import com.samsung.android.beyond.discovery.Discovery;
import com.samsung.android.beyond.ConfigType;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import com.samsung.android.beyond.module.discovery.DNSSD.DNSSDModule;

public class DiscoveryItemUnitTest {
    Context context = ApplicationProvider.getApplicationContext();

    @BeforeClass
    public static void prepareLoop() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
    }

    @Test
    public void testSetItem() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(args_server)) {
            assertNotNull(server);
            int ret = server.configure(ConfigType.CONTEXT_ANDROID, context);
            assertEquals(0, ret);

            ret = server.activate();
            assertEquals(0, ret);

            String key = "key";
            byte[] value = "hello".getBytes();
            ret = server.setItem(key, value);
            assertEquals(0, ret);

            server.deactivate();
        }
    }

    @Test
    public void testRemoveItem() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(args_server)) {
            assertNotNull(server);
            int ret = server.configure(ConfigType.CONTEXT_ANDROID, context);
            assertEquals(0, ret);
            ret = server.activate();
            assertEquals(0, ret);
            String key = "key";
            byte[] value = "hello".getBytes();
            ret = server.setItem(key, value);
            assertEquals(0, ret);

            ret = server.removeItem(key);
            assertEquals(0, ret);

            server.deactivate();
        }
    }

    @Test(expected = IllegalStateException.class)
    public void testSetItemAfterClosed() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        Discovery server = new Discovery(args_server);
        assertNotNull(server);
        server.close();

        String key = "key";
        byte[] value = "hello".getBytes();
        server.setItem(key, value);
    }

    @Test(expected = IllegalStateException.class)
    public void testRemoveItemAfterClosed() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        Discovery server = new Discovery(args_server);
        assertNotNull(server);
        server.close();

        String key = "key";
        server.removeItem(key);
    }
}
