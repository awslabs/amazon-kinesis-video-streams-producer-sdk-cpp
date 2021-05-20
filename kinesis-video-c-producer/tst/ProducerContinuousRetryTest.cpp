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

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, TRUE);

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
    THREAD_SLEEP(3 * HUNDREDS_OF_NANOS_IN_A_SECOND);
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

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames - 1; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    THREAD_SLEEP(mFrame.duration / 2);
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    THREAD_SLEEP(3 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(1, mStreamLatencyPressureFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mGetStreamingEndpointFnCount);
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, test_stream_recover_after_reset_connnection) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 10s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, TRUE);

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

TEST_F(ProducerContinuousRetryTest, test_stream_recover_after_reset_connnection_stream_session_timeout) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10; // 10s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, TRUE);

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

TEST_F(ProducerContinuousRetryTest, recover_on_retriable_producer_error) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    PRotatingStaticAuthCallbacks pAuth;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce a new session on connection staleness
    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Inject a fault
    pAuth = (PRotatingStaticAuthCallbacks) mAuthCallbacks;
    pAuth->retStatus = STATUS_INVALID_DESCRIBE_STREAM_RETURN_JSON;
    pAuth->failCount = 0;
    pAuth->recoverCount = 3; // 1 for main token, 1 for the security token for the first session
    // and only then should fail for the last token.

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    // Give a little time for the retry logic
    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Freeing the stream with stream handle %" PRIu64, (UINT64) streamHandle);

    // We are not even attempting to stop just to test the functionality of simply freeing
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mConnectionStaleFnCount);
    EXPECT_LE(1, mDescribeStreamFnCount);
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, no_recovery_on_non_retriable_producer_error) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    PRotatingStaticAuthCallbacks pAuth;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce a new session on connection staleness
    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Inject a fault
    pAuth = (PRotatingStaticAuthCallbacks) mAuthCallbacks;
    pAuth->retStatus = STATUS_MAX_CUSTOM_USER_AGENT_LEN_EXCEEDED; // Non-recoverable
    pAuth->failCount = 0;
    pAuth->recoverCount = 3; // 1 for main token, 1 for the security token for the first session
    // and only then should fail for the last token.

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Freeing the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_NE(1, mConnectionStaleFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount); // As there is no retrying on this fault, describe should have not been called again
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, recover_on_retriable_common_lib_error) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    PRotatingStaticAuthCallbacks pAuth;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce a new session on connection staleness
    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Inject a fault
    pAuth = (PRotatingStaticAuthCallbacks) mAuthCallbacks;
    pAuth->retStatus = STATUS_IOT_FAILED;
    pAuth->failCount = 0;
    pAuth->recoverCount = 3; // 1 for main token, 1 for the security token for the first session
    // and only then should fail for the last token.

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Freeing the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mConnectionStaleFnCount);
    EXPECT_LE(1, mDescribeStreamFnCount);
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, no_recovery_on_non_retriable_common_lib_error) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    PRotatingStaticAuthCallbacks pAuth;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce a new session on connection staleness
    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Inject a fault
    pAuth = (PRotatingStaticAuthCallbacks) mAuthCallbacks;
    pAuth->retStatus = STATUS_INVALID_CA_CERT_PATH; // Non-recoverable
    pAuth->failCount = 0;
    pAuth->recoverCount = 3; // 1 for main token, 1 for the security token for the first session
    // and only then should fail for the last token.

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Freeing the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_NE(1, mConnectionStaleFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount); // As there is no retrying on this fault, describe should have not been called again
    // Reset connection should trigger a new PutMedia session
    EXPECT_LE(2, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerContinuousRetryTest, contrinuous_retry_reset_failure) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    PRotatingStaticAuthCallbacks pAuth;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce a new session on connection staleness
    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Inject a fault
    pAuth = (PRotatingStaticAuthCallbacks) mAuthCallbacks;
    pAuth->retStatus = STATUS_DESCRIBE_STREAM_CALL_FAILED;
    pAuth->failCount = 0;
    pAuth->recoverCount = 1000;

    // Make describe to fail consequently
    mDescribeRecoverCount = 1000;
    mDescribeRetStatus = STATUS_DESCRIBE_STREAM_CALL_FAILED;

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration / 2); // speed up test
    }

    // Give a little time for the retry logic
    // IMPORTANT: In this case our fault injection will still make the retries happen
    // as we are "cleanly" failing the reset API call to describe, which will call
    // the handler function in the curl callbacks to call notify with the injected
    // error which, in turn, would invoke the continuous callback handler which,
    // in turn would issue a reset
    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    DLOGD("Freeing the stream with stream handle %" PRIu64, (UINT64) streamHandle);

    // We are not even attempting to stop just to test the functionality of simply freeing
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mConnectionStaleFnCount);
    EXPECT_LE(1, mDescribeStreamFnCount);

    EXPECT_LE(1, mStreamErrorFnCount);
    EXPECT_EQ(STATUS_CONTINUOUS_RETRY_RESET_FAILED, mLastError);

    // We should never reach a new PutStream state after reset
    EXPECT_EQ(1, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

}
}
}
}
