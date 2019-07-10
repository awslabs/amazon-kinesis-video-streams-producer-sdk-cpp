#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    
class ProducerContinuousRetryTest : public ProducerClientTestBase {
};

extern ProducerClientTestBase* gProducerClientTestBase;

TEST_F(ProducerContinuousRetryTest, test_stream_callbacks_connection_stale_triggered) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 130s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TRUE);

    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(1, mConnectionStaleFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_LT(0, mStreamReadyFnCount);
    // Reset connection should trigger a new PutMedia session
    EXPECT_EQ(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, test_stream_callbacks_stream_latency_triggered) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 10s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames - 1; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    THREAD_SLEEP(mFrame.duration / 2);
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(1, mStreamLatencyPressureFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mGetStreamingEndpointFnCount);
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, DISABLED_test_stream_recover_after_reset_connnection) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 10s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    kinesisVideoStreamResetConnection(streamHandle);

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mDroppedFrameFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, DISABLED_test_stream_recover_after_reset_connnection_stream_session_timeout) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 10s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    kinesisVideoStreamResetConnection(streamHandle);

    THREAD_SLEEP(31 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mDroppedFrameFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

}
}
}
}

