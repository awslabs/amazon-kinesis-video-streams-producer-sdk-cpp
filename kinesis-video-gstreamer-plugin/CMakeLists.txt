
cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 14)
project(kinesisvideosink)

set(EXEC_NAME kinesisvideosink)

set(CMAKE_CXX_STANDARD 14)

if (DEFINED ENV{KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR})
    set(KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR $ENV{KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR})
else ()
    set(KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR "../")
endif ()

set(PLUGIN_PRJ_DIR ".")

if (DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
    set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
else ()
    set(KINESIS_VIDEO_OPEN_SOURCE_SRC "${KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR}/kinesis-video-native-build/downloads")
endif ()

MESSAGE(STATUS "KINESIS_VIDEO_OPEN_SOURCE_SRC: " ${KINESIS_VIDEO_OPEN_SOURCE_SRC})

if (DEFINED ENV{KINESIS_VIDEO_PIC_SRC})
    set(KINESIS_VIDEO_PIC_SRC $ENV{KINESIS_VIDEO_PIC_SRC})
else ()
    set(KINESIS_VIDEO_PIC_SRC "${KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR}/kinesis-video-pic")
endif ()

if (DEFINED ENV{KINESIS_VIDEO_PRODUCER_SRC})
    set(KINESIS_VIDEO_PRODUCER_SRC $ENV{KINESIS_VIDEO_PRODUCER_SRC})
else ()
    set(KINESIS_VIDEO_PRODUCER_SRC "${KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR}/kinesis-video-producer")
endif ()

if (DEFINED ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
    set(KINESIS_VIDEO_GST_PLUGIN_SRC $ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
else ()
    set(KINESIS_VIDEO_GST_PLUGIN_SRC "${PLUGIN_PRJ_DIR}/plugin-src")
endif ()

if (DEFINED ENV{KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC})
    set(KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC $ENV{KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC})
else ()
    set(KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC "demo-src")
endif ()

message(STATUS "System name is ${CMAKE_SYSTEM_NAME}")

STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER_CASE)
STRING(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# Uncomment below line for debug heap
#add_definitions(-DHEAP_DEBUG)

# Uncomment below line for very verbose logging
#add_definitions(-DLOG_STREAMING)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
endif ()

set(GST_PLUGIN_SOURCE_FILES
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/config.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkRotatingCredentialProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkRotatingCredentialProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStreamCallbackProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStreamCallbackProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkClientCallbackProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkClientCallbackProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkDeviceInfoProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkDeviceInfoProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/StreamLatencyStateMachine.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/StreamLatencyStateMachine.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/ConnectionStaleStateMachine.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/ConnectionStaleStateMachine.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStaticCredentialProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStaticCredentialProvider.cpp)

include_directories(${KINESIS_VIDEO_PIC_SRC})
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include/com/amazonaws/kinesis/video/client)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include/com/amazonaws/kinesis/video/common)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include/com/amazonaws/kinesis/video/heap)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/trace/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include/com/amazonaws/kinesis/video/mkvgen)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include/com/amazonaws/kinesis/video/view)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/utils/include)

include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include/)
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include/gstreamer-1.0)
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include/glib-2.0)
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/glib-2.54.2)
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/glib-2.0/include)

include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/opensource/jsoncpp)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/tst)

link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib)
link_directories(${KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR}/kinesis-video-native-build)

add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})

add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)


add_executable(gstkvsplugintest ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvstest.cpp)

target_include_directories(gstkvssink PRIVATE ${GST_INCLUDE_DIRS})

target_link_libraries(gstkvssink
        producer
        log4cplus
        dl
        gstbase-1.0
        gstreamer-1.0
        gstapp-1.0
        gobject-2.0
        glib-2.0)

target_link_libraries(kvs_producer_plugin_demo
        gstreamer-1.0
        gobject-2.0
        glib-2.0)

target_link_libraries(kvs_producer_plugin_rtsp_demo
        gstreamer-1.0
        gobject-2.0
        glib-2.0)

target_link_libraries(gstkvsplugintest
        gstcheck-1.0
        gstreamer-1.0
        gobject-2.0
        glib-2.0)
