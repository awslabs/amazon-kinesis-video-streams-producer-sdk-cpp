Amazon Kinesis Video Streams Producer SDK Cpp

## License

This library is licensed under the Amazon Software License.

## Introduction
Amazon Kinesis Video Streams Producer SDK for C/C++ enables native code developers to create or easily integrate application with Amazon Kinesis Video.  
It contains the following sub-directories/projects:
* kinesis-video-pic - The Platform Independent Codebase which is the basic building block for the C++/Java producer SDK. The project includes multiple sub-projects for each sub-component with unit tests.
* kinesis-video-producer - The C++ Producer SDK with unit test.
* kinesis-video-producer-jni - The C++ wrapper for JNI to expose the functionality to Java/Android.
* kinesis-video-gst-demo - C++ GStreamer sample application.
* kinesis-video-native-build - Native build directory with a build script for Mac OS. This is the directory that will contain the artifacts after the build.

## Building from Source
After you've downloaded the code from GitHub, you can build it on Mac OS using /kinesis-video-native-build/install-script-mac script. This will produce the core library, the JNI library, unit tests executable and the sample GStreamer application. The script will download and build the dependent open source components in the 'downloads' directory and link against it.

### Dependencies
The build depends on
* Autoconf
* Cmake 3.7
* Bison 2.4
* Automake
* xCode (Mac OS) / clang / gcc 

## Open Source Dependencies
The projects depend on a number of open source components. Running install-script-mac will download and build the necessary components automatically.

### Launching the sample application / unit test
Define AWS_ACCESS_KEY_ID and AWS_SECRET_KEY_ID environment variables with the AWS access key id and secret key:
`export AWS_ACCESS_KEY_ID={AccessKeyId}`
`export AWS_SECRET_KEY_ID={SecretKeyId}`

### Enabling verbose logs
Define HEAP_DEBUG and LOG_STREAMING C-defines by uncommenting the appropriate lines in CMakeList.txt

## Release Notes
### Release 1.0.0 (November 2017)
* First release of the Amazon Kinesis Video Producer SDK for Cpp.
* Known issues:
    * Missing build scripts for Linux-based and Windows-based systems.
    * Missing cross-compile option.
    * Sample application/unit tests can't handle buffer pressures properly - simple print in debug log.
