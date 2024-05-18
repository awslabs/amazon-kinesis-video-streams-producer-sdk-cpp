<h1 align="center">
  Amazon Kinesis Video Streams CPP Producer, GStreamer Plugin and JNI
  <br>
</h1>

<h4 align="center"> Amazon Kinesis Video Streams | Secure Video Ingestion for Analysis &amp; Storage </h4>

<p align="center">
  <a href="https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/actions/workflows/ci.yml"> <img src="https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/actions/workflows/ci.yml/badge.svg"> </a>
  <a href="https://codecov.io/gh/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp"> <img src="https://codecov.io/gh/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/branch/master/graph/badge.svg" alt="Coverage Status"> </a>
</p>

<p align="center">
  <a href="#key-features">Key Features</a> •
  <a href="#build">Build</a> •
  <a href="#run">Run</a> •
  <a href="#documentation">Documentation</a> •
  <a href="#related">Related</a> •
  <a href="#license">License</a>
</p>

## Key Features
* C++ SDK
* GStreamer Plugin (kvssink)
* JNI

Amazon Kinesis Video Streams Producer SDK for C/C++ makes it easy to build an on-device application that securely connects to a video stream, and reliably publishes video and other media data to Kinesis Video Streams. It takes care of all the underlying tasks required to package the frames and fragments generated by the device's media pipeline. The SDK also handles stream creation, token rotation for secure and uninterrupted streaming, processing acknowledgements returned by Kinesis Video Streams, and other tasks.

## Build
### Download
To download run the following command:

`git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git`

Note: You will also need to install `pkg-config`, `CMake`, `m4` and a build enviroment. If you are building the GStreamer plugin you will also need GStreamer and GStreamer (Development Libraries).

Refer to the [FAQ](#FAQ) for platform specific instructions.

### Configure

Prepare a build directory in the newly checked out repository:

```
mkdir -p amazon-kinesis-video-streams-producer-sdk-cpp/build
cd amazon-kinesis-video-streams-producer-sdk-cpp/build
```

If you are building on Windows you need to generate `NMake Makefiles`, you should run `cmake .. -G "NMake Makefiles"`

GStreamer and JNI is NOT built by default, if you wish to build both you MUST execute `cmake .. -DBUILD_GSTREAMER_PLUGIN=ON -DBUILD_JNI=TRUE`

By default we download all the libraries from GitHub and build them locally, so should require nothing to be installed ahead of time.  

If you do wish to link to existing libraries you can do `cmake .. -DBUILD_DEPENDENCIES=OFF`
Libraries needed to build producer are: Curl, Openssl and Log4cplus. If you want to build the gstreamer plugin you will need to have gstreamer in your system.
On Mac OS you can get the libraries using homebrew
```
$ brew install pkg-config openssl cmake gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly log4cplus gst-libav
```
On Ubuntu and Raspberry Pi OS you can get the libraries by running
```
$ sudo apt-get install libssl-dev libcurl4-openssl-dev liblog4cplus-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base-apps gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-tools
```
### Setup desired log level:
Set up the desired log level. The log levels currently available with `log4cplus` are:
1. `TRACE`
2. `DEBUG` 
3. `INFO`   
4. `WARN`   
5. `ERROR`   
6. `FATAL`   

To set a log level, update the log level value [here](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/kvs_log_configuration#L1)

Note: The default log level is `DEBUG`

#### Cross-Compilation
If you wish to cross-compile `CC` and `CXX` are respected when building the library and all its dependencies. See our [ci.yml](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/develop/.github/workflows/ci.yml) for an example of this. Every commit is cross compiled to ensure that it continues to work.
Please note that GStreamer is not cross-compiled as a part of the cross-compilation of the KVS-SDK, customers will have to cross-compile it separately.


#### CMake Arguments
You can pass the following options to `cmake ..`.

* `-DBUILD_GSTREAMER_PLUGIN` -- Build kvssink GStreamer plugin. Default is OFF.
* `-DBUILD_STATIC` -- Build as static libraries. Default is OFF.
* `-DBUILD_GSTREAMER_PLUGIN_STATIC` -- If building GStreamer plugin, build it as a static library. Default is BUILD_STATIC.
* `-DBUILD_JNI` -- Build C++ wrapper for JNI to expose the functionality to Java/Android
* `-DBUILD_DEPENDENCIES` -- Build depending libraries from source
* `-DBUILD_TEST=TRUE` -- Build unit/integration tests, may be useful for confirm support for your device. `./tst/producerTest`
* `-DCODE_COVERAGE` --  Enable coverage reporting
* `-DCOMPILER_WARNINGS` -- Enable all compiler warnings
* `-DADDRESS_SANITIZER` -- Build with AddressSanitizer
* `-DMEMORY_SANITIZER` --  Build with MemorySanitizer
* `-DTHREAD_SANITIZER` -- Build with ThreadSanitizer
* `-DUNDEFINED_BEHAVIOR_SANITIZER` Build with UndefinedBehaviorSanitizer
* `-DALIGNED_MEMORY_MODEL` Build for aligned memory model only devices. Default is OFF.
* `-DBUILD_LOG4CPLUS_HOST` Specify host-name for log4cplus for cross-compilation. Default is OFF.

#### To Include JNI

JNI examples are NOT built by default.  If you wish to build JNI you MUST add `-DBUILD_JNI=TRUE` when running `cmake`:

```
cmake -DBUILD_JNI=TRUE
```

#### To Include Building GStreamer Sample Programs

The GStreamer plugin and samples are NOT built by default. If you wish to build them you MUST add `-DBUILD_GSTREAMER_PLUGIN=TRUE` when running cmake:

```
cmake -DBUILD_GSTREAMER_PLUGIN=TRUE ..
```
### Compiling 

After running cmake, in the same build directory run `make`:

```
make
```

On Windows you should run `nmake` instead of `make`

In your build directory you will now have shared objects for all the targets you have selected.

### Installing the library
If the library needs to be installed, run `make install`. This will install in default directory based on system. To install in another directory, run `cmake` with the `-DCMAKE_INSTALL_PREFIX` option with the desired directory before running `make install`
## Run
### GStreamer Plugin (kvssink)

#### Loading Element
The GStreamer plugin is located in your `build` directory.

To load this plugin set the following environment variables. This should be run from the root of the repo, NOT the `build` directory.

```
export GST_PLUGIN_PATH=`pwd`/build
export LD_LIBRARY_PATH=`pwd`/open-source/local/lib
```

The equivalent for Windows is

```
set GST_PLUGIN_PATH=%CD%\build
set PATH=%PATH%;%CD%\open-source\local\bin;%CD%\open-source\local\lib
```

Now if you execute `gst-inspect-1.0 kvssink` you should get information on the plugin like

```text
Factory Details:
  Rank                     primary + 10 (266)
  Long-name                KVS Sink
  Klass                    Sink/Video/Network
  Description              GStreamer AWS KVS plugin
  Author                   AWS KVS <kinesis-video-support@amazon.com>

Plugin Details:
  Name                     kvssink
  Description              GStreamer AWS KVS plugin
  Filename                 /Users/seaduboi/workspaces/amazon-kinesis-video-streams-producer-sdk-cpp/build/libgstkvssink.so
  Version                  1.0
  License                  Proprietary
  Source module            kvssinkpackage
  Binary package           GStreamer
  Origin URL               http://gstreamer.net
```

If the build failed, or `GST_PLUGIN_PATH` is not properly set you will get output like

```text
No such element or plugin 'kvssink'
```


#### Using Element
The kvssink element has the following required parameters:

* `stream-name` -- The name of the destination Kinesis video stream.
* `storage-size` -- The storage size of the device in megabytes. For information about configuring device storage, see StorageInfo.
* `access-key` -- The AWS access key that is used to access Kinesis Video Streams. You must provide either this parameter or credential-path.
* `secret-key` -- The AWS secret key that is used to access Kinesis Video Streams. You must provide either this parameter or credential-path.
* `credential-path` -- A path to a file containing your credentials for accessing Kinesis Video Streams. For example credential files, see Sample Static Credential and Sample Rotating Credential. For more information on rotating credentials, see Managing Access Keys for IAM Users. You must provide either this parameter or access-key and secret-key.


For examples of common use cases you can look at [Example: Kinesis Video Streams Producer SDK GStreamer Plugin](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/examples-gstreamer-plugin.html)

#### To Include Images/Events feature

The images feature is available in the sample kvs_gstreamer_audio_video_sample.cpp . To enable it include the argument "-e <event_option>"
event option is a string that can be:

notification -- for a notification event
image -- for an image event
both -- for both

The events will start on the 2nd key frame, and will reoccur every 200 key frames. If you would to change this frequence you can edit the sample.

#### To run from a file
in the kvs_gstreamer_audio_video_sample.cpp if you would like to upload from a file, include the option flag -f <file_path>

## Running in offline mode
By default, the samples run in near realtime mode. To set offline mode, set streamInfo.streamCaps.streamingType to `STREAMING_TYPE_OFFLINE`, where, `streamInfo` is of type `StreamInfo`, `streamCaps` is of type `StreamCaps` and `streamingType` is of type `STREAMING_TYPE`.

## Dockerscripts
* The sample docker scripts for RTSP plugin, raspberry pi and linux can be found in the [Kinesis demos repository](https://github.com/aws-samples/amazon-kinesis-video-streams-demos/tree/master/producer-cpp).

## DEBUG
* When building the JNI, if you run into a cmake error `Could NOT find JNI (missing: JAVA_INCLUDE_PATH JAVA_INCLUDE_PATH2 JAVA_AWT_INCLUDE_PATH)`, make sure your environment variables are set correctly:  
`export JAVA_INCLUDE_PATH2=/Library/Java/JavaVirtualMachines/<YOUR_JDK_VERSION>/Contents/Home/include` or `export JAVA_INCLUDE_PATH2=$JAVA_HOME/include` for Mac OS.  
`export JAVA_INCLUDE_PATH2='/usr/java/<JDK_VERSION>/include'` for Linux.
* If you are successfully streaming but run into issue with playback. You can do `export KVS_DEBUG_DUMP_DATA_FILE_DIR=/path/to/directory` before streaming. Producer will then dump MKV files into that path. The file is exactly what KVS will receive. You can use [MKVToolNIX](https://mkvtoolnix.download/index.html) to check that everything looks correct. You can also try to play the MKV file in compatible players.
* If you would like to visualize the GStreamer pipeline being constructed in a GStreamer application, include the following after the elements have been linked:
`GST_DEBUG_BIN_TO_DOT_FILE(<gst-bin-object>, GST_DEBUG_GRAPH_SHOW_ALL, <file-name>);`
For example, if the application created a pipeline object `GstPipeline* pipeline = gst_pipeline_new("test-pipeline")`, and you would like to see the visualized pipeline with filename pipeline, add:
`GST_DEBUG_BIN_TO_DOT_FILE((GstBin*) pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");`. Also ensure to set the path to where you would like the file to be stored. `export GST_DEBUG_DUMP_DOT_DIR=.`. The file generated would be a `.dot` format. Convert to PDF to check the visualized pipeline. Also, this requires `graphviz` to be installed. So make sure to install that.


## FAQ
* Is CPP-SDK and GStreamer supported on Mac/Windows/Linux (Supported Platforms)?  
Yes! We have FAQs and platform specific instructions for [Windows](docs/windows.md), [MacOS](docs/macos.md) and [Linux](docs/linux.md)

## Development

The repository is using develop branch as the aggregation and all of the feature development is done in appropriate feature branches. The PRs (Pull Requests) are cut on a feature branch and once approved with all the checks passed they can be merged by a click of a button on the PR tool. The master branch should always be build-able and all the tests should be passing. We are welcoming any contribution to the code base. The master branch contains our most recent release cycle from develop.

### Release
The repository is under active development and even with incremental unit test coverage where some of the tests are actually full integration tests, we require more rigorous internal testing in order to 'cut' release versions. The release is cut against a particular commit that gets approved. The general philosophy is to cut a release when a set of commits contribute to a self-containing feature or when we add major internal functionality improvements.

### Versioning
We deploy 3 digit version strings in a form of 'Major.Minor.Revision' scheme.
* Major version update - Major functionality changes. Might not have direct backward compatibility. For example, multiple public API parameter changes.
* Minor version update - Additional features. Major bug fixes. Might have some minor backward compatibility issues. For example, an extra parameter on a callback function.
* Revision version update - Minor features. Bug fixes. Full backward compatibility. For example, an extra fields added to the public structures with version bump.

## Related
* [What Is Amazon Kinesis Video Streams](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/what-is-kinesis-video.html)
* [C SDK](https://github.com/awslabs/amazon-kinesis-video-streams-producer-c)
* [Example: Kinesis Video Streams Producer SDK GStreamer Plugin](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/examples-gstreamer-plugin.html)

## License

This library is licensed under the Apache 2.0 License.
