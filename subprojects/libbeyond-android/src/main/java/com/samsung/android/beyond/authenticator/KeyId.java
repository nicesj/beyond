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

package com.samsung.android.beyond.authenticator;

public class KeyId {
    public static final int PRIVATE_KEY = 0; // Asymmetric key pair
    public static final int PUBLIC_KEY = 1;
    public static final int CERTIFICATE = 2;

    public static final int SECRET_KEY = 3; // Symmetric key

    // TODO: Add KeyId if necessary
}
