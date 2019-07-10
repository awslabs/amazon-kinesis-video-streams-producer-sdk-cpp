#### How to build and run Docker container for  Kinesis Video Streams RTSP demo application in Windows

----
#### 1. Download and install Docker in Windows
Follow instructions to download and start Docker for Windows
* [Getting started with Docker in Windows](https://docs.docker.com/docker-for-windows/)
* [Docker download instructions](https://www.docker.com/community-edition#/download)
* [Pre-requisites for installing Docker in Windows](https://docs.docker.com/docker-for-windows/install/)

----
#### 2. Build Kinesis Video Producer SDK as a Docker image
Download the `Dockerfile` and `start_rtsp_in_docker.bat` into a folder.  Once the Docker is installed and running
in the [Windows container mode](https://docs.docker.com/docker-for-windows/install/#switch-between-windows-and-linux-containers), then you can build the Docker image by using the following command.
```
  >  docker build -t windowsdockerrtsptest .
```
Once the build is complete, you can get the image id by running the command `docker images` which will display all the Docker images built in your system.
```
  > docker images
```
Use the **IMAGE_ID** from the output (e.g `02181afc49f9`):
```
    REPOSITORY          TAG                 IMAGE ID            CREATED                  SIZE
  windowsdockerrtsptest      latest      02181afc49f9        Less than a second ago   14.1GB
```

----
#### 3. Start the Docker container to run RTSP video streaming
Run the Docker container to send video to Kinesis Video Streams using the following command:
```
  > docker run -it <IMAGE_ID> <AWS_ACCESS_KEY_ID> <AWS_SECRET_ACCESS_KEY> <RTSP_URL> <YOUR_STREAM_NAME>
```
