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

import com.samsung.android.beyond.discovery.Discovery;
import com.samsung.android.beyond.module.discovery.DNSSD.DNSSDModule;

import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.assertNotNull;

import android.os.Looper;

public class DiscoveryConstructionUnitTest {
    @BeforeClass
    public static void prepareLoop() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
    }

    @Test
    public void testCreateServer() {
        String[] args = { DNSSDModule.NAME, DNSSDModule.ARGUMENT_SERVER };
        try (Discovery inst = new Discovery(args)) {
            assertNotNull(inst);
        }
    }

    @Test
    public void testCreateClient() {
        String[] args = { DNSSDModule.NAME };
        try (Discovery inst = new Discovery(args)) {
            assertNotNull(inst);
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithNullArgs() {
        try (Discovery inst = new Discovery(null)) {
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithZeroArgs() {
        String[] args = {};
        try (Discovery inst = new Discovery(args)) {
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithInvalidArgs() {
        String[] args = { "discovery_invalid" };
        try (Discovery inst = new Discovery(args)) {
        }
    }
}
