#### Instructions for installing Kinesis Video Streams Producer SDK on Linux (Ubuntu, Raspberry PI)

----
##### Pre-requisites:
The following Ubuntu platforms are supported.
```
    Ubuntu 16
    Ubuntu 17
    Ubuntu 18
    Raspberry Raspbian
```
* [Git](https://git-scm.com/downloads) is required for checking out the Kinesis Video Streams SDK.
* In order to install the build tools, an account with administrator privileges is required.

----
### Installation
In order to build the Producer SDK and download open source dependencies the following are required. One option is to use `apt-get install` to install the following build tool dependencies.

##### Install the Build tools

* cmake 3.7/3.8 https://cmake.org/
* gcc and g++
* pkgconfig
* Java JDK (if Java JNI compilation is required)

###### Install the certificate in the operating system certificate store
Kinesis Video Streams Producer SDK for C++ needs to establish trust with the backend service through TLS. This is done through validating the CAs in the public certificate store. On Linux-based models, this store is located in `/etc/ssl/` directory by default.

Please download the PEM file from [SFSRootCAG2.pem](https://www.amazontrust.com/repository/SFSRootCAG2.pem)
to `/etc/ssl/cert.pem`. If the file already exists `/etc/ssl/cert.pem` then you can append it by running `sudo cat SFSRootCAG2.pem >> /etc/ssl/cert.pem`.

Many platforms come with a cert file with a lot of the well-known public certs in them.

----
### Build the Kinesis Video Producer SDK and sample applications:

##### Build the SDK and sample applications using open source library dependencies built from source

The **install-script** will download and build the dependent open source components (from the source) into the **downloads** directory within `kinesis-video-native-build` directory (e.g. `/home/<myuser>/downloads/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads`) and link against it.

**After you've downloaded the code from GitHub, you can build it on Linux by running `./install-script` (which is inside the `kinesis-video-native-build` directory)**.

**Important:**  Change the _**current working directory**_ to `kinesis-video-native-build` directory first. Then run the `./install-script` from that directory.

 The `install-script` will take some time (about 25 minutes) to bring down and compile the open source components depending on the network connection and processor speed. At the end of the installation, this step will produce the core library, unit tests executable and the sample GStreamer application. If anything fails or the script is interrupted, re-running it will pick up from the place where it last left off. The sub-sequent run will be building just the modified SDK components or applications which is much faster.

Note that the install-script also builds the Kinesis Video Streams producer SDK as a **GStreamer plugin** (**kvssink**).

##### Alternate option to build the SDK and sample applications using system versions of open source library dependencies

The bulk of the **install-script** is building the open source dependencies. The project is based on **CMake**. So the open source components building can be skipped if the system versions of the open source dependencies are already installed that can be used for linking. See the section **Install Steps for Ubuntu 17.x and Raspbian Stretch using apt-get** for detailed instructions on how to install using `apt-get install` on **Ubuntu** .

The `./min-install-script` inside the `kinesis-video-native-build` captures these steps for building the Kinesis Video Streams Producer SDK with the system versions for linking.

##### Optionally build the native library (KinesisVideoProducerJNI) to run Java demo streaming application

The `./java-install-script` inside `kinesis-video-native-build` will build the KinesisVideoProducerJNI native library to be used by [Java Producer SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md).

----

### Install Steps for Ubuntu 17.x and Raspbian Stretch using apt-get

The following section provides guidance for installing the build tools and open source dependencies using `apt-get` and has been tested in **Ubuntu Linux** and **Raspbian Stretch**.

Install **git**:

```
$ sudo apt-get update
$ sudo apt-get install git
```
Install **cmake**:
```
sudo apt-get update
$ sudo apt-get install cmake
```
Install **g++**:
```
$ sudo apt-get install g++
$ g++ --version
g++ (Ubuntu 7.2.0-8ubuntu3) 7.2.0
Copyright (C) 2017 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
Install **Producer Library Dependencies**:

```
$ sudo apt-get update
$ sudo apt-get install libssl-dev libcurl4-openssl-dev liblog4cplus-1.1-9 liblog4cplus-dev
```
Install **Gstreamer Artifact Dependencies**:
```
$ sudo apt-get update
$ sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base-apps
$ sudo apt-get install gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-tools
```
If you are using Raspberry pi, you can also install the `gstreamer1.0-omx` package to get the omxh264enc hardware encoder
```
sudo apt-get install gstreamer1.0-omx
```
Run the build script: (within `kinesis-video-native-build` folder)
```
./min-install-script
```

Install **Open JDK** (if you are building the JNI library):
```
$ sudo apt-get install openjdk-8-jdk
$ java -showversion
openjdk version "1.8.0_151"
OpenJDK Runtime Environment (build 1.8.0_151-8u151-b12-0ubuntu0.17.10.2-b12)
OpenJDK 64-Bit Server VM (build 25.151-b12, mixed mode)
```
Set **JAVA_HOME** environment variable:
```
$ export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64/
```

Run the build script: (within `kinesis-video-native-build` folder)
```
$ ./java-install-script
```

----
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
At the end of installation (with `install-script`), environment setup is saved in the `set_kvs_sdk_env.sh`.  Next time you want to run the demo applications or use gst-launch-1.0 to send video to Kinesis Video Streams, you can run this script by `. ./set_kvs_sdk_env.sh` or `source set_kvs_sdk_env.sh`
within the `kinesis-video-native-build` directory. This set up the environment for **LD_LIBRARY_PATH**, **PATH** and **GST_PLUGIN_PATH**. You can also set these manually as below.
* Export the **LD_LIBRARY_PATH**=`<full path to your sdk cpp directory`>/kinesis-video-native-build/downloads/local/lib. For example, if you have downloaded the CPP SDK in `/opt/awssdk` directory then you can set
the LD_LIBRARY_PATH as below:
```
export LD_LIBRARY_PATH=/opt/awssdk/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
```
* (Skip this if you installed gstreamer using apt-get) The `gst-launch-1.0` and `gst-inspect-1.0` binaries are built inside the folder `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin`. You can either run the following commands from that folder using `./gst-launch-1.0` or you can include that in your **PATH** environment variable using the following export command and run `gst-launch-1.0`
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
Probing devices...


Device found:

	name  : H264 USB Camera: USB Camera
	class : Video/Source
	caps  : video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)1920, height=(int)1080, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)1920, height=(int)1080, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)1280, height=(int)720, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)800, height=(int)600, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)640, height=(int)480, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)640, height=(int)360, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)352, height=(int)288, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	        video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, width=(int)320, height=(int)240, pixel-aspect-ratio=(fraction)1/1, colorimetry=(string){ 2:4:7:1 }, framerate=(fraction){ 30/1, 25/1, 15/1 };
	properties:
		device.path = /dev/video4
		udev-probed = false
		device.api = v4l2
		v4l2.device.driver = uvcvideo
		v4l2.device.card = "H264\ USB\ Camera:\ USB\ Camera"
		v4l2.device.bus_info = usb-3f980000.usb-1.3
		v4l2.device.version = 265767 (0x00040e27)
		v4l2.device.capabilities = 2216689665 (0x84200001)
		v4l2.device.device_caps = 69206017 (0x04200001)
	gst-launch-1.0 v4l2src device=/dev/video4 ! ...
```

###### Running the `gst-launch-1.0` command to start streaming from RTSP camera source.
```
$ gst-launch-1.0 -v rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=128
```

**Note:** If you are using **IoT credentials** then you can pass them as parameters to the gst-launch-1.0 command
```
$ gst-launch-1.0 -v rtspsrc location="rtsp://YourCameraRtspUrl" short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```
You can find the RTSP URL from your IP camera manual or manufacturers product page.

###### Running the `gst-launch-1.0` command to start streaming from USB camera source in **Ubuntu**.
```
$ gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! x264enc  bframes=0 key-int-max=45 bitrate=500 tune=zerolatency ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=128 access-key="YourAccessKey" secret-key="YourSecretKey"
```
###### Running the `gst-launch-1.0` command to start streaming from USB camera source which has h264 encoded stream already in **Ubuntu** and **Raspberry-PI**.
```
$ gst-launch-1.0 -v v4l2src device=/dev/video0 ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=128 access-key="YourAccessKey" secret-key="YourSecretKey"
```

###### Running the `gst-launch-1.0` command to start streaming from camera source in **Raspberry-PI**.

```
$ gst-launch-1.0 -v v4l2src do-timestamp=TRUE device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! omxh264enc periodicity-idr=45 inline-header=FALSE ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName access-key="YourAccessKey" secret-key="YourSecretKey"
```
or use a different encoder
```
$ gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! x264enc  bframes=0 key-int-max=45 bitrate=500 tune=zerolatency ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=128 access-key="YourAccessKey" secret-key="YourSecretKey"
```


###### Running the `gst-launch-1.0` command to start streaming both audio and video in **Raspberry-PI** and **Ubuntu**.

Please ensure that audio drivers are installed first by running

`apt-get install libasound2-dev`

then you can use the following following command to find the capture card and device number.

`arecord -l (or arecord --list-devices)`

the output should look like the following:

```
**** List of CAPTURE Hardware Devices ****
card 2: U0x46d0x825 [USB Device 0x46d:0x825], device 0: USB Audio [USB Audio]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 3: Camera [H264 USB Camera], device 0: USB Audio [USB Audio]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
```

The audio recording device is represented by `hw:card_number,device_numer`. So to use the second device in the example, use `hw:3,0` as the device in `gst-launch-1.0` command.

Raspberry Pi:

```
gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,width=640,height=480,framerate=30/1,format=I420 ! omxh264enc periodicty-idr=45 inline-header=FALSE ! h264parse ! video/x-h264,stream-format=avc,alignment=au,profile=baseline ! kvssink name=sink stream-name="my-stream-name" access-key="YourAccessKey" secret-key="YourSecretKey" alsasrc device=hw:1,0 ! audioconvert ! avenc_aac ! queue ! sink.
```

Ubuntu:
```
gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,width=640,height=480,framerate=30/1,format=I420 ! x264enc  bframes=0 key-int-max=45 bitrate=500 tune=zerolatency ! h264parse ! video/x-h264,stream-format=avc,alignment=au,profile=baseline ! kvssink name=sink stream-name="my-stream-name" access-key="YourAccessKey" secret-key="YourSecretKey" alsasrc device=hw:1,0 ! audioconvert ! avenc_aac ! queue ! sink.
```

if your camera supports outputting h264 encoded stream directly, then you can use this command in both Raspberry Pi and Ubuntu:
```
gst-launch-1.0 -v v4l2src device=/dev/video0 ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="my-stream-name" access-key="YourAccessKey" secret-key="YourSecretKey" alsasrc device=hw:1,0 ! audioconvert ! avenc_aac ! queue ! sink.
```

if you get errors like `WARNING: erroneous pipeline: no element "alsasrc"`, make sure that **libasound2-dev** was installed, then delete `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/lib/libgstvideo-1.0.so` to trigger rebuilding gst-plugin-base and run the **install-script** again.

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

###### Running the `gst-launch-1.0` command to upload [MKV](https://www.matroska.org/) file that contains both *audio and video* in **Raspberry-PI**. Note that video should be H264 encoded and audio should be AAC encoded.

```
gst-launch-1.0 -v filesrc location="YourAudioVideo.mkv" ! matroskademux name=demux ! queue ! h264parse ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MP4 file that contains both *audio and video* in **Raspberry-PI**.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.mp4" ! qtdemux name=demux ! queue ! h264parse !  video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MPEG2TS file that contains both *audio and video* in **Raspberry-PI**.

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

`export AWS_KVS_AUDIO_DEVICE=hw:1,0`

You can also choose to use other video devices by doing

`export AWS_KVS_VIDEO_DEVICE=/dev/video1`

If no `AWS_KVS_VIDEO_DEVICE` environment variable was detected, the sample app will use the default video device.
After the environment variables are set, launch the sample application with a stream name and it will start streaming.
```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream>
```

##### 4. Run the demo application from Docker

Refer the **README.md** file in the  *docker_native_scripts* folder for running the build and RTSP demo application within Docker container.

##### Additional examples

For additional examples on using Kinesis Video Streams Java SDK and  Kinesis Video Streams Parsing Library refer:

##### [Kinesis Video Streams Producer Java SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md)
##### [Kinesis Video Streams Parser Library](https://github.com/aws/amazon-kinesis-video-streams-parser-library/blob/master/README.md)
##### [Kinesis Video Streams Android SDK](https://github.com/awslabs/aws-sdk-android-samples/tree/master/AmazonKinesisVideoDemoApp)
----

##### Running C++ Unit tests

The executable for **unit tests** will be built in `./start` inside the `kinesis-video-native-build` directory. Launch it and it will run the unit test and kick off dummy frame streaming.

##### Enabling verbose logs
Define `HEAP_DEBUG` and `LOG_STREAMING` C-defines by uncommenting the appropriate lines in CMakeList.txt in the kinesis-video-native-build directory.


----
#####  How to configure logging for producer SDK sample applications.

For the sample demo applications included in the producer SDK (CPP), the log configuration is referred from the file  `kvs_log_configuration` (within the `kinesis-video-native-build` folder).

Refer sample configuration in the folder `kinesis-video-native-build` for details on how to set the log level (DEBUG or INFO) and output options (whether to send log output to either console or file (or both)).
* Log output messages to console:
  By default, the log configuration `log4cplus.rootLogger=DEBUG, KvsConsoleAppender` creates console appender (KvsConsoleAppender) which outputs the log messages in the console.
* Log output messages to file: By adding file appender (KvsFileAppender) in the _rootLogger_ of log4cplus as `log4cplus.rootLogger=DEBUG, KvsConsoleAppender, KvsFileAppender` the debug messages will be stored in `kvs.log` file in the sub-folder `log` within `kinesis-video-native-build` directory.  The filename for the logs and the location can be modified by changing the line ` log4cplus.appender.KvsFileAppender.File=./log/kvs.log`

----

### Open Source Dependencies
The projects depend on the following open source components. Running `install-script` will download and build the necessary components automatically.

#### Producer SDK Core
* openssl (crypto and ssl) - [License](https://github.com/openssl/openssl/blob/master/LICENSE)
* curl lib - [Copyright](https://curl.haxx.se/docs/copyright.html)
* log4cplus - [License]( https://github.com/log4cplus/log4cplus/blob/master/LICENSE)
* jsoncpp - [License](https://github.com/open-source-parsers/jsoncpp/blob/master/LICENSE)

#### Unit Tests
* [googletest](https://github.com/google/googletest)

#### GStreamer Demo App
* gstreamer - [License]( https://gstreamer.freedesktop.org/documentation/licensing.html)
* gst-plugins-base
* gst-plugins-good
* gst-plugins-bad
* gst-plugins-ugly
* [x264]( https://www.videolan.org/developers/x264.html)

----

### Troubleshooting:

##### Ubuntu builds link issues
Ubuntu bulds link against the system versions of the open source component libraries or missing .so files (./start in the kinesis-video-native-build directory shows linkage against system versions of the open source libraries).We are working on providing fix but the immediate steps to remedy is to run
```
  rm -rf ./kinesis-video-native-build/CMakeCache.txt ./kinesis-video-native-build/CMakeFiles
```
  and run
```
 ./install-script
```
   to rebuild and re-link the project only.

##### Certificate issues during install-script build
If you see errors during the build as  _curl failed with 'unable to get local issuer certificate'_ then you can either follow the resolution steps in [81](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/issues/81)  or you can download the [cacert.pem](https://curl.haxx.se/ca/cacert.pem) into the `kinesis-video-native-build` folder and run
`./install-script -c`.

##### Library not found error when running the demo application
If any error similar to the following shows that the library path is not properly set:
```
 liblog4cplus-1.2.so.5: cannot open shared object file: No such file or directory
```
To resolve this issue, export the LD_LIBRARY_PATH=`<full path to your sdk cpp directory`>/kinesis-video-native-build/downloads/local/lib. If you have downloaded the CPP SDK in `/opt/awssdk` directory then you can set
  the LD_LIBRARY_PATH as below:

```
export LD_LIBRARY_PATH=/opt/awssdk/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
```

##### Raspberry PI failure to load the camera device.
To check this is the case run `ls /dev/video*` - it should be file not found. The remedy is to run the following:
```
$ls /dev/video*
{not found}
```
```
$vcgencmd get_camera
```
Example output:
```
supported=1 detected=1
```
if the driver does not detect the camera then

* Check for the camera setup and whether it's connected properly
* Run firmware update `$ sudo rpi-update` and restart
```
$sudo modprobe bcm2835-v4l2
```
```
$ls /dev/video*
{lists the device}
```

##### Raspberry PI timestamp/range assertion at runtime.
Update the Raspberry PI firmware.
```
$ sudo rpi-update
$ sudo reboot
```

* Raspberry PI GStreamer assertion on gst_value_set_fraction_range_full: assertion 'gst_util_fraction_compare (numerator_start, denominator_start, numerator_end, denominator_end) < 0' failed. The uv4l service running in the background. Kill the service and restart the sample app.

##### Raspberry PI seg faults after some time running on `libx264.so`.
Rebuilding the `libx264.so` library and **re-linking the demo application** fixes the issue.

##### Curl SSL issue - "unable to get local issuer certificate"
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

##### When `kinesis_video_gstreamer_audio_video_sample_app` failed with: `WARNING: erroneous pipeline: no element "alsasrc"`
if you get errors like `WARNING: erroneous pipeline: no element "alsasrc"`, make sure that **libasound2-dev** was installed, then delete `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/lib/libgstvideo-1.0.so` to trigger rebuilding gst-plugin-base and run the **install-script** again.