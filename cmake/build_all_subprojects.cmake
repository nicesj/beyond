IF(CMAKE_VERSION VERSION_LESS "3.10.0")
    IF(DEFINED BUILD_ALL_SUBPORJECTS)
        RETURN()
    ENDIF(DEFINED BUILD_ALL_SUBPORJECTS)
    SET(BUILD_ALL_SUBPORJECTS TRUE)
ELSE(CMAKE_VERSION VERSION_LESS "3.10.0")
    INCLUDE_GUARD(GLOBAL)
ENDIF(CMAKE_VERSION VERSION_LESS "3.10.0")

######
# Build the OS dependent sub-projects
#
IF(PLATFORM STREQUAL "tizen")
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-tizen-capi ${CMAKE_BINARY_DIR}/subprojects/libbeyond-tizen-capi)
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/sample-tizen_capi ${CMAKE_BINARY_DIR}/subprojects/sample-tizen-capi)
ELSEIF(PLATFORM STREQUAL "generic")
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-generic-capi ${CMAKE_BINARY_DIR}/subprojects/libbeyond-generic-capi)
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/sample-generic_capi ${CMAKE_BINARY_DIR}/subprojects/sample-generic-capi)
ELSEIF(PLATFORM STREQUAL "android")
    IF(NOT DISABLE_TOOL)
        # NOTE:
        # Build for the evaluation tool
        # It uses generic API for the android now.
        # of course, the generic CAPI also good for the Android.
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-generic-capi ${CMAKE_BINARY_DIR}/subprojects/libbeyond-generic-capi)
    ENDIF(NOT DISABLE_TOOL)

    IF(NOT PROJECT_NAME STREQUAL ${NAME}-android)
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-android ${CMAKE_BINARY_DIR}/subprojects/libbeyond-android)
    ENDIF(NOT PROJECT_NAME STREQUAL ${NAME}-android)
ENDIF(PLATFORM STREQUAL "tizen")

######
# Build the common sub-projects
#
IF(ENABLE_GTEST)
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-mock ${CMAKE_BINARY_DIR}/subprojects/libbeyond-mock)
ENDIF(ENABLE_GTEST)

# NOTE
# We are able to package the following libraries as AAR packages
IF((BEYOND_LIBRARIES STREQUAL ${NAME}) AND (NOT PROJECT_NAME STREQUAL ${NAME}))
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond ${CMAKE_BINARY_DIR}/subprojects/libbeyond)
ENDIF((BEYOND_LIBRARIES STREQUAL ${NAME}) AND (NOT PROJECT_NAME STREQUAL ${NAME}))

ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-runtime_tflite ${CMAKE_BINARY_DIR}/subprojects/libbeyond-runtime_tflite)

# Requiers openssl
ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-authenticator_ssl ${CMAKE_BINARY_DIR}/subprojects/libbeyond-authenticator_ssl)

# Requires DNSSD
ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-discovery_dns_sd ${CMAKE_BINARY_DIR}/subprojects/libbeyond-discovery_dns_sd)

IF(NOT DISABLE_PEER_NN)
    # Requiers gstreamer, prtobuf, grpc, glib
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond-peer_nn ${CMAKE_BINARY_DIR}/subprojects/libbeyond-peer_nn)
ENDIF(NOT DISABLE_PEER_NN)

IF(NOT DISABLE_TOOL)
    # Requires opencv
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/tools/evaluation ${CMAKE_BINARY_DIR}/tools/evaluation)
ENDIF(NOT DISABLE_TOOL)
