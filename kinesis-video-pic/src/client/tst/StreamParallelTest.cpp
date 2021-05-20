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
    while (!ATOMIC_LOAD_BOOL(&mStartThreads)) {
        DLOGV("Producer waiting for stream %llu TID %016llx", streamId, GETTID());

        // Sleep a while
        THREAD_SLEEP(TEST_CONSUMER_SLEEP_TIME);
    }

    // Loop until cancelled
    frame.duration = TEST_PRODUCER_SLEEP_TIME;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;

    while (!ATOMIC_LOAD_BOOL(&mTerminate)) {
        // Produce frames
        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        DLOGV("Producer for stream %llu TID %016llx", streamId, GETTID());

        // Key frame every 3rd
        frame.flags = ((index % 3) == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // Return a put stream result on 5th
        if (index == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCustomDatas[streamId], SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }

        // Sleep a while
        THREAD_SLEEP(TEST_PRODUCER_SLEEP_TIME);
        timestamp += frame.duration;
    }

    return NULL;
}

PVOID ClientTestBase::basicConsumerRoutine(UINT64 streamId)
{
    UINT32 filledSize;
    BYTE getDataBuffer[20000];
    STATUS retStatus;
    STREAM_HANDLE streamHandle = mStreamHandles[streamId];

    // Loop until we can start
    while (!ATOMIC_LOAD_BOOL(&mStartThreads)) {
        DLOGV("Consumer waiting for stream %llu TID %016llx", streamId, GETTID());

        // Sleep a while
        THREAD_SLEEP(TEST_CONSUMER_SLEEP_TIME);
    }

    // Loop until cancelled
    while (!ATOMIC_LOAD_BOOL(&mTerminate)) {
        DLOGV("Consumer for stream %llu TID %016llx", streamId, GETTID());
        // Consume frames
        retStatus = getKinesisVideoStreamData(streamHandle, TEST_UPLOAD_HANDLE, getDataBuffer, SIZEOF(getDataBuffer),
                                              &filledSize);
        EXPECT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE ||
                    retStatus == STATUS_END_OF_STREAM || retStatus == STATUS_UPLOAD_HANDLE_ABORTED);

        if (retStatus == STATUS_SUCCESS) {
            EXPECT_EQ(SIZEOF(getDataBuffer), filledSize);
        }

        // Sleep a while
        THREAD_SLEEP(TEST_CONSUMER_SLEEP_TIME);
    }

    return NULL;
}

TEST_F(StreamParallelTest, putFrame_BasicParallelPutGet)
{
    UINT32 index;
    CHAR streamName[MAX_STREAM_NAME_LEN];

    ATOMIC_STORE_BOOL(&mTerminate, FALSE);
    ATOMIC_STORE_BOOL(&mStartThreads, FALSE);

    // Set the global pointer to this
    gParallelTest = this;

    // We want frame timestamps so the timing will be precise
    mStreamInfo.streamCaps.frameTimecodes = TRUE;

    // Create and ready a stream
    for (mStreamCount = 0; mStreamCount < MAX_TEST_STREAM_COUNT; mStreamCount++) {
        SPRINTF(streamName, "%s %d", TEST_STREAM_NAME, mStreamCount);
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
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThreads[mStreamCount], staticProducerRoutine, (PVOID) (UINT64) mStreamCount));

        // Spin off the consumer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mConsumerThreads[mStreamCount], staticConsumerRoutine, (PVOID) (UINT64) mStreamCount));

        THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    // Enable the threads
    DLOGV("Enabling the execution of the threads");
    ATOMIC_STORE_BOOL(&mStartThreads, TRUE);

    // Wait for some time
    THREAD_SLEEP(TEST_SLEEP_TIME);

    // Indicate the cancel for the threads
    ATOMIC_STORE_BOOL(&mTerminate, TRUE);

    // Join the threads and wait to exit
    for (index = 0; index < MAX_TEST_STREAM_COUNT; index++) {
        EXPECT_EQ(STATUS_SUCCESS, THREAD_JOIN(mProducerThreads[index], NULL));
        EXPECT_EQ(STATUS_SUCCESS, THREAD_JOIN(mConsumerThreads[index], NULL));
    }
}
