CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)
SET(CMAKE_SKIP_BUILD_RPATH true)

PROJECT(${NAME} VERSION ${VERSION} LANGUAGES CXX)

IF(NOT DEFINED PROJECT_ROOT_DIR)
    SET(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
ENDIF(NOT DEFINED PROJECT_ROOT_DIR)

INCLUDE(${PROJECT_ROOT_DIR}/cmake/beyond_prepare.cmake)


ADD_DEFINITIONS(-DLOG_TAG="BEYOND")

#
# Processing the beyond-common
#
SET(COMMON_HEADERS
    include/${NAME}/common.h
)

SET(PLATFORM_HEADERS
    include/${PLATFORM}/${NAME}/platform/${NAME}_platform.h
)

ADD_LIBRARY(${PROJECT_NAME}-common INTERFACE)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}-common INTERFACE
    include
    include/${PLATFORM}
)

SET(PC_NAME ${PROJECT_NAME}-common)
CONFIGURE_FILE(${PROJECT_NAME}-common.pc.in ${PROJECT_NAME}-common.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-common.pc DESTINATION ${CMAKE_INSTALL_PCDIR})
INSTALL(FILES ${COMMON_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAME})
INSTALL(FILES ${PLATFORM_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAME}/platform)

#
# Processing the beyond
#
FIND_PACKAGE(Threads)
SET(BEYOND_SRC
    src/authenticator.cc
    src/authenticator_impl.cc
    src/beyond.cc
    src/command_object.cc
    src/discovery.cc
    src/discovery_impl.cc
    src/discovery_runtime.cc
    src/discovery_runtime_impl.cc
    src/event_loop.cc
    src/event_object.cc
    src/inference.cc
    src/inference_impl.cc
    src/inference_impl_distribute.cc
    src/inference_impl_edge.cc
    src/inference_impl_event_object.cc
    src/inference_impl_local.cc
    src/inference_impl_remote.cc
    src/inference_peer.cc
    src/inference_peer_impl.cc
    src/inference_runtime.cc
    src/inference_runtime_impl.cc
    src/inference_runtime_impl_async.cc
    src/resourceinfo_collector.cc
    src/timer.cc
)

IF(APPLE)
    SET(BEYOND_SRC
        ${BEYOND_SRC}
        src/event_loop_impl_mac.cc
        src/resourceinfo_collector_impl_mac.cc
        src/timer_impl_mac.cc
    )
ELSE(APPLE)
    SET(BEYOND_SRC
        ${BEYOND_SRC}
        src/event_loop_impl.cc
        src/resourceinfo_collector_impl.cc
        src/timer_impl.cc
    )
ENDIF(APPLE)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${BEYOND_SRC})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
    include
    $<TARGET_PROPERTY:${NAME}-common,INTERFACE_INCLUDE_DIRECTORIES>
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_DL_LIBS} ${LOG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

SET(PRIVATE_HEADERS
    include/${NAME}/private/${NAME}_private.h
    include/${NAME}/private/command_object_interface_private.h
    include/${NAME}/private/command_object_private.h
    include/${NAME}/private/discovery_interface_private.h
    include/${NAME}/private/discovery_private.h
    include/${NAME}/private/discovery_runtime_interface_private.h
    include/${NAME}/private/discovery_runtime_private.h
    include/${NAME}/private/event_loop_private.h
    include/${NAME}/private/event_object_base_interface_private.h
    include/${NAME}/private/event_object_interface_private.h
    include/${NAME}/private/event_object_private.h
    include/${NAME}/private/inference_interface_private.h
    include/${NAME}/private/inference_peer_interface_private.h
    include/${NAME}/private/inference_peer_private.h
    include/${NAME}/private/inference_private.h
    include/${NAME}/private/inference_runtime_interface_private.h
    include/${NAME}/private/inference_runtime_private.h
    include/${NAME}/private/log_private.h
    include/${NAME}/private/module_interface_private.h
    include/${NAME}/private/timer_private.h
    include/${NAME}/private/authenticator_interface_private.h
    include/${NAME}/private/authenticator_private.h
    include/${NAME}/private/resourceinfo_collector.h
)

SET(TARGET_HEADERS "")
FOREACH(TARGET_HEADER ${PRIVATE_HEADERS})
    IF(NOT TARGET_HEADERS STREQUAL "")
        SET(TARGET_HEADERS "${TARGET_HEADERS}\;")
    ENDIF(NOT TARGET_HEADERS STREQUAL "")
    SET(TARGET_HEADERS "${TARGET_HEADERS}${TARGET_HEADER}")
ENDFOREACH(TARGET_HEADER)

IF(PLATFORM_PACKAGE)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER ${TARGET_HEADERS}
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
ELSE(PLATFORM_PACKAGE)
    # NOTE:
    # In order to use the BeyonD for the application only (as an external library),
    # The SONAME must be ended with ".so"
    # because the Tizen packaging utility exclude the "so.${VERSION}" and "so.${VERSION}.${SOVERSION}" file from the TPK file file
    # and it causes the ELF loader cannot find necessary shared object files during loading the application.
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER ${TARGET_HEADERS}
    )
ENDIF(PLATFORM_PACKAGE)

INSTALL(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAME}/private
)

SET(PC_NAME ${PROJECT_NAME})
CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_PCDIR})

# Last
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_CMAKE_CLEAN_FILES "${PROJECT_NAME}-common.pc;${PROJECT_NAME}.pc")

IF(ENABLE_GTEST)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_GTEST)
