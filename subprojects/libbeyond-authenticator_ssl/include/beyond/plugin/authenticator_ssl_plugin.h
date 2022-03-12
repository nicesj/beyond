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

#ifndef __BEYOND_PLUGIN_AUTHENTICATOR_SSL_PLUGIN_H__
#define __BEYOND_PLUGIN_AUTHENTICATOR_SSL_PLUGIN_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME "authenticator_ssl"
#define BEYOND_PLUGIN_AUTHENTICATOR_SSL_ARGUMENT_ASYNC_MODE "--async"
#define BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL ((char)(0xfe))
#define BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SECRET_KEY ((char)(0xfd))

#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_DONE 0x08010100
#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_PREPARE_ERROR 0x08010200
#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_DONE 0x08020100
#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_DEACTIVATE_ERROR 0x08020200
#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_DONE 0x08040100
#define BEYOND_PLUGIN_AUTHENTICATOR_EVENT_TYPE_CRYPTO_ERROR 0x08040200

struct beyond_authenticator_ssl_config_ssl {
    int bits;
    int serial;
    int days;
    int isCA;                     /*!< for the CA */
    int enableBase64;             /*!< Every input/output will be treated as a base64 string */
    const char *passphrase;       /*!< Passphrase */
    const char *private_key;      /*!< Private Key string */
    const char *certificate;      /*!< X509 Certificate string */
    const char *alternative_name; /*!< IP Address */
};

struct beyond_authenticator_ssl_config_secret_key {
    const char *key; /*!< Secret Key string will be used */
    int key_bits;    /*!< Length of secret key which should be generated in */
};

#if defined(__cplusplus)
}
#endif

#endif // __BEYOND_PLUGIN_AUTHENTICATOR_SSL_PLUGIN_H__
