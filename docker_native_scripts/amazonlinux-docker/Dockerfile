# Build docker with
# docker build -t kinesis-video-producer-sdk-cpp-amazon-linux .
#
FROM amazonlinux
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
   yum install -y gcc-c++  && \
   yum install -y git && \
   yum install -y gmp-devel && \
   yum install -y libffi && \
   yum install -y libffi-devel && \
   yum install -y  libmpc-devel && \
   yum install -y libtool && \
   yum install -y m4 && \
   yum install -y mpfr-devel && \
   yum install -y pkgconfig && \
   yum install -y vim && \
   yum install -y wget


RUN wget https://cmake.org/files/v3.2/cmake-3.2.3-Linux-x86_64.tar.gz && \
    tar -zxvf cmake-3.2.3-Linux-x86_64.tar.gz && \
    rm cmake-3.2.3-Linux-x86_64.tar.gz
ENV PATH=/cmake-3.2.3-Linux-x86_64/bin/:$PATH
RUN wget https://ftp.gnu.org/gnu/gcc/gcc-7.2.0/gcc-7.2.0.tar.gz && \
    tar -xvf gcc-7.2.0.tar.gz && \
    cd gcc-7.2.0 && \
    ./configure --disable-multilib --enable-languages=c,c++ && \
    make && \
    make install && \
    cd .. && \
    rm -rf gcc-7.2.0*
RUN /usr/local/bin/gcc "--version"
WORKDIR /opt/

RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

ENV CC=/usr/local/bin/gcc
ENV CXX=/usr/local/bin/g++
RUN chmod a+x ./install-script
RUN chmod a+x ./gstreamer-plugin-install-script
# =================  HTTPS Certificate =====================================================================
RUN wget https://www.amazontrust.com/repository/SFSRootCAG2.pem
RUN cp  SFSRootCAG2.pem /opt/amazon-kinesis-video-streams-producer-sdk-cpp
# ================  Build producer sdk and gstreamer plugin ================================================
#
RUN chmod +x install-script
RUN ./install-script -a -j 4 -d
RUN cp libproducer.so /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib/
ENV KINESIS_VIDEO_STREAMS_PRODUCER_SDK_SOURCE_DIR=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/
ENV SDK_SOURCE_DIR=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/
RUN pwd
RUN ./gstreamer-plugin-install-script

#
# Copy dynamic libraries into lib folder
RUN cp /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-gstreamer-plugin/libgstkvssink.so /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib/

# Copy all demo sample application binaries kinesis-video-native-build
RUN cp /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-gstreamer-plugin/kvs_producer_plugin* /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
# Copy gst-launch-1.0 and gst-inspect-1.0 kinesis-video-native-build
RUN cp /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/bin/gst* /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

# Set environment variables for plugin and libraries
ENV LD_LIBRARY_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$LD_LIBRARY_PATH
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib
ENV PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/bin:/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build:$PATH

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/