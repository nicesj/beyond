# Third Party libraries

  In order to build the BeyonD, it is necessary to download the third party libraries such as Tensorflow, Flatbuffers, openssl, mDSNResponder, jsoncpp, and googletest.


# Tensorflow, and Flatbuffers

  The Tensorflow especially the tensorflow-lite (aka. tflite) and the Flatbuffers are used by the libbeyond-runtime\_tflite module


# openssl

  The openssl is used by the libbeyond-authenticator\_ssl module
  https://github.com/openssl/openssl/archive/refs/heads/OpenSSL_1_1_1-stable.zip


# mDNSResponder

  The mDNSResponder is used by the libbeyond-discovery\_dns\_sd module
  https://android.googlesource.com/platform/external/mdnsresponder/+archive/refs/heads/master.tar.gz


# googletest

  The googletest is used by entire project to build up the unittests for the native implementations

# jsoncpp

  The jsoncpp is used by almost modules to parse the json format string which is given by users through the configuration API
