#include "ClientTestFixture.h"

class StreamParallelTest : public ClientTestBase {
};

StreamParallelTest* gParallelTest = NULL;

PVOID staticProducerRoutine(PVOID arg)
{
    StreamParallelTest* pTest = gParallelTest;
    return pTest->basicProducerRoutine((UINT64) arg);
}

PVOID staticConsumerRoutine(PVOID arg)
{
    StreamParallelTest* pTest = gParallelTest;
    return pTest->basicConsumerRoutine((UINT64) arg);
}

PVOID ClientTestBase::basicProducerRoutine(UINT64 streamId)
{
    UINT32 index = 0;
    BYTE tempBuffer[100];
    UINT64 timestamp = 0;
    Frame frame;
    STREAM_HANDLE streamHandle = mStreamHandles[streamId];

    // Loop until we can start
    while(!mStartThreads) {
        DLOGV("Producer waiting for stream %llu TID %016llx", streamId, GETTID());

        // Sleep a while
        usleep(TEST_CONSUMER_SLEEP_TIME_IN_MICROS);
    }

    // Loop until cancelled
    frame.duration = TEST_PRODUCER_SLEEP_TIME_IN_MICROS * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;

    while(!mTerminate) {
        // Produce frames
        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        DLOGV("Producer for stream %llu TID %016llx", streamId, GETTID());

        // Key frame every 3rd
        frame.flags = index % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // Return a put stream result on 5th
        if (index == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCustomDatas[streamId], SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        // Sleep a while
        usleep(TEST_PRODUCER_SLEEP_TIME_IN_MICROS);
        timestamp += frame.duration;
    }

    return NULL;
}

PVOID ClientTestBase::basicConsumerRoutine(UINT64 streamId)
{
    UINT32 filledSize;
    BYTE getDataBuffer[20000];
    UINT64 clientStreamHandle = 0;
    STATUS retStatus;
    STREAM_HANDLE streamHandle = mStreamHandles[streamId];

    // Loop until we can start
    while(!mStartThreads) {
        DLOGV("Consumer waiting for stream %llu TID %016llx", streamId, GETTID());

        // Sleep a while
        usleep(TEST_CONSUMER_SLEEP_TIME_IN_MICROS);
    }

    // Loop until cancelled
    while(!mTerminate) {
        DLOGV("Consumer for stream %llu TID %016llx", streamId, GETTID());
        // Consume frames
        clientStreamHandle = 0;
        retStatus = getKinesisVideoStreamData(streamHandle, &clientStreamHandle, getDataBuffer, SIZEOF(getDataBuffer),
                                              &filledSize);
        EXPECT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE ||
                    retStatus == STATUS_END_OF_STREAM);

        if (retStatus == STATUS_SUCCESS) {
            EXPECT_EQ(SIZEOF(getDataBuffer), filledSize);
            EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
        }

        // Sleep a while
        usleep(TEST_CONSUMER_SLEEP_TIME_IN_MICROS);
    }

    return NULL;
}

TEST_F(StreamParallelTest, putFrame_BasicParallelPutGet)
{
    UINT32 index, maxIteration;
    BYTE tempBuffer[10000];
    UINT64 timestamp;
    CHAR streamName[MAX_STREAM_NAME_LEN];

    mTerminate = FALSE;
    mStartThreads = FALSE;

    // Set the global pointer to this
    gParallelTest = this;

    // We want frame timestamps so the timing will be precise
    mStreamInfo.streamCaps.frameTimecodes = TRUE;

    // Create and ready a stream
    for (mStreamCount = 0; mStreamCount < MAX_TEST_STREAM_COUNT; mStreamCount++) {
        sprintf(streamName, "%s %d", TEST_STREAM_NAME, mStreamCount);
        STRCPY(mStreamInfo.name, streamName);
        EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle)));

        // Store the handle and increment the counter
        mStreamHandles[mStreamCount] = mStreamHandle;

        // Store the call context
        mCustomDatas[mStreamCount] = mCallContext.customData;

        // Move to ready state
        mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
        STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
        STRCPY(mStreamDescription.streamName, streamName);
        STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
        STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
        STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
        mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
        mStreamDescription.creationTime = GETTIME();
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCustomDatas[mStreamCount], SERVICE_CALL_RESULT_OK, &mStreamDescription));

        // Ensure the get endpoint is called
        EXPECT_EQ(0, STRCMP(streamName, mStreamName));
        EXPECT_EQ(STREAM_ACCESS_MODE_READ, mAccessMode);

        // Move the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCustomDatas[mStreamCount], SERVICE_CALL_RESULT_OK,
                                                                  TEST_STREAMING_ENDPOINT));

        // Ensure the get token is called
        EXPECT_EQ(0, STRCMP(streamName, mStreamName));
        EXPECT_EQ(STREAM_ACCESS_MODE_READ, mAccessMode);

        // Move to the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCustomDatas[mStreamCount],
                                                               SERVICE_CALL_RESULT_OK,
                                                               (PBYTE) TEST_STREAMING_TOKEN,
                                                               SIZEOF(TEST_STREAMING_TOKEN),
                                                               TEST_AUTH_EXPIRATION));

        DLOGV("Creating the producer and consumer threads");

        // Spin off the producer
        EXPECT_EQ(0, pthread_create(&mProducerThreads[mStreamCount], NULL, staticProducerRoutine, (PVOID) (UINT64) mStreamCount));

        // Spin off the consumer
        EXPECT_EQ(0, pthread_create(&mConsumerThreads[mStreamCount], NULL, staticConsumerRoutine, (PVOID) (UINT64) mStreamCount));

        usleep(10);
    }

    // Enable the threads
    DLOGV("Enabling the execution of the threads");
    mStartThreads = TRUE;

    // Wait for some time
    sleep(TEST_SLEEP_TIME_IN_SECONDS);

    // Indicate the cancel for the threads
    mTerminate = TRUE;

    // Join the threads and wait to exit
    for (index = 0; index < MAX_TEST_STREAM_COUNT; index++) {
        EXPECT_EQ(0, pthread_join(mProducerThreads[index], NULL));
        EXPECT_EQ(0, pthread_join(mConsumerThreads[index], NULL));
    }
}
