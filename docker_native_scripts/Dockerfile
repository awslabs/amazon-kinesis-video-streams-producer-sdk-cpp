FROM ubuntu:18.04

RUN apt-get upgrade
RUN apt-get update

RUN apt-get install -y git vim curl g++ gcc python2.7 pkg-config cmake maven openjdk-8-jdk libssl-dev \
    libcurl4-openssl-dev liblog4cplus-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base-apps gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly \
    gstreamer1.0-tools
RUN rm -rf /var/lib/apt/lists/*
ENV JAVA_HOME=/usr/lib/jvm/java-1.8.0-openjdk-amd64/
WORKDIR /opt/

# ===== Git Checkout latest Kinesis Video Streams Producer SDK (CPP) =======================================

RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git

# ================ Build the Producer SDK (CPP) ============================================================

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
RUN  chmod a+x ./min-install-script
RUN ./min-install-script
RUN ./java-install-script

COPY start_rtsp_in_docker.sh /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build
ENV PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build:$PATH
RUN chmod a+x ./start_rtsp_in_docker.sh

# comment the following step if you would like to customize the docker image build
ENTRYPOINT ["/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/start_rtsp_in_docker.sh"]

# ====  How to build docker image and run the container once its built =====================================
#
# ==== 1. build the docker image using the Dockerfile ======================================================
# ==== Make sure the Dockerfile and start_rtsp_in_docker.sh is present in the current working directory =====
#
# $ docker build -t rtspdockertest .
#
# === List the docker images built ==================================================================================
#
# $ docker images
#  REPOSITORY          TAG                 IMAGE ID            CREATED                  SIZE
# rtspdockertest      latest              54f0d65f69b2        Less than a second ago   2.82GB
#
# === Start the container with credentials ===
# $ docker run -it 54f0d65f69b2 <AWS_ACCESS_KEY_ID> <AWS_SECRET_ACCESS_KEY> <RTSP_URL> <STREAM_NAME>
#
#
#
