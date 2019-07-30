FROM resin/rpi-raspbian:stretch

RUN apt-get update && apt-get install -y \
	git \
	curl \
	pkg-config \
	cmake \
	vim \
	make \
	openssh-server \
	gcc \
	g++ \
	bzip2 \
	libssl-dev \
	libcurl4-openssl-dev \
	liblog4cplus-dev \
	libgstreamer1.0-dev \
	libgstreamer-plugins-base1.0-dev \
	gstreamer1.0-plugins-base-apps \
	gstreamer1.0-plugins-bad \
	gstreamer1.0-plugins-good \
	gstreamer1.0-plugins-ugly \
	gstreamer1.0-tools \
	gstreamer1.0-omx


WORKDIR /opt/
RUN curl -OL https://github.com/raspberrypi/firmware/archive/1.20180417.tar.gz
RUN tar xvf 1.20180417.tar.gz
RUN cp -r /opt/firmware-1.20180417/opt/vc ./
RUN rm -rf firmware-1.20180417 1.20180417.tar.gz

###################################################
# create symlinks for libraries used by omxh264enc
###################################################

RUN     ln -s /opt/vc/lib/libopenmaxil.so /usr/lib/libopenmaxil.so && \
        ln -s /opt/vc/lib/libbcm_host.so /usr/lib/libbcm_host.so && \
        ln -s /opt/vc/lib/libvcos.so /usr/lib/libvcos.so &&  \
        ln -s /opt/vc/lib/libvchiq_arm.so /usr/lib/libvchiq_arm.so && \
        ln -s /opt/vc/lib/libbrcmGLESv2.so /usr/lib/libbrcmGLESv2.so && \
        ln -s /opt/vc/lib/libbrcmEGL.so /usr/lib/libbrcmEGL.so && \
        ln -s /opt/vc/lib/libGLESv2.so /usr/lib/libGLESv2.so && \
        ln -s /opt/vc/lib/libEGL.so /usr/lib/libEGL.so

WORKDIR /opt/
RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp
WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
RUN chmod a+x min-install-script
RUN ./min-install-script

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/:$GST_PLUGIN_PATH
