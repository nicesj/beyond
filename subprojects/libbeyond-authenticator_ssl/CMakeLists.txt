CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
PROJECT(${NAME}-authenticator_ssl CXX)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

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

ADD_DEFINITIONS(-DLOG_TAG="BEYOND_AUTH_SSL")

IF(PLATFORM STREQUAL "android")
    FIND_PACKAGE(jsoncpp REQUIRED CONFIG)
    FIND_PACKAGE(openssl REQUIRED CONFIG)

    LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS jsoncpp::jsoncpp)
    LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS openssl::ssl openssl::crypto)
    SET(DEPENDS_ON openssl::ssl openssl::crypto jsoncpp::jsoncpp)
ELSE(PLATFORM STREQUAL "android")
    PKG_CHECK_MODULES(AUTHENTICATOR_SSL_PKGS openssl)
    IF(NOT AUTHENTICATOR_SSL_PKGS_FOUND)
        PKG_CHECK_MODULES(AUTHENTICATOR_SSL_PKGS openssl1.1)
        IF(NOT AUTHENTICATOR_SSL_PKGS_FOUND)
            MESSAGE("No SSL found, build the openssl library")
            ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/openssl ${CMAKE_BINARY_DIR}/third_party/openssl)

            LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS openssl)
            LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS openssl_crypto)
            SET(DEPENDS_ON openssl openssl_crypto)
        ENDIF(NOT AUTHENTICATOR_SSL_PKGS_FOUND)
    ENDIF(NOT AUTHENTICATOR_SSL_PKGS_FOUND)

    PKG_CHECK_MODULES(JSONCPP_PKGS jsoncpp)
    IF(JSONCPP_PKGS_FOUND)
        INCLUDE_DIRECTORIES(${JSONCPP_PKGS_INCLUDE_DIRS})
        LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS ${JSONCPP_PKGS_LDFLAGS})
    ELSE(JSONCPP_PKGS_FOUND)
        SET(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
        SET(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step")
        SET(JSONCPP_WITH_WARNING_AS_ERROR OFF CACHE BOOL "Force compilation to fail if a warning occurs")
        SET(JSONCPP_WITH_STRICT_ISO OFF CACHE BOOL "Issue all the warnings demanded by strict ISO C and ISO C++")
        SET(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate and install .pc files")
        SET(JSONCPP_WITH_CMAKE_PACKAGE OFF CACHE BOOL "Generate and install cmake package files")
        SET(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "Compile JsonCpp example")
        SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a shared library.")
        SET(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp_lib as a static library.")
        SET(BUILD_OBJECT_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a object library.")
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/jsoncpp ${CMAKE_BINARY_DIR}/third_party/jsoncpp)

        LIST(APPEND AUTHENTICATOR_SSL_PKGS_LDFLAGS jsoncpp_static)
        SET(DEPENDS_ON ${DEPENDS_ON} jsoncpp_static)
    ENDIF(JSONCPP_PKGS_FOUND)
ENDIF(PLATFORM STREQUAL "android")

FOREACH(CFLAG ${AUTHENTICATOR_SSL_PKGS_CFLAGS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAG}")
ENDFOREACH(CFLAG)

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src AUTHENTICATOR_SSL_SRC)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-emain")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${AUTHENTICATOR_SSL_SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LOG_LIBRARIES} ${BEYOND_LIBRARIES} ${AUTHENTICATOR_SSL_PKGS_LDFLAGS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER include/${NAME}/plugin/authenticator_ssl_plugin.h
)
INSTALL(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAME}/plugin
)

SET(PC_NAME ${PROJECT_NAME})
CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_PCDIR})

ADD_DEPENDENCIES(${PROJECT_NAME} ${DEPENDS_ON_BEYOND} ${DEPENDS_ON})

IF(ENABLE_GTEST)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_GTEST)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_CMAKE_CLEAN_FILES "${PROJECT_NAME}.pc")
