# Kinesis GStreamer Sample Application which is part of the samples published in GitHub!

This is an sample GStreamer Application with Kinesis Video integration.

## Set up
* Install cmake

```
$ brew install cmake
```

* Install GStreamer

```
brew install gstreamer gst-plugins-base gst-plugins-bad gst-plugins-good gst-plugins-ugly
```

## Build the application

```
$ cd kinesis-video-native-build
$ ./install-script
```

## Run the application

```
$ AWS_ACCESS_KEY_ID=AKIASAMPLEKEYID AWS_SECRET_KEY_ID=MYSECRETKEYKID ./gst_demo my-stream-name
```
