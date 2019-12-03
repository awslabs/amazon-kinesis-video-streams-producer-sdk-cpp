#!/bin/bash

set -e

setup() {
	echo "Setting up for KVS WebRTC SDK installation"

	# --------- create required directories ---------------------------------------
	if [[ "$KINESIS_VIDEO_WEBRTC_ROOT" == *\ * ]]; then
	    echo "Current working path cannot have space in it !"
	    exit
	fi

	DOWNLOADS="$KINESIS_VIDEO_WEBRTC_ROOT/downloads"
	if [ ! -d "$DOWNLOADS" ]; then
	    mkdir "$DOWNLOADS"
	fi

	if [ ! -d "$DOWNLOADS/local" ]; then
	    mkdir "$DOWNLOADS/local"
	fi

	CERTS_FOLDER="$KINESIS_VIDEO_WEBRTC_ROOT/certs"
	if [ ! -d "$CERTS_FOLDER" ]; then
	    mkdir "$CERTS_FOLDER"
	fi

	SYS_CURL=`which curl` || echo "Curl not found. Please install curl."
	if [ -z "$SYS_CURL" ]; then
	  exit
	fi

	# -------- Check if cert.pem exists in the kinesis_video_native_buid/certs folder----------------
	CERT_PEM_FILE="$CERTS_FOLDER/cert.pem"

	if [ ! -f "$CERT_PEM_FILE" ];then
	  echo "cert.pem" file does not exist in the $KINESIS_VIDEO_WEBRTC_ROOT directory
	  echo "Downloading the https://www.amazontrust.com/repository/SFSRootCAG2.pem to $CERTS_FOLDER/cert.pem"
	  $SYS_CURL -L "https://www.amazontrust.com/repository/SFSRootCAG2.pem" -o "$CERTS_FOLDER/cert.pem"
	fi
}

parse_args() {
	while [[ $# -gt 0 ]]
	do
	key="$1"
		case $key in
		    -j)
		    max_parallel="$2"
		    echo "Passing -j $max_parallel to make"
		    shift # past argument
		    shift # past value
		    ;;
		    -d)
		    do_cleanup=false
		    echo "Will not clean up library installation files after finish"
		    shift # past argument
		    ;;
		    --build_debug)
			build_type=Debug
		 	echo "Setting CMAKE_BUILD_TYPE to Debug"
		    shift # past argument
		    ;;
		    --build_test)
			build_test=TRUE
		 	echo "Will build unit test"
		    shift # past argument
		    ;;
		    -h|--help)
			echo "-j: Pass value to make -j"
			echo "-d: Do not clean up library installation files after finish"
			echo "--build_debug: Set CMAKE_BUILD_TYPE to Debug"
		    exit 0
		    ;;
		    *)    # unknown option
		    echo "Unknown option $key"
		    exit 1
		    ;;
		esac
	done
}

clean_up() {
	echo "Clean up downloaded artifacts start"
	rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake
	echo "Clean up downloaded artifacts complete."
}
