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

#include <glib.h>
#include <string>
#include <fstream>
#include <vector>

#include <opencv2/opencv.hpp>
#include <beyond/beyond.h>
#include <beyond/plugin/peer_nn_plugin.h>
#include <beyond/plugin/discovery_dns_sd_plugin.h>
#include "task_runner.h"
#include "task_options.h"

#define DEBUG false

namespace beyond {
namespace evaluation {
// command line options
constexpr char RunAsOpt[] = "run";
constexpr char UseConfigOpt[] = "use_config";

// for edge
constexpr char EdgeNameOpt[]= "service_name";
constexpr char EdgeStorageOpt[] = "storage";
constexpr char EdgePortOpt[] = "port";

// for device
constexpr char DeviceModelFileOpt[] = "model_file";
constexpr char DeviceImagePathOpt[] = "image_path";
constexpr char DeviceOutputLabelsOpt[] = "output_labels";
constexpr char DeviceReqIPOpt[] = "edge_ip";
constexpr char DeviceReqPortOpt[] = "req_port";
constexpr char DeviceRepPortOpt[] = "rep_port";
constexpr char DeviceInvocationOpt[] = "invoke";
constexpr char DeviceInputFPSOpt[] = "input_fps";
constexpr char DeviceRepeatOpt[] = "repeat";

// mobilenet v1
const int MOBILENETV1_HEIGHT = 224;
const int MOBILENETV1_WIDTH = 224;
const int OUTPUT_LABELS_NUM = 1001;

const int IMG_HEIGHT = 100;
const int IMG_WIDTH = 100;

enum class TargetMode {
    UNKNOWN,
    DEVICE,
    EDGE,
    EDGELIST,
};

enum class InvokeMode {
    SYNC,
    ASYNC
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
        , input_fps_(10)
        , repeat_(1)
        , num_invoked_(0)
        , discovered_(0)
        , main_loop_(g_main_loop_new(nullptr, FALSE))
        , target_(TargetMode::UNKNOWN)
        , invoke_(InvokeMode::SYNC)
        , inference_h_(nullptr)
        , peer_h_(nullptr)
        , discovery_h_(nullptr)
    {
    }

    ~ImageClassification() override
    {
        if (discovery_h_ != nullptr) {
            beyond_discovery_deactivate(discovery_h_);
            beyond_discovery_destroy(discovery_h_);
        }

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

        g_main_loop_unref(main_loop_);
    }

protected:
    std::vector<Option> GetOptions() final;

    int RunImpl() final;

private:
    bool ReadytoRun();
    bool CheckCmdline();
    bool CheckDevice_InferencePeer();
    bool CheckDevice_Discovery();
    bool LoadLabels();
    bool CheckEdge_Peer();
    bool CheckEdge_Discovery();
    bool Run();
    bool RunDevice();
    bool RunMobilenet();
    bool PrintResult();

    static int DeviceDiscoveryCallback(beyond_discovery_h handle, beyond_event_info *event, void *data);
    static int EdgeDiscoveryCallback(beyond_discovery_h handle, beyond_event_info *event, void *data);

    std::string run_as_;
    std::string service_name_;
    std::string storage_;
    std::string model_file_path_;
    std::string input_image_path_;
    std::string output_label_path_;
    std::string edge_ip_;
    std::string peer_name_;
    std::string invocation_; /* invocation mode in sync or async */
    unsigned short req_port_;
    unsigned short rep_port_;
    unsigned short service_port_;
    int use_config_;
    int input_fps_;
    int repeat_;
    int num_invoked_;
    int discovered_;

    GMainLoop *main_loop_;
    TargetMode target_;
    InvokeMode invoke_;
    std::vector<std::string> image_labels_;

    beyond_inference_h inference_h_;
    beyond_peer_h peer_h_;
    beyond_discovery_h discovery_h_;
};

std::vector<Option> ImageClassification::GetOptions()
{
    std::vector<Option> opt_list = {
        Option::CreateOption(RunAsOpt, &run_as_,
                             "Target to run as ('device', 'edge' or 'edgelist')"),
        Option::CreateOption(UseConfigOpt, &use_config_,
                             "Peer configuration On:1, Off:0 (default: 0)\n"),
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
                             "Response Port (default: 3001)"),
        Option::CreateOption(DeviceInvocationOpt, &invocation_,
                             "Execute inference invocation in (`sync` or `async` default: sync)"),
        Option::CreateOption(DeviceInputFPSOpt, &input_fps_,
                             "For variable frame rate inputs with a given whole number (default: 10 fps | range of 1 to 60)"),
        Option::CreateOption(DeviceRepeatOpt, &repeat_,
                             "repeatedly runs inference with a given whole number (default: 1)\n"),
        Option::CreateOption(EdgeNameOpt, &service_name_,
                             "Service name to be registered"),
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

    g_main_loop_run(main_loop_);
    return 1;
}

bool ImageClassification::ReadytoRun()
{
    bool result = CheckCmdline();

    if (result == true) {
        switch (target_) {
        case TargetMode::DEVICE:
            result = CheckDevice_InferencePeer();
            break;
        case TargetMode::EDGE:
            result = (CheckEdge_Peer() & CheckEdge_Discovery());
            break;
        case TargetMode::EDGELIST:
            result = CheckDevice_Discovery();
            break;
        case TargetMode::UNKNOWN:
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

    if (run_as_.compare("device") == 0) {
        target_ = TargetMode::DEVICE;
    } else if (run_as_.compare("edge") == 0) {
        target_ = TargetMode::EDGE;
    } else if (run_as_.compare("edgelist") == 0) {
        target_ = TargetMode::EDGELIST;
    } else {
        printf("Target is incorrect\n");
        result = false;
    }

    if (target_ == TargetMode::DEVICE) {
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

        if (LoadLabels() == false) {
            printf("Failed to load labels\n");
        }

        if (invocation_.compare("async") == 0) {
            invoke_ = InvokeMode::ASYNC;
        } else {
            invoke_ = InvokeMode::SYNC;
        }

        if ((input_fps_ < 1) || (input_fps_ > 60)) {
            printf("Out of range(1 ~ 60) of input fps : %d is set to default(10)\n", input_fps_);
            input_fps_ = 10;
        }

        if (result == true) {
            printf("------------------------------\n");
            printf("Target : %s \n", run_as_.c_str());
            printf("Model File : %s \n", model_file_path_.c_str());
            printf("Input Image File : %s \n", input_image_path_.c_str());
            printf("Output Label File : %s \n", output_label_path_.c_str());
            printf("Edge IP Address : %s \n", edge_ip_.c_str());
            printf("Request Port : %d \n", req_port_);
            printf("Response Port : %d \n", rep_port_);
            printf("Peer name : %s (configuration: %d) \n", peer_name_.c_str(), use_config_);
            printf("Input fps : %d (%d ms)\n", input_fps_, 1000 / input_fps_);
            printf("Invoke : %s \n", ((invoke_ == InvokeMode::SYNC) ? "sync" : "async"));
            printf("Num of runs : %d \n", repeat_);
            printf("------------------------------\n");
        }
    } else if (target_ == TargetMode::EDGE) {
        std::ifstream storage_check(storage_);
        if (storage_check.good() == false) {
            printf("Incorrect storage path %s\n", storage_.c_str());
            result = false;
        }

        if (result == true) {
            printf("------------------------------\n");
            printf("Target : %s \n", run_as_.c_str());
            printf("Service Name : %s \n", service_name_.c_str());
            printf("Storage Path : %s \n", storage_.c_str());
            printf("Service Port : %d \n", service_port_);
            printf("Peer name : %s (configuration: %d) \n", peer_name_.c_str(), use_config_);
            printf("------------------------------\n");
        }
    }

    return result;
}

bool ImageClassification::CheckDevice_InferencePeer()
{
    const char *default_arg = BEYOND_INFERENCE_MODE_REMOTE;
    beyond_argument option = {
        .argc = 1,
        .argv = (char **)&default_arg,
    };
    inference_h_ = beyond_inference_create(&option);
    if (inference_h_ == nullptr) {
        printf("Failed to create inferehnce handle\n");
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
    peer_h_ = beyond_peer_create(&peer_option);
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

    if (invoke_ == InvokeMode::ASYNC) {
        int ret = beyond_inference_set_output_callback(
            inference_h_, [](beyond_inference_h handle, struct beyond_event_info *event, void *data) -> void {
                if ((event->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
                    return;
                }

                if ((event->type & BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) != BEYOND_EVENT_TYPE_INFERENCE_SUCCESS) {
                    return;
                }

                if (data == nullptr)
                    return;
                auto _this = static_cast<ImageClassification *>(data);

                _this->PrintResult();
                return;
            },
            static_cast<void *>(this));
        if (ret < 0) {
            printf("Failed peer_set_event_callback : %d\n", ret);
        }
    }

    if (beyond_inference_prepare(inference_h_) < 0) {
        printf("Failed to prepare\n");
        return false;
    }

    return true;
}

bool ImageClassification::CheckDevice_Discovery()
{
    int ret;
    const char *s_device_argv = BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME;
    beyond_argument discovery_option = {
        .argc = 1,
        .argv = (char **)&s_device_argv,
    };

    discovery_h_ = beyond_discovery_create(&discovery_option);

    ret = beyond_discovery_set_event_callback(discovery_h_, DeviceDiscoveryCallback, static_cast<void *>(this));
    if (ret < 0) {
        printf("Failed beyond_discovery_set_event_callback : %d\n", ret);
        return false;
    }

    ret = beyond_discovery_activate(discovery_h_);
    if (ret < 0) {
        printf("Failed beyond_discovery_activate : %d\n", ret);
        return false;
    }

    return true;
}

bool ImageClassification::LoadLabels()
{
    std::ifstream stream(output_label_path_.c_str());
    if (!stream) {
        return false;
    }

    image_labels_.clear();

    std::string line;
    while (std::getline(stream, line)) {
        image_labels_.push_back(line);
    }
    return true;
}

bool ImageClassification::CheckEdge_Peer()
{
    int ret;
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

    peer_h_ = beyond_peer_create(&peer_option);
    if (peer_h_ == nullptr) {
        printf("Failed to create peer handle\n");
        return false;
    }

    if (use_config_ == 1) {
        // NO implementation yet for service config
        struct beyond_config config = { 's', nullptr };
        ret = beyond_peer_configure(peer_h_, &config);
        if (ret < 0) {
            printf("Failed to config input : %d\n", ret);
            return false;
        }
    }

    struct beyond_peer_info info = {
        .name = const_cast<char *>("name"),
        .host = const_cast<char *>("0.0.0.0"),
        .port = { service_port_ },
        .free_memory = 0llu,
        .free_storage = 0llu,
    };
    ret = beyond_peer_set_info(peer_h_, &info);
    if (ret < 0) {
        printf("Failed to set peer info : %d\n", ret);
        return false;
    }

    ret = beyond_peer_set_event_callback(
        peer_h_, [](beyond_peer_h peer, struct beyond_event_info *event, void *data) -> int {
            if (DEBUG == true) {
                printf("Peer event callback for 0x%.8X (%p)\n", event->type, event->data);
            }
            return BEYOND_HANDLER_RETURN_RENEW;
        },
        nullptr);
    if (ret < 0) {
        printf("Failed peer_set_event_callback : %d\n", ret);
        return false;
    }

    // create connection & prepare communication channel
    ret = beyond_peer_activate(peer_h_);
    if (ret < 0) {
        printf("Failed beyond_peer_activate : %d\n", ret);
        return false;
    }

    return true;
}

bool ImageClassification::CheckEdge_Discovery()
{
    int ret;

	const char *service = service_name_.c_str();
	const char *s_edge_argv[] = {
		BEYOND_PLUGIN_DISCOVERY_DNS_SD_NAME,
		BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_SERVER,
		BEYOND_PLUGIN_DISCOVERY_DNS_SD_ARGUMENT_NAME,
		const_cast<char *>(service)
	};
	beyond_argument discovery_option = {
		.argc = sizeof(s_edge_argv) / sizeof(char *),
		.argv = const_cast<char **>(s_edge_argv)
	};

    discovery_h_ = beyond_discovery_create(&discovery_option);
    if (discovery_h_ == nullptr) {
        printf("Failed beyond_discovery_create\n");
        return false;
    }

    // TODO. key,val should be passed by cmdline
    ret = beyond_discovery_set_item(discovery_h_, "uuid", "1234", 4);
    if (ret < 0) {
        printf("Failed beyond_discovery_set_item : %d\n", ret);
        return false;
    }

    ret = beyond_discovery_set_event_callback(discovery_h_, EdgeDiscoveryCallback, nullptr);
    if (ret < 0) {
        printf("Failed beyond_discovery_set_event_callback : %d\n", ret);
        return false;
    }

    ret = beyond_discovery_activate(discovery_h_);
    if (ret < 0) {
        printf("Failed beyond_discovery_activate : %d\n", ret);
        return false;
    }

    return true;
}

bool ImageClassification::Run()
{
    bool result = true;

    switch (target_) {
    case TargetMode::DEVICE:
        result = RunDevice();
        break;
    case TargetMode::EDGE:
    case TargetMode::EDGELIST:
        break;
    case TargetMode::UNKNOWN:
    default:
        result = false;
        break;
    }

    return result;
}

bool ImageClassification::RunDevice()
{
    bool result = true;
    guint interval = 1000 / input_fps_;
    printf("run interval : %d ms\n", interval);
    g_timeout_add(
        interval, [](gpointer data) -> gboolean {
            auto _this = static_cast<ImageClassification *>(data);
            if (_this->RunMobilenet() == false) {
                g_main_loop_quit(_this->main_loop_);
            }
            return G_SOURCE_CONTINUE; }, static_cast<void *>(this));

    return result;
}

bool ImageClassification::RunMobilenet()
{
    bool result = true;

    /* in sequence :
           1 load image ->
           2 configure tensor info and create tensor handle (input) ->
           3 invoke ->
           4 (if invoke in sync) -> print label
    */

    if (repeat_ <= num_invoked_) {
        return false;
    }

    cv::Mat inputImage = cv::imread(input_image_path_, cv::IMREAD_COLOR);
    if (inputImage.empty()) {
        printf("Failed to read image\n");
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
    num_invoked_++;
    if (beyond_inference_do(inference_h_, in_tensor_h, nullptr) < 0) {
        printf("Failed beyond_inference_do\n");
        result = false;
    }
    beyond_inference_unref_tensor(in_tensor_h);

    if ((result == true) &&
        (invoke_ == InvokeMode::SYNC)) {
        result = PrintResult();
    }

    return result;
}

bool ImageClassification::PrintResult()
{
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
    int loaded = image_labels_.size();

    memcpy(&scores, tensors[0].data, tensors[0].size);

    for (int i = 0; i < tensors[0].size; i++) {
        if (scores[i] > 0 && scores[i] > max_score) {
            index = i;
            max_score = scores[i];
        }
    }

    beyond_inference_unref_tensor(out_tensor_h);

    if ((index == -1) ||
        (loaded < index)) {
        printf("Failed to get label from output\n");
        return false;
    }

    printf("\n%s\n", image_labels_[index].c_str());

    return true;
}

int ImageClassification::DeviceDiscoveryCallback(beyond_discovery_h handle, beyond_event_info *event, void *data)
{
    if ((event->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (DEBUG == true) {
        printf("Discovery event callback for 0x%.8X (%p)\n", event->type, event->data);
    }

    auto info = static_cast<beyond_peer_info *>(event->data);
    if (info == nullptr) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (data == nullptr) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }
    auto _this = static_cast<ImageClassification *>(data);

    switch (event->type) {
    case BEYOND_EVENT_TYPE_DISCOVERY_DISCOVERED:
        _this->discovered_++;
        printf("[%d] ", _this->discovered_);

        printf("name: %s , ", info->name);
        printf("host: %s , ", info->host);
        printf("port: %d , ", info->port[0]);
        printf("uuid: %s\n", info->uuid);
        break;
    case BEYOND_EVENT_TYPE_DISCOVERY_REMOVED:
        printf("service %s is removed\n", info->name ? info->name : "unknown");
        break;
    default:
        break;
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

int ImageClassification::EdgeDiscoveryCallback(beyond_discovery_h handle, beyond_event_info *event, void *data)
{
    if ((event->type & BEYOND_EVENT_TYPE_ERROR) == BEYOND_EVENT_TYPE_ERROR) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (DEBUG == true) {
        printf("Discovery event callback for 0x%.8X (%p)\n", event->type, event->data);
    }

    auto info = static_cast<char *>(event->data);
    if (info == nullptr) {
        return BEYOND_HANDLER_RETURN_CANCEL;
    }

    if (event->type == BEYOND_EVENT_TYPE_DISCOVERY_REGISTERED) {
        printf("service %s is registered\n", info);
    }

    return BEYOND_HANDLER_RETURN_RENEW;
}

std::unique_ptr<TaskRunner> CreateTaskRunner()
{
    return std::unique_ptr<TaskRunner>(new ImageClassification());
}
} // namespace evaluation
} // namespace beyond
