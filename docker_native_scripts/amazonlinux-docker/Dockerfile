# # Build docker with
# # docker build -t kinesis-video-producer-sdk-cpp-amazon-linux .
# #
FROM amazonlinux:2
RUN yum update -y && yum upgrade -y
RUN	yum install -y gcc gcc-c++ git pkgconfig vim wget make libcurl-devel openssl-devel gstreamer1-devel \
	gstreamer1-plugins-base-devel gstreamer1-plugins-bad-free gstreamer1-plugins-good tar xz

WORKDIR /opt/

RUN wget https://cmake.org/files/v3.2/cmake-3.2.3-Linux-x86_64.tar.gz && \
    tar -zxvf cmake-3.2.3-Linux-x86_64.tar.gz && \
    rm cmake-3.2.3-Linux-x86_64.tar.gz
ENV PATH=$PATH:/opt/cmake-3.2.3-Linux-x86_64/bin

RUN wget https://github.com/log4cplus/log4cplus/releases/download/REL_1_2_0/log4cplus-1.2.0.tar.xz && \
	tar -xvf ./log4cplus-1.2.0.tar.xz && cd ./log4cplus-1.2.0 && \
	./configure && make && make install

RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

# # ================  Build producer sdk and gstreamer plugin ================================================
RUN chmod a+x ./min-install-script
RUN ./min-install-script

# # Set environment variables for plugin and libraries
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
