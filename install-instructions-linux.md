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

The **install-script** will download and build the dependent open source components (from the source) into the **downloads** directory within `kinesis-video-native-build` directory (e.g. `/home/<myuser>/downloads/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads`) and link against it.

**After you've downloaded the code from GitHub, you can build it on Linux by running `./install-script` (which is inside the `kinesis-video-native-build` directory)**.

**Important:**  Change the _**current working directory**_ to `kinesis-video-native-build` directory first. Then run the `./install-script` from that directory.

 The `install-script` will take some time (about 25 minutes) to bring down and compile the open source components depending on the network connection and processor speed. At the end of the installation, this step will produce the core library, unit tests executable and the sample GStreamer application. If anything fails or the script is interrupted, re-running it will pick up from the place where it last left off. The sub-sequent run will be building just the modified SDK components or applications which is much faster.

Note that the install-script also builds the Kinesis Video Streams producer SDK as a **GStreamer plugin** (**kvssink**).

----
##### Alternate option to build the SDK and sample applications using system versions of open source library dependencies

The bulk of the **install script** is building the open source dependencies. The project is based on **CMake**. So the open source components building can be skipped if the system versions of the open source dependencies are already installed that can be used for linking. See the section **Install Steps for Ubuntu 17.x using apt-get** for detailed instructions on how to install using `apt-get install` on **Ubuntu** .

Running

```
$ cmake .
$ make
```
from the `kinesis-video-native-build` directory will build and link the SDK.

The `./min-install-script` inside the `kinesis-video-native-build` captures these steps for installing the Kinesis Video Streams Producer SDK with the system versions for linking.

##### Optionally build the native library (KinesisVideoProducerJNI) to run Java demo streaming application

The `./java-install-script` inside `kinesis-video-native-build` will build the KinesisVideoProducerJNI native library to be used by [Java Producer SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-java/blob/master/README.md).

----

### Install Steps for Ubuntu 17.x using apt-get

The following section provides guidance for installing the build tools and open source dependencies using `apt-get` in **Ubuntu Linux**.

The following are the steps to install the build-time prerequisites for Ubuntu 17.x

Install **git**:

```
$ sudo apt-get install git
$ git --version
git version 2.14.1
```
Install **cmake**:
```
$ sudo apt-get install cmake
$ cmake --version
cmake version 3.9.1
```

CMake suite maintained and supported by Kitware (kitware.com/cmake).

Install **libtool**:  (some images come preinstalled) amd **libtool-bin**
```
$ sudo apt-get install libtool
$ sudo apt-get install libtool-bin
$ libtool --version
libtool (GNU libtool) 2.4.6
Written by Gordon Matzigkeit, 1996

Copyright (C) 2014 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
Install **automake**:
```
$ sudo apt-get install automake
$ automake --version
automake (GNU automake) 1.15
Copyright (C) 2014 Free Software Foundation, Inc.
License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Tom Tromey <tromey@redhat.com>
       and Alexandre Duret-Lutz <adl@gnu.org>.
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
Install **curl**:
```
$ sudo apt-get install curl
$ curl --version
curl 7.55.1 (x86_64-pc-linux-gnu) libcurl/7.55.1 OpenSSL/1.0.2g zlib/1.2.11 libidn2/2.0.2 libpsl/0.18.0 (+libidn2/2.0.2) librtmp/2.3
Release-Date: 2017-08-14
Protocols: dict file ftp ftps gopher http https imap imaps ldap ldaps pop3 pop3s rtmp rtsp smb smbs smtp smtps telnet tftp
Features: AsynchDNS IDN IPv6 Largefile GSS-API Kerberos SPNEGO NTLM NTLM_WB SSL libz TLS-SRP UnixSockets HTTPS-proxy PSL
```
Install **pkg-config**:
```
$ sudo apt-get install pkg-config
$ pkg-config --version
0.29.1
```
Install **flex**:
```
$ sudo apt-get install flex
$ flex --version
flex 2.6.1
```
Install **bison**:
```
$ sudo apt-get install bison
$ bison -V
bison (GNU Bison) 3.0.4
Written by Robert Corbett and Richard Stallman.

Copyright (C) 2015 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
Install **Open JDK**:
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
./install-script
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
* The `gst-launch-1.0` and `gst-inspect-1.0` binaries are built inside the folder `<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin`. You can either run the following commands from that folder using `./gst-launch-1.0` or you can include that in your **PATH** environment variable using the following export command and run `gst-launch-1.0`
```
$ export PATH=<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/bin:$PATH
```
* Set the path for the producer **SDK GStreamer plugin** so that GStreamer can locate it.
```
$ export GST_PLUGIN_PATH=<YourSdkFolderPath>/kinesis-video-native-build/downloads/local/lib:$GST_PLUGIN_PATH
```

###### Running the `gst-launch-1.0` command to start streaming from RTSP camera source.
```
$ gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=512
```

**Note:** If you are using IoT credentials then you can pass them as parameters to the gst-launch-1.0 command
``` gst-launch-1.0 rtspsrc location=rtsp://YourCameraRtspUrl short-header=TRUE ! rtph264depay ! video/x-h264, format=avc,alignment=au !
 kvssink stream-name="iot-stream" iot-certificate="iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases"
```

###### Running the `gst-launch-1.0` command to start streaming from USB camera source in **Ubuntu**.
```
$ gst-launch-1.0 v4l2src do-timestamp=TRUE device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! x264enc  bframes=0 key-int-max=45 bitrate=500 ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=512
```
###### Running the `gst-launch-1.0` command to start streaming from USB camera source which has h264 encoded stream already in **Ubuntu**.
```

$ gst-launch-1.0 v4l2src do-timestamp=TRUE device=/dev/video0 ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! kvssink stream-name=YourStreamName storage-size=512

```
###### Running the `gst-launch-1.0` command to start streaming from camera source in **Raspberry-PI**.

```
$ gst-launch-1.0 v4l2src do-timestamp=TRUE device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! omxh264enc control-rate=1 target-bitrate=5120000 periodicity-idr=45 inline-header=FALSE ! h264parse ! video/x-h264,stream-format=avc,alignment=au,width=640,height=480,framerate=30/1 ! kvssink stream-name=YourStreamName frame-timestamp=dts-only access-key=YourAccessKey secret-key=YourSecretKey
```

**Note:**  Raspberry PI camera module requires `frame-timestamp=dts-only` . If USB camera is used for streaming then this property is optional.

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
