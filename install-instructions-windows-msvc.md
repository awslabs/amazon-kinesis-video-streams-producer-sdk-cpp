#### Instructions for installing Kinesis Video Streams Producer SDK on Windows using MSVC (Microsoft Visual C++)

----
#### Pre-requisites:
* The following Windows platforms are supported.
```
    Windows 7 Enterprise
    Windows 10 Enterprise
    Windows Server 2016
    Windows Server 2012 R2
```
* [Git](https://git-scm.com/downloads) is required for checking out the Kinesis Video Streams SDK.
* In order to install the build tools, account with administrator privileges is required.

----
##### Build tools:
1.  **Microsoft Visual C++** requires **.Net Framework** 4.6.1 or above and  Windows 7 includes **.Net Framework** 3.5.
For upgrading to 4.7.2 **.Net Framework**, download the [installer](https://www.microsoft.com/net/download/thank-you/net472?utm_source=ms-docs&utm_medium=referral).
Additional details for upgrading existing **.Net Framework** can be found in [Microsoft documentation](https://docs.microsoft.com/en-us/dotnet/framework/install/on-windows-7)

2. **Powershell** version 5.0 or above is required and is available by default in Windows 10. For Windows 7, follow the link to update Powershell to 5.1
 https://www.microsoft.com/en-us/download/details.aspx?id=54616 .
 Additional details for upgrading existing powershell can be found in
 https://docs.microsoft.com/en-us/powershell/scripting/setup/installing-windows-powershell?view=powershell-6#upgrading-existing-windows-powershell

3. If you already have Kinesis Video Streams SDK with **MinGW** installed, then you have to run clean up the SDK directory before installing the SDK using MSVC (**Microsoft Visual C++**) by using the steps below.
  * Clean up steps:
    *  If any issue happens in the install process, or if you have already installed and built the Kinesis Video Streams SDK, then delete the directories inside the ` kinesis-video-native-build/downloads ` before starting a new build.
      *  Remove **CMakeFiles** directory and **CMakeCachedList.txt** in `kinesis-video-native-build` directory.

----
#### Install Steps:
Follow the steps below to build the Kinesis Video Streams SDK on **Windows** platform:

###### Step 1: Get the latest Kinesis Video Streams Producer SDK from [GitHub](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/).
Running the command ` git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git ` will fetch latest Kinesis Video Producer SDK to your current directory.

_**Note:**_
* In the following steps, you need to replace `<myuser>` with your Windows username.

###### Step 2. Open Windows command prompt with administrator privileges.
You can use **any one** of the following instructions to open windows command prompt with administrator rights.
* Press `win` key to open the [windows start menu](https://support.microsoft.com/en-us/help/4028294/windows-open-the-start-menu). Type in `cmd` to search for Command Prompt. Press the keys `ctrl+shift+enter` to launch Command Prompt as administrator.
* Type ` runas /user:Administrator cmd` . Enter the password for the administrator account.

###### Step 3.Change **current work directory** to `kinesis-video-native-build`
  `cd C:\Users\myuser\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build`

###### Step 4. Run the Visual Studio build tools install script (`vs-buildtools-install.bat`)
`C:\Users\myuser\Downloads\amazon-kinesis-video-streams-producer-sdk-cp\kinesis-video-native-build> vs-buildtools-install.bat`

###### Step 5. Reboot your computer (Not required for Windows server)
This step is required to get the Visual Studio environment available for building the Kinesis Video Streams SDK.

###### Step 6. Open Windows command prompt wtih administrator privileges again.
You can use **any one** of the following instructions to open windows command prompt with administrator rights.
* Press `win` key to open the [windows start menu](https://support.microsoft.com/en-us/help/4028294/windows-open-the-start-menu). Type in `cmd` to search for **Command Prompt**. Press the keys `ctrl+shift+enter` to launch **Command Prompt** as administrator.
* Type ` runas /user:Administrator cmd `  and the password for the administrator account.

###### Step 7. Change current work directory to C:\Users\myuser\Downloads\amazon-kinesis-video-streams-producer-sdk-cp\kinesis-video-native-build
  `cd C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build`

###### Step 8. Install Kinesis Video Producer by running `windows-install.bat 64` **(recommended)** to build the SDK for Windows OS 64 bit (or `windows-install.bat 32` Windows OS 32 bit).
   Run `windows-install.bat 64` in the command prompt to build the SDK for Windows OS 64 bit.
   *  **Example:** To build 64 bit release of the Kinesis Video Streams producer SDK you can run
      *  `C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build>windows-install.bat 64`

Note that the `windows-install.bat` script also builds the Kinesis Video Streams producer SDK as a **GStreamer plugin** (**kvssink**).

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
```

Start sample application to send video stream to KVS using gstreamer plugin by executing the following command:

1.  Before running the demo applications, set the environment by following the instructions below.

```
set PATH=%PATH%:C:\Users\<myuser\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\downloads\gstreamer\1.0\x86_64\bin
set GST_PLUGIN_PATH=C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\Release
set GST_PLUGIN_SYSTEM_PATH=C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\downloads\gstreamer\1.0\x86_64\lib\gstreamer-1.0
```

2.1  Use `gst-launch-1.0` to send video to Kinesis Video Streams

**Example:**

```
gst-launch-1.0 ksvideosrc do-timestamp=TRUE ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 ! video/x-h264,profile=baseline,stream-format=avc,alignment=au ! kvssink stream-name="stream-name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey"
```

**Note:** If you are using IoT credentials then you can pass them as parameters to the gst-launch-1.0 command

```
gst-launch-1.0 rtspsrc location="rtsp://YourCameraRtspUrl" short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```

2.2 Use `gst-launch-1.0` to send audio and raw video to Kinesis Video Streams

```
gst-launch-1.0 -v ksvideosrc ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 tune=zerolatency ! video/x-h264,profile=baseline,stream-format=avc,alignment=au ! kvssink name=sink stream-name="stream-name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" wasapisrc device="\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}" ! audioconvert ! avenc_aac ! queue ! sink.
```

2.3 Use `gst-launch-1.0` to send audio and h264 encoded video to Kinesis Video Streams

```
gst-launch-1.0 -v ksvideosrc ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="stream-name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" wasapisrc device="\{0.0.1.00000000\}.\{f1245929-0c97-4389-9c28-1ca9cb01576b\}" ! audioconvert ! avenc_aac ! queue ! sink.
```

2.4 Use `gst-launch-1.0` command to upload file that contains both *audio and video*. Note that video should be H264 encoded and audio should be AAC encoded.

###### Running the `gst-launch-1.0` command to upload [MKV](https://www.matroska.org/) file that contains both *audio and video*.

```
gst-launch-1.0 -v filesrc location="YourAudioVideo.mkv" ! matroskademux name=demux ! queue ! h264parse ! kvssink name=sink stream-name="my_stream_name" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MPEG2TS file that contains both *audio and video*.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.mp4" ! qtdemux name=demux ! queue ! h264parse !  video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

###### Running the `gst-launch-1.0` command to upload MP4 file that contains both *audio and video*.

```
gst-launch-1.0 -v  filesrc location="YourAudioVideo.ts" ! tsdemux name=demux ! queue ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink name=sink stream-name="audio-video-file" access-key="YourAccessKeyId" secret-key="YourSecretAccessKey" streaming-type=offline demux. ! queue ! aacparse ! sink.
```

3. You can also run the the sample application by executing the following command which will send video to Kinesis Video Streams.
    * Change your current working directory to Release directory first.
      * ` cd C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\Release `
      * export your access key and secret key by doing:

      ```
      set AWS_ACCESS_KEY_ID=YourAccessKeyId
      set AWS_SECRET_ACCESS_KEY=YourSecretAccessKey
      ```

      * Run the demo
          * **Example**:
            * Run the sample demo application for sending **webcam video** by executing ` kinesis_video_gstreamer_sample_app.exe my-test-stream `  or
            * Run the sample application for sending **IP camera video** by executing  `kinesis_video_gstreamer_sample_app.exe my-test-rtsp-stream <rtsp_url>`
            * Run the sample application for sending **MKV File** by executing  `kinesis_video_gstreamer_sample_app.exe my-test-stream <path/to/file.mkv>`

4. You can also run the the sample application by executing the following command which will send audio and video to Kinesis Video Streams.
    * Change your current working directory to Release directory first.
      * ` cd C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\Release `
      * export your access key and secret key by doing:

      ```
      set AWS_ACCESS_KEY_ID=YourAccessKeyId
      set AWS_SECRET_ACCESS_KEY=YourSecretAccessKey
      ```

      * Figure out what your audio device is by running `gst-device-monitor-1.0` and export it as environment variable like such:

        `set AWS_KVS_AUDIO_DEVICE='{0.0.1.00000000}.{f1245929-0c97-4389-9c28-1ca9cb01576b}'`

        You can also choose to use other video devices by doing

        `set AWS_KVS_VIDEO_DEVICE='\\?\usb#vid_05c8&pid_038e&mi_00#6&20a8b444&0&0000#{6994ad05-93ef-11d0-a3cc-00a0c9223196}\global'`

        If no `AWS_KVS_VIDEO_DEVICE` environment variable was detected, the sample app will use the default device. Also note that `gst-device-monitor-1.0` output device paths that contain escape characters. Before assigning the device paths to the environment variables, they need to be unescaped as shown in the example. Also note that single quote is used to pass the exact string. Any misconfiguration of devices in windows will cause a segmentation fault.
      * Run the demo
          * **Example**:
            * Run the sample demo application for sending **webcam audio and video** by executing ` kinesis_video_gstreamer_audio_video_sample_app.exe my-test-stream `  or
            * Run the sample application for sending a MKV, MP4 or TS, file containing H264 video and AAC audio by executing  `kinesis_video_gstreamer_audio_video_sample_app.exe my-test-stream </path/to/file.mkv>`
