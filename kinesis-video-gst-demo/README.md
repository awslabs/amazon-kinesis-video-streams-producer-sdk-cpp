# Kinesis GStreamer Sample Application which is part of the samples published in GitHub!

This is an sample GStreamer Application with Kinesis Video integration.

## Set up
* Install cmake

```
brew install cmake
```

OR from source, if cmake in brew is not 3.7 (Not sure why we need 3.7): https://cmake.org/install/

* Install GStreamer

```
brew install gstreamer gst-plugins-base gst-plugins-bad gst-plugins-good gst-plugins-ugly
```

* Download dependency repositories

Download the following repositories from GitHub to the same level as kinesis-video-gstreamer-sample-app:

```
git clone https://git-codecommit.us-west-2.amazonaws.com/v1/repos/kinesis-video-producer
git clone https://git-codecommit.us-west-2.amazonaws.com/v1/repos/kinesis-video-pic
git clone https://git-codecommit.us-west-2.amazonaws.com/v1/repos/kinesis-video-native-build
```

The directory structure should look like:

```
$ tree -L 1
.
├── kinesis-video-gstreamer-sample-app
├── kinesis-video-native-build
├── kinesis-video-pic
└── kinesis-video-producer
```

## Build the application

```
cd kinesis-video-native-build
./install-script
```

## Run the application

```
AWS_ACCESS_KEY_ID=AKIASAMPLEKEYID AWS_SECRET_ACCESS_KEY=MYSECRETACCESSKEY ./kinesis_video_gstreamer_sample_app my-stream-name

optionally, AWS_SESSION_TOKEN and AWS_DEFAULT_REGION environment variables can be specified.

```
