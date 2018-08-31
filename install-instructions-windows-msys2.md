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

You can run demo samples for sending video streams from your webcam, USB camera or RTSP (IP camera) using GStreamer plugin.
1. Before running the demo applications, set the environment by following the instructions below.

- In the **MinGW** shell with the current working directory `~/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build\` run the following commands:
    * `export AWS_ACCESS_KEY_ID=YOUR_ACCESS_KEY`
    * `export AWS_SECRET_ACCESS_KEY=YOUR_SECRET_ACCESS_KEY`
    * `export GST_PLUGIN_PATH=$PWD`

2. Use `gst-launch-1.0` to send video to Kinesis Video Streams
At this point, you can use the gstreamer plugin using the command
```
gst-launch-1.0 ksvideosrc do-timestamp=TRUE ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 ! video/x-h264,profile=baseline,stream-format=avc,alignment=au,width=640,height=480,framerate=30/1 ! kvssink stream-name=stream-name access-key=YOUR_ACCESS_KEY secret-key=YOUR_SECRET_ACCESS_KEY_ID
```

**Note:** If you are using IoT credentials then you can pass them as parameters to the gst-launch-1.0 command
``` gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```

3. Now you can view the video in the Kinesis Video Streams Console (or use [HLS](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/how-hls.html) to play streams). For more information on HLS refer [here](https://aws.amazon.com/blogs/aws/amazon-kinesis-video-streams-adds-support-for-hls-output-streams/).
