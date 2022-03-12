IF(CMAKE_VERSION VERSION_LESS "3.10.0")
    IF(DEFINED BEYOND_PREPARE)
        RETURN()
    ENDIF(DEFINED BEYOND_PREPARE)
    SET(BEYOND_PREPARE TRUE)
ELSE(CMAKE_VERSION VERSION_LESS "3.10.0")
    INCLUDE_GUARD(GLOBAL)
ENDIF(CMAKE_VERSION VERSION_LESS "3.10.0")

IF(NOT CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    RETURN()
ENDIF(NOT CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

#####
# Prepare the default build settings
#
INCLUDE(FindPkgConfig)

IF(NOT DEFINED NAME)
    SET(NAME "beyond")
ENDIF(NOT DEFINED NAME)

IF(NOT DEFINED VERSION)
    SET(VERSION "0.0.1")
ENDIF(NOT DEFINED VERSION)

IF(NOT DEFINED PLATFORM_PACKAGE)
    SET(PLATFORM_PACKAGE true)
ENDIF(NOT DEFINED PLATFORM_PACKAGE)

IF(NOT DEFINED SYSCONF_INSTALL_DIR)
    SET(SYSCONF_INSTALL_DIR "/etc")
ENDIF(NOT DEFINED SYSCONF_INSTALL_DIR)

IF(NOT DEFINED PKGCONFIG_DIR)
    SET(PKGCONFIG_DIR "/usr/local/lib/pkgconfig")
ENDIF(NOT DEFINED PKGCONFIG_DIR)

IF(CMAKE_BUILD_TYPE)
    STRING(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
ENDIF(CMAKE_BUILD_TYPE)

IF(NOT PROJECT_NAME STREQUAL "beyond")
    IF(NOT DEFINED BEYOND_LIBRARIES)
        FIND_PACKAGE(libbeyond CONFIG)
        IF(libbeyond_FOUND)
            SET(BEYOND_LIBRARIES libbeyond::${NAME})
            SET(DEPENDS_ON_BEYOND libbeyond::${NAME})
        ELSE(libbeyond_FOUND)
            SET(BEYOND_LIBRARIES ${NAME})
            SET(DEPENDS_ON_BEYOND ${NAME})
        ENDIF(libbeyond_FOUND)
    ENDIF(NOT DEFINED BEYOND_LIBRARIES)
ENDIF(NOT PROJECT_NAME STREQUAL "beyond")

IF(ANDROID_ABI)
    # Override install destination directory for the android archive resource
    SET(INCLUDE_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${INCLUDE_INSTALL_DIR})
    SET(LIB_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIB_INSTALL_DIR})
    SET(PKGCONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR}/${PKGCONFIG_DIR})
ENDIF(ANDROID_ABI)

IF(CMAKE_BUILD_TYPE_LOWER MATCHES "debug")
    IF(COVERAGE)
        SET(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage")
    ENDIF(COVERAGE)
ENDIF(CMAKE_BUILD_TYPE_LOWER MATCHES "debug")

IF(NOT PLATFORM)
    SET(PLATFORM "generic")
ENDIF(NOT PLATFORM)

IF(PLATFORM STREQUAL "android")
    ADD_DEFINITIONS(-DANDROID)
    FIND_LIBRARY(ALOG NAMES log)
    IF(ALOG STREQUAL "ALOG-NOTFOUND")
        MESSAGE(FATAL_ERROR "Uable to find the android log library")
    ENDIF(ALOG STREQUAL "ALOG-NOTFOUND")
    SET(LOG_LIBRARIES ${ALOG})
ELSEIF(PLATFORM STREQUAL "tizen")
    ADD_DEFINITIONS(-DTIZEN)
    IF(STDOUT_LOG)
        ADD_DEFINITIONS(-DDLOG_STDOUT)
    ENDIF(STDOUT_LOG)
    ENABLE_TESTING()
    PKG_CHECK_MODULES(PKGS REQUIRED
        dlog
    )
    SET(LOG_LIBRARIES ${PKGS_LDFLAGS})
    FOREACH(CFLAG ${PKGS_CFLAGS})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAG}")
    ENDFOREACH(CFLAG)
ELSEIF(PLATFORM STREQUAL "generic")
    # TODO
    # "x86_64-linux-gnu" string must be built by the current build system information
    LIST(APPEND CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu")
    ADD_DEFINITIONS(-DGENERIC)
    ENABLE_TESTING()
ENDIF(PLATFORM STREQUAL "android")

ADD_DEFINITIONS(-DREVISION="${REVISION}" -DPLATFORM="${PLATFORM}" -DSYSCONFDIR="${SYSCONF_INSTALL_DIR}")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS} -Wall -Werror -std=c++17 -fvisibility=hidden")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
