CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)
PROJECT(${NAME}-runtime_tflite CXX)

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

ADD_DEFINITIONS(-DLOG_TAG="BEYOND_RUNTIME_TFLITE")

IF(PLATFORM STREQUAL "android")

    STRING(REPLACE " -Werror " " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#    SET(FLATBUFFERS_CODE_COVERAGE OFF CACHE BOOL "Enable the code coverage build option." FORCE)
#    SET(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Enable the build of tests and samples." FORCE)
#    SET(FLATBUFFERS_INSTALL OFF CACHE BOOL "Enable the installation of targets." FORCE)
#    SET(FLATBUFFERS_BUILD_FLATLIB ON CACHE BOOL "Enable the build of the flatbuffers library" FORCE)
#    SET(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Enable the build of the flatbuffers compiler" FORCE)
#    SET(FLATBUFFERS_STATIC_FLATC OFF CACHE BOOL "Build flatbuffers compiler with -static flag" FORCE)
#    SET(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "Enable the build of flathash" FORCE)
#    SET(FLATBUFFERS_BUILD_BENCHMARKS OFF CACHE BOOL "Enable the build of flatbenchmark." FORCE)
#    SET(FLATBUFFERS_BUILD_GRPCTEST OFF CACHE BOOL "Enable the build of grpctest" FORCE)
#    SET(FLATBUFFERS_BUILD_SHAREDLIB OFF CACHE BOOL "Enable the build of the flatbuffers shared library" FORCE)
#    SET(FLATBUFFERS_LIBCXX_WITH_CLANG ON CACHE BOOL "Force libc++ when using Clang" FORCE)
#    # NOTE: Sanitizer check only works on Linux & OSX (gcc & llvm).
#    SET(FLATBUFFERS_CODE_SANITIZE OFF CACHE BOOL "Add '-fsanitize' flags to 'flattests' and 'flatc' targets." FORCE)
#    SET(FLATBUFFERS_PACKAGE_REDHAT OFF CACHE BOOL "Build an rpm using the 'package' target." FORCE)
#    SET(FLATBUFFERS_PACKAGE_DEBIAN OFF CACHE BOOL "Build an deb using the 'package' target." FORCE)
#    SET(FLATBUFFERS_BUILD_CPP17 OFF CACHE BOOL
#           "Enable the build of c++17 test target. \"
#           Requirements: Clang6, GCC7, MSVC2017 (_MSC_VER >= 1914)  or higher."
#           FORCE)
#    SET(FLATBUFFERS_BUILD_LEGACY OFF CACHE BOOL
#           "Run C++ code generator with '--cpp-std c++0x' switch."
#           FORCE)
#    SET(FLATBUFFERS_ENABLE_PCH OFF CACHE BOOL
#           "Enable precompile headers support for 'flatbuffers' and 'flatc'. \"
#            Only work if CMake supports 'target_precompile_headers'. \"
#            This can speed up compilation time."
#           FORCE)
#    SET(FLATBUFFERS_SKIP_MONSTER_EXTRA OFF CACHE BOOL "Skip generating monster_extra.fbs that contains non-supported numerical types." FORCE)
#    SET(FLATBUFFERS_OSX_BUILD_UNIVERSAL OFF CACHE BOOL "Enable the build for multiple architectures on OS X (arm64, x86_64)." FORCE)
#
#    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/flatbuffers ${CMAKE_BINARY_DIR}/third_party/flatbuffers)
#    SET(TFLITE_ENABLE_RUY OFF CACHE BOOL "Enable experimental RUY integration" FORCE)
#    SET(TFLITE_ENABLE_RESOURCE OFF CACHE BOOL "Enable experimental support for resources" FORCE)
#    SET(TFLITE_ENABLE_NNAPI ON CACHE BOOL "Enable NNAPI (Android only)." FORCE)
#    SET(TFLITE_ENABLE_MMAP ON CACHE BOOL "Enable MMAP (unsupported on Windows)" FORCE)
#    SET(TFLITE_ENABLE_GPU ON CACHE BOOL "Enable GPU" FORCE)
#    SET(TFLITE_ENABLE_METAL OFF CACHE BOOL "Enable Metal delegate (iOS only)" FORCE)
    SET(TFLITE_ENABLE_XNNPACK OFF CACHE BOOL "Enable XNNPACK backend" FORCE)
#    SET(TFLITE_ENABLE_EXTERNAL_DELEGATE OFF CACHE BOOL "Enable External Delegate backend" FORCE)
#    SET(TFLITE_KERNEL_TEST OFF CACHE BOOL "Enable tflite kernel unit test" FORCE)
    ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/tensorflow/tensorflow/lite ${CMAKE_BINARY_DIR}/third_party/tensorflow)

    SET(DEPENDS_ON tensorflow-lite flatbuffers)
    SET(LIBRARIES tensorflow-lite flatbuffers)
ELSE(PLATFORM STREQUAL "android")
    PKG_CHECK_MODULES(RUNTIME_PKGS tensorflow-lite)
    IF(RUNTIME_PKGS_FOUND)
        FOREACH(CFLAG ${RUNTIME_PKGS_CFLAGS})
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAG}")
        ENDFOREACH(CFLAG)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        SET(LIBRARIES ${RUNTIME_PKGS_LDFLAGS})
    ELSE(RUNTIME_PKGS_FOUND)
        SET(FLATBUFFERS_CODE_COVERAGE OFF CACHE BOOL "Enable the code coverage build option." FORCE)
        SET(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Enable the build of tests and samples." FORCE)
        SET(FLATBUFFERS_INSTALL OFF CACHE BOOL "Enable the installation of targets." FORCE)
        SET(FLATBUFFERS_BUILD_FLATLIB ON CACHE BOOL "Enable the build of the flatbuffers library" FORCE)
        SET(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Enable the build of the flatbuffers compiler" FORCE)
        SET(FLATBUFFERS_STATIC_FLATC OFF CACHE BOOL "Build flatbuffers compiler with -static flag" FORCE)
        SET(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "Enable the build of flathash" FORCE)
        SET(FLATBUFFERS_BUILD_BENCHMARKS OFF CACHE BOOL "Enable the build of flatbenchmark." FORCE)
        SET(FLATBUFFERS_BUILD_GRPCTEST OFF CACHE BOOL "Enable the build of grpctest" FORCE)
        SET(FLATBUFFERS_BUILD_SHAREDLIB OFF CACHE BOOL "Enable the build of the flatbuffers shared library" FORCE)
        SET(FLATBUFFERS_LIBCXX_WITH_CLANG ON CACHE BOOL "Force libc++ when using Clang" FORCE)
        # NOTE: Sanitizer check only works on Linux & OSX (gcc & llvm).
        SET(FLATBUFFERS_CODE_SANITIZE OFF CACHE BOOL "Add '-fsanitize' flags to 'flattests' and 'flatc' targets." FORCE)
        SET(FLATBUFFERS_PACKAGE_REDHAT OFF CACHE BOOL "Build an rpm using the 'package' target." FORCE)
        SET(FLATBUFFERS_PACKAGE_DEBIAN OFF CACHE BOOL "Build an deb using the 'package' target." FORCE)
        SET(FLATBUFFERS_BUILD_CPP17 OFF CACHE BOOL
               "Enable the build of c++17 test target. \"
               Requirements: Clang6, GCC7, MSVC2017 (_MSC_VER >= 1914)  or higher."
               FORCE)
        SET(FLATBUFFERS_BUILD_LEGACY OFF CACHE BOOL
               "Run C++ code generator with '--cpp-std c++0x' switch."
               FORCE)
        SET(FLATBUFFERS_ENABLE_PCH OFF CACHE BOOL
               "Enable precompile headers support for 'flatbuffers' and 'flatc'. \"
                Only work if CMake supports 'target_precompile_headers'. \"
                This can speed up compilation time."
               FORCE)
        SET(FLATBUFFERS_SKIP_MONSTER_EXTRA OFF CACHE BOOL "Skip generating monster_extra.fbs that contains non-supported numerical types." FORCE)
        SET(FLATBUFFERS_OSX_BUILD_UNIVERSAL OFF CACHE BOOL "Enable the build for multiple architectures on OS X (arm64, x86_64)." FORCE)
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/flatbuffers ${CMAKE_BINARY_DIR}/third_party/flatbuffers)
        SET(TFLITE_ENABLE_RUY OFF CACHE BOOL "Enable experimental RUY integration" FORCE)
        SET(TFLITE_ENABLE_RESOURCE OFF CACHE BOOL "Enable experimental support for resources" FORCE)
        SET(TFLITE_ENABLE_NNAPI OFF CACHE BOOL "Enable NNAPI (Android only)." FORCE)
        SET(TFLITE_ENABLE_MMAP ON CACHE BOOL "Enable MMAP (unsupported on Windows)" FORCE)
        SET(TFLITE_ENABLE_GPU OFF CACHE BOOL "Enable GPU" FORCE)
        SET(TFLITE_ENABLE_METAL OFF CACHE BOOL "Enable Metal delegate (iOS only)" FORCE)
        SET(TFLITE_ENABLE_XNNPACK ON CACHE BOOL "Enable XNNPACK backend" FORCE)
        SET(TFLITE_ENABLE_EXTERNAL_DELEGATE ON CACHE BOOL "Enable External Delegate backend" FORCE)
        SET(TFLITE_KERNEL_TEST OFF CACHE BOOL "Enable tflite kernel unit test" FORCE)
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/tensorflow/tensorflow/lite ${CMAKE_BINARY_DIR}/third_party/tensorflow)

        SET(DEPENDS_ON tensorflow-lite flatbuffers)
        SET(LIBRARIES tensorflow-lite flatbuffers)
    ENDIF(RUNTIME_PKGS_FOUND)
ENDIF(PLATFORM STREQUAL "android")

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src RUNTIME_TFLITE_SRC)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-emain")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${RUNTIME_TFLITE_SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LOG_LIBRARIES} ${BEYOND_LIBRARIES} ${LIBRARIES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER include/${NAME}/plugin/runtime_tflite_plugin.h
)
INSTALL(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}/${NAME}/plugin
)

SET(PC_NAME ${PROJECT_NAME})
CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${PKGCONFIG_DIR})
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_CMAKE_CLEAN_FILES "${PROJECT_NAME}.pc")

ADD_DEPENDENCIES(${PROJECT_NAME} ${DEPENDS_ON_BEYOND} ${DEPENDS_ON})
