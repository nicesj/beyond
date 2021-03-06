CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)
PROJECT(${NAME}-discovery_dns_sd)

IF(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    IF(NOT DEFINED PROJECT_ROOT_DIR)
        SET(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    ENDIF(NOT DEFINED PROJECT_ROOT_DIR)

    INCLUDE(${PROJECT_ROOT_DIR}/cmake/beyond_prepare.cmake)

    # NOTE
    # We are able to package the following libraries as AAR packages
    IF((${BEYOND_LIBRARIES} STREQUAL ${NAME}) AND (NOT ${PROJECT_NAME} STREQUAL ${NAME}))
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/subprojects/libbeyond ${CMAKE_BINARY_DIR}/subprojects/libbeyond)
    ENDIF()
ENDIF()

ADD_DEFINITIONS(-DLOG_TAG="BEYOND_DISCOVERY_DNSSD")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

PKG_CHECK_MODULES(DNS_SD QUIET dns_sd)
IF(${DNS_SD_FOUND})
    INCLUDE_DIRECTORIES(${DNS_SD_INCLUDE_DIRS})
    LINK_DIRECTORIES(${DNS_SD_LIBRARY_DIRS})
ELSE()
    FIND_LIBRARY(DNS_SD NAMES dns_sd)
    IF(${DNS_SD} STREQUAL "DNS_SD-NOTFOUND")
        ADD_SUBDIRECTORY(${PROJECT_ROOT_DIR}/third_party/mDNSResponder ${CMAKE_BINARY_DIR}/third_party/mDNSResponder)
        SET(DNS_SD_LIBRARIES dns_sd)
        SET(DEPENDS_ON dns_sd)
    ELSE()
        SET(DNS_SD_LIBRARIES ${DNS_SD})
    ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src DISCOVERY_DNS_SD_SRC)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-emain")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${DISCOVERY_DNS_SD_SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LOG_LIBRARIES} ${BEYOND_LIBRARIES} ${DNS_SD_LIBRARIES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER include/${NAME}/plugin/discovery_dns_sd_plugin.h
)
INSTALL(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAME}/plugin
)

SET(PC_NAME ${PROJECT_NAME})
CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_PCDIR})
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_CMAKE_CLEAN_FILES "${PROJECT_NAME}.pc")

ADD_DEPENDENCIES(${PROJECT_NAME} ${DEPENDS_ON_BEYOND} ${DEPENDS_ON})

IF(ENABLE_GTEST)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_GTEST)
