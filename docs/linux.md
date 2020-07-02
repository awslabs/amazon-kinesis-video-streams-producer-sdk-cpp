### Installing libraries needed to build

* Ubuntu/Debian - `apt-get install cmake m4 git build-essential`
* RHEL/CentOS - `yum install cmake git m4 && yum groupinstall 'Development Tools'`

### How to run sample applications for sending media to KVS using [GStreamer](https://gstreamer.freedesktop.org/):

##### Setting credentials in environment variables
Define AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY environment variables with the AWS access key id and secret key:

```
$ export AWS_ACCESS_KEY_ID=YourAccessKeyId
$ export AWS_SECRET_ACCESS_KEY=YourSecretAccessKey
```
optionally, set `AWS_SESSION_TOKEN` if integrating with temporary token and `AWS_DEFAULT_REGION` for the region other than `us-west-2`

###### Discovering audio and video devices available in your system.
Run the `gst-device-monitor-1.0` command to identify available media devices in your system. An example output as follows:
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

###### Running the `gst-launch-1.0` command to start streaming from a RTSP camera source.
```
$ gst-launch-1.0 -v rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! h264parse ! kvssink stream-name=YourStreamName storage-size=128
```

**Note:** If you are using **IoT credentials** then you can pass them as parameters to the gst-launch-1.0 command
```
$ gst-launch-1.0 -v rtspsrc location="rtsp://YourCameraRtspUrl" short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 h264parse ! kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
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
$ gst-launch-1.0 -v v4l2src do-timestamp=TRUE device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! omxh264enc periodicty-idr=45 inline-header=FALSE ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName access-key="YourAccessKey" secret-key="YourSecretKey"
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

##### Running the GStreamer webcam sample application
The sample application `kinesis_video_gstreamer_sample_app` in the `build` directory uses GStreamer pipeline to get video data from the camera. Launch it with a stream name and it will start streaming from the camera. The user can also supply a streaming resolution (width and height) through command line arguments.

```
Usage: AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_sample_app <my_stream_name> -w <width> -h <height> -f <framerate> -b <bitrateInKBPS>
```
* **A.** If **resolution is provided** then the sample will try to check if the camera supports that resolution. If it does detect that the camera can support the resolution supplied in command line, then streaming starts; else, it will fail with an error message `Resolution not supported`.
* **B.** If **no resolution is specified**, the sample application will try to use these three resolutions **640x480, 1280x720 and 1920x1080** and will **start streaming** once the camera supported resolution is detected.

##### Running the GStreamer RTSP sample application
`kinesis_video_gstreamer_sample_app` supports sending video from a RTSP URL (IP camera). You can find the RTSP URL from your IP camera manual or manufacturers product page. Change your current working direcctory to `build` directory. Launch it with a stream name and `rtsp_url`  and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_sample_app <my-rtsp-stream> <my_rtsp_url>
```

##### Running the GStreamer sample application to upload a *video* file

`kinesis_video_gstreamer_sample_app` supports uploading a video that is either MKV, MPEGTS, or MP4. The sample application expects the video is encoded in H264.

Change your current working directory to `build`. Launch the sample application with a stream name and a path to the file and it will start streaming.

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

Change your current working directory to `build`. Launch the sample application with a stream name and a path to the file and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream> </path/to/file>
```

##### Running the GStreamer sample application to stream audio and video from live source

`kinesis_video_gstreamer_audio_video_sample_app` supports streaming audio and video from live sources such as a audio enabled webcam. First you need to figure out what your audio device is using the steps mentioned above and export it as environment variable like such:

`export AWS_KVS_AUDIO_DEVICE=hw:1,0`

You can also choose to use other video devices by doing

`export AWS_KVS_VIDEO_DEVICE=/dev/video1`

If no `AWS_KVS_VIDEO_DEVICE` environment variable was detected, the sample application will use the default video device.
After the environment variables are set, launch the sample application with a stream name and it will start streaming.
```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream>
```

### How to run sample applications for sending H264 video files to KVS

The sample application `kinesis_video_cproducer_video_only_sample` sends h264  video frames inside the folder `kinesis-video-c-producer/samples/h264SampleFrames` to KVS.
The following command sends the video frames in a loop for ten seconds to KVS.
`./kinesis_video_cproducer_video_only_sample YourStreamName 10`

If you want to send H264 files from another folder (`MyH264FramesFolder`) you can run the sample with the following arguments
`./kinesis_video_cproducer_video_only_sample YourStreamName 10 MyH264FramesFolder`

##### Additional examples

For additional examples on using Kinesis Video Streams Java SDK and  Kinesis Video Streams Parsing Library refer:

##### [Kinesis Video Streams Producer Java SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md)
##### [Kinesis Video Streams Parser Library](https://github.com/aws/amazon-kinesis-video-streams-parser-library/blob/master/README.md)
##### [Kinesis Video Streams Android SDK](https://github.com/awslabs/aws-sdk-android-samples/tree/master/AmazonKinesisVideoDemoApp)
----

##### Running C++ unit tests

**Note:** Please set the [credentials](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/install-instructions-linux.md#setting-credentials-in-environment-variables) before running the unit tests.

The executable for **unit tests** will be built as `./tst/producer_test` inside the `build` directory. Launch it and it will run the unit test and kick off dummy frame streaming.

##### Enabling verbose logs
Define `HEAP_DEBUG` and `LOG_STREAMING` C-defines by uncommenting the appropriate lines in CMakeList.txt

----
#####  How to configure logging for producer SDK sample applications.

For the sample demo applications included in the producer SDK (CPP), the log configuration is referred from the file  `kvs_log_configuration` (within the `build` folder).

Refer sample configuration in the folder `build` for details on how to set the log level (DEBUG or INFO) and output options (whether to send log output to either console or file (or both)).
* Log output messages to console:
  By default, the log configuration `log4cplus.rootLogger=DEBUG, KvsConsoleAppender` creates console appender (KvsConsoleAppender) which outputs the log messages in the console.
* Log output messages to file: By adding file appender (KvsFileAppender) in the _rootLogger_ of log4cplus as `log4cplus.rootLogger=DEBUG, KvsConsoleAppender, KvsFileAppender` the debug messages will be stored in `kvs.log` file in the sub-folder `log` within `build` directory.  The filename for the logs and the location can be modified by changing the line ` log4cplus.appender.KvsFileAppender.File=./log/kvs.log`

----
#####  How to enable saving c producer log into files.

By default C producer prints all logging information to stdout.

To send log information to a file (named kvsProducerLog.index), you need to use the addFileLoggerPlatformCallbacksProvider API after ClientCallbacks has been initialized.

The addFileLoggerPlatformCallbacksProvider API takes five parameters.

* First parameter is the PClientCallbacks that is created during the createCallback provider API (e.g.createDefaultCallbacksProviderWithAuthCallbacks.
* Second parameter is the size of string buffer that file logger will use. Logs are buffered in the string buffer and flushed into files when the buffer is full.
* Third parameter is the maximum number of files that file logger will generate. When the limit is reached, oldest log file will be deleted before creating the new one.
* Fourth parameter is the absolute directory path to store the log file.
* Fifth parameter uses boolean true or false and is used to allow printing logs to both stdout and a file (useful in debugging).

----

### Open Source Dependencies
The projects depend on the following open source components. Running `CMake` will download and build the necessary components automatically.

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
