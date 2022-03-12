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

#include <beyond/beyond.h>

#include "main.h"

static int peer_event_callback(beyond_peer_h peer, struct beyond_event_info *event, void *data)
{
    printf("Peer Event Callback invoked 0x%.8X (%p)\n", event->type, event->data);
    return BEYOND_HANDLER_RETURN_RENEW;
}

static int runtime_event_callback(beyond_runtime_h runtime, struct beyond_event_info *event, void *data)
{
    printf("Runtime Event Callback invoked 0x%.8X (%p)\n", event->type, event->data);
    return BEYOND_HANDLER_RETURN_RENEW;
}

static enum beyond_handler_return stdin_handler(beyond_object_h object, int type, void *data)
{
    char ch = '?';

    if ((type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        fprintf(stderr, "error\n");
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (read(STDIN_FILENO, &ch, sizeof(ch)) < 0) {
        char errMsg[80];
        strerror_r(errno, errMsg, sizeof(errMsg));
        fprintf(stderr, "read: %s\n", errMsg);
    }

    printf("Key pressed: %c\n", ch);
    if (ch == 'X' || ch == 'x') {
        beyond_session_h session_handle = (beyond_session_h)object;
        printf("session handle %p %p\n", object, data);
        int ret = beyond_session_stop(session_handle);
        printf("session_stop returns %d\n", ret);
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

static void inference_output_handler(beyond_inference_h inference_handle, struct beyond_event_info *event, void *data)
{
    int output_count;
    beyond_tensor_h output;

    if ((event->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        fprintf(stderr, "Failed to do inference!\n");
        return;
    }

    if ((event->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) != BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
        printf("Event Type: 0x%.8X (%p)\n", event->type, event->data);
        return;
    }

    int ret = beyond_inference_get_output(inference_handle, &output, &output_count);
    if (ret < 0) {
        printf("get_output: %d\n", ret);
        return;
    }

    struct beyond_tensor *tensor = BEYOND_TENSOR(output);

    printf("context: %p (should be 0xbeefbeef)\n", event->data);
    printf("output_count: %d\n", output_count);
    printf("output tensor size: %d\n", tensor->size);
    printf("output tensor data: %p\n", tensor->data);
    printf("Dump to a file \"output.raw\"\n");

    FILE *fp;
    fp = fopen("output.raw", "w+b");
    if (fp != NULL) {
        if (fwrite(tensor->data, tensor->size, 1, fp) != 1) {
            char errMsg[80];
            strerror_r(errno, errMsg, sizeof(errMsg));
            fprintf(stderr, "fwrite: %s\n", errMsg);
        }
        fclose(fp);
    }

    if (beyond_inference_unref_tensor(output) != NULL) {
        printf("tensor_unref: %d\n", ret);
    }
}

static void help(void)
{
    printf("-h --help                  Help message\n");
    printf("-m --model [filename]      model filename\n");
    printf("-p --pipeline [filename]   pipeline description filename\n");
    printf("-i --input [filename]      Input data filename\n");
    printf("-l --local                 Use local mode inference\n");
}

int main(int argc, char *argv[])
{
    printf("Hello World!\n");
    const struct option opts[] = {
        {
            .name = "help",
            .has_arg = 0,
            .flag = NULL,
            .val = 'h',
        },
        {
            .name = "model",
            .has_arg = 1,
            .flag = NULL,
            .val = 'm',
        },
        {
            .name = "pipeline",
            .has_arg = 1,
            .flag = NULL,
            .val = 'p',
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
    };
    int c;
    int idx;
    char *inputFilename = NULL;
    char *modelFilename = NULL;
    char *pipelineFilename = NULL;
    int localMode = 0;
    const char *default_arg = BEYOND_INFERENCE_MODE_REMOTE;

    while ((c = getopt_long(argc, argv, "hm:p:i:l", opts, &idx)) != -1) {
        switch (c) {
        case 'h':
            help();
            return 0;
        case 'l':
            localMode = 1;
            default_arg = BEYOND_INFERENCE_MODE_LOCAL;
            break;
        case 'm':
            modelFilename = strdup(optarg);
            if (modelFilename == NULL) {
                char errMsg[80];
                strerror_r(errno, errMsg, sizeof(errMsg));
                fprintf(stderr, "strdup: %s\n", errMsg);
                return ENOMEM;
            }
            break;
        case 'p':
            pipelineFilename = strdup(optarg);
            if (pipelineFilename == NULL) {
                char errMsg[80];
                strerror_r(errno, errMsg, sizeof(errMsg));
                fprintf(stderr, "strdup: %s\n", errMsg);
                return ENOMEM;
            }
            break;
        case 'i':
            inputFilename = strdup(optarg);
            if (inputFilename == NULL) {
                char errMsg[80];
                strerror_r(errno, errMsg, sizeof(errMsg));
                fprintf(stderr, "strdup: %s\n", errMsg);
                return ENOMEM;
            }
            break;
        default:
            break;
        }
    }

    printf("%s / %s\n", modelFilename, pipelineFilename);

    struct beyond_argument option = {
        .argc = 1,
        .argv = (char **)&default_arg,
    };
    beyond_inference_h inference_handle;
    beyond_peer_h peer_handle = NULL;
    beyond_runtime_h runtime_handle = NULL;
    beyond_session_h session_handle;
    int ret;

    session_handle = beyond_session_create(0, 0);
    if (session_handle == NULL) {
        fprintf(stderr, "Unable to create a session\n");
        return EFAULT;
    }

    beyond_event_handler_h handler = beyond_session_add_fd_handler(session_handle, STDIN_FILENO, BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR, stdin_handler, NULL, session_handle);
    if (handler == NULL) {
        fprintf(stderr, "Unable to add stdin handler\n");
        beyond_session_destroy(session_handle);
        session_handle = NULL;
        return EFAULT;
    }

    inference_handle = beyond_inference_create(session_handle, &option);
    if (inference_handle == NULL) {
        fprintf(stderr, "Unable to create an inference instance\n");
        beyond_session_remove_fd_handler(session_handle, handler);
        handler = NULL;
        beyond_session_destroy(session_handle);
        session_handle = NULL;
        return EFAULT;
    }

    ret = beyond_inference_configure(inference_handle, NULL);
    if (ret < 0) {
        printf("configure ret: %d\n", ret);
    }

    ret = beyond_inference_set_output_callback(inference_handle, inference_output_handler, (void *)0xbeefbeef);
    if (ret < 0) {
        printf("set_output_callback ret: %d\n", ret);
    }

    if (localMode == 0) {
        char *nnstreamer_argv[5] = {
            "peer_nnstreamer",
            "--framework",
            "tensorflow-lite",
            "--pipeline",
            (char *)pipelineFilename,
        };

        struct beyond_argument peer_option = {
            .argc = 5,
            .argv = nnstreamer_argv,
        };

        peer_handle = beyond_peer_create(session_handle, &peer_option);
        if (peer_handle == NULL) {
            fprintf(stderr, "Unable to create a peer instance\n");
            beyond_inference_destroy(inference_handle);
            inference_handle = NULL;
            beyond_session_remove_fd_handler(session_handle, handler);
            handler = NULL;
            beyond_session_destroy(session_handle);
            session_handle = NULL;
            return EFAULT;
        }

        struct beyond_peer_info info = {
            .name = "name",
            .host = "127.0.0.1",
            .port = { 3000, 4000 },
            .free_memory = 0llu,
            .free_storage = 0llu,
        };

        ret = beyond_peer_configure(peer_handle, NULL);
        if (ret < 0) {
            printf("configure: %d\n", ret);
            beyond_inference_destroy(inference_handle);
            inference_handle = NULL;
            beyond_session_remove_fd_handler(session_handle, handler);
            handler = NULL;
            beyond_peer_destroy(peer_handle);
            peer_handle = NULL;
            beyond_session_destroy(session_handle);
            session_handle = NULL;
            return EFAULT;
        }

        ret = beyond_peer_set_event_callback(peer_handle, peer_event_callback, NULL);
        if (ret < 0) {
            printf("set_event_callback: %d\n", ret);
        }

        ret = beyond_peer_set_info(peer_handle, &info);
        if (ret < 0) {
            fprintf(stderr, "set_info: %d\n", ret);
        }

        ret = beyond_inference_add_peer(inference_handle, peer_handle);
        if (ret < 0) {
            fprintf(stderr, "add_peer ret: %d\n", ret);
        }
    } else {
        free(pipelineFilename);
        pipelineFilename = NULL;

        char *runtime_argv[1] = {
            "runtime_tflite",
        };

        struct beyond_argument runtime_option = {
            .argc = 1,
            .argv = runtime_argv,
        };

        runtime_handle = beyond_runtime_create(session_handle, &runtime_option);
        if (runtime_handle == NULL) {
            fprintf(stderr, "Failed to create a runtime module");
            beyond_inference_destroy(inference_handle);
            inference_handle = NULL;
            beyond_session_destroy(session_handle);
            session_handle = NULL;
            return EFAULT;
        }

        ret = beyond_runtime_set_event_callback(runtime_handle, runtime_event_callback, NULL);
        if (ret < 0) {
            fprintf(stderr, "runtime_set_event_handler: %d\n", ret);
        }

        ret = beyond_runtime_configure(runtime_handle, NULL);
        if (ret < 0) {
            fprintf(stderr, "runtime_configure: %d\n", ret);
        }

        ret = beyond_inference_add_runtime(inference_handle, runtime_handle);
        if (ret < 0) {
            fprintf(stderr, "add_runtime ret: %d\n", ret);
        }
    }

    ret = beyond_inference_load_model(inference_handle, (const char **)&modelFilename, 1);
    if (ret < 0) {
        printf("load_model ret: %d\n", ret);
    }

    ret = beyond_inference_prepare(inference_handle);
    if (ret < 0) {
        printf("prepare: %d\n", ret);
    }

    const struct beyond_tensor_info *input_info = NULL;
    int input_info_size;

    ret = beyond_inference_get_input_tensor_info(inference_handle, &input_info, &input_info_size);
    if (ret < 0) {
        fprintf(stderr, "Get Input tensorinfo failed\n");
    } else if (input_info == NULL) {
        fprintf(stderr, "Input tensorinfo is NULL\n");
    } else {
        do {
            beyond_tensor_h input;
            input = beyond_inference_allocate_tensor(inference_handle, input_info, input_info_size);
            if (input == NULL) {
                fprintf(stderr, "Failed to allocate tensor\n");
                ret = -ENOMEM;
                break;
            }

            struct beyond_tensor *tensor = BEYOND_TENSOR(input);
            if (tensor->data == NULL) {
                char errMsg[80];
                strerror_r(errno, errMsg, sizeof(errMsg));
                fprintf(stderr, "malloc: %s\n", errMsg);
                ret = -EFAULT;
                break;
            }

            if (inputFilename != NULL) {
                FILE *fp = fopen(inputFilename, "rb");
                if (fp != NULL) {
                    if (fread(tensor->data, 1, input_info->size, fp) != input_info->size) {
                        fprintf(stderr, "Unable to load a file: %s\n", inputFilename);
                    }
                    fclose(fp);
                } else {
                    fprintf(stderr, "Unable to open a input file: %s\n", inputFilename);
                }

                free(inputFilename);
                inputFilename = NULL;
            } else {
                printf("Input filename is not selected\n");
            }

            ret = beyond_inference_do(inference_handle, input, (void *)0xbeefbeef);
            if (ret < 0) {
                printf("do: %d\n", ret);
            }

            if (beyond_inference_unref_tensor(input) != NULL) {
                printf("tensor_unref\n");
            }
        } while (0);
    }

    const struct beyond_tensor_info *output_info;
    int output_info_size;

    ret = beyond_inference_get_output_tensor_info(inference_handle, &output_info, &output_info_size);
    if (ret < 0) {
        fprintf(stderr, "Get Output tensorinfo failed\n");
    }

    printf("Now ready to terminate application, Press 'X'\n");
    beyond_session_run(session_handle, 10, -1, -1);
    printf("Terminate application\n");

    if (peer_handle != NULL) {
        ret = beyond_inference_remove_peer(inference_handle, peer_handle);
        if (ret < 0) {
            printf("remove_peer: %d\n", ret);
        }
    }

    if (runtime_handle != NULL) {
        ret = beyond_inference_remove_runtime(inference_handle, runtime_handle);
        if (ret < 0) {
            printf("remove_runtime: %d\n", ret);
        }
    }

    beyond_inference_destroy(inference_handle);

    if (peer_handle != NULL) {
        beyond_peer_destroy(peer_handle);
    }

    if (runtime_handle != NULL) {
        beyond_runtime_destroy(runtime_handle);
    }

    ret = beyond_session_remove_fd_handler(session_handle, handler);
    if (ret < 0) {
        printf("beyond_session_remove_fd_handler: %d\n", ret);
    }
    handler = NULL;

    beyond_session_destroy(session_handle);

    return 0;
}
