# Build docker with
# docker build -t kinesis-video-producer-sdk-cpp-amazon-linux .
#
FROM amazonlinux:2
RUN yum update -y && \
	yum upgrade -y && \
	yum install -y autoconf && \
	yum install -y automake && \
	yum install -y bison && \
	yum install -y bzip2 && \
	yum install -y cmake && \
	yum install -y curl && \
	yum install -y diffutils && \
	yum install -y flex  && \
	yum install -y gcc  && \
	yum install -y gcc-c++ && \
	yum install -y git && \
	yum install -y gmp-devel && \
	yum install -y libffi && \
	yum install -y libffi-devel && \
	yum install -y libmpc-devel && \
	yum install -y libtool && \
	yum install -y m4 && \
	yum install -y mpfr-devel && \
	yum install -y pkgconfig && \
	yum install -y vim && \
	yum install -y wget && \
	yum install -y make && \
	yum install -y xz


RUN wget https://cmake.org/files/v3.2/cmake-3.2.3-Linux-x86_64.tar.gz && \
    tar -zxvf cmake-3.2.3-Linux-x86_64.tar.gz && \
    rm cmake-3.2.3-Linux-x86_64.tar.gz
ENV PATH=/cmake-3.2.3-Linux-x86_64/bin/:$PATH
WORKDIR /opt/

RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

RUN chmod a+x ./install-script
# ================  Build producer sdk and gstreamer plugin ================================================
#
RUN chmod +x install-script
RUN ./install-script -j 4 -d

# Set environment variables for plugin and libraries
ENV LD_LIBRARY_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib
ENV PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/bin:/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build:$PATH

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
