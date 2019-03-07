#### Instructions for installing Kinesis Video Streams Producer SDK on MacOS

-----
#### Pre-requisites:

* The following MacOS platforms are supported.
```
    macOS High Sierra
    macOS Sierra
    macOS El Captian
```
* [Git](https://git-scm.com/downloads) is required for checking out the Kinesis Video Streams SDK.
* In order to install the build tools, account with administrator privileges is required.

----
### Installation:

In order to build the Producer SDK and download open source dependencies the following are required. One option is to use `brew install` (in **Mac OS X**) to install the following build tool dependencies.

##### Install the build tools
* cmake 3.7/3.8 [https://cmake.org/](https://cmake.org/)
* [xCode](https://developer.apple.com/xcode/) (Mac OS) / clang / gcc (xcode-select version 2347)
* autoconf 2.69 (License GPLv3+/Autoconf: GNU GPL version 3 or later) http://www.gnu.org/software/autoconf/autoconf.html
	* one option is to use `brew` to install e.g. `brew install autoconf`
* bison 2.4 (GNU License)
* pkgconfig
* Java JDK (optional) - if Java JNI compilation is required

_**Note:**_ If you have installed these build tools using `brew` you need to set the PATH environment variable to include the installed location.
e.g. To incldue the latest `bison` you can run `export PATH=/usr/local/Cellar/bison/3.0.4_1/bin/:$PATH`

----
### Build the Kinesis Video Producer SDK and sample applications:
The **install-script** will download and build the dependent open source components (from the source) into the **downloads** directory within `kinesis-video-native-build` directory (e.g. `/Users/<myuser>/downloads/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads`) and link against it.

**After you've downloaded the code from GitHub, you can build it on Mac OS by running `./install-script` (which is inside the `kinesis-video-native-build` directory)**.

**Important:**  Change the _**current working directory**_ to `kinesis-video-native-build` directory first. Then run the `./install-script` from that directory.

 The `install-script` will take some time (about 25 minutes) to bring down and compile the open source components depending on the network connection and processor speed. At the end of the installation, this step will produce the core library, unit tests executable and the sample GStreamer application. If anything fails or the script is interrupted, re-running it will pick up from the place where it last left off. The sub-sequent run will be building just the modified SDK components or applications which is much faster.

Note that the install-script also builds the Kinesis Video Streams producer SDK as a **GStreamer plugin** (**kvssink**).

----
### Alternate option to build the SDK and sample applications using libraries installed by Homebrew

The bulk of the **install script** is building the open source dependencies. The project is based on **CMake**. So the open source components building can be skipped if the system versions of the open source dependencies are already installed that can be used for linking.

First make sure that the following libraries have been installed using Homebrew
```
brew install pkg-config openssl cmake gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly log4cplus
```

After all required libraries have been installed, run

```
$ ./min-install-script
```
from the `kinesis-video-native-build` directory will build and link the SDK.

The `./min-install-script` inside the `kinesis-video-native-build` captures these steps for installing the Kinesis Video Streams Producer SDK with the system versions for linking.

##### Optionally build the native library (KinesisVideoProducerJNI) to run Java demo streaming application

The `./java-install-script` inside `kinesis-video-native-build` will build the KinesisVideoProducerJNI native library to be used by [Java Producer SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md).

-----
### Running Examples:
##### Setting credentials in environment variables
Define AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY environment variables with the AWS access key id and secret key:

```
$ export AWS_ACCESS_KEY_ID=YourAccessKeyId
$ export AWS_SECRET_ACCESS_KEY=YourSecretAccessKey
```
optionally, set `AWS_SESSION_TOKEN` if integrating with temporary token and `AWS_DEFAULT_REGION` for the region other than `us-west-2`

----
##### Setting the environment variables for library path
At the end of installation (with `install-script`), environment setup is saved in the `set_kvs_sdk_env.sh`.  Next time you want to run the sample applications or use **gst-launch-1.0** to send video to Kinesis Video Streams, you can run this script by `. ./set_kvs_sdk_env.sh` or ` source set_kvs_sdk_env.sh`
within the `kinesis-video-native-build` directory. This set up the environment for **LD_LIBRARY_PATH**, **PATH** and **GST_PLUGIN_PATH**. You can also set these environment variables manually as below.
* Export the **LD_LIBRARY_PATH**=`<full path to your sdk cpp directory`>/kinesis-video-native-build/downloads/local/lib. For example, if you have downloaded the CPP SDK in `/opt/awssdk` directory then you can set
the LD_LIBRARY_PATH as below:
```
export LD_LIBRARY_PATH=/opt/awssdk/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
```
* The `gst-launch-1.0` and `gst-inspect-1.0` binaries are built inside the folder `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin`. You can either run the following commands from that folder using `./gst-launch-1.0` or you can include that in your **PATH** environment variable using the following export command and run `gst-launch-1.0`
```
$ export PATH=<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin:$PATH
```
* Set the path for the producer **SDK GStreamer plugin** so that GStreamer can locate it.
```
$ export GST_PLUGIN_PATH=<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/lib:$GST_PLUGIN_PATH
```

###### Discovering available devices.
Change your current working directory to `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin`;
then run the `gst-device-monitor-1.0` command to identify available media devices in your system. An example output as follows:
```
Device found:

	name  : HD Pro Webcam C920
	class : Audio/Source
	caps  : audio/x-raw, format=(string)F32LE, layout=(string)interleaved, rate=(int)16000, channels=(int)2, channel-mask=(bitmask)0x0000000000000003;
	        audio/x-raw, format=(string){ S8, U8, S16LE, S16BE, U16LE, U16BE, S24_32LE, S24_32BE, U24_32LE, U24_32BE, S32LE, S32BE, U32LE, U32BE, S24LE, S24BE, U24LE, U24BE, S20LE, S20BE, U20LE, U20BE, S18LE, S18BE, U18LE, U18BE, F32LE, F32BE, F64LE, F64BE }, layout=(string)interleaved, rate=(int)16000, channels=(int)2, channel-mask=(bitmask)0x0000000000000003;
	        audio/x-raw, format=(string){ S8, U8, S16LE, S16BE, U16LE, U16BE, S24_32LE, S24_32BE, U24_32LE, U24_32BE, S32LE, S32BE, U32LE, U32BE, S24LE, S24BE, U24LE, U24BE, S20LE, S20BE, U20LE, U20BE, S18LE, S18BE, U18LE, U18BE, F32LE, F32BE, F64LE, F64BE }, layout=(string)interleaved, rate=(int)16000, channels=(int)1;
	gst-launch-1.0 osxaudiosrc device=67 ! ...
```

----
###### Running the `gst-launch-1.0` command to start streaming from RTSP camera source.

```
$ gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=128 access-key="YourAccessKey" secret-key="YourSecretKey"
```

**Note:** If you are using **IoT credentials** then you can pass them as parameters to the gst-launch-1.0 command

```
$ gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```
You can find the RTSP URL from your IP camera manual or manufacturers product page.

###### Running the `gst-launch-1.0` command to start streaming from camera source in **Mac-OS**.

```
$ gst-launch-1.0 autovideosrc ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! vtenc_h264_hw allow-frame-reordering=FALSE realtime=TRUE max-keyframe-interval=45 bitrate=500 ! h264parse ! video/x-h264,stream-format=avc,alignment=au,profile=baseline ! kvssink stream-name=YourStreamName storage-size=128 access-key="YourAccessKey" secret-key="YourSecretKey"
```

###### Running the `gst-launch-1.0` command to start streaming both audio and raw video in **Mac-OS**.

```
gst-launch-1.0 -v avfvideosrc ! videoconvert ! vtenc_h264_hw allow-frame-reordering=FALSE realtime=TRUE max-keyframe-interval=45 ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" osxaudiosrc ! audioconvert ! avenc_aac ! queue ! sink.
```

###### Running the `gst-launch-1.0` command to start streaming both audio and h264 encoded video in **Mac-OS**.

```
gst-launch-1.0 -v avfvideosrc device-index=1 ! h264parse ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" osxaudiosrc ! audioconvert ! avenc_aac ! queue ! sink.
```

The pipeline above uses default video and audio source on a Mac. If you have an audio enable webcam plugged in, you can first use `gst-device-monitor-1.0` command mentioned above to find out the index for webcam's microphone. The example audio video pipeline using the webcam looks like follows:

```
gst-launch-1.0 -v avfvideosrc device-index=1 ! videoconvert ! vtenc_h264_hw allow-frame-reordering=FALSE realtime=TRUE max-keyframe-interval=45 ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" osxaudiosrc device=67 ! audioconvert ! avenc_aac ! queue ! sink.
```

##### Running the GStreamer webcam sample application
The sample application `kinesis_video_gstreamer_sample_app` in the `kinesis-video-native-build` directory uses GStreamer pipeline to get video data from the camera. Launch it with a stream name and it will start streaming from the camera. The user can also supply a streaming resolution (width and height) through command line arguments.

```
Usage: AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_sample_app <my_stream_name> -w <width> -h <height> -f <framerate> -b <bitrateInKBPS>
```
* **A.** If **resolution is provided** then the sample will try to check if the camera supports that resolution. If it does detect that the camera can support the resolution supplied in command line, then streaming starts; else, it will fail with an error message `Resolution not supported`.
* **B.** If **no resolution is specified**, the sample application will try to use these three resolutions **640x480, 1280x720 and 1920x1080** and will **start streaming** once the camera supported resolution is detected.

##### Running the GStreamer RTSP sample application
`kinesis_video_gstreamer_sample_app` supports sending video from a RTSP URL (IP camera). You can find the RTSP URL from your IP camera manual or manufacturers product page. Change your current working direcctory to `kinesis-video-native-build` directory. Launch it with a stream name and `rtsp_url`  and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_sample_app <my-rtsp-stream> <my_rtsp_url>
```

##### Running the GStreamer sample application to upload a *video* file

`kinesis_video_gstreamer_sample_app` supports uploading a video that is either MKV, MPEGTS, or MP4. The sample application expects the video is encoded in H264.

Change your current working directory to `kinesis-video-native-build`. Launch the sample application with a stream name and a path to the file and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_sample_app <my-stream> </path/to/file>
```

###### Running the `gst-launch-1.0` command to upload [MKV](https://www.matroska.org/) file that contains both *audio and video* in **Mac-OS**. Note that video should be H264 encoded and audio should be AAC encoded.

```
gst-launch-1.0 -v filesrc location="YourAudioVideo.mkv" ! matroskademux name=demux ! queue ! h264parse ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MP4 file that contains both *audio and video* in **Mac-OS**.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.mp4" ! qtdemux name=demux ! queue ! h264parse !  video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MPEG2TS file that contains both *audio and video* in **Mac-OS**.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.ts" ! tsdemux name=demux ! queue ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

##### Running the GStreamer sample application to upload a *audio and video* file

`kinesis_video_gstreamer_audio_video_sample_app` supports uploading a video that is either MKV, MPEGTS, or MP4. The sample application expects the video is encoded in H264 and audio is encoded in AAC format. Note: If your media uses a different format, then you can revise the pipeline elements in the sample application to suit your media format.

Change your current working directory to `kinesis-video-native-build`. Launch the sample application with a stream name and a path to the file and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream> </path/to/file>
```

##### Running the GStreamer sample application to stream audio and video from live source

`kinesis_video_gstreamer_audio_video_sample_app` supports streaming audio and video from live sources such as a audio enabled webcam. First you need to figure out what your audio device is using the steps mentioned above and export it as environment variable like such:

`export AWS_KVS_AUDIO_DEVICE=67`

You can also choose to use other video devices by doing

`export AWS_KVS_VIDEO_DEVICE=1`

If no `AWS_KVS_VIDEO_DEVICE` or `AWS_KVS_AUDIO_DEVICE` environment variable was detected, the sample app will use the default device.
After the environment variables are set, launch the sample application with a stream name and it will start streaming.
```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream>
```

##### Using Docker to run the sample application
Refer the **README.md** file in the  *docker_native_scripts* folder for running the build and **RTSP sample application** to start streaming from **IP camera** within Docker container.

##### Additional examples
For additional examples on using Kinesis Video Streams Java SDK and  Kinesis Video Streams Parsing Library refer:

##### [Kinesis Video Streams Producer Java SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md)
##### [Kinesis Video Streams Parser Library](https://github.com/aws/amazon-kinesis-video-streams-parser-library/blob/master/README.md)
##### [Kinesis Video Streams Android SDK](https://github.com/awslabs/aws-sdk-android-samples/tree/master/AmazonKinesisVideoDemoApp)
-----

##### Running C++ Unit tests
The executable for **unit tests** will be built in `./start` inside the `kinesis-video-native-build` directory. Launch it and it will run the unit test and kick off dummy frame streaming.

##### Enabling verbose logs
Define `HEAP_DEBUG` and `LOG_STREAMING` C-defines by uncommenting the appropriate lines in CMakeList.txt in the kinesis-video-native-build directory.

----
#####  How to configure logging for producer SDK sample applications.

For the sample applications included in the producer SDK (CPP), the log configuration is referred from the file  `kvs_log_configuration` (within the `kinesis-video-native-build` folder).

Refer sample configuration in the folder `kinesis-video-native-build` for details on how to set the log level (DEBUG or INFO) and output options (whether to send log output to either console or file (or both)).
* Log output messages to console:
  By default, the log configuration `log4cplus.rootLogger=DEBUG, KvsConsoleAppender` creates console appender (KvsConsoleAppender) which outputs the log messages in the console.
* Log output messages to file: By adding file appender (KvsFileAppender) in the _rootLogger_ of log4cplus as `log4cplus.rootLogger=DEBUG, KvsConsoleAppender, KvsFileAppender` the debug messages will be stored in `kvs.log` file in the sub-folder `log` within `kinesis-video-native-build` directory.  The filename for the logs and the location can be modified by changing the line `log4cplus.appender.KvsFileAppender.File=./log/kvs.log`

----
#### Troubleshooting:

##### Library not found error when running the sample application
If any error similar to the following shows that the library path is not properly set:
```
 liblog4cplus-1.2.so.5: cannot open shared object file: No such file or directory
```
To resolve this issue, export the LD_LIBRARY_PATH=`<full path to your sdk cpp directory`>/kinesis-video-native-build/downloads/local/lib. If you have downloaded the CPP SDK in `/opt/awssdk` directory then you can set
	the LD_LIBRARY_PATH as below:

```
export LD_LIBRARY_PATH=/opt/awssdk/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
```

##### Certificate issues during install-script build
If you see errors during the build as  _curl failed with 'unable to get local issuer certificate'_ then you can either follow the resolution steps in [81](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/issues/81)  or you can download the [cacert.pem](https://curl.haxx.se/ca/cacert.pem) into the `kinesis-video-native-build` folder and run
`./install-script -c`.

#####  Curl SSL issue - "unable to get local issuer certificate"
If curl throws *"Peer certificate cannot be authenticated with given CA certificates: SSL certificate problem: unable to get local issuer certificate"* error while sending data to KVS, then the local `curl`
was not built properly with `--with-ca-bundle` path. So please remove the curl binaries and libraries and rebuilt it again by following below steps.
```
rm <producer_sdk_path/kinesis-video-native-build/downloads/local/lib/libcurl*
rm <producer_sdk_path/kinesis-video-native-build/downloads/local/bin/curl*
cd <producer_sdk_path/kinesis-video-native-build/downloads/curl-7.57.0
export DOWNLOADS=<producer_sdk_path>/kinesis-video-native-build/downloads
make clean
./configure --prefix=$DOWNLOADS/local/ --enable-dynamic --disable-rtsp --disable-ldap --without-zlib --with-ssl=$DOWNLOADS/local/ --with-ca-bundle=/etc/ssl/cert.pem
make
make install
```

----
##### Open source dependencies

The projects depend on the following open source components. Running `install-script` will download and build the necessary components automatically.You can also install them in **Mac-OS** using `brew` and use the `min-install-script`.

###### Producer SDK core dependencies
* openssl (crypto and ssl) - [License](https://github.com/openssl/openssl/blob/master/LICENSE)
* libcurl - [Copyright](https://curl.haxx.se/docs/copyright.html)
* log4cplus - [License]( https://github.com/log4cplus/log4cplus/blob/master/LICENSE)
* automake 1.15.1 (GNU License)
* flex 2.5.35 Apple(flex-31)
* libtool (Apple Inc. version cctools-898)
* jsoncpp - [License](https://github.com/open-source-parsers/jsoncpp/blob/master/LICENSE)

###### Unit test dependencies
* [googletest](https://github.com/google/googletest)

###### GStreamer sample application dependencies
* gstreamer - [License]( https://gstreamer.freedesktop.org/documentation/licensing.html)
* gst-plugins-base
* gst-plugins-good
* gst-plugins-bad
* gst-plugins-ugly
* [x264]( https://www.videolan.org/developers/x264.html)
