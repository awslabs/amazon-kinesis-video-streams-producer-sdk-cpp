#!/bin/bash
#
#
# Install the open source dependencies for
# kinesis-video-streams-webrtc-sdk-c including GStreamer
#

set -e

# KINESIS_VIDEO_WEBRTC_ROOT should point to kinesis-video-webrtc-native-build directory
if [ -z $KINESIS_VIDEO_WEBRTC_ROOT ]; then
  KINESIS_VIDEO_WEBRTC_ROOT=`pwd`
fi

if [[ ! $KINESIS_VIDEO_WEBRTC_ROOT =~ kinesis-video-webrtc-native-build$ ]]; then
  echo "Please set KINESIS_VIDEO_WEBRTC_ROOT environment variable to /path/to/kvssdk/kinesis-video-webrtc-native-build
  or run the install-script inside kinesis-video-native-webrtc-build directory"
  exit 1
fi

source $KINESIS_VIDEO_WEBRTC_ROOT/webrtc-install-script-common.sh

max_parallel=2

build_type=Release
build_test=TRUE
build_gst_artifact=TRUE
build_producer=TRUE

env_var=""
parse_args $@

setup


# --------- check platform -----------------------------------------------------
PLATFORM='unknown'
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   PLATFORM='linux'
   echo "======= make sure the following libraries have been installed ======="
   echo "* pkg-config"
   echo "* libssl-dev"
   echo "* cmake"
   echo "* libgstreamer1.0-dev"
   echo "* libgstreamer-plugins-base1.0-dev"
   echo "* gstreamer1.0-plugins-base-apps"
   echo "* gstreamer1.0-plugins-bad"
   echo "* gstreamer1.0-plugins-good"
   echo "* gstreamer1.0-plugins-ugly"
   echo "* gstreamer1.0-omx (if on raspberry pi)"
   echo "* gstreamer1.0-tools"
   echo "* libsrtp2-dev"

elif [[ "$unamestr" == 'Darwin' ]]; then
   PLATFORM='mac'
   echo "======= make sure the following libraries have been installed (library name from homebrew) ======="
   echo "* pkg-config"
   echo "* openssl"
   echo "* cmake"
   echo "* gstreamer"
   echo "* gst-plugins-base"
   echo "* gst-plugins-good"
   echo "* gst-plugins-bad"
   echo "* gst-plugins-ugly"
   echo "* srtp"
else
   PLATFORM='unknown'
   echo "Required libraries for $unamestr unknown"
   exit 0
fi

echo "Checking jsmn at $DOWNLOADS/local/lib/libjsmn.a"
if [ ! -f $DOWNLOADS/local/lib/libjsmn.a ]; then
  echo "jsmn lib not found. Installing"
  if [ ! -f $DOWNLOADS/jsmn.zip ]; then
    cd $DOWNLOADS
    $SYS_CURL -L "https://github.com/zserge/jsmn/archive/v1.0.0.zip" -o "jsmn.zip"
  fi
  cd $DOWNLOADS
  unzip -oq jsmn.zip -d jsmn
  cd $DOWNLOADS/jsmn/jsmn-1.0.0
  make CFLAGS="-fPIC" -j $max_parallel
  rsync libjsmn.a $DOWNLOADS/local/lib/
  rsync jsmn.h $DOWNLOADS/local/include/
fi

#------------ libWebSockets -----------------------------------------#

echo "Checking libWebSockets at $DOWNLOADS/local/lib/libwebsockets.a"

if [ ! -f $DOWNLOADS/local/lib/libwebsockets.* ]; then
  echo "libWebSockets not found. Installing"
  if [ ! -f $DOWNLOADS/libWebSockets.tar.gz ]; then
    cd $DOWNLOADS
    $SYS_CURL -L "https://github.com/warmcat/libwebsockets/archive/v3.2.0.tar.gz" -o "libWebSockets.tar.gz"
  fi

  cd $DOWNLOADS
  tar -xvf libWebSockets.tar.gz
  cd $DOWNLOADS/libwebsockets-3.2.0

  rm -rf build
  mkdir build
  cd build
  if [[ "$PLATFORM" == 'mac' ]]; then
    export OPENSSL_ROOT_DIR="/usr/local/opt/openssl"
  fi
  cmake -DCMAKE_INSTALL_PREFIX=$DOWNLOADS/local/ \
         -DLWS_WITH_HTTP2=1 \
         -DLWS_HAVE_HMAC_CTX_new=1 \
         -DLWS_HAVE_SSL_EXTRA_CHAIN_CERTS=1 \
         -DLWS_HAVE_OPENSSL_ECDH_H=1 \
         -DLWS_HAVE_EVP_MD_CTX_free=1 \
         -DLWS_WITHOUT_SERVER=1 \
         -DLWS_WITHOUT_TESTAPPS=1 \
         -DLWS_WITH_THREADPOOL=1 \
         -DLWS_WITHOUT_TEST_SERVER_EXTPOLL=1 \
         -DLWS_WITHOUT_TEST_PING=1 \
         -DLWS_WITHOUT_TEST_CLIENT=1 \
         -DLWS_WITH_STATIC=1 \
         -DLWS_WITH_SHARED=0 \
         -DLWS_STATIC_PIC=1 \
         -DLWS_WITH_ZLIB=0 \
         -DCMAKE_BUILD_TYPE=RELEASE .. || true

  make -j $max_parallel
  make install
fi

#------------ libusrsctp -----------------------------------------#

echo "Checking usrsctp at $DOWNLOADS/local/lib/libusrsctp.a"

if [ ! -f $DOWNLOADS/local/lib/libusrsctp.a ]; then

  echo "usrsctp not found. Installing"
  if [ ! -f $DOWNLOADS/usrsctp.tar.gz ]; then
    cd $DOWNLOADS
    $SYS_CURL -L "https://github.com/sctplab/usrsctp/archive/913de3599feded8322882bdae69f346da5a258fc.tar.gz" -o "usrsctp.tar.gz"
  fi

  cd $DOWNLOADS
  tar -xvf usrsctp.tar.gz
  cd $DOWNLOADS/usrsctp-913de3599feded8322882bdae69f346da5a258fc

  rm -rf build
  mkdir build
  cd build

  cmake .. -DCMAKE_C_FLAGS="-Wno-error=format-truncation" -DCMAKE_INSTALL_PREFIX:PATH=$DOWNLOADS/local/ \
           -DCMAKE_BUILD_TYPE=RELEASE

  make -j $max_parallel
  make install
fi


cd $KINESIS_VIDEO_WEBRTC_ROOT

export PKG_CONFIG_PATH=$DOWNLOADS/local/lib/pkgconfig

cmake_args="-DCMAKE_BUILD_TYPE=${build_type} -DUSE_SYS_LIBRARIES=TRUE \
  -DBUILD_TEST=FALSE -DBUILD_GST_ARTIFACT=${build_gst_artifact} \
  -DMIN_BUILD=TRUE"

cmake ${cmake_args} .

make -j ${max_parallel}

echo "**********************************************************"
echo Success!!!
echo "**********************************************************"
