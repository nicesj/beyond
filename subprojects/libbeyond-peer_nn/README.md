# Overview


## Tips for the GStreamer

 If you have new installed gstreamer elements, you have to update gst element cache on the target device (Tizen)
 In order to update the element cache, you should run the following script.

``` bash
$ /etc/gstreamer/gstreamer-registry.sh
```

## Control Channel

 Using Grpc protobuf

## Data (inference) Channel

 Using gstreamer gdppay or streaming element (eg. rtpjpegpay)
