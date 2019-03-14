#!/bin/bash
# Demo GStreamer Sample Application for RTSP Streaming to Kinesis Video Streams
# To be run inside the Docker container

cd /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

# Start the demo rtsp application to send video streams
export LD_LIBRARY_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH

# $3 for STREAM_NAME and $4 is RTSP_URL
AWS_ACCESS_KEY_ID=$1 AWS_SECRET_ACCESS_KEY=$2 ./kinesis_video_gstreamer_sample_app $4 $3
