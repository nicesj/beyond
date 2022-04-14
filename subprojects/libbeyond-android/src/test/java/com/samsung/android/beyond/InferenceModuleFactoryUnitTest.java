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

package com.samsung.android.beyond;

import android.content.Context;

import com.samsung.android.beyond.inference.InferenceModuleFactory;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.powermock.core.classloader.annotations.SuppressStaticInitializationFor;
import org.powermock.modules.junit4.PowerMockRunner;

/*
@RunWith(PowerMockRunner.class)
@SuppressStaticInitializationFor({"com.samsung.android.beyond.NativeInstance", "com.samsung.android.beyond.inference.Peer"})
public class InferenceModuleFactoryUnitTest {

    private static Context context;

    @Test(expected = NullPointerException.class)
    public void testCreateHandlerWithNullArgument() {
        InferenceModuleFactory.createHandler(null);
    }

    @Test(expected = NullPointerException.class)
    public void testCreatePeerWithNullArguments() {
        InferenceModuleFactory.createPeerServer(context, null);

        InferenceModuleFactory.createPeerServer(null, "DummyModule");

        InferenceModuleFactory.createPeerClient(context, null);

        InferenceModuleFactory.createPeerClient(null, "DummyModule");
    }
}
*/
