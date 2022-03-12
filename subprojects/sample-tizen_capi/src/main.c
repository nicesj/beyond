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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include <dlog/dlog.h>

#include <beyond/beyond.h>
#include <beyond/plugin/authenticator_ssl_plugin.h>
#include <beyond/plugin/peer_nn_plugin.h>
#include <beyond/plugin/runtime_tflite_plugin.h>
#include <glib.h>

#include "main.h"

#define UUID "481ef7e9-8264-4903-b978-7280e4c593f0"
#define LOCALHOST "127.0.0.1"

enum Type {
    DEVICE = 0,
    EDGE = 1,
};

struct DeviceContext {
    char *inputFilename;
    char *modelFilename;
    char *mode;
    beyond_inference_h inference_handle;
    beyond_peer_h peer_handle;
    beyond_runtime_h runtime_handle;
};

struct EdgeContext {
    beyond_peer_h peer_handle;
};

struct SSLContext {
    struct {
        char *private;
        char *certificate;
        beyond_authenticator_h auth;
    } root;

    struct {
        char *private;
        char *certificate;
        beyond_authenticator_h auth;
    } endpoint;

    int generate;
};

static void FreeDeviceContext(struct DeviceContext *deviceContext)
{
    int ret;

    free(deviceContext->inputFilename);
    deviceContext->inputFilename = NULL;

    free(deviceContext->modelFilename);
    deviceContext->modelFilename = NULL;

    if (deviceContext->inference_handle == NULL) {
        return;
    }

    if (deviceContext->peer_handle != NULL) {
        ret = beyond_inference_remove_peer(deviceContext->inference_handle, deviceContext->peer_handle);
        if (ret < 0) {
            LOGE("remove_peer: %d", ret);
        }

        beyond_peer_destroy(deviceContext->peer_handle);
        deviceContext->peer_handle = NULL;
    }

    if (deviceContext->runtime_handle != NULL) {
        ret = beyond_inference_remove_runtime(deviceContext->inference_handle, deviceContext->runtime_handle);
        if (ret < 0) {
            LOGE("remove_runtime: %d", ret);
        }

        beyond_runtime_destroy(deviceContext->runtime_handle);
        deviceContext->runtime_handle = NULL;
    }

    beyond_inference_destroy(deviceContext->inference_handle);
    deviceContext->inference_handle = NULL;
}

static void FreeEdgeContext(struct EdgeContext *edgeContext)
{
    int ret;

    if (edgeContext->peer_handle == NULL) {
        return;
    }

    ret = beyond_peer_deactivate(edgeContext->peer_handle);
    if (ret < 0) {
        LOGE("Failed to deactivate the peer: %d", ret);
    }

    beyond_peer_destroy(edgeContext->peer_handle);
    edgeContext->peer_handle = NULL;
}

static void FreeSSLContext(struct SSLContext *sslContext)
{
    free(sslContext->root.private);
    sslContext->root.private = NULL;
    free(sslContext->root.certificate);
    sslContext->root.certificate = NULL;
    free(sslContext->endpoint.private);
    sslContext->endpoint.private = NULL;
    free(sslContext->endpoint.certificate);
    sslContext->endpoint.certificate = NULL;

    if (sslContext->root.auth != NULL) {
        beyond_authenticator_deactivate(sslContext->root.auth);
        beyond_authenticator_destroy(sslContext->root.auth);
    }

    if (sslContext->endpoint.auth != NULL) {
        beyond_authenticator_deactivate(sslContext->endpoint.auth);
        beyond_authenticator_destroy(sslContext->endpoint.auth);
    }
}

static void DumpDeviceContext(struct DeviceContext *deviceContext)
{
    LOGD("Input: %s", deviceContext->inputFilename);
    LOGD("Model: %s", deviceContext->modelFilename);
    LOGD("Mode: %s", deviceContext->mode);
}

static void DumpEdgeContext(struct EdgeContext *edgeContext)
{
}

static void DumpSSLContext(struct SSLContext *sslContext)
{
    LOGD("Root.Private: [%s]", sslContext->root.private);
    LOGD("Root.Certificate: [%s]", sslContext->root.certificate);
    LOGD("Private: [%s]", sslContext->endpoint.private);
    LOGD("Certificate: [%s]", sslContext->endpoint.certificate);
}

static int peer_event_callback(beyond_peer_h peer, struct beyond_event_info *event, void *data)
{
    LOGD("Peer Event Callback invoked 0x%.8X (%p)", event->type, event->data);
    return BEYOND_HANDLER_RETURN_RENEW;
}

static int runtime_event_callback(beyond_runtime_h runtime, struct beyond_event_info *event, void *data)
{
    LOGD("Runtime Event Callback invoked 0x%.8X (%p)", event->type, event->data);
    return BEYOND_HANDLER_RETURN_RENEW;
}

static void inference_event_handler(beyond_inference_h handle, struct beyond_event_info *event, void *data)
{
    int output_count;

    if ((event->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        LOGE("Error!");
        return;
    }

    if ((event->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) != BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
        printf("Event Type: 0x%.8X (%p)\n", event->type, event->data);
        return;
    }

    beyond_tensor_h output;
    int ret = beyond_inference_get_output(handle, &output, &output_count);
    if (ret < 0) {
        LOGD("get_output: %d", ret);
    }

    struct beyond_tensor *tensor = BEYOND_TENSOR(output);

    LOGD("context: %p (should be 0xbeefbeef)", event->data);
    LOGD("output_count: %d", output_count);
    LOGD("output tensor size: %d", tensor->size);
    LOGD("output tensor size: %p", tensor->data);
    LOGD("Dump to a file \"output.raw\"");

    for (int i = 0; i < output_count; i++) {
        FILE *fp;
        char fname[256];
        snprintf(fname, sizeof(fname), "output%d_%d.raw", i, output_count);
        fp = fopen(fname, "w+b");
        if (fp != NULL) {
            if (fwrite(tensor->data, tensor->size, 1, fp) != 1) {
                LOGE("fwrite: %m");
            }
            fclose(fp);
        }
    }

    if (beyond_inference_unref_tensor(output) != NULL) {
        LOGD("destroy_tensor: %d", ret);
    }
}

static gboolean key_handler(GIOChannel *channel, GIOCondition cond, gpointer data)
{
    if ((cond & G_IO_ERR) == G_IO_ERR) {
        LOGE("Ooops");
        return FALSE;
    }

    GMainLoop *gLoop = (GMainLoop *)data;

    int fd = g_io_channel_unix_get_fd(channel);
    char ch;
    if (read(fd, &ch, sizeof(ch)) < 0) {
        LOGE("read: %m");
        return FALSE;
    }

    if (ch == 'X') {
        g_main_loop_quit(gLoop);
    }

    return TRUE;
}

static int CreateAuthenticator(enum Type type, const char *host, struct SSLContext *sslContext)
{
    int ret;

    const char *default_auth_arg[] = {
        BEYOND_PLUGIN_AUTHENTICATOR_SSL_NAME,
    };

    struct beyond_argument option = {
        .argc = sizeof(default_auth_arg) / sizeof(char *),
        .argv = (char **)default_auth_arg,
    };

    struct beyond_authenticator_ssl_config_ssl ssl_config = {
        .bits = 4096,
        .serial = -1,
        .days = -1,
        .isCA = -1,
        .enableBase64 = -1,
        .passphrase = NULL,
        .private_key = NULL,
        .certificate = NULL,
        .alternative_name = NULL,
    };

    struct beyond_config config = {
        .type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL,
        .object = (void *)&ssl_config,
    };

    if (sslContext->generate == 1) {
        if (type == DEVICE) {
            LOGE("Device cannot generate certificates");
            return -EINVAL;
        }

        sslContext->root.auth = beyond_authenticator_create(&option);
        if (sslContext->root.auth == NULL) {
            LOGE("Unable to create an authenticator module instance");
            return -EFAULT;
        }

        config.type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL;
        ssl_config.alternative_name = host;
        config.object = (void *)&ssl_config;
        ret = beyond_authenticator_configure(sslContext->root.auth, &config);
        if (ret < 0) {
            LOGE("Unable to configure an authenticatorm module instance");
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        ret = beyond_authenticator_activate(sslContext->root.auth);
        if (ret < 0) {
            LOGE("Unable to activate an authenticator module instance");
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        ret = beyond_authenticator_prepare(sslContext->root.auth);
        if (ret < 0) {
            LOGE("Unable to prepare an authenticator module instance");
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        LOGD("CA Authenticator is prepared");

        sslContext->endpoint.auth = beyond_authenticator_create(&option);
        if (sslContext->root.auth == NULL) {
            LOGE("Unable to create an endpoint authenticator module instance");
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return -EFAULT;
        }

        config.type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL;
        ssl_config.isCA = 0;
        ssl_config.alternative_name = host;
        config.object = (void *)&ssl_config;
        ret = beyond_authenticator_configure(sslContext->endpoint.auth, &config);
        if (ret < 0) {
            LOGE("Unable to configure an endpoint authenticator module instance");
            beyond_authenticator_destroy(sslContext->endpoint.auth);
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        config.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
        config.object = (void *)sslContext->root.auth;
        ret = beyond_authenticator_configure(sslContext->endpoint.auth, &config);
        if (ret < 0) {
            LOGE("Unable to configure an endpoint authenticator module instance");
            beyond_authenticator_destroy(sslContext->endpoint.auth);
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        ret = beyond_authenticator_activate(sslContext->endpoint.auth);
        if (ret < 0) {
            LOGE("Unable to activate an auth instance");
            beyond_authenticator_destroy(sslContext->endpoint.auth);
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        ret = beyond_authenticator_prepare(sslContext->endpoint.auth);
        if (ret < 0) {
            LOGE("Unable to prepare an auth instance");
            beyond_authenticator_deactivate(sslContext->endpoint.auth);
            beyond_authenticator_destroy(sslContext->endpoint.auth);
            beyond_authenticator_deactivate(sslContext->root.auth);
            beyond_authenticator_destroy(sslContext->root.auth);
            return ret;
        }

        LOGD("EE Authenticator is prepared");
    } else {
        if (type == DEVICE) {
            if (sslContext->root.certificate == NULL) {
                LOGE("Root Certificate was not specified");
                return -EINVAL;
            }

            sslContext->root.auth = beyond_authenticator_create(&option);
            if (sslContext->root.auth == NULL) {
                LOGE("Unable to create a authenticator module instance");
                return -EFAULT;
            }

            config.type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL;
            ssl_config.certificate = sslContext->root.certificate;
            config.object = (void *)&ssl_config;
            ret = beyond_authenticator_configure(sslContext->root.auth, &config);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_activate(sslContext->root.auth);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_prepare(sslContext->root.auth);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }
        } else {
            if (sslContext->endpoint.certificate == NULL) {
                LOGE("Certificate was not specified");
                return -EINVAL;
            }

            if (sslContext->endpoint.private == NULL) {
                LOGE("Private key was not specified");
                return -EINVAL;
            }

            if (sslContext->root.certificate == NULL) {
                LOGE("Root Certificate was not specified");
                return -EINVAL;
            }

            if (sslContext->root.private == NULL) {
                LOGE("Root Private key was not specified");
                return -EINVAL;
            }

            sslContext->root.auth = beyond_authenticator_create(&option);
            if (sslContext->root.auth == NULL) {
                LOGE("Unable to create a authenticator module instance");
                return -EFAULT;
            }

            config.type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL;
            ssl_config.certificate = sslContext->root.certificate;
            ssl_config.private_key = sslContext->root.private;
            config.object = (void *)&ssl_config;
            ret = beyond_authenticator_configure(sslContext->root.auth, &config);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_activate(sslContext->root.auth);
            if (ret < 0) {
                LOGE("Unable to activate an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_prepare(sslContext->root.auth);
            if (ret < 0) {
                LOGE("Unable to activate an authenticatorm module instance");
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            sslContext->endpoint.auth = beyond_authenticator_create(&option);
            if (sslContext->endpoint.auth == NULL) {
                LOGE("Unable to create a authenticator module instance");
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return -EFAULT;
            }

            config.type = BEYOND_PLUGIN_AUTHENTICATOR_SSL_CONFIG_SSL;
            ssl_config.certificate = sslContext->endpoint.certificate;
            ssl_config.private_key = sslContext->endpoint.private;
            config.object = (void *)&ssl_config;
            ret = beyond_authenticator_configure(sslContext->endpoint.auth, &config);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->endpoint.auth);
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            config.type = BEYOND_CONFIG_TYPE_AUTHENTICATOR;
            config.object = (void *)sslContext->root.auth;
            ret = beyond_authenticator_configure(sslContext->endpoint.auth, &config);
            if (ret < 0) {
                LOGE("Unable to configure an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->endpoint.auth);
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_activate(sslContext->endpoint.auth);
            if (ret < 0) {
                LOGE("Unable to activate an authenticatorm module instance");
                beyond_authenticator_destroy(sslContext->endpoint.auth);
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }

            ret = beyond_authenticator_prepare(sslContext->endpoint.auth);
            if (ret < 0) {
                LOGE("Unable to activate an authenticatorm module instance");
                beyond_authenticator_deactivate(sslContext->endpoint.auth);
                beyond_authenticator_destroy(sslContext->endpoint.auth);
                beyond_authenticator_deactivate(sslContext->root.auth);
                beyond_authenticator_destroy(sslContext->root.auth);
                return ret;
            }
        }
    }

    return 0;
}

static char *LoadFile(const char *filename, int *_size)
{
    char *contents = NULL;

    FILE *fp = fopen(optarg, "r");
    if (fp == NULL) {
        LOGE("fopen: %m");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) < 0) {
        LOGE("fseek: %m");
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0) {
        LOGE("ftell: %m");
        fclose(fp);
        return NULL;
    }

    LOGD("Size: %ld", size);
    if (fseek(fp, 0, SEEK_SET) < 0) {
        LOGE("fseek: %m");
        fclose(fp);
        return NULL;
    }

    contents = calloc(size, 1);
    if (contents == NULL) {
        LOGE("malloc: %m");
        fclose(fp);
        return NULL;
    }

    if (fread(contents, 1, size, fp) != size) {
        LOGE("malloc: %m");
        fclose(fp);
        free(contents);
        return NULL;
    }

    if (fclose(fp) < 0) {
        LOGE("fclose: %m");
    }

    if (_size != NULL) {
        *_size = (int)size;
    }

    return contents;
}

int CreateDevice(const char *host, int port, struct DeviceContext *deviceContext, struct SSLContext *sslContext)
{
    int ret;

    struct beyond_argument option = {
        .argc = 1,
        .argv = (char **)&deviceContext->mode,
    };

    deviceContext->inference_handle = beyond_inference_create(&option);
    if (deviceContext->inference_handle == NULL) {
        LOGE("Unable to create an inference instance");
        return -EFAULT;
    }

    ret = beyond_inference_set_output_callback(deviceContext->inference_handle, inference_event_handler, (void *)0xbeefbeef);
    if (ret < 0) {
        LOGE("unable to add output callback");
    }

    if (deviceContext->mode != NULL && strcmp(deviceContext->mode, BEYOND_INFERENCE_MODE_LOCAL) != 0) {
        char *peer_nn_argv[] = {
            BEYOND_PLUGIN_PEER_NN_NAME,
        };

        struct beyond_argument peer_option = {
            .argc = sizeof(peer_nn_argv) / sizeof(char *),
            .argv = peer_nn_argv,
        };

        deviceContext->peer_handle = beyond_peer_create(&peer_option);
        if (deviceContext->peer_handle == NULL) {
            LOGE("Unable to create a peer instance");
            beyond_inference_destroy(deviceContext->inference_handle);
            deviceContext->inference_handle = NULL;
            return -EFAULT;
        }

        if (sslContext->root.auth != NULL) {
            struct beyond_config config = {
                .type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR,
                .object = sslContext->root.auth,
            };

            ret = beyond_peer_configure(deviceContext->peer_handle, &config);
            if (ret < 0) {
                LOGE("Unable to configure a peer instance");
                beyond_peer_destroy(deviceContext->peer_handle);
                deviceContext->peer_handle = NULL;
                beyond_inference_destroy(deviceContext->inference_handle);
                deviceContext->inference_handle = NULL;
                return -EFAULT;
            }
        }

        struct beyond_peer_info info = {
            .name = (char *)"name",
            .host = (char *)host,
            .port = { port },
            .free_memory = 0llu,
            .free_storage = 0llu,
            .uuid = UUID,
        };

        ret = beyond_peer_set_event_callback(deviceContext->peer_handle, peer_event_callback, NULL);
        if (ret < 0) {
            LOGD("set_event_callback: %d", ret);
        }

        ret = beyond_peer_set_info(deviceContext->peer_handle, &info);
        if (ret < 0) {
            LOGE("set_info: %d", ret);
        }

        ret = beyond_inference_add_peer(deviceContext->inference_handle, deviceContext->peer_handle);
        if (ret < 0) {
            LOGE("add_peer ret: %d", ret);
        }
    } else {
        char *runtime_argv[1] = {
            BEYOND_PLUGIN_RUNTIME_TFLITE_NAME,
        };

        struct beyond_argument runtime_option = {
            .argc = 1,
            .argv = runtime_argv,
        };

        deviceContext->runtime_handle = beyond_runtime_create(&runtime_option);
        if (deviceContext->runtime_handle == NULL) {
            fprintf(stderr, "Failed to create a runtime instance");
            beyond_inference_destroy(deviceContext->inference_handle);
            deviceContext->inference_handle = NULL;
            return -EFAULT;
        }

        ret = beyond_runtime_set_event_callback(deviceContext->runtime_handle, runtime_event_callback, NULL);
        if (ret < 0) {
            fprintf(stderr, "runtime_set_event_callback: %d\n", ret);
        }

        ret = beyond_inference_add_runtime(deviceContext->inference_handle, deviceContext->runtime_handle);
        if (ret < 0) {
            fprintf(stderr, "add_runtime ret: %d\n", ret);
        }
    }

    ret = beyond_inference_load_model(deviceContext->inference_handle, (const char **)&deviceContext->modelFilename, 1);
    if (ret < 0) {
        LOGD("load_model ret: %d", ret);
    }

    ret = beyond_inference_prepare(deviceContext->inference_handle);
    if (ret < 0) {
        LOGD("prepare: %d", ret);
    }

    const struct beyond_tensor_info *input_info;
    int input_info_size;
    ret = beyond_inference_get_input_tensor_info(deviceContext->inference_handle, &input_info, &input_info_size);
    if (ret < 0) {
        LOGE("Failed to get input tensor info");
    } else if (input_info == NULL) {
        LOGE("Input tensor info is nullptr");
    } else {
        beyond_tensor_h input;
        input = beyond_inference_allocate_tensor(deviceContext->inference_handle, input_info, input_info_size);
        if (input == NULL) {
            LOGE("malloc: %m");

            if (deviceContext->peer_handle != NULL) {
                beyond_inference_remove_peer(deviceContext->inference_handle, deviceContext->peer_handle);
                beyond_peer_destroy(deviceContext->peer_handle);
                deviceContext->peer_handle = NULL;
            }

            if (deviceContext->runtime_handle != NULL) {
                beyond_inference_remove_runtime(deviceContext->inference_handle, deviceContext->runtime_handle);
                beyond_runtime_destroy(deviceContext->runtime_handle);
                deviceContext->runtime_handle = NULL;
            }

            beyond_inference_destroy(deviceContext->inference_handle);
            deviceContext->inference_handle = NULL;
            return -ENOMEM;
        }

        struct beyond_tensor *tensor = BEYOND_TENSOR(input);

        if (deviceContext->inputFilename != NULL) {
            int size;
            void *data = LoadFile(deviceContext->inputFilename, &size);
            if (size != input_info->size) {
                LOGE("Invalid input tensor size %d %d", size, input_info->size);
            } else if (data == NULL) {
                LOGE("Failed to load input data");
            } else {
                memcpy(tensor->data, data, input_info->size);
            }
            free(data);
        } else {
            LOGI("Input filename is not selected");
        }

        ret = beyond_inference_do(deviceContext->inference_handle, input, (void *)0xbeefbeef);
        if (ret < 0) {
            LOGD("do: %d", ret);
        }

        if (beyond_inference_unref_tensor(input) != NULL) {
            LOGD("tensor_unref");
        }
    }

    const struct beyond_tensor_info *output_info;
    int output_info_size;
    ret = beyond_inference_get_output_tensor_info(deviceContext->inference_handle, &output_info, &output_info_size);
    if (ret < 0) {
        LOGE("Failed to get output tensor info");
    }

    return 0;
}

int CreateEdge(int port, struct EdgeContext *edgeContext, struct SSLContext *sslContext)
{
    int ret;

    char *peer_nn_argv[] = {
        BEYOND_PLUGIN_PEER_NN_NAME,
        BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER,
    };

    struct beyond_argument peer_option = {
        .argc = sizeof(peer_nn_argv) / sizeof(char *),
        .argv = peer_nn_argv,
    };

    edgeContext->peer_handle = beyond_peer_create(&peer_option);
    if (edgeContext->peer_handle == NULL) {
        LOGE("Unable to create a peer instance");
        return -EFAULT;
    }

    if (sslContext->root.auth != NULL) {
        struct beyond_config config = {
            .type = BEYOND_PLUGIN_PEER_NN_CONFIG_CA_AUTHENTICATOR,
            .object = sslContext->root.auth,
        };

        ret = beyond_peer_configure(edgeContext->peer_handle, &config);
        if (ret < 0) {
            LOGE("Unable to configure a peer instance");
            beyond_peer_destroy(edgeContext->peer_handle);
            edgeContext->peer_handle = NULL;
            return -EFAULT;
        }

        LOGD("Peer configured using CA Authenticator");
    }

    if (sslContext->endpoint.auth != NULL) {
        struct beyond_config config = {
            .type = BEYOND_CONFIG_TYPE_AUTHENTICATOR,
            .object = sslContext->endpoint.auth,
        };

        ret = beyond_peer_configure(edgeContext->peer_handle, &config);
        if (ret < 0) {
            LOGE("Unable to configure a peer instance");
            beyond_peer_destroy(edgeContext->peer_handle);
            edgeContext->peer_handle = NULL;
            return -EFAULT;
        }

        LOGD("Peer configured using Authenticator");
    }

    ret = beyond_peer_set_event_callback(edgeContext->peer_handle, peer_event_callback, NULL);
    if (ret < 0) {
        LOGD("set_event_callback: %d", ret);
    }

    struct beyond_peer_info info = {
        .name = (char *)"name",
        .host = (char *)"0.0.0.0",
        .port = { port },
        .free_memory = 0llu,
        .free_storage = 0llu,
        .uuid = UUID,
    };

    ret = beyond_peer_set_info(edgeContext->peer_handle, &info);
    if (ret < 0) {
        LOGE("set_info: %d", ret);
    }

    ret = beyond_peer_activate(edgeContext->peer_handle);
    if (ret < 0) {
        beyond_peer_destroy(edgeContext->peer_handle);
        edgeContext->peer_handle = NULL;
        return ret;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    LOGD("Hello World!");
    const struct option opts[] = {
        {
            .name = "model",
            .has_arg = 1,
            .flag = NULL,
            .val = 'm',
        },
        {
            .name = "input",
            .has_arg = 1,
            .flag = NULL,
            .val = 'i',
        },
        {
            .name = "local",
            .has_arg = 0,
            .flag = NULL,
            .val = 'l',
        },
        {
            .name = "host",
            .has_arg = 1,
            .flag = NULL,
            .val = 'h',
        },
        {
            .name = "port",
            .has_arg = 1,
            .flag = NULL,
            .val = 'p',
        },
        {
            .name = "certificate",
            .has_arg = 1,
            .flag = NULL,
            .val = 'c',
        },
        {
            .name = "key",
            .has_arg = 1,
            .flag = NULL,
            .val = 'k',
        },
        {
            .name = "root_certificate",
            .has_arg = 1,
            .flag = NULL,
            .val = 'C',
        },
        {
            .name = "root_private",
            .has_arg = 1,
            .flag = NULL,
            .val = 'K',
        },
        {
            .name = "edge",
            .has_arg = 0,
            .flag = NULL,
            .val = 'e',
        },
        {
            .name = "generate",
            .has_arg = 0,
            .flag = NULL,
            .val = 'g',
        },
    };
    int c;
    int idx;

    struct DeviceContext deviceContext = {
        NULL,
        NULL,
        BEYOND_INFERENCE_MODE_REMOTE,
        NULL,
        NULL,
        NULL,
    };

    struct EdgeContext edgeContext = {
        NULL,
    };

    struct SSLContext sslContext = {
        .root = {
            NULL,
            NULL,
            NULL,
        },
        .endpoint = {
            NULL,
            NULL,
            NULL,
        },
        0,
    };

    enum Type type = DEVICE;
    char *host = NULL;
    int port;
    int ret;

    while ((c = getopt_long(argc, argv, "h:m:p:i:lc:k:sC:K:ge", opts, &idx)) != -1) {
        switch (c) {
        case 'g':
            sslContext.generate = 1;
            break;
        case 'h':
            free(host);
            host = strdup(optarg);
            if (host == NULL) {
                LOGE("strdup: %m");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return ENOMEM;
            }
            break;
        case 'l':
            deviceContext.mode = BEYOND_INFERENCE_MODE_LOCAL;
            break;
        case 'e':
            type = EDGE;
            break;
        case 'm':
            free(deviceContext.modelFilename);
            deviceContext.modelFilename = strdup(optarg);
            if (deviceContext.modelFilename == NULL) {
                LOGE("strdup: %m");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return ENOMEM;
            }
            break;
        case 'p':
            if (sscanf(optarg, "%d", &port) != 1) {
                LOGE("Failed to parse the port");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return EINVAL;
            }
            break;
        case 'i':
            free(deviceContext.inputFilename);
            deviceContext.inputFilename = strdup(optarg);
            if (deviceContext.inputFilename == NULL) {
                LOGE("strdup: %m");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return ENOMEM;
            }
            break;
        case 'C':
            LOGD("Root.Certificate: %s", optarg);
            free(sslContext.root.certificate);
            sslContext.root.certificate = LoadFile(optarg, NULL);
            if (sslContext.root.certificate == NULL) {
                LOGE("Failed to load a file");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return EINVAL;
            }
            LOGD("Contents: [%s]", sslContext.root.certificate);
            break;
        case 'K':
            LOGD("Root.Private: %s", optarg);
            free(sslContext.root.private);
            sslContext.root.private = LoadFile(optarg, NULL);
            if (sslContext.root.private == NULL) {
                LOGE("Failed to load a file");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return EINVAL;
            }
            break;
        case 'c':
            LOGD("Certificate: %s", optarg);
            free(sslContext.endpoint.certificate);
            sslContext.endpoint.certificate = LoadFile(optarg, NULL);
            if (sslContext.endpoint.certificate == NULL) {
                LOGE("Failed to load a file");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return EINVAL;
            }
            break;
        case 'k':
            LOGD("Private: %s", optarg);
            free(sslContext.endpoint.private);
            sslContext.endpoint.private = LoadFile(optarg, NULL);
            if (sslContext.endpoint.private == NULL) {
                LOGE("Failed to load a file");
                FreeSSLContext(&sslContext);
                FreeEdgeContext(&edgeContext);
                FreeDeviceContext(&deviceContext);
                return EINVAL;
            }
            break;
        default:
            break;
        }
    }

    if (host == NULL) {
        host = strdup(LOCALHOST);
        if (host == NULL) {
            ret = errno;
            LOGE("strdup: %m");
            return ret;
        }
    }

    DumpDeviceContext(&deviceContext);
    DumpEdgeContext(&edgeContext);
    DumpSSLContext(&sslContext);

    GMainLoop *gLoop = g_main_loop_new(NULL, FALSE);
    if (gLoop == NULL) {
        LOGE("Unable to create a g_main_loop");
        FreeSSLContext(&sslContext);
        FreeEdgeContext(&edgeContext);
        FreeDeviceContext(&deviceContext);
        return EFAULT;
    }

    ret = CreateAuthenticator(type, host, &sslContext);
    if (ret < 0) {
        g_main_loop_unref(gLoop);
        gLoop = NULL;
        FreeSSLContext(&sslContext);
        FreeEdgeContext(&edgeContext);
        FreeDeviceContext(&deviceContext);
        return EFAULT;
    }

    if (type == EDGE) {
        ret = CreateEdge(port, &edgeContext, &sslContext);
    } else if (type == DEVICE) {
        ret = CreateDevice(host, port, &deviceContext, &sslContext);
    }

    if (ret < 0) {
        LOGE("Unable to prepate the application");
        g_main_loop_unref(gLoop);
        gLoop = NULL;
        FreeSSLContext(&sslContext);
        FreeEdgeContext(&edgeContext);
        FreeDeviceContext(&deviceContext);
        return EFAULT;
    }

    GIOChannel *channel = g_io_channel_unix_new(STDIN_FILENO);
    guint watchId = g_io_add_watch(channel, G_IO_IN | G_IO_ERR, key_handler, gLoop);
    LOGD("Now ready to terminate application, Press 'X'");
    g_main_loop_run(gLoop);
    LOGD("Terminate application");

    if (watchId != 0) {
        g_source_remove(watchId);
    }
    g_io_channel_unref(channel);
    g_main_loop_unref(gLoop);
    gLoop = NULL;

    FreeSSLContext(&sslContext);
    FreeEdgeContext(&edgeContext);
    FreeDeviceContext(&deviceContext);
    return 0;
}
