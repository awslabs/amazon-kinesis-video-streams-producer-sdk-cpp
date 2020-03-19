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

if(DEFINED ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
  set(KINESIS_VIDEO_GST_PLUGIN_SRC $ENV{KINESIS_VIDEO_GST_PLUGIN_SRC})
else()
  set(KINESIS_VIDEO_GST_PLUGIN_SRC "../kinesis-video-gstreamer-plugin")
endif()

if(DEFINED ENV{JAVA_HOME})
  set(JAVA_HOME $ENV{JAVA_HOME})
endif()

message(STATUS "System name is ${CMAKE_SYSTEM_NAME}")

string(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER_CASE)

message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_SRC}")
message(STATUS "Kinesis Video JNI path is ${KINESIS_VIDEO_PRODUCER_JNI_SRC}")
message(
  STATUS
    "Kinesis Video Gstreamer demp app path is ${KINESIS_VIDEO_GST_DEMO_SRC}")
message(
  STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

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

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# Uncomment below line for debug heap add_definitions(-DHEAP_DEBUG)

# Uncomment below line for very verbose logging add_definitions(-DLOG_STREAMING)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

set(LIBRARY_PATH ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
    set(CMAKE_MODULE_LINKER_FLAGS
        "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols")
    set(LIBRARY_PATH $ENV{MINGW_PREFIX})
  endif()
  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN
                  -D_HAS_ITERATOR_DEBUGGING=0)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS
                  -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

if(BUILD_JNI)
  find_package(JNI REQUIRED)
endif()

if(CONSTRAINED_DEVICE)
  add_definitions(-DCONSTRAINED_DEVICE)
endif()

# get dependencies
if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
  find_package(PkgConfig REQUIRED)

  if(BUILD_PRODUCER)
    if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND USE_SYS_LIBRARIES)
      set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif()

    find_package(Log4cplus)
    if(NOT LOG4CPLUS_FOUND)
      message(
        "Log4cplus is not found. Only c producer will be built. ${Log4cplus}")
    endif()
    find_package(OpenSSL REQUIRED)
    find_package(CURL REQUIRED)
    find_library(
      Jsmn
      NAMES jsmn REQUIRED
      PATHS ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib
      NO_DEFAULT_PATH)
  endif()

  if(BUILD_GST_ARTIFACT)
    if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND USE_SYS_LIBRARIES)
      set(ENV{PKG_CONFIG_PATH}
          "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/libffi/lib/pkgconfig")
      find_library(Intl "intl" REQUIRED PATHS "/usr/local/opt/gettext/lib")
      link_directories("/usr/local/opt/gettext/lib")
    endif()

    pkg_check_modules(GLIB2 REQUIRED glib-2.0)
    pkg_check_modules(GST REQUIRED gstreamer-1.0)
    pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
    pkg_check_modules(GOBJ2 REQUIRED gobject-2.0)
    pkg_check_modules(GST_CHECK REQUIRED gstreamer-check-1.0)
    pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
  endif()

  if(BUILD_TEST)
    # in mingw, gtest is pre-built. Otherwise it's installed in
    # ${KINESIS_VIDEO_OPEN_SOURCE_SRC}
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      set(GTEST_ROOT ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)
    endif()
    find_package(GTest REQUIRED)
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

  set(PRODUCER_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}
                         ${CURL_LIBRARIES} ${LOG4CPLUS_LIBRARIES})

  set(GST_DEMO_LIBRARIES ${GST_LIBRARIES} ${GLIB2_LIBRARIES}
                         ${GST_APP_LIBRARIES} ${GOBJ2_LIBRARIES})

  set(KVSSINK_LIBRARIES
      ${GST_LIBRARIES} ${GLIB2_LIBRARIES} ${GST_APP_LIBRARIES}
      ${GOBJ2_LIBRARIES} ${GST_BASE_LIBRARIES})

  set(KVSSINK_DEMO_LIBRARIES ${GLIB2_LIBRARIES} ${GST_LIBRARIES}
                             ${GOBJ2_LIBRARIES})
endif()

file(GLOB PIC_SOURCE_FILES "${KINESIS_VIDEO_PIC_SRC}/src/*/src/*.c")

file(GLOB_RECURSE PRODUCER_SOURCE_FILES
     "${KINESIS_VIDEO_PRODUCER_SRC}/src/*.cpp")

file(GLOB C_PRODUCER_SOURCE_FILES
     "${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/*.c")

file(GLOB C_PRODUCER_COMMON_SOURCE_FILES
     "${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Common/*.c")

file(GLOB C_PRODUCER_COMMON_CURL_SOURCE_FILES
     "${KINESIS_VIDEO_C_PRODUCER_SRC}/src/source/Common/Curl/*.c")

file(GLOB PIC_TEST_SOURCE_FILES "${KINESIS_VIDEO_PIC_SRC}/src/*/tst/*.cpp")

file(GLOB PRODUCER_TEST_SOURCE_FILES "${KINESIS_VIDEO_PRODUCER_SRC}/tst/*.cpp")

file(GLOB C_PRODUCER_TEST_SOURCE_FILES
     "${KINESIS_VIDEO_C_PRODUCER_SRC}/tst/*.cpp")

file(
  GLOB
  PRODUCER_SOURCE_FILES_JNI
  "${KINESIS_VIDEO_PRODUCER_JNI_SRC}/src/source/com/amazonaws/kinesis/video/producer/jni/*.cpp"
)

set(GST_DEMO_APP
    ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_app.cpp)

file(GLOB_RECURSE GST_PLUGIN_SOURCE_FILES
     "${KINESIS_VIDEO_GST_PLUGIN_SRC}/src/*.cpp")

include_directories(${KINESIS_VIDEO_PIC_SRC})
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include)
include_directories(
  ${KINESIS_VIDEO_PIC_SRC}/src/client/include/com/amazonaws/kinesis/video/client
)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include)
include_directories(
  ${KINESIS_VIDEO_PIC_SRC}/src/common/include/com/amazonaws/kinesis/video/common
)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include)
include_directories(
  ${KINESIS_VIDEO_PIC_SRC}/src/heap/include/com/amazonaws/kinesis/video/heap)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/trace/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/state/include)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include)
include_directories(
  ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include/com/amazonaws/kinesis/video/mkvgen
)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst)
include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include)
include_directories(
  ${KINESIS_VIDEO_PIC_SRC}/src/view/include/com/amazonaws/kinesis/video/view)
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
include_directories(${KINESIS_VIDEO_C_PRODUCER_SRC}/sample)
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include)
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(${JNI_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  include_directories("${JAVA_HOME}/include/win32")
  include_directories("${JAVA_HOME}/include")
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/include"
  )
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/include")
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/curl-7.60.0/builds/libcurl-vc15-${KINESIS_VIDEO_BUILD_BITNESS}-release-static-sspi-winssl/include"
  )
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/include")
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/gstreamer-1.0"
  )
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/include/glib-2.0"
  )
  include_directories(
    "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/lib/glib-2.0/include"
  )
  include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/jsmn-1.0.0")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND NOT "${CMAKE_CXX_COMPILER_ID}"
                                              STREQUAL "GNU")
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
          CMAKE_CXX_FLAGS_RELWITHDEBINFO)

      if(${MSVS_CRT_TYPE} STREQUAL "" OR ${MSVS_CRT_TYPE} STREQUAL "static")
        message(STATUS "Using static CRT")
        foreach(variable ${variables})
          if(${variable} MATCHES "/MD")
            if("Debug" STREQUAL "${CMAKE_BUILD_TYPE}"
               OR "Debug" STREQUAL "${CMAKE_CONFIGURATION_TYPES}")
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
  force_msvs_crt_type()

  add_definitions(-DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN
                  -DKVS_BUILD_WITH_CURL -D_HAS_ITERATOR_DEBUGGING=0)

  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib)
  link_directories(
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/curl-7.60.0/builds/libcurl-vc15-${KINESIS_VIDEO_BUILD_BITNESS}-release-static-sspi-winssl/lib
  )
  link_directories(
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/gstreamer/1.0/${KINESIS_VIDEO_BUILD_BITNESS_NAME}/lib
  )
  link_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/jsmn-1.0.0/Release)

  link_directories(${OPEN_SRC_LIBRARY_DIRS})

  add_library(
    cproducer MODULE
    SHARED ${C_PRODUCER_SOURCE_FILES} ${C_PRODUCER_COMMON_SOURCE_FILES}
    ${C_PRODUCER_COMMON_CURL_SOURCE_FILES})

  add_library(
    cproducer_static STATIC
    ${C_PRODUCER_SOURCE_FILES} ${C_PRODUCER_COMMON_SOURCE_FILES}
    ${C_PRODUCER_COMMON_CURL_SOURCE_FILES})

  add_executable(
    kinesis_video_cproducer_video_only_sample
    ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsVideoOnlyStreamingSample.c)

  add_library(producer STATIC ${PRODUCER_SOURCE_FILES})
  add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI}
                                             ${PIC_SOURCE_FILES})
  add_executable(pic_test ${PIC_TEST_SOURCE_FILES})
  add_executable(producer_test ${PRODUCER_TEST_SOURCE_FILES}
                               ${PIC_SOURCE_FILES})
  add_executable(cproducer_test ${C_PRODUCER_TEST_SOURCE_FILES})

  add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})

  add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})

  add_executable(
    kvs_producer_plugin_demo
    ${KINESIS_VIDEO_GST_PLUGIN_SRC}/demo/kvs_producer_plugin_demo.cpp)
  add_executable(
    kvs_producer_plugin_rtsp_demo
    ${KINESIS_VIDEO_GST_PLUGIN_SRC}/demo/kvs_producer_plugin_rtsp_demo.cpp)

  target_include_directories(kinesis_video_gstreamer_sample_app
                             PRIVATE ${GST_INCLUDE_DIRS})

  target_link_libraries(KinesisVideoProducerJNI)

  add_library(kvspic STATIC ${PIC_SOURCE_FILES})
  target_link_libraries(kvspic ${CMAKE_DL_LIBS} Threads::Threads)

  target_link_libraries(
    pic_test
    optimized
    kvspic
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib
  )

  target_link_libraries(
    producer_test
    producer
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib
  )

  target_link_libraries(
    cproducer_test
    cproducer_static
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest.lib
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/googletest-release-1.8.0/googletest/msvc/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/Release/gtest_main.lib
  )

  target_link_libraries(kinesis_video_gstreamer_sample_app producer
                        gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0)

  target_link_libraries(
    producer
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/log4cplus-1.2.1/msvc10/${KINESIS_VIDEO_BUILD_BITNESS_TYPE}/bin.Release/log4cplusS.lib
    cproducer)

  target_link_libraries(
    gstkvssink
    producer
    gstbase-1.0
    gstreamer-1.0
    gstapp-1.0
    gobject-2.0
    glib-2.0)

  target_link_libraries(kvs_producer_plugin_demo producer gstreamer-1.0
                        gobject-2.0 glib-2.0)

  target_link_libraries(kvs_producer_plugin_rtsp_demo gstreamer-1.0 gobject-2.0
                        glib-2.0)

  target_link_libraries(
    cproducer
    ws2_32
    crypt32
    Wldap32
    Normaliz
    libcurl_a
    optimized
    kvspic
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libssl.lib
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libcrypto.lib
    jsmn)

  target_link_libraries(
    cproducer_static
    ws2_32
    crypt32
    Wldap32
    Normaliz
    libcurl_a
    optimized
    kvspic
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libssl.lib
    optimized
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/openssl-OpenSSL_1_1_0f/OpenSSL/lib/libcrypto.lib
    jsmn)

  target_link_libraries(kinesis_video_cproducer_video_only_sample cproducer)
else()

  link_directories(${OPEN_SRC_LIBRARY_DIRS})

  add_library(kvspic STATIC ${PIC_SOURCE_FILES})

  if(UNIX AND NOT APPLE)
    # rt needed for clock_gettime
    target_link_libraries(kvspic ${CMAKE_DL_LIBS} Threads::Threads rt)
  else()
    target_link_libraries(kvspic ${CMAKE_DL_LIBS} Threads::Threads)
  endif()

  find_package(AWSSDK REQUIRED COMPONENTS monitoring events logs)

  add_definitions(-DKVS_BUILD_WITH_CURL)
  if(BUILD_PRODUCER)
    add_library(
      cproducer SHARED
      ${C_PRODUCER_SOURCE_FILES} ${C_PRODUCER_COMMON_SOURCE_FILES}
      ${C_PRODUCER_COMMON_CURL_SOURCE_FILES})
    # Use PRIVATE otherwise anything linking against cproducer will also link with pic again,
    # causing duplicate symbols
    target_link_libraries(cproducer PRIVATE ${PRODUCER_LIBRARIES} ${Jsmn} kvspic)

    add_executable(
      kinesis_video_cproducer_video_only_sample
      ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsVideoOnlyStreamingSample.c)
    target_link_libraries(kinesis_video_cproducer_video_only_sample cproducer)

    add_executable(
            kinesis_video_cproducer_video_only_sample_cw
            ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsVideoOnlyStreamingSampleCloudwatch.cpp
            ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/CanaryStreamCallbacks.cpp)

    target_link_libraries(kinesis_video_cproducer_video_only_sample_cw cproducer ${AWSSDK_LINK_LIBRARIES})

    add_executable(
      kinesis_video_cproducer_aac_audio_video_sample
      ${KINESIS_VIDEO_C_PRODUCER_SRC}/samples/KvsAacAudioVideoStreamingSample.c)
    target_link_libraries(kinesis_video_cproducer_aac_audio_video_sample cproducer)

    if(LOG4CPLUS_FOUND)
      add_library(producer SHARED ${PRODUCER_SOURCE_FILES})
      target_link_libraries(producer ${PRODUCER_LIBRARIES} cproducer)
    endif()
  endif()

  if(BUILD_JNI)
    add_library(KinesisVideoProducerJNI SHARED ${PRODUCER_SOURCE_FILES_JNI}
                                               ${PIC_SOURCE_FILES})
    target_link_libraries(KinesisVideoProducerJNI)
  endif()

  if(BUILD_TEST)
    add_executable(pic_test ${PIC_TEST_SOURCE_FILES})
    target_link_libraries(pic_test ${GTEST_BOTH_LIBRARIES} kvspic)
    add_executable(producer_test ${PRODUCER_TEST_SOURCE_FILES})
    target_link_libraries(producer_test producer ${GTEST_BOTH_LIBRARIES})
    add_executable(cproducer_test ${C_PRODUCER_TEST_SOURCE_FILES})
    target_link_libraries(cproducer_test cproducer ${GTEST_BOTH_LIBRARIES})
  endif()

  if(BUILD_GST_ARTIFACT)
    add_executable(kinesis_video_gstreamer_sample_app ${GST_DEMO_APP})
    target_link_libraries(kinesis_video_gstreamer_sample_app producer
                          ${GST_DEMO_LIBRARIES} ${Intl})

    add_executable(
      kinesis_video_gstreamer_sample_multistream_app
      ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_sample_multistream_app.cpp
    )
    target_link_libraries(kinesis_video_gstreamer_sample_multistream_app
                          producer ${GST_DEMO_LIBRARIES} ${Intl})

    add_executable(
      kinesis_video_gstreamer_audio_video_sample_app
      ${KINESIS_VIDEO_GST_DEMO_SRC}/kinesis_video_gstreamer_audio_video_sample_app.cpp
    )
    target_link_libraries(kinesis_video_gstreamer_audio_video_sample_app
                          producer ${GST_DEMO_LIBRARIES} ${Intl})

    add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})
    target_link_libraries(gstkvssink ${KVSSINK_LIBRARIES} producer ${Intl})

    add_executable(
      kvs_producer_plugin_demo
      ${KINESIS_VIDEO_GST_PLUGIN_SRC}/demo/kvs_producer_plugin_demo.cpp)
    target_link_libraries(kvs_producer_plugin_demo ${KVSSINK_DEMO_LIBRARIES}
                          producer ${Intl})

    add_executable(
      kvs_producer_plugin_rtsp_demo
      ${KINESIS_VIDEO_GST_PLUGIN_SRC}/demo/kvs_producer_plugin_rtsp_demo.cpp)
    target_link_libraries(kvs_producer_plugin_rtsp_demo
                          ${KVSSINK_DEMO_LIBRARIES} ${Intl})

    add_executable(gstkvsplugintest EXCLUDE_FROM_ALL
                   ${KINESIS_VIDEO_GST_PLUGIN_SRC}/tst/gstkvstest.cpp)
    target_link_libraries(gstkvsplugintest ${GST_CHECK_LIBRARIES}
                          ${KVSSINK_DEMO_LIBRARIES} ${Intl})
  endif()
endif()
