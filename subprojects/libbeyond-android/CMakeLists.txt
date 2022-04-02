CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)
SET(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
PROJECT(${NAME}-android CXX)

IF(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    IF(NOT DEFINED PROJECT_ROOT_DIR)
        SET(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    ENDIF(NOT DEFINED PROJECT_ROOT_DIR)

    INCLUDE(${PROJECT_ROOT_DIR}/cmake/beyond_prepare.cmake)

    # NOTE
    # We are able to package the following libraries as AAR packages
    IF((BEYOND_LIBRARIES STREQUAL ${NAME}) AND (NOT PROJECT_NAME STREQUAL ${NAME}))
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond ${CMAKE_BINARY_DIR}/subprojects/libbeyond)
    ENDIF((BEYOND_LIBRARIES STREQUAL ${NAME}) AND (NOT PROJECT_NAME STREQUAL ${NAME}))
ENDIF(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

ADD_DEFINITIONS(-DLOG_TAG="BEYOND_ANDROID")

INCLUDE_DIRECTORIES(src/main/jni)

SET(ANDROID_SRC
    src/main/jni/inference/beyond-peer_jni.cc
    src/main/jni/inference/beyond-inference_jni.cc
    src/main/jni/inference/tensor/beyond-tensor_jni.cc
    src/main/jni/beyond-android_jni.cc
    src/main/jni/authenticator/beyond-authenticator_jni.cc
    src/main/jni/discovery/beyond-discovery_jni.cc
    src/main/jni/JNIHelper.cc
    src/main/jni/NativeInterface.cc
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

IF(USE_PEER_NN STREQUAL "ON")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PEER_NN=1")
ENDIF(USE_PEER_NN STREQUAL "ON")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${ANDROID_SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LOG_LIBRARIES} ${BEYOND_LIBRARIES} ${CMAKE_DL_LIBS} android)
ADD_DEPENDENCIES(${PROJECT_NAME} ${DEPENDS_ON_BEYOND})

INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
