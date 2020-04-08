cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
include(Utilities)
project(KinesisVideoProducerCpp)

set(CMAKE_CXX_STANDARD 11)

# User Flags
option(BUILD_GSTREAMER_PLUGIN "Build kvssink GStreamer plugin" OFF)
option(BUILD_JNI "Build C++ wrapper for JNI to expose the functionality to Java/Android" OFF)
option(BUILD_STATIC "Build with static linkage" OFF)
option(ADD_MUCLIBC "Add -muclibc c flag" OFF)
option(BUILD_DEPENDENCIES "Whether or not to build depending libraries from source" ON)
option(BUILD_OPENSSL_PLATFORM "If buildng OpenSSL what is the target platform" OFF)

# Developer Flags
option(BUILD_TEST "Build the testing tree" OFF)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
option(COMPILER_WARNINGS "Enable all compiler warnings" OFF)
option(ADDRESS_SANITIZER "Build with AddressSanitize." OFF)
option(MEMORY_SANITIZER "Build with MemorySanitizer" OFF)
option(THREAD_SANITIZER "Build with ThreadSanitizer" OFF)
option(UNDEFINED_BEHAVIOR_SANITIZER "Build with UndefinedBehaviorSanitizer" OFF)

set(CMAKE_MACOSX_RPATH TRUE)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# static settings
if(BUILD_STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  set(LINKAGE STATIC)
else()
  set(LINKAGE SHARED)
endif()

set(KINESIS_VIDEO_PIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/open-source/amazon-kinesis-video-streams-pic/")
set(KINESIS_VIDEO_PRODUCER_C_SRC "${CMAKE_CURRENT_SOURCE_DIR}/open-source/amazon-kinesis-video-streams-producer-c/")
set(KINESIS_VIDEO_PRODUCER_CPP_SRC "${CMAKE_SOURCE_DIR}")
set(KINESIS_VIDEO_OPEN_SOURCE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/open-source)

message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
message(STATUS "Kinesis Video C Producer path is ${KINESIS_VIDEO_PRODUCER_C_SRC}")
message(STATUS "Kinesis Video WebRTC Client path is ${KINESIS_VIDEO_PRODUCER_CPP_SRC}")
message(STATUS "dependencies install path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

if(NOT EXISTS ${KINESIS_VIDEO_PIC_SRC})
  message(FATAL_ERROR "KINESIS_VIDEO_PIC_SRC ${KINESIS_VIDEO_PIC_SRC} does not exist")
elseif(NOT EXISTS ${KINESIS_VIDEO_PRODUCER_CPP_SRC})
  message(FATAL_ERROR "KINESIS_VIDEO_PRODUCER_CPP_SRC ${KINESIS_VIDEO_PRODUCER_CPP_SRC} does not exist")
elseif(NOT EXISTS ${KINESIS_VIDEO_PRODUCER_C_SRC})
  message(FATAL_ERROR "KINESIS_VIDEO_PRODUCER_C_SRC ${KINESIS_VIDEO_PRODUCER_C_SRC} does not exist")
endif()

# pass ca cert location to sdk
add_definitions(-DKVS_CA_CERT_PATH="${CMAKE_SOURCE_DIR}/certs/cert.pem")
add_definitions(-DCMAKE_DETECTED_CACERT_PATH)

if(BUILD_DEPENDENCIES)
  if(NOT EXISTS ${KINESIS_VIDEO_OPEN_SOURCE_SRC})
    file(MAKE_DIRECTORY ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)
  endif()

  set(OPEN_SRC_INSTALL_PREFIX ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local)
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${OPEN_SRC_INSTALL_PREFIX}/lib/pkgconfig")
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${OPEN_SRC_INSTALL_PREFIX})

  message(STATUS "Begin building dependencies.")

  build_dependency(openssl -DBUILD_STATIC=${BUILD_STATIC} -DBUILD_OPENSSL_PLATFORM=${BUILD_OPENSSL_PLATFORM})
  build_dependency(curl -DBUILD_STATIC=${BUILD_STATIC} -DUSE_OPENSSL=${USE_OPENSSL} -DUSE_MBEDTLS=${USE_MBEDTLS})

  build_dependency(jsmn)
  build_dependency(log4cplus)

  if(BUILD_TEST)
    build_dependency(gtest)
  endif()

  message(STATUS "Finished building dependencies.")
endif()

############# find dependent libraries ############
find_package(Threads)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(LIBCURL REQUIRED libcurl)

set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
link_directories(${LIBCURL_LIBRARY_DIRS})

set(OPEN_SRC_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})

find_library(
  Log4cplus
  NAMES log4cplus REQUIRED
  PATHS ${OPEN_SRC_INSTALL_PREFIX}/lib)


find_library(Jsmn
  NAMES jsmn REQUIRED
  PATHS ${OPEN_SRC_INSTALL_PREFIX}/lib)

############# Enable Sanitizers ############
if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

  if(ADD_MUCLIBC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -muclibc")
  endif()

  if(CODE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()

  if(ADDRESS_SANITIZER)
    enableSanitizer("address")
  endif()
  if(MEMORY_SANITIZER)
    enableSanitizer("memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-memory-track-origins")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-memory-track-origins")
  endif()
  if(THREAD_SANITIZER)
    enableSanitizer("thread")
  endif()
  if(UNDEFINED_BEHAVIOR_SANITIZER)
    enableSanitizer("undefined")
  endif()
endif()

############# Build Targets ############
file(GLOB PRODUCER_CPP_SOURCE_FILES "src/*.cpp" "src/common/*.cpp" "src/credential-providers/*.cpp")
file(GLOB GST_PLUGIN_SOURCE_FILES "src/gstreamer/*.cpp" "src/gstreamer/Util/*.cpp")
file(GLOB_RECURSE JNI_SOURCE_FILES "src/JNI/*.cpp")
file(GLOB PIC_HEADERS "${KINESIS_VIDEO_PIC_SRC}/src/*/include")

include_directories(${PIC_HEADERS})
include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/include)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/credential-providers)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/common)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/JNI/include)
include_directories(${KINESIS_VIDEO_PRODUCER_C_SRC}/src/include)

set(BUILD_COMMON_LWS
    FALSE
    CACHE BOOL "Build ProducerC without LWS Support" FORCE)
set(BUILD_COMMON_CURL
    TRUE
    CACHE BOOL "Build ProducerC with CURL Support" FORCE)
add_subdirectory(${KINESIS_VIDEO_PRODUCER_C_SRC} ./kvscproducer EXCLUDE_FROM_ALL)

add_library(KinesisVideoProducer ${LINKAGE} ${PRODUCER_CPP_SOURCE_FILES})
target_link_libraries(
  KinesisVideoProducer
  PUBLIC kvspic
         kvspicUtils
         kvspicClient
         kvspicState
         kvsCommonCurl
         cproducer
         ${OPENSSL_SSL_LIBRARY}
         ${OPENSSL_CRYPTO_LIBRARY}
         ${Jsmn}
         ${Log4cplus}
         ${LIBCURL_LIBRARIES})

if(BUILD_JNI)
  find_package(JNI REQUIRED)
  include_directories(${JNI_INCLUDE_DIRS})

  add_library(KinesisVideoProducerJNI SHARED ${JNI_HEADERS} ${JNI_SOURCE_FILES})
  target_link_libraries(KinesisVideoProducerJNI kvspic)
endif()


if(BUILD_GSTREAMER_PLUGIN)
  pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
  include_directories(${GST_APP_INCLUDE_DIRS})
  link_directories(${GST_APP_LIBRARY_DIRS})

  add_library(gstkvssink MODULE ${GST_PLUGIN_SOURCE_FILES})
  target_link_libraries(gstkvssink PRIVATE ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kinesis_video_gstreamer_sample_app samples/kinesis_video_gstreamer_sample_app.cpp)
  target_link_libraries(kinesis_video_gstreamer_sample_app ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kinesis_video_gstreamer_sample_multistream_app samples/kinesis_video_gstreamer_sample_multistream_app.cpp)
  target_link_libraries(kinesis_video_gstreamer_sample_multistream_app ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kinesis_video_gstreamer_audio_video_sample_app samples/kinesis_video_gstreamer_audio_video_sample_app.cpp)
  target_link_libraries(kinesis_video_gstreamer_audio_video_sample_app ${GST_APP_LIBRARIES} KinesisVideoProducer)
endif()

if(BUILD_TEST)
  set(ENV{KVS_GTEST_ROOT} ${KINESIS_VIDEO_OPEN_SOURCE_SRC})
  add_subdirectory(tst)
endif()
