FROM resin/rpi-raspbian:stretch

RUN apt-get update && apt-get install -y \
	git \
	curl \
	ca-certificates-java \
	openjdk-8-jre \
	openjdk-8-jdk \
	pkg-config \
	cmake \
	vim \
	binutils \
	make \
	openssh-server \
	gcc \
	g++ \
	python \
	bzip2

ENV JAVA_HOME=/usr/lib/jvm/java-1.8.0-openjdk-armhf/

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
RUN chmod a+x install-script
RUN ./install-script -a

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
RUN chmod a+x ./gstreamer-plugin-install-script
RUN ./gstreamer-plugin-install-script

WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/
ENV PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/bin:$PATH
ENV GST_PLUGIN_PATH=/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/downloads/local/lib:$GST_PLUGIN_PATH
