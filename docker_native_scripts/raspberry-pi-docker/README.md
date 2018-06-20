# Using Docker images for Producer SDK (CPP and GStreamer plugin):

Please follow the four steps below to get the Docker image for Producer SDK (includes CPP and GStreamer plugin) and start streaming to Kinesis Video.

#### Pre-requisite:

This requires docker is installed in your system.

Follow instructions to download and start Docker

* [Docker download instructions](https://www.docker.com/community-edition#/download)
* [Getting started with Docker](https://docs.docker.com/get-started/)


Once you have docker installed, you can download the Kinesis Video Producer SDK (CPP and GStreamer plugin) from Amazon ECR (Elastic Container Service) using `docker pull` command.


#### Step 1:

Authenticate your Docker client to the Amazon ECR registry that you intend to pull your image from. Authentication tokens must be obtained for each registry used, and the tokens are valid for 12 hours. For more information, see Registry Authentication.

Example:

You can authenticate to Amazon ECR using the following two commands:

1.  `aws ecr get-login --no-include-email --region us-west-2 --registry-ids 546150905175`

you will see an **output** similar to the one below.

2.  `docker login -u AWS -p <Password>   https://<YourAccountId>.dkr.ecr.us-west-2.amazonaws.com`

Run the command as such in your command prompt. This will authorize the `docker pull` which you will be running next.


#### Step 2: Pull Image

##### 2.1 Steps for getting the Docker images built with Raspbian Stretch

Run the following command to get the AmazonLinux Docker image to your local environment. E.g.

`sudo docker pull 546150905175.dkr.ecr.us-west-2.amazonaws.com/kinesis-video-producer-sdk-cpp-raspberry-pi:latest`

you can verify the images in your local docker environment by running `docker images`.


#### Step 3: Run the Docker Image (start the container)

Run the following command to start the kinesis video sdk container

`sudo docker run -it --device=/dev/video0 --device=/dev/vchiq -v /opt/vc:/opt/vc 546150905175.dkr.ecr.us-west-2.amazonaws.com/kinesis-video-producer-sdk-cpp-raspberry-pi:latest /bin/bash`


#### Step 4: Run the GStreamer plugin

Follow the next few steps to start Producer SDK GStreamer plugin element for sending video streams from webcamera to Kinesis Video:

##### 4.1 Set these environment variable:

`export LD_LIBRARY_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH`

`export PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/bin:$PATH`

`export GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$GST_PLUGIN_PATH`


##### 4.2 Start the streaming with `gst-launch-1.0` command.

`gst-launch-1.0 v4l2src do-timestamp=TRUE device=/dev/video0 ! videoconvert ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! omxh264enc control-rate=2 target-bitrate=512000 inline-header=FALSE periodicty-idr=20 ! h264parse ! video/x-h264,stream-format=avc,alignment=au,width=640,height=480,framerate=30/1,profile=baseline ! kvssink stream-name="YOURSTREAMNAME" access-key=YOURACCESSKEY secret-key=YOURSECRETKEY`

For more information refer [AWS Doc](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/examples-gstreamer-plugin.html#examples-gstreamer-plugin-docker)

For additional examples refer [Readme](https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/README.md) in  https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.

