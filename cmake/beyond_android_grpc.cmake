IF(CMAKE_VERSION VERSION_LESS "3.10.0")
    IF(DEFINED BEYOND_ANDROID_GRPC)
        RETURN()
    ENDIF(DEFINED BEYOND_ANDROID_GRPC)
    SET(BEYOND_ANDROID_GRPC TRUE)
ELSE(CMAKE_VERSION VERSION_LESS "3.10.0")
    INCLUDE_GUARD(GLOBAL)
ENDIF(CMAKE_VERSION VERSION_LESS "3.10.0")

INCLUDE_DIRECTORIES(
    ${PROJECT_ROOT_DIR}/third_party/grpc/third_party/protobuf/src
    ${PROJECT_ROOT_DIR}/third_party/grpc/include
    ${PROJECT_ROOT_DIR}/third_party/grpc/third_party/abseil-cpp
)

STRING(REPLACE "libbeyond-peer_nn" "grpc" CMAKE_GRPC_BINARY_DIR ${CMAKE_BINARY_DIR})

STRING(REPLACE "subprojects" "android" CMAKE_GRPC_BINARY_DIR ${CMAKE_GRPC_BINARY_DIR})

LINK_DIRECTORIES(${CMAKE_GRPC_BINARY_DIR};${CMAKE_GRPC_BINARY_DIR}/third_party/protobuf;${CMAKE_GRPC_BINARY_DIR}/third_party/zlib;${CMAKE_GRPC_BINARY_DIR}/third_party/cares/cares/lib;${CMAKE_GRPC_BINARY_DIR}/third_party/re2;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/hash;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/container;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/base;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/status;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/types;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/strings;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/synchronization;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/debugging;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/time;${CMAKE_GRPC_BINARY_DIR}/third_party/abseil-cpp/absl/numeric)

IF(${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
    SET(PROTOBUF_LIB protobufd)
    SET(PROTOC_LIB protocd)
ELSE()
    SET(PROTOBUF_LIB protobuf)
    SET(PROTOC_LIB protoc)
ENDIF()

SET(GRPC_LIBRARIES ${PROTOBUF_LIB} ${PROTOC_LIB} grpc++_reflection grpc++ grpc z cares address_sorting re2 upb absl_hash absl_city absl_raw_hash_set absl_hashtablez_sampler absl_exponential_biased absl_statusor absl_bad_variant_access gpr absl_status absl_cord absl_str_format_internal absl_synchronization absl_stacktrace absl_symbolize absl_debugging_internal absl_demangle_internal absl_graphcycles_internal absl_malloc_internal absl_time absl_strings absl_throw_delegate absl_strings_internal absl_base absl_spinlock_wait absl_int128 absl_civil_time absl_time_zone absl_bad_optional_access absl_raw_logging_internal absl_log_severity ${CMAKE_GRPC_BINARY_DIR}/third_party/boringssl-with-bazel/libssl.a ${CMAKE_GRPC_BINARY_DIR}/third_party/boringssl-with-bazel/libcrypto.a)

SET(PROTOBUF_LIBRARY ${CMAKE_GRPC_BINARY_DIR}/third_party/protobuf/lib${PROTOBUF_LIB}.a)
SET(PROTOBUF_PROTOC_LIBRARY ${CMAKE_GRPC_BINARY_DIR}/third_party/protobuf/lib${PROTOBUF_LIB}.a)
SET(PROTOBUF_INCLUDE_DIR ${PROJECT_ROOT_DIR}/third_party/grpc/third_party/protobuf/src)

SET(GRPC_GRPC++_LIBRARY ${CMAKE_GRPC_BINARY_DIR}/libgrpc++.a)
SET(GRPC_LIBRARY ${CMAKE_GRPC_BINARY_DIR}/libgrpc.a)
SET(GRPC_GRPC++_REFLECTION_LIBRARY ${CMAKE_GRPC_BINARY_DIR}/libgrpc++_reflection.a)
SET(GRPC_INCLUDE_DIR ${PROJECT_ROOT_DIR}/third_party/grpc/include)
