/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <fstream>
#include <vector>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <beyond/beyond.h>
#include <beyond/plugin/peer_nn_plugin.h>
#include "task_runner.h"
#include "task_options.h"

static enum beyond_handler_return stdin_handler(beyond_object_h object, int type, void *data)
{
    if ((type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        printf("error\n");
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    char ch = '?';

    if (read(STDIN_FILENO, &ch, sizeof(ch)) < 0) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (ch == 'X' || ch == 'x') {
        beyond_session_h session_handle = (beyond_session_h)object;
        printf("session_h_ %p, dataptr %p\n", object, data);
        int ret = beyond_session_stop(session_handle);
        printf("beyond_session_stop returns %d\n", ret);
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

namespace beyond {
namespace evaluation {

// command line options
constexpr char RunAsOpt[] = "run";
constexpr char UseConfigOpt[] = "use_config";
constexpr char EdgeStorageOpt[] = "storage";
constexpr char EdgePortOpt[] = "port";
constexpr char DeviceModelFileOpt[] = "model_file";
constexpr char DeviceImagePathOpt[] = "image_path";
constexpr char DeviceOutputLabelsOpt[] = "output_labels";
constexpr char DeviceReqIPOpt[] = "edge_ip";
constexpr char DeviceReqPortOpt[] = "req_port";
constexpr char DeviceRepPortOpt[] = "rep_port";

// mobilenet v1
const int MOBILENETV1_HEIGHT = 224;
const int MOBILENETV1_WIDTH = 224;
const int OUTPUT_LABELS_NUM = 1001;

const int IMG_HEIGHT = 100;
const int IMG_WIDTH = 100;

enum class Mode {
    UNKNOWN,
    DEVICE,
    EDGE
};

class ImageClassification : public TaskRunner {
public:
    ImageClassification()
        : edge_ip_("127.0.0.1")
        , peer_name_(BEYOND_PLUGIN_PEER_NN_NAME)
        , req_port_(3000)
        , rep_port_(3001)
        , service_port_(3000)
        , use_config_(0)
        , mode_(Mode::UNKNOWN)
        , session_h_(nullptr)
        , evt_h_(nullptr)
        , inference_h_(nullptr)
        , peer_h_(nullptr)
    {
    }

    ~ImageClassification() override
    {
        if (peer_h_ != nullptr) {
            if (inference_h_ != nullptr) {
                beyond_inference_remove_peer(inference_h_, peer_h_);
            }
            beyond_peer_deactivate(peer_h_);
            beyond_peer_destroy(peer_h_);
        }

        if (inference_h_ != nullptr) {
            beyond_inference_destroy(inference_h_);
        }

        if (evt_h_ != nullptr) {
            beyond_session_remove_fd_handler(session_h_, evt_h_);
        }

        if (session_h_ != nullptr) {
            beyond_session_destroy(session_h_);
        }
    }

protected:
    std::vector<Option> GetOptions() final;

    int RunImpl() final;

private:
    bool ReadytoRun();
    bool CheckCmdline();
    bool CheckDevice();
    bool LoadLabels(std::vector<std::string> *labels);
    bool CheckEdge();
    bool Run();
    bool RunMobilenet();
    bool RunEdge();

    std::string target_mode_;
    std::string storage_;
    std::string model_file_path_;
    std::string input_image_path_;
    std::string output_label_path_;
    std::string edge_ip_;
    std::string peer_name_;
    unsigned short req_port_;
    unsigned short rep_port_;
    unsigned short service_port_;
    int use_config_;

    Mode mode_;

    beyond_session_h session_h_;
    beyond_event_handler_h evt_h_;
    beyond_inference_h inference_h_;
    beyond_peer_h peer_h_;
};

std::vector<Option> ImageClassification::GetOptions()
{
    std::vector<Option> opt_list = {
        Option::CreateOption(RunAsOpt, &target_mode_,
                             "Target to run as ('device' or 'edge')"),
        Option::CreateOption(UseConfigOpt, &use_config_,
                             "Peer configuration On:1, Off:0 (default: 0)"),
        Option::CreateOption(DeviceModelFileOpt, &model_file_path_,
                             "Path to tflite model file"),
        Option::CreateOption(DeviceImagePathOpt, &input_image_path_,
                             "Path to input image file that will be evaluated"),
        Option::CreateOption(DeviceOutputLabelsOpt, &output_label_path_,
                             "Path to labels file"),
        Option::CreateOption(DeviceReqIPOpt, &edge_ip_,
                             "Edge IP address (default: 127.0.0.1)"),
        Option::CreateOption(DeviceReqPortOpt, &req_port_,
                             "Request Port (default: 3000)"),
        Option::CreateOption(DeviceRepPortOpt, &rep_port_,
                             "Response Port (default: 3001)\n"),
        Option::CreateOption(EdgeStorageOpt, &storage_,
                             "Path to storage for edge service"),
        Option::CreateOption(EdgePortOpt, &service_port_,
                             "Edge service port (default: 3000)"),
    };
    return opt_list;
}

int ImageClassification::RunImpl()
{
    if (ReadytoRun() == false)
        return -1;

    if (Run() == false)
        return -1;

    return 1;
}

bool ImageClassification::ReadytoRun()
{
    bool result = CheckCmdline();

    if (result == true) {
        switch (mode_) {
        case Mode::DEVICE:
            result = CheckDevice();
            break;
        case Mode::EDGE:
            result = CheckEdge();
            break;
        case Mode::UNKNOWN:
        default:
            result = false;
            break;
        }
    }

    printf("%s for ready\n", result ? "Success":"Fail" );

    return result;
}

bool ImageClassification::CheckCmdline()
{
    bool result = true;

    if (target_mode_.compare("device") == 0) {
        mode_ = Mode::DEVICE;
    } else if (target_mode_.compare("edge") == 0) {
        mode_ = Mode::EDGE;
    } else {
        printf("Target is incorrect\n");
        result = false;
    }

    if (mode_ == Mode::DEVICE) {
        std::ifstream model_check(model_file_path_);
        if (model_check.good() == false) {
            printf("Incorrect Model file\n");
            result = false;
        }
        std::ifstream input_check(input_image_path_);
        if (input_check.good() == false) {
            printf("Incorrect Input Image file\n");
            result = false;
        }
        std::ifstream output_check(output_label_path_);
        if (output_check.good() == false) {
            printf("Incorrect Output Label file\n");
            result = false;
        }

        if (result == true) {
            printf("------------------------------\n");
            printf("Target : %s \n", target_mode_.c_str());
            printf("Model File : %s \n", model_file_path_.c_str());
            printf("Input Image File : %s \n", input_image_path_.c_str());
            printf("Output Label File : %s \n", output_label_path_.c_str());
            printf("Edge IP Address : %s \n", edge_ip_.c_str());
            printf("Request Port : %d \n", req_port_);
            printf("Response Port : %d \n", rep_port_);
            printf("Peer name : %s (configuration: %d) \n", peer_name_.c_str(), use_config_);
            printf("------------------------------\n");
        }
    } else if (mode_ == Mode::EDGE) {
        std::ifstream storage_check(storage_);
        if (storage_check.good() == false) {
            printf("Incorrect storage path %s\n", storage_.c_str());
            result = false;
        }

        if (result == true) {
            printf("------------------------------\n");
            printf("Target : %s \n", target_mode_.c_str());
            printf("Storage Path : %s \n", storage_.c_str());
            printf("Service Port : %d \n", service_port_);
            printf("Peer name : %s (configuration: %d) \n", peer_name_.c_str(), use_config_);
            printf("------------------------------\n");
        }
    }

    return result;
}

bool ImageClassification::CheckDevice()
{
    bool result = true;

    session_h_ = beyond_session_create(0, 0);
    if (session_h_ == nullptr) {
        printf("Failed to create session handle\n");
        return false;
    }

    evt_h_ = beyond_session_add_fd_handler(session_h_, STDIN_FILENO, BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR, stdin_handler, NULL, session_h_);
    if (evt_h_ == nullptr) {
        printf("Failed to create evt handle\n");
        beyond_session_destroy(session_h_);
        session_h_ = nullptr;
        return false;
    }

    const char *default_arg = BEYOND_INFERENCE_MODE_REMOTE;
    beyond_argument option = {
        .argc = 1,
        .argv = (char **)&default_arg,
    };
    inference_h_ = beyond_inference_create(session_h_, &option);
    if (inference_h_ == nullptr) {
        printf("Failed to create inferehnce handle\n");
        beyond_session_remove_fd_handler(session_h_, evt_h_);
        evt_h_ = nullptr;
        beyond_session_destroy(session_h_);
        session_h_ = nullptr;
        return false;
    }

    const int args_cnt = 5;
    const char *module_name = peer_name_.c_str();
    const char *peer_device_argv[args_cnt] = {
        const_cast<char *>(module_name),
        BEYOND_INFERENCE_OPTION_FRAMEWORK,
        "tensorflow-lite",
        BEYOND_INFERENCE_OPTION_FRAMEWORK_ACCEL,
        "cpu"
    };
    beyond_argument peer_option = {
        .argc = args_cnt,
        .argv = const_cast<char **>(peer_device_argv),
    };
    peer_h_ = beyond_peer_create(session_h_, &peer_option);
    if (peer_h_ == nullptr) {
        printf("Failed to create peer handle\n");
        return false;
    }

    if (use_config_ == 1) {
        struct beyond_input_image_config image_config = {
            .format = "BGR",
            .width = IMG_WIDTH,
            .height = IMG_HEIGHT,
            .convert_format = "RGB",
            .convert_width = MOBILENETV1_WIDTH,
            .convert_height = MOBILENETV1_HEIGHT,
            .transform_mode = "typecast",
            .transform_option = "uint8"
        };
        struct beyond_input_config input_config;
        input_config.input_type = BEYOND_INPUT_TYPE_IMAGE;
        input_config.config.image = image_config;
        struct beyond_config config;
        config.type = BEYOND_CONFIG_TYPE_INPUT;
        config.object = &input_config;
        if (beyond_peer_configure(peer_h_, &config) < 0) {
            printf("Failed to config input\n");
            return false;
        }
        /* if there were no input configuration, the input format should fit on model's */
    }

    int ret = beyond_peer_set_event_callback(
        peer_h_, [](beyond_peer_h peer, struct beyond_event_info *event, void *data) -> int {
            printf("Peer event callback for 0x%.8X (%p)\n", event->type, event->data);
            return BEYOND_HANDLER_RETURN_RENEW;
        },
        nullptr);
    if (ret < 0) {
        printf("peer_set_event_callback : %d", ret);
    }

    const char *host = edge_ip_.c_str();
    beyond_peer_info info = {
        .name = const_cast<char *>("name"),
        .host = const_cast<char *>(host),
        .port = { req_port_, rep_port_ },
        .free_memory = 0llu,
        .free_storage = 0llu,
    };
    if (beyond_peer_set_info(peer_h_, &info) < 0) {
        printf("Failed to set peer info\n");
        return false;
    }

    // if (beyond_peer_activate(peer_h_) < 0) {
    //     printf("Failed to activate peer\n");
    //     return false;
    // }

    if (beyond_inference_add_peer(inference_h_, peer_h_) < 0) {
        printf("Failed inference handle to add peer\n");
        return false;
    }

    const char *model = model_file_path_.c_str();
    if (beyond_inference_load_model(inference_h_, &model, 1) < 0) {
        printf("Failed to load model\n");
        return false;
    }

    if (beyond_inference_prepare(inference_h_) < 0) {
        printf("Failed to prepare\n");
        return false;
    }

    return result;
}

bool ImageClassification::LoadLabels(std::vector<std::string> *labels)
{
    if (labels == nullptr) {
        return false;
    }
    std::ifstream stream(output_label_path_.c_str());
    if (!stream) {
        return false;
    }
    std::string line;
    while (std::getline(stream, line)) {
        labels->push_back(line);
    }
    return true;
}

bool ImageClassification::CheckEdge()
{
    bool result = true;

    session_h_ = beyond_session_create(0, 0);
    if (session_h_ == nullptr) {
        printf("Failed to create session handle\n");
        return false;
    }

    evt_h_ = beyond_session_add_fd_handler(session_h_, STDIN_FILENO, BEYOND_EVENT_TYPE_READ | BEYOND_EVENT_TYPE_ERROR, stdin_handler, NULL, session_h_);
    if (evt_h_ == nullptr) {
        printf("Failed to create evt handle\n");
        beyond_session_destroy(session_h_);
        session_h_ = nullptr;
        return false;
    }

    const char *storage = storage_.c_str();
    const char *peer_edge_argv[4] = {
        BEYOND_PLUGIN_PEER_NN_NAME,
        BEYOND_PLUGIN_PEER_NN_ARGUMENT_SERVER,
        "--path",
        const_cast<char *>(storage)
    };
    beyond_argument peer_option = {
        .argc = 4,
        .argv = const_cast<char **>(peer_edge_argv),
    };
    peer_h_ = beyond_peer_create(session_h_, &peer_option);
    if (peer_h_ == nullptr) {
        printf("Failed to create peer handle\n");
        return false;
    }

    if (use_config_ == 1) {
        // NO implementation yet for service config
        struct beyond_config config = { 's', nullptr };
        if (beyond_peer_configure(peer_h_, &config) < 0) {
            printf("Failed to config input\n");
            result = false;
        }
    }

    struct beyond_peer_info info = {
        .name = const_cast<char *>("name"),
        .host = const_cast<char *>("0.0.0.0"),
        .port = { service_port_ },
        .free_memory = 0llu,
        .free_storage = 0llu,
    };
    if (beyond_peer_set_info(peer_h_, &info) < 0) {
        printf("Failed to set peer info\n");
        return false;
    }

    int ret = beyond_peer_set_event_callback(
        peer_h_, [](beyond_peer_h peer, struct beyond_event_info *event, void *data) -> int {
            printf("Peer event callback for 0x%.8X (%p)\n", event->type, event->data);
            return BEYOND_HANDLER_RETURN_RENEW;
        },
        nullptr);
    if (ret < 0) {
        printf("peer_set_event_callback : %d", ret);
    }

    // create connection & prepare communication channel
    ret = beyond_peer_activate(peer_h_);
    if (ret < 0) {
        printf("beyond_peer_activate : %d\n", ret);
        result = false;
    }

    return result;
}

bool ImageClassification::Run()
{
    bool result = true;

    switch (mode_) {
    case Mode::DEVICE:
        result = RunMobilenet();
        break;
    case Mode::EDGE:
        result = RunEdge();
        break;
    case Mode::UNKNOWN:
    default:
        result = false;
        break;
    }

    return result;
}

bool ImageClassification::RunMobilenet()
{
    bool result = true;

    /* in sequence :
       1 load image ->
       2 configure tensor info and create tensor handle (input) ->
       3 invoke ->
       4 print label
     */

    cv::Mat inputImage = cv::imread(input_image_path_, cv::IMREAD_COLOR);
    if (inputImage.empty()) {
        printf("Failed to read image");
        return false;
    }
    cv::Mat resizedImage;
    if (use_config_ == 1) {
        cv::resize(inputImage, resizedImage,
                   cv::Size(IMG_WIDTH, IMG_HEIGHT));
    } else {
        cv::resize(inputImage, resizedImage,
                   cv::Size(MOBILENETV1_WIDTH, MOBILENETV1_HEIGHT));
        cv::cvtColor(resizedImage, resizedImage, cv::COLOR_BGR2RGB);
    }

    int size_in_byte = resizedImage.size().width * resizedImage.size().height * resizedImage.channels() * sizeof(unsigned char);

    // configure tensor_info, tensor (by user for a tempoerary) ?
    // input tensor
    const struct beyond_tensor_info *input_info;
    int num_inputs;
    if (beyond_inference_get_input_tensor_info(inference_h_, &input_info, &num_inputs)) {
        printf("beyond_inference_get_input_tensor_info\n");
        return false;
    }

    auto in_tensor_h = beyond_inference_allocate_tensor(inference_h_, input_info, num_inputs);
    if (in_tensor_h == nullptr) {
        printf("Failed beyond_inference_allocate_tensor for input\n");
        return false;
    }

    beyond_tensor *beyond_tensors = BEYOND_TENSOR(in_tensor_h);
    auto &in = beyond_tensors[0];
    in.type = BEYOND_TENSOR_TYPE_UINT8;
    in.size = size_in_byte;
    memcpy(in.data, resizedImage.data, size_in_byte);

    // invoke
    if (beyond_inference_do(inference_h_, in_tensor_h, nullptr) < 0) {
        printf("Failed beyond_inference_do\n");
        result = false;
    }
    beyond_inference_unref_tensor(in_tensor_h);

    // print label if invoke success
    if (result == true) {
        beyond_tensor_h out_tensor_h;
        int count; // dummy: Could not get count in this api beyond_inference_get_output
        if (beyond_inference_get_output(inference_h_, &out_tensor_h, &count) < 0) {
            printf("Failed beyond_inference_get_output\n");
            return false;
        }

        beyond_tensor *tensors = BEYOND_TENSOR(out_tensor_h);
        if ((tensors[0].type != BEYOND_TENSOR_TYPE_UINT8) ||
            (tensors[0].size != OUTPUT_LABELS_NUM)) {
            printf("incorrect output( type : %d, size : %d)\n", tensors[0].type, tensors[0].size);
            printf("expected result( type : %d, size : %d)\n", BEYOND_TENSOR_TYPE_UINT8, OUTPUT_LABELS_NUM);
        }

        uint8_t max_score = 0;
        uint8_t scores[OUTPUT_LABELS_NUM];
        int index = -1;

        memcpy(&scores, tensors[0].data, tensors[0].size);

        for (int i = 0; i < tensors[0].size; i++) {
            if (scores[i] > 0 && scores[i] > max_score) {
                index = i;
                max_score = scores[i];
            }
        }

        if (index == -1) {
            printf("Failed to get label from output\n");
        }

        std::vector<std::string> image_labels;
        if (LoadLabels(&image_labels) == false) {
            printf("Failed to load labels\n");
        }

        printf("\n%s\n", image_labels[index].c_str());

        beyond_inference_unref_tensor(out_tensor_h);
    }

    beyond_session_run(session_h_, 10, -1, -1);

    return result;
}

bool ImageClassification::RunEdge()
{
    bool result = true;
    int ret;
    ret = beyond_session_run(session_h_, 10, -1, -1);
    if (ret < 0) {
        result = false;
        printf("Failed beyond_session_run\n");
    }
    return result;
}

std::unique_ptr<TaskRunner> CreateTaskRunner()
{
    return std::unique_ptr<TaskRunner>(new ImageClassification());
}

} // namespace evaluation
} // namespace beyond
