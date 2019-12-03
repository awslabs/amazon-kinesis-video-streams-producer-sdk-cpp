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

    if(NOT KINESIS_VIDEO_PIC_SRC)
        if(DEFINED ENV{KINESIS_VIDEO_PIC_SRC})
            set(KINESIS_VIDEO_PIC_SRC $ENV{KINESIS_VIDEO_PIC_SRC})
        else()
            set(KINESIS_VIDEO_PIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-pic")
        endif()
    endif()

    if(NOT KINESIS_VIDEO_PRODUCER_C_SRC)
        if(DEFINED ENV{KINESIS_VIDEO_PRODUCER_C_SRC})
            set(KINESIS_VIDEO_PRODUCER_C_SRC $ENV{KINESIS_VIDEO_PRODUCER_C_SRC})
        else()
            set(KINESIS_VIDEO_PRODUCER_C_SRC "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
    endif()

    if(NOT KINESIS_VIDEO_OPEN_SOURCE_SRC)
        if(DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
            set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
        else()
            set(KINESIS_VIDEO_OPEN_SOURCE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-native-build/downloads/local")
        endif()
    endif()

    message(STATUS "Kinesis Video PIC path is ${KINESIS_VIDEO_PIC_SRC}")
    message(STATUS "Kinesis Video Producer path is ${KINESIS_VIDEO_PRODUCER_C_SRC}")
    message(STATUS "Kinesis Video Open Source path is ${KINESIS_VIDEO_OPEN_SOURCE_SRC}")

    if (NOT EXISTS ${KINESIS_VIDEO_PIC_SRC})
        message(FATAL_ERROR "KINESIS_VIDEO_PIC_SRC ${KINESIS_VIDEO_PIC_SRC} does not exist")
    elseif(NOT EXISTS ${KINESIS_VIDEO_PRODUCER_C_SRC})
        message(FATAL_ERROR "KINESIS_VIDEO_PRODUCER_C_SRC ${KINESIS_VIDEO_PRODUCER_C_SRC} does not exist")
    elseif(NOT EXISTS ${KINESIS_VIDEO_OPEN_SOURCE_SRC})
        message(FATAL_ERROR "KINESIS_VIDEO_OPEN_SOURCE_SRC ${KINESIS_VIDEO_OPEN_SOURCE_SRC} does not exist.")
    endif()

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

        if (NOT OPENSSL_FOUND)
            message("looking for openssl")
            if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
            endif()

            find_package(OpenSSL    REQUIRED)

            set(OPEN_SRC_INCLUDE_DIRS
                ${OPEN_SRC_INCLUDE_DIRS}
                ${OPENSSL_INCLUDE_DIR})
        endif()

        if (NOT CURL_FOUND AND BUILD_COMMON_CURL)
            message("looking for curl")
            find_package(CURL       REQUIRED)

            set(OPEN_SRC_INCLUDE_DIRS
                ${OPEN_SRC_INCLUDE_DIRS}
                ${CURL_INCLUDE_DIRS})
        endif()

        if (NOT LIBSRTP_FOUND AND BUILD_COMMON_LWS)
            message("looking for libwebsocket")
            set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib/pkgconfig")
            pkg_check_modules(LIBSRTP REQUIRED libsrtp2)

            set(OPEN_SRC_INCLUDE_DIRS
                    ${LIBWEBSOCKET_INCLUDE_DIRS})
            link_directories(${LIBWEBSOCKET_LIBRARY_DIRS})
        endif()
    endif()

    if (NOT Jsmn)
        find_library(Jsmn
            NAMES jsmn
            REQUIRED
            PATHS   ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib
            NO_DEFAULT_PATH)
    endif()

    # Uncomment below line for debug heap
    #add_definitions(-DHEAP_DEBUG)

    # Uncomment below line for very verbose logging
    #add_definitions(-DLOG_STREAMING)

    file(GLOB KVS_COMMON_SOURCE_FILES_BASE
        "src/source/Common/*.c"
    )

    file(GLOB KVS_COMMON_SOURCE_FILES_LWS
        "src/source/Common/Lws/*.c"
    )

    file(GLOB KVS_COMMON_SOURCE_FILES_CURL
        "src/source/Common/Curl/*.c"
    )

    file(GLOB PRODUCER_C_SOURCE_FILES
        "src/source/*.c"
    )

    file(GLOB PIC_HEADERS
        "${KINESIS_VIDEO_PIC_SRC}/src/*/include"
    )

    include_directories(${PIC_HEADERS})
    include_directories(${KINESIS_VIDEO_PRODUCER_C_SRC}/src/include)
    include_directories(${OPEN_SRC_INCLUDE_DIRS})
    include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/include)

    add_subdirectory(${KINESIS_VIDEO_PIC_SRC} ./kvspic EXCLUDE_FROM_ALL)

    if(BUILD_COMMON_LWS)
        add_library(kvsCommonLws STATIC ${KVS_COMMON_SOURCE_FILES_BASE} ${KVS_COMMON_SOURCE_FILES_LWS})
        target_compile_definitions(kvsCommonLws PRIVATE KVS_BUILD_WITH_LWS=1)
        target_link_libraries(kvsCommonLws
                ${Jsmn}
                ${OPENSSL_CRYPTO_LIBRARY}
                ${OPENSSL_SSL_LIBRARY}
                ${LIBWEBSOCKET_LIBRARIES})
    endif()

    # producer only uses curl right now
    add_library(kvsCommonCurl STATIC ${KVS_COMMON_SOURCE_FILES_BASE} ${KVS_COMMON_SOURCE_FILES_CURL})
    target_compile_definitions(kvsCommonCurl PRIVATE KVS_BUILD_WITH_CURL=1)
    target_link_libraries(kvsCommonCurl
            ${Jsmn}
            ${OPENSSL_CRYPTO_LIBRARY}
            ${OPENSSL_SSL_LIBRARY}
            ${CURL_LIBRARIES})

    add_library(cproducer SHARED ${PRODUCER_C_SOURCE_FILES})

    target_link_libraries(cproducer
        kvspic
        kvsCommonCurl)

    add_executable(kvsVideoOnlyStreamingSample ${KINESIS_VIDEO_PRODUCER_C_SRC}/samples/KvsVideoOnlyStreamingSample.c)
    target_link_libraries(kvsVideoOnlyStreamingSample
        cproducer)

    if (BUILD_TEST)
        add_subdirectory(tst)
    endif()
endif()
