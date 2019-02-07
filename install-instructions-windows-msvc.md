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
Start sample application to send video stream to KVS using gstreamer plugin by executing the following command:
1.  Before running the demo applications, set the environment by following the instructions below.
```
set PATH=%PATH%:C:\Users\<myuser\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\downloads \gstreamer\1.0\x86_64\bin
set GST_PLUGIN_PATH=C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\Release
set GST_PLUGIN_SYSTEM_PATH=C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\downloads\gstreamer\1.0\x86_64\lib\gstreamer-1.0
```
2.  Use `gst-launch-1.0` to send video to Kinesis Video Streams

  **Example:**
```
  gst-launch-1.0 ksvideosrc do-timestamp=TRUE ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc bframes=0 key-int-max=45 bitrate=512 ! video/x-h264,profile=baseline,stream-format=avc,alignment=au,width=640,height=480,framerate=30/1 ! kvssink stream-name=“stream-name” access-key=your_accesskey_id secret-key=your_secret_access_key
```

**Note:** If you are using IoT credentials then you can pass them as parameters to the gst-launch-1.0 command
``` gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```

3. You can also run the the sample application by executing the following command which will send video to Kinesis Video Streams.
    * Change your current working directory to Release directory first.
      * ` cd C:\Users\<myuser>\Downloads\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\Release `
      * Run the demo
          * **Example**:
            * Run the sample demo application for sending **webcam video** by executing ` kinesis_video_gstreamer_sample_app.exe my-test-stream `  or
            * Run the sample application for sending **IP camera video** by executing  `kinesis_video_gstreamer_sample_app.exe my-test-rtsp-stream <rtsp_url>`
