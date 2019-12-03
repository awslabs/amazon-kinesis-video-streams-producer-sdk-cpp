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

echo " ===========================Starting to build ===========================
              Kinesis Video Stream webrtc SDK and Demo Application
       ========================================================================
       Following open source libraries and tools will be downloaded and built
       before building the producer sdk.
       ========================================================================
       # ------------------------- BUILD-TOOLS -------------------------------#
        autoconf   automake
        bison      flex
	gettext    googletest
	iconv 	   libffi
	libpcre    libtools
	log4cplus  m4
	openssl    zlib
	jsmn       libwebsockets
	libsrtp2   usrsctp
        # ------------------------- GSTREAMER - for demo application --------#
        GStreamer, GStreamer plugins, yasm, gst-libav
        ------------------------------------------------------------------------

 "



max_parallel=2

build_type=Release
build_test=TRUE
build_gst_artifact=TRUE
build_producer=TRUE

env_var=""
parse_args $@

setup

# --------- install-script build options --------------------------------------


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

# check if we are on raspberry pi. If so then direct user to use
#package managers and run webrtc-install-script.sh (minimal dependeicies only)

if [[ -e /proc/device-tree/model ]]; then
    model=$(tr -d '\0' </proc/device-tree/model)
    if [[ "$model" =~ ^Raspberry ]]; then
        echo "install-script takes hours to complete on raspberry pi."
        exit 0
    fi
fi


#--------- build + install libs -----------------------------------------------
if [[ $build_producer == TRUE ]]; then
  echo "Checking openssl at $DOWNLOADS/local/lib/libssl.a"
  if [ ! -f $DOWNLOADS/local/lib/libssl.a ]; then
    echo "openssl lib not found. Installing"
    if [ ! -f $DOWNLOADS/openssl-1.1.0l.tar.gz ]; then
      cd $DOWNLOADS

       $SYS_CURL -L "https://www.openssl.org/source/openssl-1.1.0l.tar.gz" -o "openssl-1.1.0l.tar.gz"
    fi
    cd $DOWNLOADS
    tar -xvf openssl-1.1.0l.tar.gz
    cd $DOWNLOADS/openssl-1.1.0l
    ./config  --prefix=$DOWNLOADS/local/ --openssldir=$DOWNLOADS/local/
    make -j $max_parallel
    make install
  fi

  echo "Checking log4cplus at $DOWNLOADS/local/lib/liblog4cplus.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/liblog4cplus.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/liblog4cplus.so ]; then
      echo "log4cplus lib not found. Installing"
      if [ ! -f $DOWNLOADS/log4cplus-1.2.0.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://github.com/log4cplus/log4cplus/releases/download/REL_1_2_0/log4cplus-1.2.0.tar.xz" -o "log4cplus-1.2.0.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf log4cplus-1.2.0.tar.xz
      cd $DOWNLOADS/log4cplus-1.2.0
      ./configure --prefix=$DOWNLOADS/local/
      make -j $max_parallel
      make install
    fi
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

  # --------- googletest--------------------------------------------------------

    echo "Checking googletest at $DOWNLOADS"
    # mingw has prebuilt gtest
    if [[ "$PLATFORM" != 'mingw' ]]; then
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
    fi

if [[ $build_gst_artifact == TRUE ]]; then
  echo "Checking gettext at $DOWNLOADS/local/lib/libgettextlib.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libgettextlib.dylib ]; then
   if [ ! -f $DOWNLOADS/local/lib/libgettextlib.so ]; then
     echo "gettext lib not found. Installing"
     if [ ! -f $DOWNLOADS/gettext-0.19.8.tar.xz ]; then
       cd $DOWNLOADS
       $SYS_CURL -L "https://ftp.gnu.org/pub/gnu/gettext/gettext-0.19.8.tar.xz" -o "gettext-0.19.8.tar.xz"
     fi
     cd $DOWNLOADS
     tar -xvf gettext-0.19.8.tar.xz
     cd $DOWNLOADS/gettext-0.19.8
     ./configure --prefix=$DOWNLOADS/local/
     make -j $max_parallel
     make install
   fi
  fi
fi


  echo "Checking libffi at $DOWNLOADS/local/lib/libffi.a"
  if [ ! -f $DOWNLOADS/local/lib/libffi.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libffi.a ]; then
      echo "libffi lib not found. Installing"
      if [ ! -f $DOWNLOADS/libffi-3.2.1.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://sourceware.org/ftp/libffi/libffi-3.2.1.tar.gz" -o "libffi-3.2.1.tar.gz"
      fi
      cd $DOWNLOADS
      tar -xvf libffi-3.2.1.tar.gz
      cd $DOWNLOADS/libffi-3.2.1
      ./configure --prefix=$DOWNLOADS/local/
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking pcre at $DOWNLOADS/local/lib/libpcre.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libpcre.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libpcre.so ]; then
      echo "pcre lib not found. Installing"
      if [ ! -f $DOWNLOADS/pcre-8.41.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://ftp.pcre.org/pub/pcre/pcre-8.41.tar.gz" -o "pcre-8.41.tar.gz"
      fi
      cd $DOWNLOADS
      tar -xvf pcre-8.41.tar.gz
      cd $DOWNLOADS/pcre-8.41
      ./configure --prefix=$DOWNLOADS/local/
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking zlib at $DOWNLOADS/local/lib/libz.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libz.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libz.so ]; then
      echo "zlib lib not found. Installing"
      if [ ! -f $DOWNLOADS/zlib-1.2.11.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://www.zlib.net/zlib-1.2.11.tar.gz" -o "zlib-1.2.11.tar.gz"
      fi
      cd $DOWNLOADS
      tar -xvf zlib-1.2.11.tar.gz
      cd $DOWNLOADS/zlib-1.2.11
      ./configure --prefix=$DOWNLOADS/local/
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking iconv at $DOWNLOADS/local/lib/libiconv.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libiconv.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libiconv.so ]; then
      echo "iconv lib not found. Installing"
      if [ ! -f $DOWNLOADS/libiconv-1.15.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://ftp.gnu.org/gnu/libiconv/libiconv-1.15.tar.gz" -o "libiconv-1.15.tar.gz"
      fi
      cd $DOWNLOADS
      tar -xvf libiconv-1.15.tar.gz
      cd $DOWNLOADS/libiconv-1.15
      ./configure --prefix=$DOWNLOADS/local/
      make -j $max_parallel
      make install
    fi
  fi

  if [[ $PLATFORM == 'linux' ]]; then
    echo "Checking util-linux at $DOWNLOADS/local/lib/libmount.dylib/.so"
    if [ ! -f $DOWNLOADS/local/lib/libmount.dylib ]; then
      if [ ! -f $DOWNLOADS/local/lib/libmount.so ]; then
        echo "libmount lib not found. Installing"
        if [ ! -f $DOWNLOADS/util-linux-2.31.tar.gz ]; then
          cd $DOWNLOADS
          $SYS_CURL -L "https://www.kernel.org/pub/linux/utils/util-linux/v2.31/util-linux-2.31.tar.gz" -o "util-linux-2.31.tar.gz"
        fi
        cd $DOWNLOADS
        tar -xvf util-linux-2.31.tar.gz
        cd $DOWNLOADS/util-linux-2.31
        ./configure --prefix=$DOWNLOADS/local/ --disable-all-programs --enable-libmount --enable-libblkid
        make -j $max_parallel
        make install
      fi
    fi
  fi

  #-------------------------install program m4 --------------------------------------------------------------------------
  if [ ! -f $DOWNLOADS/local/bin/m4 ]; then
    echo "m4 not found. Installing"
    if [ ! -f $DOWNLOADS/m4-1.4.10.tar.gz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "https://ftp.gnu.org/gnu/m4/m4-1.4.10.tar.gz" -o "m4-1.4.10.tar.xz"
    fi
    cd $DOWNLOADS
    tar -xvf m4-1.4.10.tar.xz
    cd $DOWNLOADS/m4-1.4.10
    ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
    make -j $max_parallel
    make install
  fi
  #-----------------------liby.a (bison) --------------------------------------------------------------------------------
  if [[ $PLATFORM != 'mac' || $KERNEL_VERSION < 17 ]]; then
    if [ ! -f $DOWNLOADS/local/lib/liby.a ]; then
      echo "bison not found. Installing"
      if [ ! -f $DOWNLOADS/bison-3.0.4.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://ftp.gnu.org/gnu/bison/bison-3.0.4.tar.gz" -o "bison-3.0.4.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf bison-3.0.4.tar.xz
      cd $DOWNLOADS/bison-3.0.4
      ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi
  # -----  libfl.2.dylib and libfl.2.so ---------------------------------------------------------------------------------
  if [ ! -f $DOWNLOADS/local/lib/libfl.2.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libfl.2.so ]; then
      if [ ! -f $DOWNLOADS/local/lib/libfl.so ]; then
        echo "flex not found. Installing"
        if [ ! -f $DOWNLOADS/flex-2.6.3.tar.gz ]; then
          cd $DOWNLOADS
          $SYS_CURL -L "https://github.com/westes/flex/releases/download/v2.6.3/flex-2.6.3.tar.gz" -o "flex-2.6.3.tar.xz"
        fi
        cd $DOWNLOADS
        tar -xvf flex-2.6.3.tar.xz
        cd $DOWNLOADS/flex-2.6.3
        ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
        make -j $max_parallel
        make install
      fi
    fi
  fi
  # --------- libltdl (libtool)--------------------------------------------------
  if [ ! -f $DOWNLOADS/local/lib/libltdl.7.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libltdl.7.so ]; then
      if [ ! -f $DOWNLOADS/local/lib/libltdl.so ]; then
        echo "libtools not found. Installing"
        if [ ! -f $DOWNLOADS/libtool-2.4.6.tar.gz ]; then
          cd $DOWNLOADS
          $SYS_CURL -L "https://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.xz" -o "libtool-2.4.6.tar.xz"
        fi
        cd $DOWNLOADS
        tar -xvf libtool-2.4.6.tar.xz
        cd $DOWNLOADS/libtool-2.4.6
        ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
        make -j $max_parallel
        make install
      fi
    fi
  fi

  # --------- glib --------------------------------------------------------------
  echo "Checking glib at $DOWNLOADS/local/lib/libglib-2.0.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libglib-2.0.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libglib-2.0.so ]; then
      export LIBFFI_CFLAGS="-I$DOWNLOADS/local/include -I$DOWNLOADS/local/lib/libffi-3.2.1/include -L$DOWNLOADS/local/lib"
      export LIBFFI_LIBS="-L$DOWNLOADS/local/lib -lffi"
      export LDFLAGS="-L$DOWNLOADS/local/lib"
      export CPPFLAGS="-I$DOWNLOADS/local/include"

      echo "glib lib not found. Installing"
      if [ ! -f $DOWNLOADS/glib-2.54.2.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "http://ftp.gnome.org/pub/gnome/sources/glib/2.54/glib-2.54.2.tar.xz" -o "glib-2.54.2.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf glib-2.54.2.tar.xz
      cd $DOWNLOADS/glib-2.54.2
      # autoreconf -f -i
      ./configure --prefix=$DOWNLOADS/local/ --with-pcre --with-libiconv --disable-gtk-doc  --disable-man
      make -j $max_parallel
      make install
    fi
  fi

  #-------------------autoconf, autoheader, autom4te, autoreconf, autoscan, autoupdate, and ifnames----------------------

  if [[ $PLATFORM != 'mac' || $KERNEL_VERSION < 17 ]]; then
    if [ ! -f $DOWNLOADS/local/bin/autoconf ]; then
      echo "autoconf not found. Installing"
      if [ ! -f $DOWNLOADS/autoconf-2.69.tar.gz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.xz" -o "autoconf-2.69.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf autoconf-2.69.tar.xz
      cd $DOWNLOADS/autoconf-2.69
      ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi
  #--------aclocal, aclocal-1.15 (hard linked with aclocal), automake, and automake-1.15 (hard linked with automake)--
  if [ ! -f $DOWNLOADS/local/bin/automake ]; then
    echo "automake not found. Installing"
    if [ ! -f $DOWNLOADS/automake-1.15.1.tar.gz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "http://ftp.gnu.org/gnu/automake/automake-1.15.1.tar.xz" -o "automake-1.15.1.tar.xz"
    fi
    cd $DOWNLOADS
    tar -xvf automake-1.15.1.tar.xz
    cd $DOWNLOADS/automake-1.15.1
    ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
    make -j $max_parallel
    make install
  fi

  export GLIB_CFLAGS="-I$DOWNLOADS/local/include/glib-2.0 -I$DOWNLOADS/local/include/glib-2.0/glib -I$DOWNLOADS/local/lib/glib-2.0/include"

  #----------- GStreamer and plugins --------------------------------------------------------------------------------
  echo "Checking gstreamer at $DOWNLOADS/local/lib/libgstreamer-1.0.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libgstreamer-1.0.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libgstreamer-1.0.so ]; then
      echo "gstreamer lib not found. Installing"
      export ARCHFLAGS="-arch x86_64"
      if [ ! -f $DOWNLOADS/gstreamer-1.12.3.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-1.12.3.tar.xz" -o "gstreamer-1.12.3.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf gstreamer-1.12.3.tar.xz
      cd $DOWNLOADS/gstreamer-1.12.3

      if [ ! -f configure ]; then
        $DOWNLOADS/local/bin/autoreconf -f -i
      fi

      ./configure --prefix=$DOWNLOADS/local --disable-gtk-doc -- CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking gst-plugin-base at $DOWNLOADS/local/lib/libgstvideo-1.0.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libgstvideo-1.0.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libgstvideo-1.0.so ]; then
      echo "gst-plugin-base not found. Installing"
      if [ ! -f $DOWNLOADS/gst-plugins-base-1.12.3.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-1.12.3.tar.xz" -o "gst-plugins-base-1.12.3.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf gst-plugins-base-1.12.3.tar.xz
      cd $DOWNLOADS/gst-plugins-base-1.12.3

      if [ ! -f configure ]; then
        $DOWNLOADS/local/bin/autoreconf -f -i
      fi

      ./configure --prefix=$DOWNLOADS/local --disable-gtk-doc -- CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking gst-plugin-good at $DOWNLOADS/local/lib/gstreamer-1.0/libgstavi.la"
  if [ ! -f $DOWNLOADS/local/lib/gstreamer-1.0/libgstavi.la ]; then
    echo "gst-plugin-good not found. Installing"
    if [ ! -f $DOWNLOADS/gst-plugins-good-1.12.3.tar.xz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-1.12.3.tar.xz" -o "gst-plugins-good-1.12.3.tar.xz"
    fi
    cd $DOWNLOADS
    tar -xvf gst-plugins-good-1.12.3.tar.xz
    cd $DOWNLOADS/gst-plugins-good-1.12.3

    if [ ! -f configure ]; then
      $DOWNLOADS/local/bin/autoreconf -f -i
    fi

    ./configure --prefix=$DOWNLOADS/local --disable-examples --disable-x --disable-gtk-doc  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
    make -j $max_parallel
    make install
  fi

  echo "Checking gst-plugin-bad at $DOWNLOADS/local/lib/libgstbadvideo-1.0.dylib/.so"
  if [ ! -f $DOWNLOADS/local/lib/libgstbadvideo-1.0.dylib ]; then
    if [ ! -f $DOWNLOADS/local/lib/libgstbadvideo-1.0.so ]; then
      echo "gst-plugin-bad not found. Installing"
      if [ ! -f $DOWNLOADS/gst-plugins-bad-1.12.3.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-1.12.3.tar.xz" -o "gst-plugins-bad-1.12.3.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf gst-plugins-bad-1.12.3.tar.xz
      cd $DOWNLOADS/gst-plugins-bad-1.12.3

      if [ ! -f configure ]; then
        $DOWNLOADS/local/bin/autoreconf -f -i
      fi

      ./configure --prefix=$DOWNLOADS/local --disable-gtk-doc --disable-wayland  --disable-opencv --disable-ldap --disable-rtsp --without-zlib --without-ssl --disable-yadif --disable-sdl --disable-debug --disable-dependency-tracking LIBTOOL="$DOWNLOADS/gst-plugins-bad-1.12.3/libtool --tag=CC" CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi

  echo "Checking for yasm"
  if [ ! -f $DOWNLOADS/local/bin/yasm ]; then
    echo "yasm-1.3.0.tar.gz not found. Installing"
    if [ ! -f $DOWNLOADS/yasm-1.3.0.tar.gz ]; then
      cd $DOWNLOADS
      $SYS_CURL -L "http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz" -o "yasm-1.3.0.tar.gz"
      cd $DOWNLOADS
    fi
    cd $DOWNLOADS
    tar -xvf yasm-1.3.0.tar.gz
    cd $DOWNLOADS/yasm-1.3.0

    if [ ! -f configure ]; then
      $DOWNLOADS/local/bin/autoreconf -f -i
    fi

    ./configure --prefix=$DOWNLOADS/local  CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
    make -j $max_parallel
    make install
  fi

  echo "Checking gst-libav in $DOWNLOADS/local/lib/"
  if [ ! -f $DOWNLOADS/local/lib/gstreamer-1.0/libgstlibav.so ]; then
    if [ ! -f $DOWNLOADS/local/lib/gstreamer-1.0/libgstlibav.dylib ]; then
      echo "gst-libav not found. Installing"
      if [ ! -f $DOWNLOADS/gst-libav-1.12.4.tar.xz ]; then
        cd $DOWNLOADS
        $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gst-libav/gst-libav-1.12.4.tar.xz" -o "gst-libav-1.12.4.tar.xz"
      fi
      cd $DOWNLOADS
      tar -xvf gst-libav-1.12.4.tar.xz
      cd $DOWNLOADS/gst-libav-1.12.4

      if [ ! -f configure ]; then
        $DOWNLOADS/local/bin/autoreconf -f -i
      fi

      ./configure --prefix=$DOWNLOADS/local CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
      make -j $max_parallel
      make install
    fi
  fi

  # ------------- install omx plugin of we are in Raspberry Pi ------------------------------------------------------
  sys_info=`uname -m`
  shopt -s nocasematch
  if [[ $sys_info =~ ^arm ]]; then
    # we are in a raspberry pi
    echo "Checking gst-omx "
    if [ ! -f $DOWNLOADS/local/lib/gstreamer-1.0/llibgstomx.dylib ]; then
      if [ ! -f $DOWNLOADS/local/lib/gstreamer-1.0/libgstomx.so ]; then
        echo "gst-omx not found. Installing"
        if [ ! -f $DOWNLOADS/gst-omx-1.10.4.tar.xz ]; then
          cd $DOWNLOADS
          $SYS_CURL -L "https://gstreamer.freedesktop.org/src/gst-omx/gst-omx-1.10.4.tar.xz" -o "gst-omx-1.10.4.tar.xz"
        fi
        cd $DOWNLOADS
        tar -xvf gst-omx-1.10.4.tar.xz
        cd $DOWNLOADS/gst-omx-1.10.4

        if [ ! -f configure ]; then
          $DOWNLOADS/local/bin/autoreconf -f -i
        fi

        ./configure --prefix=$DOWNLOADS/local --disable-gtk-doc --with-omx-header-path=/opt/vc/include/IL --with-omx-target=rpi CFLAGS="-I$DOWNLOADS/local/include" LDFLAGS="-L$DOWNLOADS/local/lib"
        make -j $max_parallel
        make install
      fi
    fi
  fi
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
           -DLWS_WITH_STATIC=0 \
           -DLWS_WITH_SHARED=1 \
           -DLWS_STATIC_PIC=1 \
           -DLWS_WITH_ZLIB=0 \
           -DCMAKE_BUILD_TYPE=RELEASE

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

if [[ "$unamestr" == 'Linux' ]]; then
  cmake .. -DCMAKE_C_FLAGS="-Wno-error=format-truncation" -DCMAKE_INSTALL_PREFIX:PATH=$DOWNLOADS/local/ \
           -DCMAKE_BUILD_TYPE=RELEASE
  else 
  cmake ..  -DCMAKE_INSTALL_PREFIX:PATH=$DOWNLOADS/local/ \
           -DCMAKE_BUILD_TYPE=RELEASE
fi

  make -j $max_parallel
  make install
fi


## --------- build kinesis video --------------------------------------------------------------------------------------
# note PKG_CONFIG_LIBDIR has been set above

cd $KINESIS_VIDEO_WEBRTC_ROOT


if [[ ! -z $env_var ]]; then
  export $env_var
fi

cmake -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_PREFIX_PATH="$DOWNLOADS/local" \
      -DBUILD_TEST=${build_test} -DBUILD_GST_ARTIFACT=${build_gst_artifact} -DBUILD_PRODUCER=${build_producer} \
      -DBUILD_JNI=${build_jni_only} \
      .
make -j $max_parallel

export BUILD_GST_ARTIFACT=TRUE


cmake -f CMakeLists.txt
make

echo "**********************************************************"
echo Success in building the Kinesis Video Streams WEBRTC SDK !!!
echo "**********************************************************"


