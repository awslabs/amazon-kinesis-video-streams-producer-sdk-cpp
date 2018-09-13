
# Dockerfile to build Kinesis Video Streams Producer CPP as a Windows container

FROM microsoft/windowsservercore
WORKDIR /opt/

# ================= Install Git ============================================================================
RUN @powershell iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1')) 
RUN cinst -y git

# =================  HTTPS Certificate =====================================================================

#RUN wget https://www.amazontrust.com/repository/SFSRootCAG2.pem 
#RUN cp  SFSRootCAG2.pem /etc/ssl/cert.pem

# ===== Git Checkout latest Kinesis Video Streams Producer SDK (CPP) =======================================

RUN git clone https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git

# ===== Build the Producer SDK (CPP) using Microsoft Visual C++ =============================================
WORKDIR /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

RUN "vs-buildtools-install.bat && windows-install.bat 64 && vs_buildtools.exe uninstall --wait --quiet --installPath C:\BuildTools" 
COPY start_rtsp_in_docker.bat /opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/

# You can comment the following step if you would like to start the container first and make changes before
# starting the demo application
ENTRYPOINT ["/opt/amazon-kinesis-video-streams-producer-sdk-cpp/kinesis-video-native-build/start_rtsp_in_docker.bat"]

# ===== Steps to build docker image and run the container ===================================================
# 
# ===== Step 1. Run docker bbuild command to create the docker image  ========================================
# =====Make sure the Dockerfile and start_rtsp_in_docker.bat are present in the current working directory ===
#
# > docker build -t windowsdockerrtsptest .
# 
# ===== Step 2. List the docker images built to get the image id ==============================================
#
# > docker images
#   REPOSITORY                  TAG                 IMAGE ID            CREATED              SIZE
# windowsdockerrtsptest         latest              02181afc49f9        4 seconds ago       15GB
#
# ===== Step 3. Start the container with credentials ==========================================================
# > docker run -it 02181afc49f9 <AWS_ACCESS_KEY_ID> <AWS_SECRET_ACCESS_KEY> <RTSP_URL> <STREAM_NAME>
# 


