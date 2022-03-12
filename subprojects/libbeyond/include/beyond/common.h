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

#ifndef __BEYOND_COMMON_H__
#define __BEYOND_COMMON_H__

#define UNUSED __attribute__((unused))
#define API __attribute__((visibility("default")))

// 0xAaBbCcDd
// Aa: Component - 00: Base, 01: INFERENCE, 02: PEER, 04: DISCOVERY, 08: AUTHENTICATOR
// Bb: Reserved (Customized event type - for 2nd/3rd parties)
// Cc: Event type of each component
// Dd: Event type of base component (primitive events such as read/write and error)
enum beyond_event_type {
    BEYOND_EVENT_TYPE_NONE = 0x00000000,
    BEYOND_EVENT_TYPE_READ = 0x00000010,
    BEYOND_EVENT_TYPE_WRITE = 0x00000020,
    BEYOND_EVENT_TYPE_ERROR = 0x00000040,
    BEYOND_EVENT_TYPE_EVENT_MASK = 0x000000FF,

    BEYOND_EVENT_TYPE_INFERENCE_UNKNOWN = 0x01000000,
    BEYOND_EVENT_TYPE_INFERENCE_READY = 0x01000100,
    BEYOND_EVENT_TYPE_INFERENCE_SUCCESS = 0x01000200,
    BEYOND_EVENT_TYPE_INFERENCE_STOPPED = 0x01000400,  // All inference process is stopped
    BEYOND_EVENT_TYPE_INFERENCE_CANCELED = 0x01000800, // A single inference is canceled
    BEYOND_EVENT_TYPE_INFERENCE_ERROR = 0x01001000,
    BEYOND_EVENT_TYPE_INFERENCE_MASK = 0x0100FF00,
    // ...

    BEYOND_EVENT_TYPE_PEER_UNKNOWN = 0x02000000,
    BEYOND_EVENT_TYPE_PEER_CONNECTED = 0x02000100,
    BEYOND_EVENT_TYPE_PEER_DISCONNECTED = 0x02000200,
    BEYOND_EVENT_TYPE_PEER_INFO_UPDATED = 0x02000400, // When a peer information is changed, this is going to be called.
    BEYOND_EVENT_TYPE_PEER_ERROR = 0x02000800,
    BEYOND_EVENT_TYPE_PEER_MASK = 0x0200FF00, // When a peer information is changed, this is going to be called.
    // ...

    BEYOND_EVENT_TYPE_DISCOVERY_UNKNOWN = 0x04000000,
    BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED = 0x04000100,
    BEYOND_EVENT_TYPE_DISCOVERY_REMOVED = 0x04000200,
    BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED = 0x04000400,
    // ...
    BEYOND_EVENT_TYPE_DISCOVERY_ERROR = 0x04000800,
    BEYOND_EVENT_TYPE_DISCOVERY_MASK = 0x0400FF00,

    BEYOND_EVENT_TYPE_AUTHENTICATOR_UNKNOWN = 0x08000000,
    BEYOND_EVENT_TYPE_AUTHENTICATOR_COMPLETED = 0x80000100, // When the encryption and decryption is completed
    // ...
    BEYOND_EVENT_TYPE_AUTHENTICATOR_ERROR = 0x08000200,
    BEYOND_EVENT_TYPE_AUTHENTICATOR_MASK = 0x0800FF00,
};

enum beyond_handler_return {
    BEYOND_HANDLER_RETURN_CANCEL = 0x00,
    BEYOND_HANDLER_RETURN_RENEW = 0x01,
};

struct beyond_argument {
    // argv[0] is a module name. e.g) runtime_tflite, peer_aws,
    // the name can be anything given by providers. each runtime should be implemented as a loadable module (.so)
    // and the so filename should be "libbeyond-${MODULE_NAME}.so" e.g) "libbeyond-runtime_tflite"
    int argc;
    char **argv;
};

#define BEYOND_CONFIG_TYPE_AUTHENTICATOR ((char)(0x0e))

#define BEYOND_CONFIG_TYPE_JSON ((char)(0x0d))
#define BEYOND_CONFIG_TYPE_INPUT 'i'

struct beyond_config {
    char type;
    void *object;
};

enum beyond_tensor_type {
    BEYOND_TENSOR_TYPE_INT8 = 0x00,
    BEYOND_TENSOR_TYPE_INT16 = 0x01,
    BEYOND_TENSOR_TYPE_INT32 = 0x02,
    BEYOND_TENSOR_TYPE_INT64 = 0x04,
    BEYOND_TENSOR_TYPE_UINT8 = 0x10,
    BEYOND_TENSOR_TYPE_UINT16 = 0x11,
    BEYOND_TENSOR_TYPE_UINT32 = 0x12,
    BEYOND_TENSOR_TYPE_UINT64 = 0x14,
    BEYOND_TENSOR_TYPE_FLOAT16 = 0x21,
    BEYOND_TENSOR_TYPE_FLOAT32 = 0x22,

    // TODO: Add more tensor type

    BEYOND_TENSOR_TYPE_UNSUPPORTED = 0xff,
};

struct beyond_tensor {
    enum beyond_tensor_type type;
    int size;
    void *data;
};

struct beyond_tensor_info {
    enum beyond_tensor_type type;
    int size; // size of tensor data in bytes
    char *name;
    struct dimensions {
        int size;    // count of dimension e.g N*W*H*C == 4
        int data[1]; // for implementing the zero-sized array within the ISO standard.
    } * dims;
};

enum beyond_authenticator_key_id {
    BEYOND_AUTHENTICATOR_KEY_ID_PRIVATE_KEY = 0, // Asymmetric key pair
    BEYOND_AUTHENTICATOR_KEY_ID_PUBLIC_KEY = 1,
    BEYOND_AUTHENTICATOR_KEY_ID_CERTIFICATE = 2,

    BEYOND_AUTHENTICATOR_KEY_ID_SECRET_KEY = 3, // Symmetric key

    // TODO: Add more key id
};

enum beyond_input_type {
    BEYOND_INPUT_TYPE_VIDEO = 0x01,
    BEYOND_INPUT_TYPE_IMAGE = 0x02,
    BEYOND_INPUT_TYPE_UNSUPPORTED = 0xff,

    // TODO: The following input types are not supported yet
    // BEYOND_INPUT_TYPE_TENSOR = 0x00,
    // BEYOND_INPUT_TYPE_AUDIO = 0x03,
    // BEYOND_INPUT_TYPE_TEXT = 0x04,
    // BEYOND_INPUT_TYPE_OCTAT = 0x05,
};

// format
// // https://gstreamer.freedesktop.org/documentation/jpeg/jpegenc.html?gi-language=c#sink
// { "I420", "YV12", "YUY2", "UYVY", "Y41B", "Y42B", "YVYU", "Y444", "NV21", "NV12", "RGB", "BGR", "RGBx", "xRGB", "BGRx", "xBGR", "GRAY8" };
// convert_format
// https://gstreamer.freedesktop.org/documentation/jpeg/jpegdec.html?gi-language=c#src
// https://github.com/nnstreamer/nnstreamer/blob/main/gst/nnstreamer/tensor_converter/converter-media-info-video.h
// { "RGB", "BGR", "RGBx", "xRGB", "BGRx", "xBGR", "GRAY8" }
struct beyond_input_image_config {
    const char *format;
    int width;
    int height;
    const char *convert_format;
    int convert_width;
    int convert_height;
    const char *transform_mode;
    const char *transform_option;
};

struct beyond_input_video_config {
    struct beyond_input_image_config frame;
    int fps;      // frames per second
    int duration; // duration in seconds
};

struct beyond_input_config {
    enum beyond_input_type input_type;
    union config {
        struct beyond_input_image_config image;
        struct beyond_input_video_config video;
        // TODO:
        // struct beyond_input_audio_config audio;
        // struct beyond_input_text_config text;
        // ...
    } config;
};

// From the inference.h
// In order to do not include public C api from the internal implementation,
// the following declarations and definitions are moved into the common.h

// No peers. beyond_inference_add_peer() returns an error always.
#define BEYOND_INFERENCE_MODE_LOCAL "local"

// Local inference is not going to be invoked.
// Inference is going to be done by a remote machine.
// The remote machine must be able to access model files
#define BEYOND_INFERENCE_MODE_REMOTE "remote"

// TODO:
// Pre-inference is going to be invoked on a local and
// the post-inference is going to be invoked on a remote machine.
// The remote machine must be able to access necessary model files
// #define BEYOND_INFERENCE_MODE_EDGE "edge"

// TODO:
// Inference requests can be forwarded between peers, so the inference request can be finished by
// not the peer who gets the request at the first time.
// Also the inference request can be spreaded to multiple remote machines simultaneously,
// and then BeyonD will take the fastest one.
// The remote machine must be able to access necessary model files
// #define BEYOND_INFERENCE_MODE_DISTRIBUTE "distribute"

// remove option delimeters from the given option string
#define BEYOND_GET_OPTION_NAME(option) (((char *)option) + 2)

// Inference options
// Automatically split the provided models by inference circumstances.
#define BEYOND_INFERENCE_OPTION_AUTO_SPLIT "--split"

// Select the inference framework (runtime default: tensorflow-lite)
#define BEYOND_INFERENCE_OPTION_FRAMEWORK "--framework"

// Select the inference framework (runtime) acceleration (default: gpu)
#define BEYOND_INFERENCE_OPTION_FRAMEWORK_ACCEL "--acceleration"


// runtime devices
// The runtime option can be defined by runtime provider.
// Therefore, You should check the runtime manual first.
// For an example, if there is a runtime_tflite, and it provides extra options such as "--edgetpu",
// in that case, you can use it as an option.
#define BEYOND_INFERENCE_RUNTIME_DEVICE_CPU "--cpu"
#define BEYOND_INFERENCE_RUNTIME_DEVICE_GPU "--gpu"
#define BEYOND_INFERENCE_RUNTIME_DEVICE_NPU "--npu"
#define BEYOND_INFERENCE_RUNTIME_DEVICE_DSP "--dsp"

struct beyond_event_info {
    int type; // OR'd type of beyond_event_type in the same category

    void *data;
};

struct beyond_peer_info_device {
    char *name;
};

struct beyond_peer_info_runtime {
    char *name;
    int count_of_devices;
    struct beyond_peer_info_device *devices;
};

#define BEYOND_UUID_LEN 37 // null byte included

struct beyond_peer_info {
    char *name;
    char *host; // this value can be changed when the peer module can find and switch the remote device by its own algorithm.
                // e.g) libbeyond-peer_vine
    unsigned short port[2];
    unsigned long long free_memory;
    unsigned long long free_storage;
    char uuid[BEYOND_UUID_LEN];

    int count_of_runtimes; // count_of_runtimes = 2;

    struct beyond_peer_info_runtime *runtimes;
    // {
    //     {
    //         .name = "tensorflow-lite",
    //         .count_of_devices = 2,
    //         .devices = {
    //             {
    //                 .name = BEYOND_INFERENCE_RUNTIME_DEVICE_CPU,
    //             },
    //             {
    //                 .name = BEYOND_INFERENCE_RUNTIME_DEVICE_GPU,
    //             },
    //         },
    //
    //     },
    //     {
    //         .name = "snpe",
    //         .count_of_devices = 1,
    //         .devices = {
    //             {
    //                 .name = BEYOND_INFERENCE_RUNTIME_DEVICE_DSP,
    //             },
    //         },
    //
    //     },
    // }
};

typedef void *beyond_inference_h;
typedef void *beyond_peer_h;
typedef void *beyond_runtime_h;
typedef void *beyond_discovery_h;
typedef void *beyond_authenticator_h;

// beyond_inference_h, beyond_peer_h, beyond_runtime_h, beyond_authenticator_h and beyond_discovery_h can be casted to the beyond_object_h
typedef void *beyond_object_h;
typedef enum beyond_handler_return (*beyond_event_handler_t)(beyond_object_h obj, int type, struct beyond_event_info *eventInfo, void *data);

typedef void *beyond_event_loop_h;
typedef void *beyond_event_handler_h;

#endif // __BEYOND_COMMON_H__
