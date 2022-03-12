CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)

PROJECT(${NAME}_eval CXX)

IF(PLATFORM STREQUAL "android")
    SET(BUILD_ANDROID_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
    SET(BUILD_ANDROID_SERVICE OFF CACHE BOOL "Disable android service" FORCE)
    SET(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
    SET(BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
    SET(BUILD_PERF_TESTS OFF CACHE BOOL "Disable performance tests" FORCE)
    SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Do not build shared libs" FORCE)
    SET(BUILD_OPENCV_APPS OFF CACHE BOOL "Disable applications" FORCE)
    SET(BUILD_DOCS OFF CACHE BOOL "Documents" FORCE)
    SET(BUILD_JAVA OFF CACHE BOOL "Disable JAVA" FORCE)
    SET(BUILD_PROTOBUF OFF CACHE BOOL "Disable Protobuf build" FORCE)
    SET(WITH_PROTOBUF OFF CACHE BOOL "Without protobuf?" FORCE)
    SET(OLD_EXEC_PATH ${EXECUTABLE_OUTPUT_PATH})
    SET(OLD_LIB_PATH ${LIBRARY_OUTPUT_PATH})
    SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/opencv/bin)
    SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/opencv/lib)
    SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/opencv/bin)
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/opencv ${CMAKE_BINARY_DIR}/third_party/opencv)
    SET(EXECUTABLE_OUTPUT_PATH ${OLD_EXEC_PATH})
    SET(LIBRARY_OUTPUT_PATH ${OLD_LIB_PATH})

    LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/third_party/opencv/lib/${ANDROID_ABI} ${CMAKE_BINARY_DIR}/third_party/opencv/3rdparty/lib/${ANDROID_ABI})
    LIST(APPEND EVAL_PKGS_CFLAGS -I${CMAKE_BINARY_DIR})
    LIST(APPEND EVAL_PKGS_CFLAGS -I${PROJECT_ROOT_DIR}/third_party/opencv/include)

    FOREACH(LIB -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_objdetect -lopencv_photo -lopencv_video -lopencv_calib3d -lopencv_features2d -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core -lIlmImf -lcpufeatures -littnotify -llibjasper -llibjpeg-turbo -llibpng -llibtiff -llibwebp -lquirc -ltegra_hal -lz)
        LIST(APPEND EVAL_PKGS_LDFLAGS ${LIB})
    ENDFOREACH(LIB)
    FOREACH(MOD core calib3d features2d flann highgui imgcodecs videoio imgproc ml objdetect photo cudabgsegm dnn gapi java js python stitching ts video world)
        LIST(APPEND EVAL_PKGS_CFLAGS -I${PROJECT_ROOT_DIR}/third_party/opencv/modules/${MOD}/include)
    ENDFOREACH(MOD)
ELSE(PLATFORM STREQUAL "android")
    INCLUDE(FindPkgConfig)
    IF(PLATFORM STREQUAL "tizen")
        PKG_CHECK_MODULES(GLIB2 REQUIRED glib-2.0)
    ENDIF(PLATFORM STREQUAL "tizen")
    PKG_CHECK_MODULES(OPENCV4 opencv4)
    IF(OPENCV4_FOUND)
        SET(EVAL_PKGS_CFLAGS ${OPENCV4_CFLAGS} ${GLIB2_CFLAGS})
        SET(EVAL_PKGS_LDFLAGS ${OPENCV4_LDFLAGS} ${GLIB2_LDFLAGS})
    ELSE(OPENCV4_FOUND)
        PKG_CHECK_MODULES(OPENCV3 opencv3)
        IF(OPENCV3_FOUND)
            SET(EVAL_PKGS_CFLAGS ${OPENCV3_CFLAGS} ${GLIB2_CFLAGS})
            SET(EVAL_PKGS_LDFLAGS ${OPENCV3_LDFLAGS} ${GLIB2_LDFLAGS})
        ELSE(OPENCV3_FOUND)
            PKG_CHECK_MODULES(OPENCV opencv)
            IF(OPENCV_FOUND)
                SET(EVAL_PKGS_CFLAGS ${OPENCV_CFLAGS} ${GLIB2_CFLAGS})
                SET(EVAL_PKGS_LDFLAGS ${OPENCV_LDFLAGS} ${GLIB2_LDFLAGS})
            ELSE(OPENCV_FOUND)
                MESSAGE(FATAL_ERROR "opencv is not installed")
            ENDIF(OPENCV_FOUND)
        ENDIF(OPENCV3_FOUND)
    ENDIF(OPENCV4_FOUND)
ENDIF(PLATFORM STREQUAL "android")

FOREACH(CFLAG ${EVAL_PKGS_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAG}")
ENDFOREACH(CFLAG)

INCLUDE_DIRECTORIES(
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-authenticator_ssl/include
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-peer_nn/include
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-discovery_dns_sd/include
    .)
SET(EVAL_SRCS
    main.cc
    task_options.cc
    task_runner.cc
    tensordata_util.cc
    tflite_runner.cc
    remote_peer_runner.cc
)

IF(PLATFORM STREQUAL "tizen")
    SET(IC_SRCS ${EVAL_SRCS} tasks/inference_peer/tizen.native/image_classification.cc)
    SET(LIBRARIES ${EVAL_PKGS_LDFLAGS} ${NAME}-tizen-capi)
    SET(DEPEND_TARGET ${NAME}-tizen-capi)

    SET(DIFF_SRCS ${EVAL_SRCS} tasks/inference_diff/inference_diff.cc)
ELSE(PLATFORM STREQUAL "tizen")
    # Generic, Android
    SET(IC_SRCS ${EVAL_SRCS} tasks/inference_peer/generic/image_classification.cc)
    SET(LIBRARIES ${EVAL_PKGS_LDFLAGS} ${NAME}-generic-capi)
    SET(DEPEND_TARGET ${NAME}-generic-capi)

    SET(DIFF_SRCS ${EVAL_SRCS} tasks/inference_diff/inference_diff.cc)
ENDIF(PLATFORM STREQUAL "tizen")

ADD_EXECUTABLE(${PROJECT_NAME} ${IC_SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES})
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
ADD_DEPENDENCIES(${PROJECT_NAME} ${DEPEND_TARGET})

ADD_EXECUTABLE(beyond_eval_diff ${DIFF_SRCS})
TARGET_LINK_LIBRARIES(beyond_eval_diff ${LIBRARIES})
INSTALL(TARGETS beyond_eval_diff DESTINATION bin)
ADD_DEPENDENCIES(beyond_eval_diff ${DEPEND_TARGET})
