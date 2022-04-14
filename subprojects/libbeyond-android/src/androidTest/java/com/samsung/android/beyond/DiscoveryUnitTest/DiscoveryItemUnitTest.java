package com.samsung.android.beyond.DiscoveryUnitTest;

import android.content.Context;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;

import com.samsung.android.beyond.discovery.Discovery;

import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

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
        try (Discovery server = new Discovery(context, args_server)) {
            assertNotNull(server);
            assertTrue(server.activate());

            String key = "key";
            byte[] value = "hello".getBytes();
            assertTrue(server.setItem(key, value));

            server.deactivate();
        }
    }

    @Test
    public void testRemoveItem() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery server = new Discovery(context, args_server)) {
            assertNotNull(server);
            assertTrue(server.activate());
            String key = "key";
            byte[] value = "hello".getBytes();
            assertTrue(server.setItem(key, value));

            assertTrue(server.removeItem(key));

            server.deactivate();
        }
    }

    @Test
    public void testSetItemAfterClosed() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        Discovery server = new Discovery(context, args_server);
        assertNotNull(server);
        server.close();

        String key = "key";
        byte[] value = "hello".getBytes();
        assertFalse(server.setItem(key, value));
    }

    @Test
    public void testRemoveItemAfterClosed() {
        String[] args_server = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        Discovery server = new Discovery(context, args_server);
        assertNotNull(server);
        server.close();

        String key = "key";
        assertFalse(server.removeItem(key));
    }
}
