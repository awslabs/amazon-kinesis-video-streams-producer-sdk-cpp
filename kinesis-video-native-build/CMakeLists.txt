cmake_minimum_required(VERSION 3.2)

project(start)

set(EXEC_NAME start)

set(CMAKE_CXX_STANDARD 11)

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

if(DEFINED ENV{KINESIS_VIDEO_C_PRODUCER_SRC})
    set(KINESIS_VIDEO_C_PRODUCER_SRC $ENV{KINESIS_VIDEO_C_PRODUCER_SRC})
else()
    set(KINESIS_VIDEO_C_PRODUCER_SRC "../kinesis-video-c-producer")
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
endif()

message(STATUS "System name is ${CMAKE_SYSTEM_NAME}")

STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER_CASE)

message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_SRC}")
message(STATUS "Kinesis Video JNI path is ${KINESIS_VIDEO_PRODUCER_JNI_SRC}")
message(STATUS "Kinesis Video Gstreamer demp app path is ${KINESIS_VIDEO_GST_DEMO_SRC}")
message(STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

message(STATUS "Configuration ${CMAKE_GENERATOR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

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

# For Linux and Pi include rt lib as well.
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(EXE_LIBRARIES ${EXE_LIBRARIES} rt)
endif()

message(STATUS "exe libraries ${EXE_LIBRARIES}")

set(LIBRARY_PATH ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
    set(LIBRARY_PATH $ENV{MINGW_PREFIX})
  endif()
  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -D_HAS_ITERATOR_DEBUGGING=0)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

if (BUILD_JNI)
  find_package(JNI REQUIRED)
endif()

if (CONSTRAINED_DEVICE)
  add_definitions(-DCONSTRAINED_DEVICE)
endif()

# get dependencies
if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
  find_package(PkgConfig REQUIRED)

  if (BUILD_PRODUCER)
    if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND USE_SYS_LIBRARIES)
      set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif()

    find_package(Log4cplus)
    if (NOT LOG4CPLUS_FOUND)
      message("Log4cplus is not found. Only c producer will be built. ${Log4cplus}")
    endif()
    find_package(OpenSSL    REQUIRED)
    find_package(CURL       REQUIRED)
    find_library(Jsmn
            NAMES jsmn
            REQUIRED
            PATHS   ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib
            NO_DEFAULT_PATH)
  endif()

  if (BUILD_GST_ARTIFACT)
    if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND USE_SYS_LIBRARIES)
      set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/libffi/lib/pkgconfig")
      find_library(Intl "intl" REQUIRED PATHS "/usr/local/opt/gettext/lib")
      link_directories("/usr/local/opt/gettext/lib")
    endif()

    pkg_check_modules (GLIB2        REQUIRED   glib-2.0)
    pkg_check_modules (GST          REQUIRED   gstreamer-1.0)
    pkg_check_modules (GST_APP      REQUIRED   gstreamer-app-1.0)
    pkg_check_modules (GOBJ2        REQUIRED   gobject-2.0)
    pkg_check_modules (GST_CHECK    REQUIRED   gstreamer-check-1.0)
    pkg_check_modules (GST_BASE     REQUIRED   gstreamer-base-1.0)
  endif()

  if (BUILD_TEST)
    # in mingw, gtest is pre-built. Otherwise it's installed in ${KINESIS_VIDEO_OPEN_SOURCE_SRC}
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      set(GTEST_ROOT ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)
    endif()
    find_package(GTest       REQUIRED)
  endif()

  set(OPEN_SRC_INCLUDE_DIRS
          ${GLIB2_INCLUDE_DIRS}
          ${CURL_INCLUDE_DIRS}
          ${GST_APP_INCLUDE_DIRS}
          ${GST_INCLUDE_DIRS}
          ${LOG4CPLUS_INCLUDE_DIR}
          ${GOBJ2_INCLUDE_DIRS}
          ${OPENSSL_INCLUDE_DIR}
          ${GST_CHECK_INCLUDE_DIRS}
          ${GST_BASE_INCLUDE_DIRS})

  set(OPEN_SRC_LIBRARY_DIRS
          ${GLIB2_LIBRARY_DIRS}
          ${CURL_LIBRARY_DIRS}
          ${GST_APP_LIBRARY_DIRS}
          ${GST_LIBRARY_DIRS}
          ${LOG4CPLUS_LIBRARY_DIRS}
          ${GOBJ2_LIBRARY_DIRS}
          ${GST_CHECK_LIBRARY_DIRS}
          ${GST_BASE_LIBRARY_DIRS})

  set(PRODUCER_LIBRARIES
          ${OPENSSL_CRYPTO_LIBRARY}
          ${OPENSSL_SSL_LIBRARY}
          ${CURL_LIBRARIES}
          ${LOG4CPLUS_LIBRARIES})

  set(GST_DEMO_LIBRARIES
          ${GST_LIBRARIES}
          ${GLIB2_LIBRARIES}
          ${GST_APP_LIBRARIES}
          ${GOBJ2_LIBRARIES}
          ${EXE_LIBRARIES})

  set(KVSSINK_LIBRARIES
          ${GST_LIBRARIES}
          ${GLIB2_LIBRARIES}
          ${GST_APP_LIBRARIES}
          ${GOBJ2_LIBRARIES}
          ${GST_BASE_LIBRARIES})

  set(KVSSINK_DEMO_LIBRARIES
          ${GLIB2_LIBRARIES}
          ${GST_LIBRARIES}
          ${GOBJ2_LIBRARIES}
          ${EXE_LIBRARIES})
endif()

set(PIC_SOURCE_FILES
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/AuthIntegration.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Callbacks.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Client.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/ClientEvent.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/ClientState.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/InputValidator.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/State.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/AckParser.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/Stream.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/StreamEvent.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/StreamState.c
        ${KINESIS_VIDEO_PIC_SRC}/src/client/src/FrameOrderCoordinator.c

        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/AivHeap.c
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Common.c
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/Heap.c
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/HybridHeap.c
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/src/SystemHeap.c

        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/MkvGenerator.c
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/MkvStatics.c
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/NalAdapter.c
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src/SpsParser.c

        ${KINESIS_VIDEO_PIC_SRC}/src/trace/src/TraceProfiler.c

        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Allocators.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Base64.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/BitField.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/BitReader.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Directory.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/DoubleLinkedList.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/DynamicLibrary.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Endianness.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/FileIo.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/HashTable.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Hex.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Mutex.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/SingleLinkedList.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/StackQueue.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/String.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Thread.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Time.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Logger.c
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/src/Version.c

	${KINESIS_VIDEO_PIC_SRC}/src/view/src/ContentView.c)

set(PRODUCER_SOURCE_FILES
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoProducer.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/KinesisVideoStream.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/Auth.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CallbackProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultCallbackProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/CachingEndpointOnlyCallbackProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/DefaultDeviceInfoProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamDefinition.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/StreamTags.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/GetTime.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/IotCertCredentialProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers/RotatingCredentialProvider.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/src/common/PutFrameHelper.cpp)

set(C_PRODUCER_SOURCE_FILES
        ${PIC_SOURCE_FILES}
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Auth.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/AwsV4Signer.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/CallbacksProvider.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/CurlApiCallbacks.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/DeviceInfoProvider.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/StreamInfoProvider.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Producer.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Request.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Response.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Util.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/FileAuthCallbacks.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/StaticAuthCallbacks.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/IotAuthCallback.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/ConnectionStaleStateMachine.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/ContinuousRetryStreamCallbacks.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/StreamLatencyStateMachine.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/StreamCallbacksProvider.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/FileLoggerPlatformCallbackProvider.c
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Version.c)

set(PIC_TEST_SOURCE_FILES
        ${PIC_SOURCE_FILES}
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/AcksFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/CallbacksAndPressuresFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/IntermittentProducerFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/MockConsumer.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/MockProducer.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StateTransitionFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiFunctionalityScenarioTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiServiceCallsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamDeviceTagsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamingSession.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamParallelTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamPutGetTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamRecoveryFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamStateTransitionsTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamStoppingFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamTokenRotationTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/TokenRotationFunctionalityTest.cpp

        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HybridHeapTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBCpdNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AvccNalAdapterTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/SpsParserTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitField.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitReader.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Directory.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Thread.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/DoubleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/HashTable.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/IntegerToString.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/SingleLinkedList.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StackQueue.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StringToInteger.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/UtilTestFixture.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiTest.cpp
        ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewTestFixture.cpp)

set(PRODUCER_TEST_SOURCE_FILES
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerTestFixture.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/main.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerApiTest.cpp
        ${KINESIS_VIDEO_PRODUCER_SRC}/tst/ProducerFunctionalityTest.cpp)

set(C_PRODUCER_TEST_SOURCE_FILES
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/AuthCallbackTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/CallbacksProviderApiFunctionalityTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/RotatingStaticAuthCallbacks.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/CallbacksProviderApiTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/ProducerClientBasicTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/ProducerBasicUsageTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/ProducerFunctionalityTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/ProducerTestFixture.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/ProducerApiTest.cpp
        ${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/FileLoggerFunctionalityTest.cpp)

set(PRODUCER_SOURCE_FILES_JNI
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/KinesisVideoClientWrapper.cpp
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/NativeProducerInterface.cpp
        ${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/Parameters.cpp)

set(GST_DEMO_APP
        ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_app.cpp)

set(GST_PLUGIN_SOURCE_FILES
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvssink.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkStreamCallbackProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkClientCallbackProvider.cpp
        ${KINESIS_VIDEO_GST_PLUGIN_SRC}/KvsSinkDeviceInfoProvider.cpp
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
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/tst)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src/credential-providers)
include_directories(${KINESIS_VIDEO_PRODUCER_SRC}/src/common)
include_directories(${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/include/)
include_directories(${KINESIS_VIDEO_C_PRODUCER_SRC}/src/include)
include_directories(${KINESIS_VIDEO_C_PRODUCER_SRC})
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include)
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(${JNI_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  include_directories("${JAVA_HOME}/include/win32")
  include_directories("${JAVA_HOME}/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/curl-7.60.0/builds/libcurl-vc15-${KINESIS_VIDEO_BUILD_BITNESS}-release-static-sspi-winssl/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/gstreamer-1.0")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/glib-2.0")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/lib/glib-2.0/include")
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/jsmn-1.0.0")
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
  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/jsmn-1.0.0/Release)

  link_directories(${OPEN_SRC_LIBRARY_DIRS})

  add_library(cproducer MODULE SHARED ${C_PRODUCER_SOURCE_FILES})
  add_executable(kinesis_video_cproducer_video_only_sample ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsVideoOnlyStreamingSample.c)

  add_library(producer STATIC ${PRODUCER_SOURCE_FILES})
  add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI} ${PIC_SOURCE_FILES})
  add_executable(pic_test ${PIC_TEST_SOURCE_FILES})
  add_executable(producer_test ${PRODUCER_TEST_SOURCE_FILES} ${PIC_SOURCE_FILES})
  add_executable(cproducer_test ${C_PRODUCER_TEST_SOURCE_FILES} ${C_PRODUCER_SOURCE_FILES})

  add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})

  add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})

  add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
  add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)

  target_include_directories(kinesis_video_gstreamer_sample_app PRIVATE ${GST_INCLUDE_DIRS})

  target_link_libraries(KinesisVideoProducerJNI)


  target_link_libraries(pic_test
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib)

  target_link_libraries(producer_test
          producer
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib)

  target_link_libraries(cproducer_test
          cproducer
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib)

  target_link_libraries(kinesis_video_gstreamer_sample_app
          producer
          gstreamer-1.0
          gstapp-1.0
          gobject-2.0
          glib-2.0)

  target_link_libraries(producer
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/msvc10/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/bin.Release/log4cplusS.lib
          cproducer)

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

  target_link_libraries(cproducer
          ws2_32
          crypt32
          Wldap32
          Normaliz
          libcurl_a
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libssl.lib
          optimized ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libcrypto.lib
          jsmn)

  target_link_libraries(kinesis_video_cproducer_video_only_sample cproducer)
else()
  if ((NOT USE_SYS_LIBRARIES) AND BUILD_GST_ARTIFACT)
    link_directories(${OPEN_SRC_LIBRARY_DIRS})
  endif()

  if (BUILD_PRODUCER)
    add_library(cproducer SHARED ${C_PRODUCER_SOURCE_FILES})
    target_link_libraries(cproducer
          ${PRODUCER_LIBRARIES}
          ${Jsmn})

    add_executable(kinesis_video_cproducer_video_only_sample ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsVideoOnlyStreamingSample.c)
    target_link_libraries(kinesis_video_cproducer_video_only_sample
            cproducer
            ${EXE_LIBRARIES})

    if (LOG4CPLUS_FOUND)
      add_library(producer SHARED ${PRODUCER_SOURCE_FILES})
      target_link_libraries(producer
            ${PRODUCER_LIBRARIES}
            cproducer)
    endif()
  endif()

  if (BUILD_JNI)
    add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI} ${PIC_SOURCE_FILES})
    target_link_libraries(KinesisVideoProducerJNI)
  endif()

  if (BUILD_TEST)
    add_executable(pic_test ${PIC_TEST_SOURCE_FILES})
    target_link_libraries(pic_test
            ${GTEST_BOTH_LIBRARIES}
            ${EXE_LIBRARIES})
    add_executable(producer_test ${PRODUCER_TEST_SOURCE_FILES})
    target_link_libraries(producer_test
            producer
            ${GTEST_BOTH_LIBRARIES}
            ${EXE_LIBRARIES})
    add_executable(cproducer_test ${C_PRODUCER_TEST_SOURCE_FILES})
    target_link_libraries(cproducer_test
            cproducer
            ${GTEST_BOTH_LIBRARIES}
            ${EXE_LIBRARIES})
  endif()

  if (BUILD_GST_ARTIFACT)
    add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})
    target_link_libraries(kinesis_video_gstreamer_sample_app
            producer
            ${GST_DEMO_LIBRARIES}
            ${Intl})

    add_executable(kinesis_video_gstreamer_sample_multistream_app ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_multistream_app.cpp)
    target_link_libraries(kinesis_video_gstreamer_sample_multistream_app
            producer
            ${GST_DEMO_LIBRARIES}
            ${Intl})

    add_executable(kinesis_video_gstreamer_audio_video_sample_app ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_audio_video_sample_app.cpp)
    target_link_libraries(kinesis_video_gstreamer_audio_video_sample_app
            producer
            ${GST_DEMO_LIBRARIES}
            ${Intl})

    add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})
    target_link_libraries(gstkvssink
            ${KVSSINK_LIBRARIES}
            producer
            ${Intl})

    add_executable(kvs_producer_plugin_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_demo.cpp)
    target_link_libraries(kvs_producer_plugin_demo
            ${KVSSINK_DEMO_LIBRARIES}
            producer
            ${Intl})

    add_executable(kvs_producer_plugin_rtsp_demo ${KINESIS_VIDEO_GST_PLUGIN_DEMO_SRC}/kvs_producer_plugin_rtsp_demo.cpp)
    target_link_libraries(kvs_producer_plugin_rtsp_demo
            ${KVSSINK_DEMO_LIBRARIES}
            ${Intl})

    add_executable(gstkvsplugintest EXCLUDE_FROM_ALL ${KINESIS_VIDEO_GST_PLUGIN_SRC}/gstkvstest.cpp)
    target_link_libraries(gstkvsplugintest
           ${GST_CHECK_LIBRARIES}
           ${KVSSINK_DEMO_LIBRARIES}
           ${Intl})
  endif()
endif()
