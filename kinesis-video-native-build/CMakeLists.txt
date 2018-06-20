cmake_minimum_required(VERSION 3.2)

project(start)

set(EXEC_NAME start)

set(CMAKE_CXX_STANDARD 14)

if(DEFINED ENV{KINESIS_VIDEO_PIC_SRC})
    set(KINESIS_VIDEO_PIC_SRC $ENV{KINESIS_VIDEO_PIC_SRC})
else()
    set(KINESIS_VIDEO_PIC_SRC "../kinesis-video-pic")
endif()

if(DEFINED ENV{KINESIS_VIDEO_PRODUCER_SRC})
    set(KINESIS_VIDEO_PRODUCER_SRC $ENV{KINESIS_VIDEO_PRODUCER_SRC})
else()
    set(KINESIS_VIDEO_PRODUCER_SRC "../kinesis-video-producer")
endif()

if(DEFINED ENV{KINESIS_VIDEO_PRODUCER_JNI_SRC})
    set(KINESIS_VIDEO_PRODUCER_JNI_SRC $ENV{KINESIS_VIDEO_PRODUCER_JNI_SRC})
else()
    set(KINESIS_VIDEO_PRODUCER_JNI_SRC "../kinesis-video-producer-jni")
endif()

if(DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
    set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
else()
    set(KINESIS_VIDEO_OPEN_SOURCE_SRC "${PROJECT_BINARY_DIR}/downloads")
endif()

if(DEFINED ENV{KINESIS_VIDEO_GST_DEMO_SRC})
    set(KINESIS_VIDEO_GST_DEMO_SRC $ENV{KINESIS_VIDEO_GST_DEMO_SRC})
else()
    set(KINESIS_VIDEO_GST_DEMO_SRC "../kinesis-video-gst-demo")
endif()

set(JAVA_HOME $ENV{JAVA_HOME})
message(STATUS "System name is ${CMAKE_SYSTEM_NAME}")

STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER_CASE)

message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_SRC}")
message(STATUS "Kinesis Video JNI path is ${KINESIS_VIDEO_PRODUCER_JNI_SRC}")
message(STATUS "Kinesis Video Gstreamer demp app path is ${KINESIS_VIDEO_GST_DEMO_SRC}")
message(STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

STRING(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# Uncomment below line for debug heap
#add_definitions(-DHEAP_DEBUG)

# Uncomment below line for very verbose logging
#add_definitions(-DLOG_STREAMING)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules (GLIB2        REQUIRED   glib-2.0)
pkg_check_modules (CRYPTO       REQUIRED   libcrypto)
pkg_check_modules (CURL         REQUIRED   libcurl)
pkg_check_modules (GST          REQUIRED   gstreamer-1.0)
pkg_check_modules (GST_APP      REQUIRED   gstreamer-app-1.0)
pkg_check_modules (LOG4CPLUS    REQUIRED   log4cplus)
pkg_check_modules (GOBJ2        REQUIRED   gobject-2.0)
pkg_check_modules (SSL          REQUIRED   libssl)
find_library(GTEST gtest REQUIRED HINTS ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local PATH_SUFFIXES lib)
find_library(GTEST_MAIN gtest_main REQUIRED HINTS ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local PATH_SUFFIXES lib)

set(OPEN_SRC_INCLUDE_DIRS
        ${GLIB2_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_INCLUDE_DIRS}
        ${LOG4CPLUS_INCLUDE_DIRS}
        ${GOBJ2_INCLUDE_DIRS}
        ${SSL_INLCUDE_DIRS})

set(OPEN_SRC_LIBRARY_DIRS
        ${GLIB2_LIBRARY_DIRS}
        ${CRYPTO_LIBRARY_DIRS}
        ${CURL_LIBRARY_DIRS}
        ${GST_APP_LIBRARY_DIRS}
        ${GST_LIBRARY_DIRS}
        ${LOG4CPLUS_LIBRARY_DIRS}
        ${GOBJ2_LIBRARY_DIRS}
        ${SSL_LIBRARY_DIRS})

set(PRODUCER_LIBRARIES
        ${CRYPTO_LIBRARIES}
        ${SSL_LIBRARIES}
        ${CURL_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${GOBJ2_LIBRARIES})

set(GST_DEMO_LIBRARIES
        ${GLIB2_LIBRARIES}
        pthread
        ${GST_APP_LIBRARIES}
        ${GST_LIBRARIES}
        ${GOBJ2_LIBRARIES})

set(PIC_SOURCE_FILES
        ${KINESIS_VIDEO_PIC_SRC}/src/client/include/com/amazonaws/kinesis/video/client/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/AuthIntegration.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Callbacks.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Client.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/ClientEvent.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/ClientState.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Include_i.h
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/InputValidator.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/InputValidator.h
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/State.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/State.h
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/AckParser.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Stream.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Stream.h
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/StreamEvent.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/StreamState.cpp

        ${KINESIS_VIDEO_PIC_SRC}/src/common/include/com/amazonaws/kinesis/video/common/CommonDefs.h
        ${KINESIS_VIDEO_PIC_SRC}/src/common/include/com/amazonaws/kinesis/video/common/PlatformUtils.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/include/com/amazonaws/kinesis/video/heap/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/AivHeap.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/AivHeap.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Common.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Common.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Heap.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/HybridHeap.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/HybridHeap.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Include_i.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/SystemHeap.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/SystemHeap.h

        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include/com/amazonaws/kinesis/video/mkvgen/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/Include_i.h
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/MkvGenerator.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/MkvStatics.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/NalAdapter.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/SpsParser.cpp

        ${KINESIS_VIDEO_PIC_SRC}/src/trace/include/com/amazonaws/kinesis/video/trace/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/src/Include_i.h
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/src/TraceProfiler.cpp

        ${KINESIS_VIDEO_PIC_SRC}/src/utils/include/com/amazonaws/kinesis/video/utils/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Allocators.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Base64.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/BitField.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/BitReader.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Directory.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/dlfcn_win_stub.h
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/DoubleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/DynamicLibrary.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Endianness.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/FileIo.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/HashTable.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Hex.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Include_i.h
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Mutex.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/SingleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/StackQueue.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/String.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Threads.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Time.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Logger.cpp

        ${KINESIS_VIDEO_PIC_SRC}/src/view/include/com/amazonaws/kinesis/video/view/Include.h
        ${KINESIS_VIDEO_PIC_SRC}/src/view/src/ContentView.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/src/Include_i.h)

set(PRODUCER_SOURCE_FILES
        ${PIC_SOURCE_FILES}
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoProducer.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoProducer.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoStream.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoStream.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Auth.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Auth.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/AwsV4Signer.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/AwsV4Signer.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CallbackProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CallbackProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/ClientCallbackProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CurlCallManager.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamCallbackProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CurlCallManager.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultCallbackProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/OngoingStreamState.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/OngoingStreamState.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultCallbackProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultDeviceInfoProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultDeviceInfoProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DeviceInfoProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Logger.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/OpenSSLThreadCallbacks.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/OpenSSLThreadCallbacks.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Request.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Request.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Response.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Response.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamDefinition.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamTags.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamTags.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/ThreadSafeMap.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/opensource/jsoncpp/jsoncpp.cpp)

set(TST_PRODUCER_SOURCE_FILES
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamPutGetTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientTestFixture.h
        #${KINESIS_VIDEO_PIC_SRC}/src/client/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiServiceCallsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamDeviceTagsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamParallelTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamStateTransitionsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamTokenRotationTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapTestFixture.h
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HybridHeapTest.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/main.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenTestFixture.h
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBCpdNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AvccNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/SpsParserTest.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceTestFixture.h
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitField.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitReader.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Directory.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/DoubleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/HashTable.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/IntegerToString.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/SingleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StackQueue.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StringToInteger.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/view/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewTestFixture.h

        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerTestFixture.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerTestFixture.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/main.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerApiTest.cpp)

set(PRODUCER_SOURCE_FILES_JNI
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/KinesisVideoClientWrapper.cpp
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/NativeProducerInterface.cpp
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/Parameters.cpp)

set(GST_DEMO_APP
        ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_app.cpp)


set(RTSP_DEMO_APP
        ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_rtsp_app.cpp)

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
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/opensource/jsoncpp)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/tst)
include_directories(${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/include/)
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(/System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers)
include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/${CMAKE_SYSTEM_NAME_LOWER_CASE})

link_directories(${OPEN_SRC_LIBRARY_DIRS})
add_library(producer SHARED ${PRODUCER_SOURCE_FILES})
add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI} ${PIC_SOURCE_FILES})
add_executable(start ${TST_PRODUCER_SOURCE_FILES})
add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})
add_executable(kinesis_video_gstreamer_sample_rtsp_app ${RTSP_DEMO_APP})

target_link_libraries(KinesisVideoProducerJNI)

target_link_libraries(producer
        ${PRODUCER_LIBRARIES}
        ${GTEST_MAIN}
        ${GTEST})

target_link_libraries(start
        producer
        pthread)

target_link_libraries(kinesis_video_gstreamer_sample_app
        producer
        ${GST_DEMO_LIBRARIES})

target_link_libraries(kinesis_video_gstreamer_sample_rtsp_app
        producer
        ${GST_DEMO_LIBRARIES})