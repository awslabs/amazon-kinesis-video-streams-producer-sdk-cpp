### Amazon Kinesis Video Streams Producer SDK C/C++
----
### Attention
----
If you are updating from any release before 1.7.0, you need to specify a track id for each Frame as shown in [sample](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/kinesis-video-gst-demo/kinesis_video_gstreamer_sample_app.cpp#L312), otherwise you will get a 0x32000026 which means track not found.

----
### Introduction
----
Amazon Kinesis Video Streams makes it easy to securely stream video from connected devices to AWS for analytics, machine learning (ML), and other processing.

Amazon Kinesis Video Streams Producer SDK for C/C++ makes it easy to build an on-device application that securely connects to a video stream, and reliably publishes video and other media data to Kinesis Video Streams. It takes care of all the underlying tasks required to package the frames and fragments generated by the device's media pipeline. The SDK also handles stream creation, token rotation for secure and uninterrupted streaming, processing acknowledgements returned by Kinesis Video Streams, and other tasks.

----
### Kinesis Video Streams Producer SDK for C/C++
----
Amazon Kinesis Video Streams Producer SDK for C/C++ contains the following sub-directories/projects:
* kinesis-video-pic - The Platform Independent Codebase which is the basic building block for the C++/Java producer SDK. The project includes multiple sub-projects for each sub-component with unit tests.
* kinesis-video-producer - The C++ Producer SDK with unit test.
* kinesis-video-c-producer - The C Producer SDK with unit test.
* kinesis-video-producer-jni - The C++ wrapper for JNI to expose the functionality to Java/Android.
* kinesis-video-gst-demo - C++ GStreamer sample application (for webcam, USB camera and RTSP) .
* kinesis-video-gstreamer-plugin - GStreamer plugin sink element (kvssink) and demo application.
* **kinesis-video-native-build** - Native build directory with a build script for **Mac OS/Linux/Raspberry PI/Windows**. This is the directory that will contain the artifacts **(executable binaries)** and JNI libraries after the build.
  * This should be your **current working directory** for running the install-scripts.

----
### Supported Platforms
----
Kinesis Video Streams Producer SDK was tested on following platforms:
```
    Ubuntu 16
    Ubuntu 17
    Ubuntu 18
    Raspberry Raspbian
    MacOS El Captian
    MacOS Sierra
    MacOS High Sierra
    Windows 7 Enterprise
    Windows 10 Enterprise
    Windows Server 2012 R2
    Windows Server 2016
```

----
### Build and install Kinesis Video Streams Producer SDK and sample applications
----
There are few build-time tools/dependencies which need to be installed in order to compile the core producer SDK libraries and the sample applications. Sample applications are provided to demonstrate _**how to send video**_ from **webcam**, **USB camera**, or **IP camera** (RTSP).

Please follow the **installation instructions** applicable to your operating system.
* Linux
  * **[Install instructions for Ubuntu and Raspberry PI](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/install-instructions-linux.md)**
* Mac OS
  * **[Install instructions for MacOS](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/install-instructions-macos.md)**
* Windows OS
  * **[Install instructions for Windows using MinGW msys2](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/install-instructions-windows-msys2.md)**
  * **[Install instructions for Windows using Microsoft Visual C++](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/install-instructions-windows-msvc.md)**

----
### Documentation

  [Kinesis Video Producer SDK CPP documentation](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/producer-sdk-cpp.html)

  [Kinesis Video C Producer documentation](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/producer-sdk-c-api.html)

### License

This library is licensed under the Apache License, 2.0.

----
### Release notes

#### Release 2.1.0 (2nd Dec 2019)
* C/CPP-Producer - Updates
  * Fix bug where iot and file credentials failed to get cycled promptly.
  * Reworked stream and client shutdown flow.
  * Other minor stability and bug fixes.

#### Release 2.0.1 (25th Jul 2019)
* C-Producer - Updates
  * Rotating file logger (small feature).
  * Validating Stream description returned from Describe stream API call and warning if there is a mismatch.
  * Additional tests and minor compiler warning fixes.
* CPP Producer - Updates
  * Memory leak fix.
  * Minor header include change.
  * Test stability fix for RPi.
* Platform Independent Code - Updates
  * Enabling put frame calls during async stream creation.
  * Fixes for memory corruption on 32 bit platforms.
  * Using PTS for file upload cases where DTS and frame durations are 0.
  * Fix invalid error ACK timestamp mapping causing rollback/retry logic to issue a warning.
  * Improved test coverage. More stability for RPi tests.
* Kinesis Video GStreamer Plugin version matching SDK version.

#### Release 2.0.0 (9th Jul 2019)
* License update: KVS SDK and GStreamer kvssink plugin are under Apache 2.0 license now.
* C-Producer - New feature
  * Language level is C89. It should work for all GCC versions, tested minimum GCC version is 4.1.
  * The Kinesis Video Streams C producer library wraps kinesis-video-pic (Platform Independent Code) with additional layer of API that allows scenario and platform-specific callbacks and events.
  * Callbacks for sample retry logic implementation are now supported through C Producer
* CPP Producer - Updates
  * Language level is C++11 instead of C++14. Required minimum GCC version is 4.8.
  * Kinesis Video Producer CPP (kinesis-video-producer directory) is now revised to a wrapper of C Producer SDK which interfaces with the Platform Independent Repository (PIC, kinesis-video-pic directory).
* Platform Independent Code - New functionality
  * Automatic CPD (codec private data) extraction from the stream
  * Reset stream and reset connection functionality in PIC - In case of unrecoverable errors users can simply call reset stream; internally SDK will free & create the stream.
  * Log level supported in PIC. It can be set in member variable of DeviceInfo.ClientInfo.
* Platform Independent Code - Bug Fixes
  * Skip over error fragments - SDK will continue skip any invalid fragments are ingested through SDK earlier and continue streaming with valid ingested fragments. It will not just loop back and keep resending the broken fragment now.
* All unit tests (pic_test, cproducer_test, producer_test) are at C++11 level. Required minimum GCC version is 4.8.

* Please note the following changes if you have customized the SDK for your integration and not using samples directly.
  * Changes to utility functions:
    * RotatingStaticCredentialProvider is removed from C++ producer. Customer can use StaticCredentialProvider instead.
    * CredentialProviderUtil and SerializedCredentials are removed from C++ producer.
  * Following functions are removed from DefaultCallbackProvider: sleepUntilWithTimeCallback, shutdownStream, notifyResult, getControlPlaneUri, safeFreeBuffer, getStreamStatusFromString.
  * Changes to the object model:
    * Following attributes have been added: Version number is added to TrackInfo, Frame, StreamDescription, DeviceInfo, StreamInfo.
    * Following attributes have been removed: certPath is removed from DeviceInfo in PIC.

#### Release 1.7.9 (13th Mar 2019)
* Fix memory leak in kinesis_video_gstreamer_audio_video_sample_app.

#### Release 1.7.8 (6th Mar 2019)
* Updated CMakeLists.txt and install scripts to be able to link against libraries in default system path.
* Optimization and bug fixes.

#### Release 1.7.7 (22th Feb 2019)
* Stability and bug fixes.

#### Release 1.7.6 (21th Feb 2019)
* Audio integration for Kinesis Video Stream. Now Kinesis Video Streams Producer SDK CPP supports ingesting video and audio.

#### Release 1.7.5 (16th Feb 2019)
* Stability and bug fixes.

#### Release 1.7.4 (14th Feb 2019)
* Stability and bug fixes.

#### Release 1.7.3 (12th Feb 2019)
* Enforce non-zero TrackUid and SegmentUUID.

#### Release 1.7.2 (8th Feb 2019)
* JNI free object logic fix used by Java KVS SDK.

#### Release 1.7.1 (6th Feb 2019)
* Removed kinesis_video_gstreamer_sample_rtsp_app as the kinesis_video_gstreamer_sample_app has become multi-purposed for webcams, rtsp and video files.
* stability fixes.

#### Release 1.7.0 (14th Dec 2018)
* Applications need to specify a trackId for every Frame as shown in create_kinesis_video_frame function in the samples
* Added support for uploading files(offline mode) to Kinesis Video Stream
* Additional fixes

#### Release 1.6.0 (3rd Dec 2018)
* JNI namespace change
* Improve kvssink plugin retry behavior
* Unify GetTimeFunc in C++ producer for override
* Support for custom user agent string

#### Release 1.5.1 (28th Sep 2018)
* Fragment Metadata Support for Producer SDK
* Add fallback frame rate to avoid negotiation issue on certain machines like macbook air

#### Release 1.5.0 (24th Aug 2018)
* Windows Support for Producer SDK
* Intermittent producer support
* Various bug fixes

#### Release 1.4.3 (20th June 2018)
* Added prebuilt docker images for AmazonLinux and Raspbian Stretch
* Updated install-script to accept commandline argument -j and pass the value to `make` to speed up building
* Updated install-script to accept commandline argument -d to remove opensource library installation files after finishing
* Updated CMakeLists.txt in kinesis-video-native-build to link up libraries properly

#### Release 1.4.2 (14th June 2018)
* Release first version of gstreamer plugin kvssink
* Fix gstreamer demo issue when running on raspberry pi

#### Release 1.4.1 (8th May 2018)
* Update log4cplus download link in install-script

#### Release 1.4.0 (25th April 2018)
* Fix for crash caused by latest Mac tool chain issue
* Fix for callbacks returning incorrect custom data in gstreamer sample app
* Support for custom logger
* Fix for multiple callbacks when triggering connection staleness

#### Release 1.3.1 (5th April 2018)
* Fixed video source negotiation error caused by camera with fractional fps
* Docker suport for RTSP streaming

#### Release 1.3.0 (15th March 2018)
* Fixed producer intermittent termination issue for some edge cases involving re-streaming on error.

#### Release 1.2.3 (1st March 2018)
* Updated install-script to fix the local certificate trust issue for curl.
* Added steps in README troubleshooting section for curl trust issues.

#### Release 1.2.2 (March 2018)
* Remove open-source dependencies from KinesisVideoProducerJNI native library. java-install-script can be used to build KinesisVideoProducerJNI native library fast.
* README note improved.

#### Release 1.2.1 (February 2018)
* Bug fix for producer timestamp *video playback* in the console should be fixed if proper timestamp is provided to SDK. Current setting in sample app uses Gstreamer frame timecode and relative timestamp (used by SDK).
* `install-script` is updated to automatically detect OS version and avoid dependency issue on Mac High Sierra and Ubuntu 17.10.
* Known issue: Producer timestamp mode *video playback in console* will not work if GStreamer demoapp is configured to use frame timecode and absolute timestamp (used by SDK).

#### Release 1.2.0 (February 2018)
* Bug fixes and performance enhancement
* Streaming error recovery improvements
* Minor API changes:
  * create stream APIs return shared pointers instead of unique pointers
  * Addition of StreamClosed callback to notify the caller application when the stream is finished draining the existing buffered frames before closing in the graceful termination case.
#### Release 1.1.3 (February 2018)
* Added **RTSP Demo Sample**
* Run the demo using:
```
AWS_ACCESS_KEY_ID=<MYACCESSKEYID> AWS_SECRET_ACCESS_KEY=<MYSECRETKEY> ./kinesis_video_gstreamer_sample_rtsp_app <rtspurl> <stream-name>
```

#### Release 1.1.2 (January 2018)
* Allowed devices to output h.264 streams directly
* The user can also supply a streaming resolution through command line arguments.
  * If resolution is provided then the sample will try to check if the camera supports that resolution. If it does then streaming starts; else, it will fail with an error msg "Resolution not supported"
  * If no resolution is specified, the demo will try to use resolutions 1920x1080, 1280x720 and 640x480 in that order (highest resolution first)and will start streaming once the camera supported resolution is detected.
* Known issues:
  * When streaming on raspberry pi. Some green artifacts might be observed on the preview screen. Reducing the resolution can fix the issue.

#### Release 1.1.1 (December 2017)
* Fix USB webcam support
* Known issues:
    * If USB webcam doesn't support 720p, then gstreamer negotiation will fail. Trying lower resolution as mentioned in Troubleshooting may fix this issue.

#### Release 1.1.0 (December 2017)
* Addition of a received application ACK notification callback
* Lifecycle management improvements
* Exposed failure on progressive back-off/retry logic
* Hardening/fixing various edge-cases
* Fixing Raspberry PI frame dropping issue

#### Release 1.0.0 (November 2017)
* First release of the Amazon Kinesis Video Producer SDK for Cpp.
* Known issues:
    * Missing build scripts for Windows-based systems.
    * Missing cross-compile option.
    * Sample application/unit tests can't handle buffer pressures properly - simple print in debug log.

