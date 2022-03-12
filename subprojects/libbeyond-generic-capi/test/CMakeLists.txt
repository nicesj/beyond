CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)

PROJECT(${NAME}-generic-capi-test CXX)

AUX_SOURCE_DIRECTORY(. TEST_SRCS)

ADD_EXECUTABLE(${PROJECT_NAME} ${TEST_SRCS})

INCLUDE_DIRECTORIES(
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-authenticator_ssl/include
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-discovery_dns_sd/include
    ${PROJECT_ROOT_DIR}/subprojects/libbeyond-peer_nn/include
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} gtest ${LOG_LIBRARIES} ${NAME}-generic-capi)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
ADD_DEPENDENCIES(${PROJECT_NAME} ${NAME}-generic-capi)

ADD_TEST(
    NAME
        ${PROJECT_NAME}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}/../../libbeyond:${CMAKE_CURRENT_BINARY_DIR}/../:${CMAKE_CURRENT_BINARY_DIR}/../../libbeyond-peer_nn/:${CMAKE_CURRENT_BINARY_DIR}/../../libbeyond-authenticator_ssl/:${CMAKE_CURRENT_BINARY_DIR}/../../libbeyond-discovery_dns_sd/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

SET_TESTS_PROPERTIES(${PROJECT_NAME} PROPERTIES TIMEOUT ${TEST_TIMEOUT})
