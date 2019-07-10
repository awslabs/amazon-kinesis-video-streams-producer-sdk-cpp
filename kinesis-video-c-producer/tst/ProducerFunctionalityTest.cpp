#include <com/amazonaws/kinesis/video/mkvgen/Include.h>
#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerFunctionalityTest : public ProducerClientTestBase {
};

extern ProducerClientTestBase* gProducerClientTestBase;

#define FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT    5 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define FUNCTIONALITY_TEST_STRESS_TEST_ITERATION    3

TEST_F(ProducerFunctionalityTest, start_stopsync_terminate)
{
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[0]));
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, create_stream_async_free_stress_test)
{
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);
    UINT32 i = 0, wait;
    SPRINTF(mStreamInfo.name, "ScaryTestStream_%d", 0);

    for(i = 0; i < 10; i++) {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreams[0]));
        wait = RAND() % 100 + 200;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[0]));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND); // avoid throttling.
    }
}

TEST_F(ProducerFunctionalityTest, create_two_stream_async_free_stress_test)
{
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);
    UINT32 i = 0, j = 0, wait, totalStreams = 10;

    for(i = 0; i < 10; i++) {
        for(j = 0; j < totalStreams; j++) {
            SPRINTF(mStreamInfo.name, "ScaryTestStream_%d", j);
            EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreams[j]));
        }
        wait = RAND() % 100 + 200;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

        for(j = 0; j < totalStreams; j++) {
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
        THREAD_SLEEP(2 * HUNDREDS_OF_NANOS_IN_A_SECOND); // avoid throttling.
    }
}

TEST_F(ProducerFunctionalityTest, reset_stream_on_stream_error_before_putFrame)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 6;
    mResetStreamCounter = 1;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamErrorFnCount);
    EXPECT_LT(0, mStreamClosedFnCount);
    EXPECT_LT(0, mFragmentAckReceivedFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;

}

TEST_F(ProducerFunctionalityTest, reset_stream_on_stream_error_with_frame_in_buffer)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames / 2; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    // induce stream error in the middle of streaming
    mGetEndpointStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mGetEndpointCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 6;
    mResetStreamCounter = 1;

    // gonna cause getEndpoint error
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetConnection(streamHandle));

    for(; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamErrorFnCount);
    EXPECT_LT(0, mStreamClosedFnCount);
    EXPECT_LT(0, mFragmentAckReceivedFnCount);
    // some fragments are gonna be dropped when resetStream is triggered.
    EXPECT_LT(mPersistedFragmentCount, totalFragments);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;

}

TEST_F(ProducerFunctionalityTest, repeated_create_client_create_stream_sync_and_free_everything)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }
}

TEST_F(ProducerFunctionalityTest, repeated_create_client_create_stream_sync_and_free_everything_multi_stream)
{
    UINT32 i, j;
    UINT32 streamCount = 10;

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;

        for(j = 0; j < streamCount; j++) {
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, repeated_create_client_create_stream_put_frame_stop_sync_free_everything)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for(j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }
}

TEST_F(ProducerFunctionalityTest, repeated_create_client_create_stream_put_frame_stop_sync_free_everything_multi_stream)
{
    UINT32 i, j, k;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 10;

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for(k = 0; k < totalFrames; k++) {
            for(j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;

        for(j = 0; j < streamCount; j++) {
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for(j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_free_stream_multi_stream)
{
    UINT32 i, j, k;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 10;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for(k = 0; k < totalFrames; k++) {
            for(j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_stop_stream_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for(j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_stop_stream_free_stream_multi_stream)
{
    UINT32 i, j, k;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 10;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for(i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for(k = 0; k < totalFrames; k++) {
            for(j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
            THREAD_SLEEP(mFrame.duration);
        }

        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreams[j]));
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFrames = 60 * TEST_FPS; // need to stream until token rotation
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    EXPECT_EQ(60, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFrames = 60 * TEST_FPS; // need to stream until token rotation

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(60, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFrames = 60 * TEST_FPS; // need to stream until token rotation
    UINT32 pausePutFrameInterval = 40; // pause putFrame for 5 sec every 40 frames.
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        updateFrame();
        if (mFrame.index % pausePutFrameInterval == 0) {
            THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        } else {
            THREAD_SLEEP(mFrame.duration);
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    EXPECT_EQ(60, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFrames = 60 * TEST_FPS; // need to stream until token rotation
    UINT32 pausePutFrameInterval = 40; // pause putFrame for 5 sec every 40 frames.

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        if (mFrame.index % pausePutFrameInterval == 0) {
            THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        } else {
            THREAD_SLEEP(mFrame.duration);
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(60, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_buffer_duration_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 120;
    UINT32 totalFrames = totalFragments * TEST_FPS; // need to stream until token rotation
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        updateFrame();
        handlePressure(&mBufferDurationInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_buffer_duration_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 120;
    UINT32 totalFrames = totalFragments * TEST_FPS; // need to stream until token rotation

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        handlePressure(&mBufferDurationInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_storage_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 300;
    UINT32 totalFrames = totalFragments * TEST_FPS; // need to stream until token rotation
    mCurlEasyPerformInjectionCount = 1;

    // bump frame size to help stream long enough until token rotation
    mFrameSize = 6000;
    MEMFREE(mFrameBuffer);
    mFrameBuffer = (PBYTE) MEMALLOC(mFrameSize);
    mFrame.size = mFrameSize;
    mFrame.frameData = mFrameBuffer;

    mDeviceInfo.storageInfo.storageSize = 10 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 240 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        updateFrame();
        handlePressure(&mBufferStorageInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_storage_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 300;
    UINT32 totalFrames = totalFragments * TEST_FPS; // need to stream until token rotation

    // bump frame size to help stream long enough until token rotation
    mFrameSize = 6000;
    MEMFREE(mFrameBuffer);
    mFrameBuffer = (PBYTE) MEMALLOC(mFrameSize);
    mFrame.size = mFrameSize;
    mFrame.frameData = mFrameBuffer;

    mDeviceInfo.storageInfo.storageSize = 10 * 1024 * 1024;
    mStreamInfo.streamCaps.replayDuration = 240 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 240 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        handlePressure(&mBufferStorageInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}


TEST_F(ProducerFunctionalityTest, offline_upload_limited_buffer_duration)
{
    UINT32 totalFragments = 120;
    UINT32 totalFrames = totalFragments * TEST_FPS; // need to stream until token rotation
    UINT32 i;
    STREAM_HANDLE streamHandle;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, 4 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);
    EXPECT_EQ(FALSE, mBufferDurationInPressure);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}



TEST_F(ProducerFunctionalityTest, offline_upload_limited_storage)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 240; // stream more fragments to reach token rotation
    UINT32 totalFrames = totalFragments * TEST_FPS;

    // bump frame size to reach token rotation
    mFrameSize = 10000;
    MEMFREE(mFrameBuffer);
    mFrameBuffer = (PBYTE) MEMALLOC(mFrameSize);
    mFrame.size = mFrameSize;
    mFrame.frameData = mFrameBuffer;

    mDeviceInfo.storageInfo.storageSize = 10 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_file_upload)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 j, k, clipCount = 10;
    UINT32 fragmentsPerClip = 30;
    UINT32 framesPerClip = fragmentsPerClip * TEST_FPS;
    UINT32 pauseBetweenClipSeconds = 10;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(j = 0; j < clipCount; ++j) {
        for(k = 0; k < framesPerClip; ++k) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        DLOGD("Pausing for next clip");
        THREAD_SLEEP(pauseBetweenClipSeconds * HUNDREDS_OF_NANOS_IN_A_SECOND);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(fragmentsPerClip * clipCount, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_file_upload_with_eofr)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 j, k, clipCount = 10;
    UINT32 fragmentsPerClip = 30;
    UINT32 framesPerClip = fragmentsPerClip * TEST_FPS;
    UINT32 pauseBetweenClipSeconds = 10;
    Frame eofrFrame = EOFR_FRAME_INITIALIZER;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(j = 0; j < clipCount; ++j) {
        for(k = 0; k < framesPerClip; ++k) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &eofrFrame));
        DLOGD("Pausing for next clip");
        THREAD_SLEEP(pauseBetweenClipSeconds * HUNDREDS_OF_NANOS_IN_A_SECOND);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(fragmentsPerClip * clipCount, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, high_fragment_rate_file_upload) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalDuration = 100;
    UINT32 totalFrames = totalDuration * TEST_FPS;
    mKeyFrameInterval = 4;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFrames / mKeyFrameInterval, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

/*
 * This test verifies that BufferDurationPressure, StreamLatency, StorageOverflow, DroppedFrame and StreamReady callbacks
 * are called.
 */
TEST_F(ProducerFunctionalityTest, stream_callbacks_going_off_properly) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 80; // 80s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    // limit storage size to trigger storage overflow callback.
    mDeviceInfo.storageInfo.storageSize = 1.5 * 1024 * 1024;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 70 * HUNDREDS_OF_NANOS_IN_A_SECOND));
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
    EXPECT_LT(0, mBufferDurationInPressure);
    EXPECT_LT(0, mStorageOverflowCount);
    EXPECT_LT(0, mDroppedFrameFnCount);
    EXPECT_LT(0, mStreamReadyFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, connection_stale_callbacks_going_off_properly) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

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
    EXPECT_LT(0, mConnectionStaleFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stream_error_callbacks_going_off_properly) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);
    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamErrorFnCount);
    EXPECT_LT(0, mStreamClosedFnCount);
    EXPECT_LT(0, mFragmentAckReceivedFnCount);
    EXPECT_EQ(totalFragments, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, basic_reset_connection) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS, resetConnectionCount = 3;
    UINT64 resetConnectionTime = GETTIME() + (RAND() % 2000 + 3000) * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for(i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);

        if (GETTIME() > resetConnectionTime && resetConnectionCount > 0) {
            resetConnectionCount--;
            resetConnectionTime = GETTIME() + (RAND() % 2000 + 3000) * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetConnection(streamHandle));
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamClosedFnCount);
    EXPECT_LT(0, mFragmentAckReceivedFnCount);
    EXPECT_LT(totalFragments, mPersistedFragmentCount);
    EXPECT_LT(4, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stream_latency_handling_with_continuous_retry) {
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 50; // 50 in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND, 50 * HUNDREDS_OF_NANOS_IN_A_SECOND));
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
    EXPECT_LT(0, mBufferDurationInPressure);
    EXPECT_LT(0, mDroppedFrameFnCount);
    EXPECT_LT(0, mStreamReadyFnCount);
    EXPECT_LT(3, mPutStreamFnCount); // 1 initial putStream, 1 from token rotation, 1 from streamLatencyHandlingStateMachine
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, offline_mode_multiple_stream_streaming) {
    UINT32 j, k;
    UINT32 totalFragments = 20;
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 10;

    mDeviceInfo.storageInfo.storageSize = 4 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for(j = 0; j < streamCount; j++) {
        EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
    }

    for(k = 0; k < totalFrames; k++) {
        for(j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
        }
        updateFrame();
    }

    for(j = 0; j < streamCount; j++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
    }

    for(j = 0; j < streamCount; j++) {
        mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(totalFragments * streamCount, mPersistedFragmentCount);
}

}
}
}
}

