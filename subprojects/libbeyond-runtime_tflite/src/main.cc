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

#if !defined(NDEBUG)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <getopt.h>
#include <unistd.h>

#include <beyond/private/beyond_private.h>
#include "runtime.h"

extern "C" {
extern void *_main(int argc, char *argv[]);

static void help(void)
{
    fprintf(stderr,
            "Help message\n"
            "-----------------------------------------\n"
            "-h                         help\n"
            "-i filename                input filename\n"
            "-t input_tensor type       input filename\n"
            "-m filename                model filename\n"
            "\n");
}

// ApplicationContext
int main(void)
{
    const option opts[] = {
        {
            .name = "help",
            .has_arg = 0,
            .flag = nullptr,
            .val = 'h',
        },
        {
            .name = "model",
            .has_arg = 1,
            .flag = nullptr,
            .val = 'm',
        },
        {
            .name = "input",
            .has_arg = 1,
            .flag = nullptr,
            .val = 'i',
        },
        {
            .name = "type",
            .has_arg = 1,
            .flag = nullptr,
            .val = 't',
        },
        {
            .name = "size", // Count of input tensor
            .has_arg = 1,
            .flag = nullptr,
            .val = 's',
        },
        // TODO:
        // Add more options
    };
    static const char *optstr = "-:hm:i:t:s:";

    char *model = nullptr;
    char *type = nullptr;
    int argc = 0;
    char **argv = nullptr;
    beyond_tensor *input = nullptr;
    beyond_tensor *output = nullptr;

    FILE *fp = fopen("input.txt", "r");
    if (fp != nullptr) {
        if (fscanf(fp, "%d\n", &argc) == 1 && argc > 0) {
            if (argc < 0 || static_cast<unsigned int>(argc) > strlen(optstr)) {
                fprintf(stderr, "Too many arguments: %d\n", argc);
                _exit(EINVAL);
            }

            argv = static_cast<char **>(calloc(argc, sizeof(char *)));
            if (argv == nullptr) {
                char errMsg[80] = { '\0' };
                fprintf(stderr, "calloc: %s\n", strerror_r(errno, errMsg, sizeof(errMsg)));
                _exit(ENOMEM);
            }

            for (int i = 0; i < argc; i++) {
                char buffer[1024];
                if (fscanf(fp, "%1023s\n", buffer) == 1) {
                    argv[i] = strdup(buffer);
                } else {
                    ErrPrint("Unable to parse the argument buffer");
                    argv[i] = nullptr;
                }
            }
        }
        fclose(fp);
        fp = nullptr;
    }

    int input_size = 0;
    int input_idx = 0;
    if (argc > 0 && argv != nullptr) {
        int c;
        int idx;
        while ((c = getopt_long(argc, argv, optstr, opts, &idx)) != -1) {
            switch (c) {
            case 'h':
                help();
                _exit(0);
                break;
            case 'm':
                model = strdup(optarg);
                break;
            case 's':
                if (sscanf(optarg, "%d", &input_size) != 1) {
                    fprintf(stderr, "Invalid input size parameter\n");
                    _exit(EINVAL);
                }

                //Temporary max size
                if (1024 < input_size || input_size < 0) {
                    fprintf(stderr, "Invalid input size(%d) parameter\n", input_size);
                    _exit(EINVAL);
                }

                input = static_cast<beyond_tensor *>(calloc(input_size, sizeof(beyond_tensor)));
                if (input == nullptr) {
                    int ret = errno;
                    char errMsg[80] = { '\0' };
                    fprintf(stderr, "calloc: %s\n", strerror_r(errno, errMsg, sizeof(errMsg)));
                    _exit(ret);
                }
                break;
            case 'i':
                if (input_idx == input_size) {
                    fprintf(stderr, "Invalid input tensor, input_size must be given first (%d, %d)\n", input_idx, input_size);
                    _exit(EINVAL);
                }

                if (optarg != nullptr) {
                    FILE *fp = fopen(optarg, "r");
                    if (fp != nullptr) {
                        fseek(fp, 0L, SEEK_END);
                        input[input_idx].size = ftell(fp);
                        fseek(fp, 0L, SEEK_SET);

                        input[input_idx].data = malloc(input[input_idx].size);
                        if (input[input_idx].data == nullptr) {
                            char errMsg[80] = { '\0' };
                            fprintf(stderr, "malloc: %s\n", strerror_r(errno, errMsg, sizeof(errMsg)));
                        } else {
                            if (fread(input[input_idx].data, input[input_idx].size, 1, fp) != 1) {
                                char errMsg[80] = { '\0' };
                                fprintf(stderr, "fread: %s\n", strerror_r(ferror(fp), errMsg, sizeof(errMsg)));
                                _exit(EIO);
                            }
                        }

                        if (fclose(fp) < 0) {
                            char errMsg[80] = { '\0' };
                            fprintf(stderr, "Unable to close the file pointer: %s\n", strerror_r(errno, errMsg, sizeof(errMsg)));
                        }
                    }
                }
                input_idx++;
                break;
            case 't':
                if (nullptr == input) {
                    fprintf(stderr, "Invalid input(NULL)");
                    break;
                }
                if (strcmp(optarg, "int8") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_INT8;
                } else if (strcmp(optarg, "int16") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_INT16;
                } else if (strcmp(optarg, "int32") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_INT32;
                } else if (strcmp(optarg, "uint8") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_UINT8;
                } else if (strcmp(optarg, "uint16") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_UINT16;
                } else if (strcmp(optarg, "uint32") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_UINT32;
                } else if (strcmp(optarg, "float16") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_FLOAT16;
                } else if (strcmp(optarg, "float32") == 0) {
                    input[input_idx].type = BEYOND_TENSOR_TYPE_FLOAT32;
                }
                break;
            default:
                break;
            }
        }
    }

    if (model == nullptr || input == nullptr || input_idx < input_size) {
        fprintf(stderr, "Invalid arguments (%d,%d)\n", input_idx, input_size);
        help();
        _exit(1);
    }

    optind = 0;
    opterr = 0;
    Runtime *runtime = reinterpret_cast<Runtime *>(_main(argc, argv));
    if (runtime == nullptr) {
        _exit(EFAULT);
    }

    runtime->LoadModel(model);
    runtime->Prepare();

    runtime->Invoke(input, input_size);
    int output_size;
    runtime->GetOutput(output, output_size);

    //
    // Runtime has no main loop.
    // It runs once
    //
    // TODO: manipulate the output tensor
    //
    runtime->FreeTensor(output, output_size);

    runtime->Destroy();

    free(type);
    free(model);
    for (int i = 0; i < input_size; i++) {
        free(input[i].data);
    }
    free(input);

    if (argc > 0) {
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
            argv[i] = nullptr;
        }
        free(argv);
        argv = nullptr;
        argc = 0;
    }

    _exit(0);
}
} // extern "C"
#endif // NDEBUG
