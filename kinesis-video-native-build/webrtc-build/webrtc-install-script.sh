#!/bin/bash
#
# Install the open source dependencies for the following SDK:
#
#    kinesis-video-streams-webrtc-sdk-c
# 

# Uses the following open source dependencies
# libcurl jsmn libwebsockets libsrtp2 libcrypto libssl gtest


# **** NOTE ****
#   KINESIS_VIDEO_WEBRTC_ROOT should point to kinesis-video-webrtc-native-build directory
#  
if [ -z $KINESIS_VIDEO_WEBRTC_ROOT ]; then
  KINESIS_VIDEO_WEBRTC_ROOT=`pwd`
fi

if [[ ! $KINESIS_VIDEO_WEBRTC_ROOT =~ kinesis-video-webrtc-native-build$ ]]; then
  echo "Please set KINESIS_VIDEO_WEBRTC_ROOT environment variable to /path/to/kvswebrtcsdk/kinesis-video-webrtc-native-build 
        or run the install-script inside kinesis-video-webrtc-native-build directory"
  exit 1
fi

source $KINESIS_VIDEO_WEBRTC_ROOT/webrtc-install-script-common.sh
 
build_extras="Y"

max_parallel=2
do_cleanup=false
build_type=Release
build_test=false
 
parse_args $@

setup


LIB_DESTINATION_FOLDER_PATH="$KINESIS_VIDEO_WEBRTC_ROOT/downloads/local/lib"

# -------- Setting environment variables for cmake ----------------------------

export PATH="$KINESIS_VIDEO_WEBRTC_ROOT/downloads/local/bin:$PATH"
export PKG_CONFIG_LIBDIR="$DOWNLOADS/local/lib/pkgconfig"
export CMAKE_PREFIX_PATH="$DOWNLOADS/local"

PLATFORM='unknown'
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   PLATFORM='linux'
elif [[ "$unamestr" == 'FreeBSD' ]]; then
   PLATFORM='freebsd'
elif [[ "$unamestr" == 'Darwin' ]]; then
   PLATFORM='mac'
fi
KERNEL_VERSION=`uname -r|cut -f1 -d"."`


#--------- libssl ----------------------------------------------#

echo "Checking openssl at $DOWNLOADS/local/lib/libssl.a"

if [ ! -f $DOWNLOADS/local/lib/libssl.a ]; then
  echo "openssl lib not found. Installing"
  if [ ! -f $DOWNLOADS/openssl-1.1.0f.tar.gz ]; then
    cd $DOWNLOADS
    $SYS_CURL -L "https://www.openssl.org/source/openssl-1.1.0f.tar.gz" -o "openssl-1.1.0f.tar.gz"
  fi

  cd $DOWNLOADS
  tar -xvf openssl-1.1.0f.tar.gz
  cd $DOWNLOADS/openssl-1.1.0f

  ./config  --prefix=$DOWNLOADS/local/ --openssldir=$DOWNLOADS/local/
  make -j $max_parallel
  make install

fi

#------------ libcurl -----------------------------------------#

echo "Checking curl at $DOWNLOADS/local/lib/libcurl.a"
  if [ ! -f $DOWNLOADS/local/lib/libcurl.a ]; then
    echo "Curl lib not found. Installing"
    if [ ! -f $DOWNLOADS/curl-7.57.0.tar.xz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "https://curl.haxx.se/download/curl-7.57.0.tar.xz" -o "curl-7.57.0.tar.xz"
    fi
    cd $DOWNLOADS
    tar -xvf curl-7.57.0.tar.xz
    cd $DOWNLOADS/curl-7.57.0

    if [ -f "$CERT_PEM_FILE" ] ;then
      ./configure --prefix=$DOWNLOADS/local/ --enable-dynamic --disable-rtsp --disable-ldap --without-zlib --with-ssl=$DOWNLOADS/local/ --with-ca-bundle=$CERT_PEM_FILE
    else
      echo "Cannot find the cert.pem at $CERTS_FOLDER -- libcurl build requires cert.pem; existing."
      echo "Download the https://www.amazontrust.com/repository/SFSRootCAG2.pem to $CERTS_FOLDER/cert.pem"
      exit
    fi
    make -j $max_parallel
    make install
  fi
  
#--------- libsrtp2  ----------------------------------------------#

echo "Checking libsrtp at $DOWNLOADS/local/lib/libsrtp2.a/dylib/so"

if [ ! -f $DOWNLOADS/local/lib/libsrtp2.dylib ]; then
  if [ ! -f $DOWNLOADS/local/lib/libsrtp2.a ]; then
    echo "libsrtp lib not found. Installing"
    if [ ! -f $DOWNLOADS/libsrtp-2.2.0.tar.gz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "https://github.com/cisco/libsrtp/archive/v2.2.0.tar.gz" -o "libsrtp-2.2.0.tar.gz"
    fi

    cd $DOWNLOADS
    tar -xvf libsrtp-2.2.0.tar.gz
    cd $DOWNLOADS/libsrtp-2.2.0
  
    ./configure --prefix=$DOWNLOADS/local/ --enable-openssl 
    make -j $max_parallel
    make install
  
  fi
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
  
  cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$DOWNLOADS/local/ \
           -DOPENSSL_ROOT_DIR=$DOWNLOADS/local/include/ \
           -DOPENSSL_INCLUDE_DIR=$DOWNLOADS/local/include/ \
           -DCMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE=$DOWNLOADS/local/include/ \
           -DLWS_OPENSSL_INCLUDE_DIRS=$DOWNLOADS/local/include/openssl \
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
           -DCMAKE_BUILD_TYPE=RELEASE 
 
  make -j $max_parallel
  make install
fi

  #------------ jsmn ------------------------------------------------------------------------------------------------
  echo "Checking jsmn at $DOWNLOADS/local/lib/libjsmn.a"
  if [ ! -f $DOWNLOADS/local/lib/libjsmn.* ]; then
    echo "jsmn lib not found. Installing"
    if [ ! -f $DOWNLOADS/jsmn.tar.gz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "https://github.com/zserge/jsmn/archive/v1.0.0.tar.gz" -o "jsmn.tar.gz"
    fi
    cd $DOWNLOADS
    tar -xvf jsmn.tar.gz
    cd $DOWNLOADS/jsmn-1.0.0
    make clean && make CFLAGS="-fPIC" -j $max_parallel
    if [ ! -d "$DOWNLOADS/local/lib" ]; then
      mkdir "$DOWNLOADS/local/lib"
    fi
    if [ ! -d "$DOWNLOADS/local/include" ]; then
      mkdir "$DOWNLOADS/local/include"
    fi
    cp libjsmn.a $DOWNLOADS/local/lib
    cp jsmn.h $DOWNLOADS/local/include
  fi
fi

  #------------ gtest  ------------------------------------------------------------------------------------------------

 echo "Checking googletest at $DOWNLOADS"
 
   if [ ! -f $DOWNLOADS/local/lib/libgtest.* ]; then
     echo "gtest lib not found. Installing"
     if [ ! -d $DOWNLOADS/googletest-release-1.8.0 ]; then
       cd $DOWNLOADS
       $SYS_CURL -L "https://github.com/google/googletest/archive/release-1.8.0.tar.gz" -o "google-test-1.8.0.gz"
       tar -xvf google-test-1.8.0.gz
     fi
     cd $DOWNLOADS/googletest-release-1.8.0
     cmake -DCMAKE_INSTALL_PREFIX=$DOWNLOADS/local .
     make -j $max_parallel && make install
   fi
   
env_var="$env_var KVS_GTEST_ROOT=$DOWNLOADS/local"
 


# cmake CMakeList for WebRTC, PIC and C Producer  
cmake -f CMakeLists.txt
make 

