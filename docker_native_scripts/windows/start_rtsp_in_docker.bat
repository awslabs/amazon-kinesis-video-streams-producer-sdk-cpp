REM Demo GStreamer Sample Application for RTSP Streaming to Kinesis Video Streams
REM To be run inside the Docker container

cd /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/Release

REM Start the demo rtsp application to send video streams
set PATH=%PATH%;C:\opt\amazon-kinesis-video-streams-producer-sdk-cpp\kinesis-video-native-build\downloads\gstreamer\1.0\x86_64\bin
set AWS_ACCESS_KEY_ID=%1
set AWS_SECRET_ACCESS_KEY=%2

REM %3 is RTSP_URL and %4 for STREAM_NAME
kinesis_video_gstreamer_sample_app.exe %3 %4
