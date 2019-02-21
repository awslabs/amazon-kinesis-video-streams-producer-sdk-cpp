#### Instructions for installing Kinesis Video Streams Producer SDK on Windows using [msys2](https://www.msys2.org/)

----
#### Pre-requisites:

* The following Windows platforms are supported.
```
    Windows 7 Enterprise
    Windows 10 Enterprise
    Windows Server 2016
    Windows Server 2012 R2
```
* In order to install the build tools, account with administrator privileges is required.

----
#### Installation:
Follow the steps below to build the Kinesis Video Streams SDK on **Windows** platform:

###### Step 1:  Download and install the correct version of [msys2](https://www.msys2.org/)
MinGW (Minimalist GNU for Windows) is a minimalist development environment for creating native Microsoft Windows applications. Please refer [install instructions](https://github.com/msys2/msys2/wiki/MSYS2-installation) for **downloading and installing** [msys2](https://www.msys2.org/).
> msys2 provides a bash shell, Autotools, revision control systems and the like for building native Windows applications using MinGW-w64 toolchains.
>

###### Step 2: Start the MinGW shell
Use Windows explorer to navigate to **mingw64.exe** (or mingw32.exe) which is installed in the MSYS2 installation directory **`C:\msys64\`** (or `C:\msys32\`). Start up the MinGW shell window by double clicking the  **mingw64.exe** (or mingw32.exe). This will open the MinGW shell.

###### Step 3: Install Git
Run the following command to install Git.
`pacman -S git`

###### Step 4: Get the latest Kinesis Video Streams Producer SDK from [GitHub](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/).
Run the command `git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git` to get the latest SDK.
The current directory path should look like `C:\msys64\home\<myuser>\amazon-kinesis-video-streams-producer-sdk-cpp\`.


###### Step 5: Navigate to the build directory in the Kinesis Video Streams SDK (`~/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build\`)
The MinGW shell by default places the prompt in your home directory. In msys2, `~` corresponds to `C:\msys64\home\<myuser>`. Now, inside MinGW shell, navigate to the directory kinesis-video-native-build within the SDK.

**Example:**
   `cd ~/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build`

Now the MingW shell prompt will look similar to the one below.
```
<myuser>@SE MINGW64 ~/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build
$
```

###### Step 6: Build the Kinesis Video Streams Producer SDK using msys2-install-script.
Build the SDK by running the command  `./msys2-install-script -a`
Press Enter to all prompts that show up during the install the process. When the install script finishes, the demo executables and libraries will be in the kinesis-video-native-build directory. Please note that this step could take about **25 mins**.

----
#### Running the sample demo applications to stream video to Kinesis Video Streams:

##### Discovering available devices:

Run the command `gst-device-monitor-1.0` to discover available devices. A sample output of the command looks like following:
```
Probing devices...


Device found:

        name  : HP Wide Vision HD
        class : Video/Source
        caps  : video/x-raw, format=(string)YUY2, width=(int)640, height=(int)480, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                video/x-raw, format=(string)YUY2, width=(int)176, height=(int)144, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)12/11;
                video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                video/x-raw, format=(string)YUY2, width=(int)352, height=(int)288, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)12/11;
                video/x-raw, format=(string)YUY2, width=(int)640, height=(int)360, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                video/x-raw, format=(string)YUY2, width=(int)1280, height=(int)720, framerate=(fraction)10/1, pixel-aspect-ratio=(fraction)1/1;
                image/jpeg, width=(int)640, height=(int)480, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                image/jpeg, width=(int)176, height=(int)144, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)12/11;
                image/jpeg, width=(int)320, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                image/jpeg, width=(int)352, height=(int)288, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)12/11;
                image/jpeg, width=(int)640, height=(int)360, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
                image/jpeg, width=(int)1280, height=(int)720, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1;
        gst-launch-1.0 ksvideosrc device-path="\\\\\?\\usb\#vid_05c8\&pid_038e\&mi_00\#6\&20a8b444\&0\&0000\#\{6994ad05-93ef-11d0-a3cc-00a0c9223196\}\\global" ! ...


Device found:

        name  : Microphone Array (Conexant ISST Audio)
        class : Audio/Source
        caps  : audio/x-raw, format=(string)F32LE, layout=(string)interleaved, rate=(int)48000, channels=(int)2, channel-mask=(bitmask)0x0000000000000003;
        properties:
                device.api = wasapi
                device.strid = "\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}"
                wasapi.device.description = "Microphone\ Array\ \(Conexant\ ISST\ Audio\)"
        gst-launch-1.0 wasapisrc device="\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}" ! ...


Device found:

        name  : Speakers (Conexant ISST Audio)
        class : Audio/Sink
        caps  : audio/x-raw, format=(string)F32LE, layout=(string)interleaved, rate=(int)48000, channels=(int)2, channel-mask=(bitmask)0x0000000000000003;
        properties:
                device.api = wasapi
                device.strid = "\{0.0.0.00000000\}.\{42dfdea4-9253-459c-80a0-c6c107ac5def\}"
                wasapi.device.description = "Speakers\ \(Conexant\ ISST\ Audio\)"
        gst-launch-1.0 ... ! wasapisink device="\{0.0.0.00000000\}.\{42dfdea4-9253-459c-80a0-c6c107ac5def\}"
```


##### Running the `kinesis_video_gstreamer_sample_app`

You can run demo samples for sending video streams from your webcam, USB camera or RTSP (IP camera) using GStreamer plugin.
1. Before running the demo applications, set the environment by following the instructions below.

- In the **MinGW** shell with the current working directory `~/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build\` run the following commands:
    * `export AWS_ACCESS_KEY_ID=YOUR_ACCESS_KEY`
    * `export AWS_SECRET_ACCESS_KEY=YOUR_SECRET_ACCESS_KEY`
    * `export GST_PLUGIN_PATH=$PWD`

2. Use `gst-launch-1.0` to send video to Kinesis Video Streams
At this point, you can use the gstreamer plugin using the command
```
gst-launch-1.0 ksvideosrc ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 tune=zerolatency ! video/x-h264,profile=baseline,stream-format=avc,alignment=au ! kvssink stream-name=stream-name access-key=YOUR_ACCESS_KEY secret-key=YOUR_SECRET_ACCESS_KEY_ID
```

**Note:** If you are using IoT credentials then you can pass them as parameters to the gst-launch-1.0 command
```
gst-launch-1.0 rtspsrc location="rtsp://YourCameraRtspUrl" short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```

3. Now you can view the video in the Kinesis Video Streams Console (or use [HLS](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/how-hls.html) to play streams). For more information on HLS refer [here](https://aws.amazon.com/blogs/aws/amazon-kinesis-video-streams-adds-support-for-hls-output-streams/).

----
###### Running the GStreamer sample application to upload a *audio and video* file

`kinesis_video_gstreamer_audio_video_sample_app` supports uploading a video that is either MKV, MPEGTS, or MP4. The sample application expects the video is encoded in H264 and audio is encoded in AAC format. Note: If your media uses a different format, then you can revise the pipeline elements in the sample application to suit your media format.

Change your current working directory to `kinesis-video-native-build`. Launch the sample application with a stream name and a path to the file and it will start streaming.

```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream> </path/to/file>
```

###### Running the GStreamer sample application to stream audio and video from live source

`kinesis_video_gstreamer_audio_video_sample_app` supports streaming audio and video from live sources such as an audio enabled webcam. First you need to figure out what your audio device is using the steps mentioned above and export it as environment variable like such:

`export AWS_KVS_AUDIO_DEVICE='{0.0.1.00000000}.{f1245929-0c97-4389-9c28-1ca9cb01576b}'`

You can also choose to use other video devices by doing

`export AWS_KVS_VIDEO_DEVICE='\\?\usb#vid_05c8&pid_038e&mi_00#6&20a8b444&0&0000#{6994ad05-93ef-11d0-a3cc-00a0c9223196}\global'`

If no `AWS_KVS_VIDEO_DEVICE` environment variable was detected, the sample app will use the default device. Also note that `gst-device-monitor-1.0` output device paths that contain escape characters. Before assigning the device paths to the environment variables, they need to be unescaped as shown in the example. Also note that single quote is used to pass the exact string. Any misconfiguration of devices in windows will cause a segmentation fault.
After the environment variables are set, launch the sample application with a stream name and it will start streaming.
```
AWS_ACCESS_KEY_ID=YourAccessKeyId AWS_SECRET_ACCESS_KEY=YourSecretAccessKey ./kinesis_video_gstreamer_audio_video_sample_app <my-stream>
```

----
###### Running the `gst-launch-1.0` command to upload [MKV](https://www.matroska.org/) file that contains both *audio and video* in **Mingw Shell**. Note that video should be H264 encoded and audio should be AAC encoded.

```
gst-launch-1.0 -v filesrc location="YourAudioVideo.mkv" ! matroskademux name=demux ! queue ! h264parse ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MPEG2TS file that contains both *audio and video* in **Mingw Shell**.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.mp4" ! qtdemux name=demux ! queue ! h264parse !  video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MP4 file that contains both *audio and video* in **Mingw Shell**.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.ts" ! tsdemux name=demux ! queue ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to live stream audio and raw video in **Mingw Shell**.

```
gst-launch-1.0 -v ksvideosrc ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 tune=zerolatency ! video/x-h264,profile=baseline,stream-format=avc,alignment=au ! kvssink name=sink stream-name=stream-name access-key=YOUR_ACCESS_KEY secret-key=YOUR_SECRET_ACCESS_KEY_ID wasapisrc device="\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}" ! audioconvert ! avenc_aac ! queue ! sink.
```

###### Running the `gst-launch-1.0` command to live stream audio and h264 encoded video in **Mingw Shell**.

```
gst-launch-1.0 -v ksvideosrc ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name=stream-name access-key=YOUR_ACCESS_KEY secret-key=YOUR_SECRET_ACCESS_KEY_ID wasapisrc device="\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}" ! audioconvert ! avenc_aac ! queue ! sink.
```
