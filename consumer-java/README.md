# Java parser library based Consumer application

## Introduction
The application is a simple frame parser that is developed to run with the producer SDK to get some end to end metrics and emit to cloudwatch. The app can be used to run long term canaries and is a demo to introduce parser library based application integration with cloudwatch services. 

## Requirements

1. `Maven`
2. `OpenJDK` (Note: The application is tested with JDK version 13.0.1)
3. `make`

## Build
To download run the following command:

`git clone https://github.com/aws-samples/amazon-kinesis-video-streams-demos.git`

Move to the directory containing the `pom.xml` file:

`cd canary/consumer-java`

Next, run `make`. This will take of the build steps and generate the necessary classpath string of dependencies

## Running the application

Since this is developed keeping in mind the end to end scenario with producer SDK, the following environment variables need to be exported to generate the same stream name that is generated in the producer SDK. Take a look at [this](https://github.com/aws-samples/amazon-kinesis-video-streams-demos/blob/master/canary/producer-c/init.sh) to check out the exports. Additionally, you also need to export the following:

1. `AWS_ACCESS_KEY_ID` 
2. `AWS_SECRET_ACCESS_KEY`
3. `AWS_DEFAULT_REGION`

Once this is done, run the following command:

`java -classpath target/aws-kinesisvideo-producer-sdk-canary-consumer-1.0-SNAPSHOT.jar:$(cat tmp_jar) -Daws.accessKeyId=${AWS_ACCESS_KEY_ID} -Daws.secretKey=${AWS_SECRET_ACCESS_KEY} com.amazon.kinesis.video.canary.consumer.ProducerSdkCanaryConsumer`

This should get the application running. 


## Metrics being collected currently

When run in an end to end scenario, the following metrics are collected and can be viewed on Cloudwatch:

| Metric	                 | Frequency	    | Unit         | Description	           
|--------------------|:-------------:|:-------------:|:-------------|
| FrameDataMatches                       | Every frame receive at consumer  | None         | The frame packet received the consumer contains a checksum, which is compared with the checksum calculated at the consumer with the received packet. If equal, 1.0 is pushed as a metric, else 0.0 is pushed
| FrameSizeMatch	                     | Every frame receive at consumer| None           | The size of the frame received with the packet is compared to the size calculated on the received frame at the consumer. If equal, 1.0 is emitted, else 0.0 is emitted
| FrameDropped	                         | Every frame receive at consumer| None           | The metric indicates if any frames were dropped. The frame index is compared index of previous frame received and if the index does not indicate lastFrameIndex + 1, this metric is set to 1.0
| FrameTimeMatchesProducerTimestamp	     | Every frame receive at consumer| None           | The metric indicates if frameTimestampInsideData matches sum of pts and frame timecode. If true, this metric is set to 1.0, else it is set to 0.0

## Cloudwatch

Every metric is available in two dimensions:
1. Per stream: This will be available under `KinesisVideoSDKCanary->ProducerSDKCanaryStreamName` in cloudwatch console
2. Aggregated over all streams based on `canary-type`. `canary-type` is set by running `export CANARY_LABEL=value`. This will be available under `KinesisVideoSDKCanary->ProducerSDKCanaryType` in cloudwatch console

## References

1. To checkout how this is integrated with the producer SDK, look [here](https://github.com/aws-samples/amazon-kinesis-video-streams-demos/tree/master/canary/producer-c)
2. [Parser library](https://github.com/aws/amazon-kinesis-video-streams-parser-library)


