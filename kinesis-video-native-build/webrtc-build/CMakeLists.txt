
 cmake_minimum_required(VERSION 3.8)

 project(KinesisVideoWebRTCClient)

 set(CMAKE_CXX_STANDARD 14)

 # Note: Set this OFF if GStreamer sample is not required
if (DEFINED ENV{BUILD_GST_ARTIFACT})

 set(BUILD_GST_ARTIFACT ON)

endif()

 # Components

 if (DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
     set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
 else ()
     set(KINESIS_VIDEO_OPEN_SOURCE_SRC "${PROJECT_BINARY_DIR}/downloads")
 endif ()

 if (DEFINED ENV{KINESIS_VIDEO_PIC_SRC})
     set(KINESIS_VIDEO_PIC_SRC $ENV{KINESIS_VIDEO_PIC_SRC})
 else ()
     set(KINESIS_VIDEO_PIC_SRC "../kinesis-video-pic")
 endif ()

 if (DEFINED ENV{KINESIS_VIDEO_CPRODUCER_SRC})
     set(KINESIS_VIDEO_CPRODUCER_SRC $ENV{KINESIS_VIDEO_CPRODUCER_SRC})
 else ()
     set(KINESIS_VIDEO_CPRODUCER_SRC "../kinesis-video-cproducer")
 endif ()

 if (DEFINED ENV{KINESIS_VIDEO_WEBRTC_SRC})
     set(KINESIS_VIDEO_WEBRTC_SRC $ENV{KINESIS_VIDEO_WEBRTC_SRC})
 else ()
     set(KINESIS_VIDEO_WEBRTC_SRC "../kinesis-video-webrtc-c")
 endif ()

set(USRSCTP_LIBRARIES usrsctp)

# Sanitizers

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-optimize-sibling-calls -g -O1")

# GStreamer required steps

if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
  find_package(PkgConfig REQUIRED)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif()
    find_package(OpenSSL    REQUIRED)
    pkg_check_modules(LIBSRTP REQUIRED libsrtp2)
    pkg_check_modules(LIBWEBSOCKET REQUIRED libwebsockets)

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
        pkg_check_modules (GST_BASE     REQUIRED   gstreamer-base-1.0)
    endif()

endif()


 # SDK Sources
 file(GLOB_RECURSE PIC_SOURCE_FILES        "${KINESIS_VIDEO_PIC_SRC}/src/*.c")
 file(GLOB C_PRODUCER_COMMON_FILES         "${KINESIS_VIDEO_CPRODUCER_SRC}/src/source/Common/*.c")
 file(GLOB C_PRODUCER_COMMON_LWS_FILES     "${KINESIS_VIDEO_CPRODUCER_SRC}/src/source/Common/Lws/*.c")
 file(GLOB_RECURSE WEBRTC_SOURCE_FILES     "${KINESIS_VIDEO_WEBRTC_SRC}/src/*.c")
 file(GLOB_RECURSE WEBRTC_TST_FILES        "${KINESIS_VIDEO_WEBRTC_SRC}/tst/*.c")

 file(COPY "${KINESIS_VIDEO_WEBRTC_SRC}/samples/opusSampleFrames" DESTINATION .)
 file(COPY "${KINESIS_VIDEO_WEBRTC_SRC}/samples/h264SampleFrames" DESTINATION .)

if (NOT MIN_BUILD)
 # Includes

 # GStreamer specific
  set(OPEN_SRC_INCLUDE_DIRS
          ${GLIB2_INCLUDE_DIRS}
          ${GST_APP_INCLUDE_DIRS}
          ${GST_INCLUDE_DIRS}
          ${GOBJ2_INCLUDE_DIRS}
          ${GST_BASE_INCLUDE_DIRS})

 include_directories(${OPEN_SRC_INCLUDE_DIRS})

 # SDK Includes

 include_directories(${KINESIS_VIDEO_PIC_SRC})
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/client/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/client/include/com/amazonaws/kinesis/video/client")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/client/src")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/client/tst")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/common/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/common/include/com/amazonaws/kinesis/video/common")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/heap/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/heap/include/com/amazonaws/kinesis/video/heap")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/heap/src")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/heap/tst")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/trace/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include/com/amazonaws/kinesis/video/mkvgen")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/src")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/view/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/view/include/com/amazonaws/kinesis/video/view")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/view/src")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/view/tst")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/utils/include")
 include_directories("${KINESIS_VIDEO_PIC_SRC}/src/state/include")

 include_directories(${KINESIS_VIDEO_WEBRTC_SRC})
 include_directories("${KINESIS_VIDEO_WEBRTC_SRC}/src/include")
 include_directories("${KINESIS_VIDEO_WEBRTC_SRC}/src/source")

 include_directories(${KINESIS_VIDEO_CPRODUCER_SRC})
 include_directories("${KINESIS_VIDEO_CPRODUCER_SRC}/src")
 include_directories("${KINESIS_VIDEO_CPRODUCER_SRC}/src/include")
 include_directories("${KINESIS_VIDEO_CPRODUCER_SRC}/src/source")

 include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include/")
 include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include/srtp2")
 include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}local/include/libwebsockets")
 include_directories("${KINESIS_VIDEO_OPEN_SOURCE_SRC}local/include/usrsctp")

 # Libraries

 if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     set(SSL_CRYPTO_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libcrypto.a")
     set(SSL_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libssl.a")
     set(SCTP_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libusrsctp.dylib")
     set(WS_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libwebsockets.dylib")
        else()
     set(SSL_CRYPTO_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libcrypto.so")
     set(SSL_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libssl.so")
     set(SCTP_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libusrsctp.so")
     set(WS_LIBRARIES "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libwebsockets.so")
     set(OTHER_LIBRARIES dl pthread cap)
 endif()

 set(DEPENDENT_LIBS
         "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libgtest.a"
         "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libgtest_main.a"
         "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libjsmn.a"
         "${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib/libsrtp2.a"
         "${SSL_CRYPTO_LIBRARIES}"
         "${SSL_LIBRARIES}"
         "${WS_LIBRARIES}"
         "${SCTP_LIBRARIES}")

# GStreamer specifics

 set(GST_DEMO_LIBRARIES
          ${GST_LIBRARIES}
          ${GLIB2_LIBRARIES}
          ${GST_APP_LIBRARIES}
          ${GOBJ2_LIBRARIES}
          ${EXE_LIBRARIES})

 link_directories(${GLIB2_LIBRARY_DIRS})
 link_directories(${GST_LIBRARY_DIRS})
 link_directories(${GST_APP_LIBRARY_DIRS})
 link_directories(${GOBJ2_LIBRARY_DIRS})

 # Executables
 add_definitions(-DKVS_BUILD_WITH_LWS=1)
 add_library(kvswebrtcclient SHARED
        ${WEBRTC_SOURCE_FILES}
        ${PIC_SOURCE_FILES}
        ${C_PRODUCER_COMMON_FILES}
        ${C_PRODUCER_COMMON_LWS_FILES})

 target_link_libraries(kvswebrtcclient
        ${Jsmn}
        ${WEBRTC_CLIENT_LIBRARIES}
        ${DEPENDENT_LIBS}
        ${OTHER_LIBRARIES})

 add_executable(webrtcViewer
        "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientViewer.c" "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

 target_link_libraries(webrtcViewer
         kvswebrtcclient)

 add_executable(webrtcMaster
       "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientMaster.c" "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

 target_link_libraries(webrtcMaster
     ${GST_DEMO_LIBRARIES}
     kvswebrtcclient)

 if (BUILD_GST_ARTIFACT)
    add_executable(kvsWebRTCClientMasterGstreamerSample
             ${WEBRTC_SOURCE_FILES}
             ${PIC_SOURCE_FILES}
             ${C_PRODUCER_COMMON_FILES}
             ${C_PRODUCER_COMMON_LWS_FILES}
             "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientMasterGstreamerSample.c"
             "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

    target_link_libraries(kvsWebRTCClientMasterGstreamerSample
              ${GST_DEMO_LIBRARIES}
              kvswebrtcclient)

endif()

else()
# do MIN_BUILD

    message("Do MIN_BUILD")

    # Includes
    set(OPEN_SRC_INCLUDE_DIRS
            ${LIBSRTP_INCLUDE_DIRS}
            ${OPENSSL_INCLUDE_DIR}
            ${LIBUV_INCLUDE_DIRS}
            ${LIBWEBSOCKET_INCLUDE_DIRS}
            ${LIBEVENT_INCLUDE_DIRS}

            ${GLIB2_INCLUDE_DIRS}
            ${GST_INCLUDE_DIRS}
            ${GST_APP_INCLUDE_DIRS}
            ${GOBJ2_INCLUDE_DIRS})
    include_directories(${OPEN_SRC_INCLUDE_DIRS})

    # Libraries
    set(WEBRTC_CLIENT_LIBRARIES
                    ${OPENSSL_CRYPTO_LIBRARY}
                    ${OPENSSL_SSL_LIBRARY}
                    ${LIBSRTP_LIBRARIES}
		    ${USRSCTP_LIBRARIES}
                    ${LIBWEBSOCKET_LIBRARIES})

    set(GST_SAMPLE_LIBRARIES
                    ${GLIB2_LIBRARIES}
                    ${GST_LIBRARIES}
                    ${GST_APP_LIBRARIES}
                    ${GOBJ2_LIBRARIES}
                    ${Intl})

    link_directories(${LIBSRTP_LIBRARY_DIRS})
    link_directories(${LIBWEBSOCKET_LIBRARY_DIRS})

    link_directories(${GLIB2_LIBRARY_DIRS})
    link_directories(${GST_LIBRARY_DIRS})
    link_directories(${GST_APP_LIBRARY_DIRS})
    link_directories(${GOBJ2_LIBRARY_DIRS})

    find_library(Jsmn
                NAMES jsmn
                REQUIRED
                PATHS   ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/lib
                NO_DEFAULT_PATH)

    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/trace/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/utils/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/state/include)

    include_directories(${KINESIS_VIDEO_CPRODUCER_SRC}/src)
    include_directories(${KINESIS_VIDEO_WEBRTC_SRC}/src/include)
    include_directories(${KINESIS_VIDEO_WEBRTC_SRC}/src)
    include_directories(${OPEN_SRC_INCLUDE_DIRS})

    include_directories(${KINESIS_VIDEO_CPRODUCER_SRC})
    include_directories(${KINESIS_VIDEO_CPRODUCER_SRC}/src/include)
    include_directories(${KINESIS_VIDEO_CPRODUCER_SRC}/src/include)
    include_directories(${KINESIS_VIDEO_OPEN_SOURCE_SRC}/local/include)

    add_definitions(-DKVS_BUILD_WITH_LWS=1)
    add_library(kvswebrtcclient SHARED
        ${WEBRTC_SOURCE_FILES}
        ${PIC_SOURCE_FILES}
        ${C_PRODUCER_COMMON_FILES}
        ${C_PRODUCER_COMMON_LWS_FILES})

    target_link_libraries(kvswebrtcclient
        ${Jsmn}
        ${WEBRTC_CLIENT_LIBRARIES})

    add_executable(webrtcMaster
        "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientMaster.c" "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

    target_link_libraries(webrtcMaster
         kvswebrtcclient)

    add_executable(webrtcViewer
        "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientViewer.c" "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

    target_link_libraries(webrtcViewer
         kvswebrtcclient)

    if (BUILD_GST_ARTIFACT)
        add_executable(kvsWebRTCClientMasterGstreamerSample
            "${KINESIS_VIDEO_WEBRTC_SRC}/samples/kvsWebRTCClientMasterGstreamerSample.c" "${KINESIS_VIDEO_WEBRTC_SRC}/samples/Common.c")

        target_link_libraries(kvsWebRTCClientMasterGstreamerSample
            kvswebrtcclient
            ${GST_SAMPLE_LIBRARIES})
    endif()
endif()

