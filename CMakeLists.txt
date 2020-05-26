if(APPLE AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET} AND NOT DEFINED ENV{SDKROOT})
  set(CMAKE_OSX_DEPLOYMENT_TARGET ${CMAKE_SYSTEM_VERSION})
endif()

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
elseif(WIN32)
  set(LINKAGE STATIC)
else()
  set(LINKAGE SHARED)
endif()

set(KINESIS_VIDEO_PRODUCER_CPP_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS ${KINESIS_VIDEO_PRODUCER_CPP_SRC})
  message(FATAL_ERROR "KINESIS_VIDEO_PRODUCER_CPP_SRC ${KINESIS_VIDEO_PRODUCER_CPP_SRC} does not exist")
endif()

# pass ca cert location to sdk
add_definitions(-DKVS_CA_CERT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/certs/cert.pem")
add_definitions(-DCMAKE_DETECTED_CACERT_PATH)

if(BUILD_DEPENDENCIES)
  if (NOT OPEN_SRC_INSTALL_PREFIX)
    set(OPEN_SRC_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/open-source)
    set(ENV{PKG_CONFIG_PATH}
          "$ENV{PKG_CONFIG_PATH}:${OPEN_SRC_INSTALL_PREFIX}/lib/pkgconfig")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${OPEN_SRC_INSTALL_PREFIX})
    set(ENV{PATH} "$ENV{PATH}:${OPEN_SRC_INSTALL_PREFIX}/bin")
  endif()

  if(NOT EXISTS ${OPEN_SRC_INSTALL_PREFIX})
    file(MAKE_DIRECTORY ${OPEN_SRC_INSTALL_PREFIX})
  endif()

  message(STATUS "Begin building dependencies.")

  if (NOT WIN32)
    build_dependency(autoconf)
    build_dependency(automake)
  endif()
  build_dependency(log4cplus)
  build_dependency(kvspic)
  build_dependency(cproducer)

  message(STATUS "Finished building dependencies.")
endif()

############# find dependent libraries ############
find_package(Threads)
find_package(PkgConfig REQUIRED)

if (WIN32)
  find_library(LIBKVSPIC_LIBRARIES kvspic REQUIRED)
  find_library(LIBKVSPIC_UTILS_LIBRARIES kvspicUtils REQUIRED)
  find_library(LIBKVSPIC_CLIENT_LIBRARIES kvspicClient REQUIRED)
  find_library(LIBKVSPIC_STATE_LIBRARIES kvspicState REQUIRED)
  find_library(LIBCPRODUCER_LIBRARIES cproducer REQUIRED)
  find_library(LIBKVS_COMMON_CURL_LIBRARIES kvsCommonCurl REQUIRED)
else()
  pkg_check_modules(LIBKVSPIC REQUIRED libkvspic)
  pkg_check_modules(LIBKVSPIC_UTILS REQUIRED libkvspicUtils)
  pkg_check_modules(LIBKVSPIC_CLIENT REQUIRED libkvspicClient)
  pkg_check_modules(LIBKVSPIC_STATE REQUIRED libkvspicState)
  pkg_check_modules(LIBCPRODUCER REQUIRED libcproducer)
  pkg_check_modules(LIBKVS_COMMON_CURL REQUIRED libkvsCommonCurl)
endif()

set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} 
  ${LIBKVSPIC_INCLUDE_DIRS} ${LIBKVSPIC_UTILS_INCLUDE_DIRS}
  ${LIBKVSPIC_CLIENT_INCLUDE_DIRS} ${LIBKVSPIC_STATE_INCLUDE_DIRS}
  ${LIBCPRODUCER_INCLUDE_DIRS} ${LIBKVS_COMMON_CURL_INCLUDE_DIRS})
link_directories(${LIBKVSPIC_LIBRARY_DIRS} ${LIBCPRODUCER_LIBRARY_DIRS})

find_package(OpenSSL REQUIRED)
set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})

# if (OPEN_SRC_INSTALL_PREFIX)
#   find_package(CURL REQUIRED PATHS ${OPEN_SRC_INSTALL_PREFIX})
# else()
#   find_package(CURL REQUIRED)
# endif()

set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})

if (WIN32)
  find_package(Log4cplus
    NAMES log4cplus REQUIRED
    PATHS ${OPEN_SRC_INSTALL_PREFIX}/lib)
else()
  find_library(Log4cplus
    NAMES log4cplus REQUIRED
    PATHS ${OPEN_SRC_INSTALL_PREFIX}/lib)
endif()

set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} ${LOG4CPLUS_INCLUDE_DIRS})
link_directories(${LOG4CPLUS_LIBRARY_DIRS})

find_library(Jsmn REQUIRED)
set(OPEN_SRC_INCLUDE_DIRS ${OPEN_SRC_INCLUDE_DIRS} ${JSMN_INCLUDE_DIRS})

if (WIN32)
       set(PKG_CONFIG_EXECUTABLE "C:\\gstreamer\\1.0\\x86_64\\bin\\pkg-config.exe")
endif()

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

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

############# Build Targets ############
file(GLOB PRODUCER_CPP_SOURCE_FILES "src/*.cpp" "src/common/*.cpp" "src/credential-providers/*.cpp")
file(GLOB GST_PLUGIN_SOURCE_FILES "src/gstreamer/*.cpp" "src/gstreamer/Util/*.cpp")
file(GLOB_RECURSE JNI_SOURCE_FILES "src/JNI/*.cpp")

include_directories(${OPEN_SRC_INCLUDE_DIRS})
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/credential-providers)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/common)
include_directories(${KINESIS_VIDEO_PRODUCER_CPP_SRC}/src/JNI/include)

if (WIN32)
  SET(Log4cplus "log4cplus::log4cplus")
endif()

add_library(KinesisVideoProducer ${LINKAGE} ${PRODUCER_CPP_SOURCE_FILES})
target_link_libraries(
  KinesisVideoProducer
  PUBLIC ${LIBKVSPIC_LIBRARIES}
         ${LIBKVSPIC_UTILS_LIBRARIES}
         ${LIBKVSPIC_CLIENT_LIBRARIES}
         ${LIBKVSPIC_STATE_LIBRARIES}
         ${LIBKVS_COMMON_CURL_LIBRARIES}
         ${LIBCPRODUCER_LIBRARIES}
         ${OPENSSL_SSL_LIBRARY}
         ${OPENSSL_CRYPTO_LIBRARY}
         ${JSMN_LIBRARY}
         ${Log4cplus})

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

  add_executable(kvs_gstreamer_sample samples/kvs_gstreamer_sample.cpp)
  target_link_libraries(kvs_gstreamer_sample ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kvs_gstreamer_multistream_sample samples/kvs_gstreamer_multistream_sample.cpp)
  target_link_libraries(kvs_gstreamer_multistream_sample ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kvs_gstreamer_audio_video_sample samples/kvs_gstreamer_audio_video_sample.cpp)
  target_link_libraries(kvs_gstreamer_audio_video_sample ${GST_APP_LIBRARIES} KinesisVideoProducer)

  add_executable(kvs_gstreamer_file_uploader_sample samples/kvs_gstreamer_file_uploader_sample.cpp)
  target_link_libraries(kvs_gstreamer_file_uploader_sample ${GST_APP_LIBRARIES})
endif()

if(BUILD_TEST)
  set(ENV{KVS_GTEST_ROOT} ${KINESIS_VIDEO_OPEN_SOURCE_SRC})
  add_subdirectory(tst)
endif()
