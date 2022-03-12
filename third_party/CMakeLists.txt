CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
PROJECT(${NAME}-third_party CXX)

IF(NOT DEFINED PROJECT_ROOT_DIR)
    MESSAGE(FATAL_ERROR "Third party libraries are not able to be build by independently, the build must be triggered from the proper subproject or the root project")
ENDIF(NOT DEFINED PROJECT_ROOT_DIR)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party-flatbuffers ALL
    COMMAND git clone https://github.com/google/flatbuffers
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}/third_party
    COMMENT "Clone flatbuffers"
    VERBATIM
)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party-tensorflow ALL
    COMMAND git clone https://github.com/tensorflow/tensorflow
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}/third_party
    COMMENT "Clone tensorflow"
    VERBATIM
)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party-jsoncpp ALL
    COMMAND git clone https://github.com/open-source-parsers/jsoncpp
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}/third_party
    COMMENT "Clone jsoncpp"
    VERBATIM
)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party-openssl ALL
    COMMAND git clone https://github.com/openssl/openssl
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}/third_party
    COMMENT "Clone openssl"
    VERBATIM
)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party-gstreamer ALL
    COMMAND https://github.com/GStreamer/gstreamer
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}/third_party
    COMMENT "Clone gstreamer"
    VERBATIM
)

ADD_CUSTOM_TARGET(
    ${NAME}-third_party ALL
    DEPENDS ${NAME}-third_party-flatbuffer ${NAME}-third_party-tensorflow ${NAME}-third_party-jsoncpp ${NAME}-third_party-openssl ${NAME}-third_party-gstreamer
    COMMENT "Clone third-party libraries"
)
