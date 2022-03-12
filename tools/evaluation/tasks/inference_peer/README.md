# Beyond API Evaluation

## Description
This page describes how you can check beyond APIs to verify both on desktop machines(Ubuntu) and on Tizen RPI.
Running a simple binary, it evaluates the following parameters of [image classification model](https://www.tensorflow.org/lite/examples/image_classification/overview#model_description)

## Development environment prerequisite
In order to use this tool on your development environment
you need Android device as an Edge and RPI or Ubuntu as a Device

- Android
  - build https://github.sec.samsung.net/BeyonD/beyond/tree/master/android/beyond
  - install beyond.apk and
  - launch application

- RPI
  - [how to setup RPI4](/docs/howto_use_rpi.md)

- Ubuntu
  - check https://github.sec.samsung.net/BeyonD/infra

## Downloading Data

download the [model, labels](https://storage.googleapis.com/download.tensorflow.org/models/tflite/mobilenet_v1_1.0_224_quant_and_labels.zip)
trained and hosted by TensorFlow
and any images(such as [orange.png](https://library.kissclipart.com/20191027/guq/kissclipart-orange-a7e12b31bf505613.png)) from labels

## Parameters

The binary takes the following required parameters:

*   `run`: `string` \
    Offloading inference run as (`device` or `edge`)
*   `model_file`: `string` \
    The path to the TFLite model file.
*   `image_path`: `string` \
    The path to the image file that will be classified.
*   `output_labels`: `string` \
    The path to the labels file.
*   `edge_ip`: `string` (default="127.0.0.1)\
    Edge IP address
*   `req_port`: `unsigned short` (default=3000) \
    Request Port
*   `rep_port`: `unsigned short` (default=3001) \
    Respond Port

## Running the binary

### On RPI
(0) Refer to
https://github.sec.samsung.net/BeyonD/beyond#tizen-rpm-package
for configuring gbs and build

(1) Connect your RPI. Push the binary to your target with sdb push (make the
directory if required) and install it
```
$ sdb push beyond-tools-0.0.1-0.armv7l.rpm /tmp
$ sdb shell rpm -ivh --nodeps --force /tmp/beyond-tools-0.0.1-0.armv7l.rpm
```

(2) Push the TFLite model, label and images that you need to test. For example:
```
$ sdb push mobilenet_v1_1.0_224_quant.tflite labels_mobilenet_quant_v1_224.txt /tmp
$ sdb push orange.png /tmp
```

(3) Run the binary
```
sdb shell
\# chsmack -e 'System' /usr/bin/beyond_eval *\#For avoiding smack deny*
\# beyond_eval \
  --run=device \
  --model_file=/tmp/mobilenet_v1_1.0_224_quant.tflite \
  --image_path=/tmp/orange.png \
  --output_labels=/tmp/labels_mobilenet_quant_v1_224.txt \
  --edge_ip=192.168.1.6
------------------------------
Target : device
Model File : /tmp/mobilenet_v1_1.0_224_quant.tflite
Input Image File : /tmp/orange.png
Output Label File : /tmp/labels.txt
Edge IP Address : 192.168.1.6
Request Port : 3000
Response Port : 3001
------------------------------

orange
```

## On Desktop

(0) Refer to
https://github.sec.samsung.net/BeyonD/beyond#ubuntu-deb-package
for docker setup and build

(1) Install
```
$ dpkg -i beyond-tools_*.deb
```

(2) Go to the path that the TFLite model, label and images are ready

(3) Run the binary
```
$ beyond_eval \
  --run=device \
  --model_file=mobilenet_v1_1.0_224_quant.tflite \
  --image_path=orange.png \
  --output_labels=labels_mobilenet_quant_v1_224.txt \
  --edge_ip=192.168.1.6
------------------------------
Target : device
Model File : mobilenet_v1_1.0_224_quant.tflite
Input Image File : orange.png
Output Label File : labels_mobilenet_quant_v1_224.txt
Edge IP Address : 192.168.1.6
Request Port : 3000
Response Port : 3001
------------------------------

orange
```
