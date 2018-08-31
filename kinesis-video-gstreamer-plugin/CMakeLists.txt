
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

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

set(EXE_LIBRARIES Threads::Threads ${CMAKE_DL_LIBS})

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
    set(EXE_LIBRARIES ${EXE_LIBRARIES} rt)
endif ()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -D_HAS_ITERATOR_DEBUGGING=0)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules (GLIB2        REQUIRED    glib-2.0)
pkg_check_modules (GST          REQUIRED    gstreamer-1.0)
pkg_check_modules (GST_APP      REQUIRED    gstreamer-app-1.0)
pkg_check_modules (LOG4CPLUS    REQUIRED    log4cplus=1.2.0)
pkg_check_modules (GOBJ2        REQUIRED    gobject-2.0)
pkg_check_modules (GST_CHECK    REQUIRED    gstreamer-check-1.0)
pkg_check_modules (GST_BASE     REQUIRED    gstreamer-base-1.0)
pkg_check_modules (CURL         REQUIRED    libcurl)
pkg_check_modules (CRYPTO       REQUIRED    libcrypto)
find_library(PRODUCER producer REQUIRED HINTS ${KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR}/kinesis-video-native-build)
message(STATUS "producer ${CMAKE_SYSTEM_NAME}")

set(OPEN_SRC_INCLUDE_DIRS
        ${GLIB2_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_INCLUDE_DIRS}
        ${LOG4CPLUS_INCLUDE_DIRS}
        ${GOBJ2_INCLUDE_DIRS}
        ${GST_CHECK_INCLUDE_DIRS}
        ${GST_BASE_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS})

set(OPEN_SRC_LIBRARY_DIRS
        ${GLIB2_LIBRARY_DIRS}
        ${GST_APP_LIBRARY_DIRS}
        ${GST_LIBRARY_DIRS}
        ${LOG4CPLUS_LIBRARY_DIRS}
        ${GOBJ2_LIBRARY_DIRS}
        ${GST_CHECK_LIBRARY_DIRS}
        ${GST_BASE_LIBRARY_DIRS}
        ${CURL_LIBRARY_DIRS}
        ${CRYPTO_LIBRARY_DIRS})

set(KVSSINK_LIBRARIES
        ${GST_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${GOBJ2_LIBRARIES}
        ${GST_BASE_LIBRARIES}
        ${CURL_LIBRARIES}
        ${CRYPTO_LIBRARIES})

set(KVSSINK_DEMO_LIBRARIES
        ${GLIB2_LIBRARIES}
        ${GST_LIBRARIES}
        ${GOBJ2_LIBRARIES}
        ${EXE_LIBRARIES})

set(GST_PLUGIN_SOURCE_FILES
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.h
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
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStaticCredentialProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkIotCertCredentialProvider.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkIotCertCredentialProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/Util/KvsSinkUtil.h
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/Util/KvsSinkUtil.cpp)

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
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/utils/src)
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/opensource/jsoncpp)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/tst)

link_directories(${OPEN_SRC_LIBRARY_DIRS})

add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})

add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)

add_executable(gstkvsplugintest ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvstest.cpp)

target_include_directories(gstkvssink PRIVATE ${GST_INCLUDE_DIRS})

target_link_libraries(gstkvssink
        ${KVSSINK_LIBRARIES}
        ${PRODUCER})

target_link_libraries(kvs_producer_plugin_demo
        ${KVSSINK_DEMO_LIBRARIES}
        ${PRODUCER})

target_link_libraries(kvs_producer_plugin_rtsp_demo
        ${KVSSINK_DEMO_LIBRARIES})

target_link_libraries(gstkvsplugintest
       ${GST_CHECK_LIBRARIES}
       ${KVSSINK_DEMO_LIBRARIES})
