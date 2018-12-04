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

if(DEFINED ENV{PLUGIN_PRJ_DIR})
    set(PLUGIN_PRJ_DIR $ENV{PLUGIN_PRJ_DIR})
else()
    set(PLUGIN_PRJ_DIR "../kinesis-video-gstreamer-plugin")
endif()

if (DEFINED ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
    set(KINESIS_VIDEO_GST_PLUGIN_SRC $ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
else ()
    set(KINESIS_VIDEO_GST_PLUGIN_SRC "${PLUGIN_PRJ_DIR}/plugin-src")
endif ()

if (DEFINED ENV{KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC})
    set(KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC $ENV{KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC})
else ()
    set(KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC "${PLUGIN_PRJ_DIR}/demo-src")
endif ()

if(DEFINED ENV{JAVA_HOME})
    set(JAVA_HOME $ENV{JAVA_HOME})
else()
    set(JAVA_HOME "C:/Program Files/Java/jdk1.8.0_171")
endif()

message(STATUS "System name is ${CMAKE_SYSTEM_NAME}")

STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER_CASE)

message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_SRC}")
message(STATUS "Kinesis Video JNI path is ${KINESIS_VIDEO_PRODUCER_JNI_SRC}")
message(STATUS "Kinesis Video Gstreamer demp app path is ${KINESIS_VIDEO_GST_DEMO_SRC}")
message(STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

message(STATUS "Configuration ${CMAKE_GENERATOR}")

if(CMAKE_SIZEOF_VOID_P STREQUAL 4)
    message(STATUS "Bitness 32 bits")
	set(KINESIS_VIDEO_BUILD_BITNESS "x86")
	set(KINESIS_VIDEO_BUILD_BITNESS_TYPE "Win32")
	set(KINESIS_VIDEO_BUILD_BITNESS_NAME "x86")
elseif(CMAKE_SIZEOF_VOID_P STREQUAL 8)
	message(STATUS "Bitness 64 bits")
	set(KINESIS_VIDEO_BUILD_BITNESS "x64")
	set(KINESIS_VIDEO_BUILD_BITNESS_TYPE "x64")
	set(KINESIS_VIDEO_BUILD_BITNESS_NAME "x86_64")
else()
	message(FATAL_ERROR "Unknown bitness")
endif()

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

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(EXE_LIBRARIES ${EXE_LIBRARIES} rt)
endif()

message(STATUS "exe libraries ${EXE_LIBRARIES}")

set(LIBRARY_PATH ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
  endif()
  set(LIBRARY_PATH $ENV{MINGW_PREFIX})
  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -D_HAS_ITERATOR_DEBUGGING=0)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

if(NOT BUILD_JNI_ONLY AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
  find_package(PkgConfig REQUIRED)

  pkg_check_modules (CRYPTO       REQUIRED   libcrypto)
  pkg_check_modules (CURL         REQUIRED   libcurl)
  pkg_check_modules (LOG4CPLUS    REQUIRED   log4cplus=1.2.0)
  pkg_check_modules (SSL          REQUIRED   libssl)

  if (NOT BUILD_PRODUCER_ONLY)
    pkg_check_modules (GLIB2        REQUIRED   glib-2.0)
    pkg_check_modules (GST          REQUIRED   gstreamer-1.0)
    pkg_check_modules (GST_APP      REQUIRED   gstreamer-app-1.0)
    pkg_check_modules (GOBJ2        REQUIRED   gobject-2.0)
    pkg_check_modules (GST_CHECK    REQUIRED   gstreamer-check-1.0)
    pkg_check_modules (GST_BASE     REQUIRED   gstreamer-base-1.0)
  endif()

  if (NOT NO_UNIT_TEST)
    find_library(GTEST NAMES gtest REQUIRED HINTS ${LIBRARY_PATH} PATH_SUFFIXES lib)
    find_library(GTEST_MAIN NAMES gtest_main REQUIRED HINTS ${LIBRARY_PATH} PATH_SUFFIXES lib)
  endif()

  set(OPEN_SRC_INCLUDE_DIRS
          ${GLIB2_INCLUDE_DIRS}
          ${CRYPTO_INCLUDE_DIRS}
          ${CURL_INCLUDE_DIRS}
          ${GST_APP_INCLUDE_DIRS}
          ${GST_INCLUDE_DIRS}
          ${LOG4CPLUS_INCLUDE_DIRS}
          ${GOBJ2_INCLUDE_DIRS}
          ${SSL_INCLUDE_DIRS}
          ${GTEST_INCLUDE_DIRS}
          ${GTEST_MAIN_INCLUDE_DIRS}
          ${GST_CHECK_INCLUDE_DIRS}
          ${GST_BASE_INCLUDE_DIRS})

  set(OPEN_SRC_LIBRARY_DIRS
          ${GLIB2_LIBRARY_DIRS}
          ${CRYPTO_LIBRARY_DIRS}
          ${CURL_LIBRARY_DIRS}
          ${GST_APP_LIBRARY_DIRS}
          ${GST_LIBRARY_DIRS}
          ${LOG4CPLUS_LIBRARY_DIRS}
          ${GOBJ2_LIBRARY_DIRS}
          ${SSL_LIBRARY_DIRS}
          ${GTEST_LIBRARY_DIRS}
          ${GTEST_MAIN_LIBRARY_DIRS}
          ${GST_CHECK_LIBRARY_DIRS}
          ${GST_BASE_LIBRARY_DIRS})

  set(PRODUCER_LIBRARIES
          ${CRYPTO_LIBRARIES}
          ${SSL_LIBRARIES}
          ${CURL_LIBRARIES}
          ${LOG4CPLUS_LIBRARIES}
          ${GOBJ2_LIBRARIES})

  set(GST_DEMO_LIBRARIES
          ${GLIB2_LIBRARIES}
          ${GST_APP_LIBRARIES}
          ${GST_LIBRARIES}
          ${GOBJ2_LIBRARIES}
          ${EXE_LIBRARIES})

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
endif()

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
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Thread.cpp
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
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Version.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Version.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/GetTime.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/GetTime.cpp
	${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/CredentialProviderUtil.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/CredentialProviderUtil.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/IotCertCredentialProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/IotCertCredentialProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/RotatingCredentialProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/RotatingCredentialProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/RotatingStaticCredentialProvider.h
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/RotatingStaticCredentialProvider.cpp
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
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Thread.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/DoubleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/HashTable.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/IntegerToString.cpp
        #${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/main.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/SingleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StackQueue.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StringToInteger.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/UtilTestFixture.cpp
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

set(GST_PLUGIN_SOURCE_FILES
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.h
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
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/opensource/jsoncpp)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/tst)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers)
include_directories(${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/include/)
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(/System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers)
include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/${CMAKE_SYSTEM_NAME_LOWER_CASE})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  include_directories("${JAVA_HOME}/include/win32")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/curl-7.60.0/builds/libcurl-vc15-${KINESIS_VIDEO_BUILD_BITNESS}-release-static-sspi-winssl/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/gstreamer-1.0")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/glib-2.0")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/lib/glib-2.0/include")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  macro(FORCE_MSVS_CRT_TYPE)
    if(MSVC)
      set(variables
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
      )

      if(${MSVS_CRT_TYPE} STREQUAL "" OR ${MSVS_CRT_TYPE} STREQUAL "static")
        message(STATUS "Using static CRT")
        foreach(variable ${variables})
          if(${variable} MATCHES "/MD")
	    if("Debug" STREQUAL "${CMAKE_BUILD_TYPE}" OR "Debug" STREQUAL "${CMAKE_CONFIGURATION_TYPES}")
              string(REGEX REPLACE "/MD" "/MTd" ${variable} "${${variable}}")
            else()
              string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
            endif()
          endif()
        endforeach()
      else()
        message(STATUS "Using dynamic CRT")
        foreach(variable ${variables})
          if(${variable} MATCHES "/MT")
            if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
              string(REGEX REPLACE "/MT" "/MDd" ${variable} "${${variable}}")
            else()
              string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
            endif()
          endif()
        endforeach()
      endif()
    endif()
  endmacro()

  set(MSVS_CRT_TYPE "static")
  FORCE_MSVS_CRT_TYPE()

  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -D_HAS_ITERATOR_DEBUGGING=0)

  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib)
  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/curl-7.60.0/builds/libcurl-vc15-${KINESIS_VIDEO_BUILD_BITNESS}-release-static-sspi-winssl/lib)
  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/lib)

  link_directories(${OPEN_SRC_LIBRARY_DIRS})
  add_library(producer STATIC ${PRODUCER_SOURCE_FILES})
  add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI} ${PIC_SOURCE_FILES})
  add_executable(start ${TST_PRODUCER_SOURCE_FILES})
  add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})
  add_executable(kinesis_video_gstreamer_sample_rtsp_app ${RTSP_DEMO_APP})

  add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})

  add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
  add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)

  target_include_directories(kinesis_video_gstreamer_sample_app PRIVATE ${GST_INCLUDE_DIRS})
  target_include_directories(kinesis_video_gstreamer_sample_rtsp_app PRIVATE ${GST_INCLUDE_DIRS})

  target_link_libraries(KinesisVideoProducerJNI)


  target_link_libraries(start
          producer)

  target_link_libraries(producer
          ws2_32
          crypt32
          Wldap32
          Normaliz
          libcurl_a)

  target_link_libraries(kinesis_video_gstreamer_sample_app
          producer
          gstreamer-1.0
          gstapp-1.0
          gobject-2.0
          glib-2.0)

  target_link_libraries(kinesis_video_gstreamer_sample_rtsp_app
          producer
          gstreamer-1.0
          gstapp-1.0
          gobject-2.0
          glib-2.0)

  target_link_libraries(producer
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/msvc10/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/bin.Release/log4cplusS.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libssl.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libcrypto.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib)

  target_link_libraries(gstkvssink
          producer
          gstbase-1.0
          gstreamer-1.0
          gstapp-1.0
          gobject-2.0
          glib-2.0)

  target_link_libraries(kvs_producer_plugin_demo
          producer
          gstreamer-1.0
          gobject-2.0
          glib-2.0)

  target_link_libraries(kvs_producer_plugin_rtsp_demo
          gstreamer-1.0
          gobject-2.0
          glib-2.0)
else()
  link_directories(${OPEN_SRC_LIBRARY_DIRS})

  add_library(producer SHARED ${PRODUCER_SOURCE_FILES})
  target_link_libraries(producer
          ${PRODUCER_LIBRARIES}
          ${GTEST}
          ${GTEST_MAIN})

  add_library(KinesisVideoProducerJNI SHARED EXCLUDE_FROM_ALL ${PRODUCER_SOURCE_FILES_JNI} ${PIC_SOURCE_FILES})
  target_link_libraries(KinesisVideoProducerJNI)

  add_executable(start ${TST_PRODUCER_SOURCE_FILES})
  target_link_libraries(start
          producer
          Threads::Threads
          ${CMAKE_DL_LIBS}
          ${LOG4CPLUS_LIBRARIES})

  add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})
  target_link_libraries(kinesis_video_gstreamer_sample_app
          producer
          ${GST_DEMO_LIBRARIES})

  add_executable(kinesis_video_gstreamer_sample_rtsp_app ${RTSP_DEMO_APP})
  target_link_libraries(kinesis_video_gstreamer_sample_rtsp_app
          producer
          ${GST_DEMO_LIBRARIES})

  add_executable(kinesis_video_gstreamer_sample_multistream_app ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_multistream_app.cpp)
  target_link_libraries(kinesis_video_gstreamer_sample_multistream_app
          producer
          ${GST_DEMO_LIBRARIES})

  add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})
  target_link_libraries(gstkvssink
          ${KVSSINK_LIBRARIES}
          producer)

  add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
  target_link_libraries(kvs_producer_plugin_demo
          ${KVSSINK_DEMO_LIBRARIES}
          producer)

  add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)
  target_link_libraries(kvs_producer_plugin_rtsp_demo
          ${KVSSINK_DEMO_LIBRARIES})

  add_executable(gstkvsplugintest ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvstest.cpp)
  target_link_libraries(gstkvsplugintest
         ${GST_CHECK_LIBRARIES}
         ${KVSSINK_DEMO_LIBRARIES})
endif()
