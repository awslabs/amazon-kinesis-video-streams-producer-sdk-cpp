cmake_minimum_required(VERSION 2.8)

set(CMAKE_MACOSX_RPATH TRUE)

project(KinesisVideoProducerC LANGUAGES C)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

if(BP_BUILD_LIB)
    # Required for compilation of AL2012
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ldl -lrt -lpthread")
    endif()

    include_directories(${BP_BUILD_INCLUDE_DIRS})
    include_directories(${BP_TESTBUILD_INCLUDE_DIRS})

    link_directories(${BP_BUILD_LIB})
    link_directories(${BP_TESTBUILD_LIB})

    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      find_package(PkgConfig REQUIRED)
      find_package(OpenSSL    REQUIRED)
      find_package(CURL       REQUIRED)

      set(OPEN_SRC_INCLUDE_DIRS
              ${CURL_INCLUDE_DIRS}
              ${OPENSSL_INCLUDE_DIR})

      set(OPEN_SRC_LIBRARY_DIRS
              ${CURL_LIBRARY_DIRS}
              ${OPENSSL_CRYPTO_LIBRARY_DIRS}
              ${OPENSSL_SSL_LIBRARY_DIRS})

      include_directories(${OPEN_SRC_INCLUDE_DIRS})
      link_directories(${OPEN_SRC_LIBRARY_DIRS})
    endif()

    get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

    # Include source base directory to allow absolute header paths
    include_directories(${CMAKE_SOURCE_DIR})

    enable_testing()
    add_subdirectory(src)

    add_executable(kvs-c-sample samples/KvsVideoOnlyStreamingSample.c)
    target_link_libraries(kvs-c-sample ${PROJECT_NAME})
    install(TARGETS kvs-c-sample DESTINATION lib)

    if ((NOT CMAKE_C_COMPILER_ID STREQUAL "GNU") OR (NOT
        CMAKE_C_COMPILER_VERSION VERSION_LESS "4.4.6"))
        add_subdirectory(tst)
    endif()
else()

    if(DEFINED ENV{KINESIS_VIDEO_PIC_SRC})
        set(KINESIS_VIDEO_PIC_SRC $ENV{KINESIS_VIDEO_PIC_SRC})
    else()
        set(KINESIS_VIDEO_PIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-pic")
    endif()

    if(DEFINED ENV{KINESIS_VIDEO_PRODUCER_C_SRC})
        set(KINESIS_VIDEO_PRODUCER_C_SRC $ENV{KINESIS_VIDEO_PRODUCER_C_SRC})
    else()
        set(KINESIS_VIDEO_PRODUCER_C_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-producer-c")
    endif()

    if(DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
        set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
    else()
        set(KINESIS_VIDEO_OPEN_SOURCE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-native-build/downloads")
    endif()

    if (DEFINED ENV{KVS_JSMN_ROOT})
        set(KVS_JSMN_ROOT $ENV{KVS_JSMN_ROOT})
    else()
        message( FATAL_ERROR "No JSMN root specified." )
    endif()

    message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
    message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_SRC}")
    message(STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

    set(CMAKE_INSTALL_RPATH "${KVS_INSTALL_PATH}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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

    if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

        find_package(PkgConfig REQUIRED)

        if (USE_SYS_LIBRARIES AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
        endif()

        find_package(OpenSSL    REQUIRED)
        find_package(CURL       REQUIRED)

        set(OPEN_SRC_INCLUDE_DIRS
                ${CURL_INCLUDE_DIRS}
                ${OPENSSL_INCLUDE_DIR})

        set(PRODUCER_C_LIBRARIES
                ${OPENSSL_CRYPTO_LIBRARY}
                ${OPENSSL_SSL_LIBRARY}
                ${CURL_LIBRARIES})
    endif()

    find_library(KvsPic
            NAMES kvspic
            REQUIRED
            PATHS ${KINESIS_VIDEO_PIC_SRC}
            NO_DEFAULT_PATH)
    find_library(Jsmn
            NAMES jsmn
            REQUIRED
            PATHS   ${KVS_JSMN_ROOT}/lib
            NO_DEFAULT_PATH)

    # Uncomment below line for debug heap
    #add_definitions(-DHEAP_DEBUG)

    # Uncomment below line for very verbose logging
    #add_definitions(-DLOG_STREAMING)

    set(PRODUCER_C_SOURCE_FILES
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Auth.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/AwsV4Signer.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/CallbacksProvider.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/CurlApiCallbacks.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/DeviceInfoProvider.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StreamInfoProvider.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Producer.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Request.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Response.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Util.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/FileAuthCallbacks.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StaticAuthCallbacks.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/IotAuthCallback.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/ConnectionStaleStateMachine.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/ContinuousRetryStreamCallbacks.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StateMachineData.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StreamErrorReportStateMachine.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StreamLatencyStateMachine.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/StreamCallbacksProvider.c
            ${KINESIS_VIDEO_PRODUCER_C_SRC}/src/source/Version.c)

    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/trace/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/utils/include)
    include_directories(${KINESIS_VIDEO_PRODUCER_C_SRC})
    include_directories(${OPEN_SRC_INCLUDE_DIRS})
    include_directories(${KVS_JSMN_ROOT}/include)

    add_library(kvsproducerc SHARED ${PRODUCER_C_SOURCE_FILES})

    target_link_libraries(kvsproducerc
            ${KvsPic}
            ${Jsmn}
            ${PRODUCER_C_LIBRARIES})

    add_executable(kvsVideoOnlyStreamingSample ${KINESIS_VIDEO_PRODUCER_C_SRC}/samples/KvsVideoOnlyStreamingSample.c)
    target_link_libraries(kvsVideoOnlyStreamingSample
            kvsproducerc)

    if (BUILD_TEST)
        add_subdirectory(tst)
    endif()

    install(TARGETS kvsproducerc
            DESTINATION ${KVS_INSTALL_PATH})
    install(TARGETS kvsVideoOnlyStreamingSample
            DESTINATION ${KVS_INSTALL_PATH})
endif()
