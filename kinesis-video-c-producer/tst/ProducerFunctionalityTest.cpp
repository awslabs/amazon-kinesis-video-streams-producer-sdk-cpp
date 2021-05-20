#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerFunctionalityTest : public ProducerClientTestBase {
};

extern ProducerClientTestBase* gProducerClientTestBase;

#define FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT    30 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define FUNCTIONALITY_TEST_STOP_STREAM_TIMEOUT      60 * HUNDREDS_OF_NANOS_IN_A_SECOND
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

    for (i = 0; i < 5; i++) {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreams[0]));
        wait = RAND() % 100 + 200;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[0]));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
        THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND); // avoid throttling.
    }
}

TEST_F(ProducerFunctionalityTest, create_two_stream_async_free_stress_test)
{
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);
    UINT32 i = 0, j = 0, wait, totalStreams = 10;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < totalStreams; j++) {
            SPRINTF(mStreamInfo.name, "ScaryTestStream_%d", j);
            EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreams[j]));
        }
        wait = RAND() % 100;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

        for (j = 0; j < totalStreams; j++) {
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
        THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND); // avoid throttling.
    }
}

TEST_F(ProducerFunctionalityTest, reset_stream_on_stream_error_before_putFrame)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 6;
    mResetStreamCounter = 1;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
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

TEST_F(ProducerFunctionalityTest, reset_stream_on_stream_error_with_frame_in_buffer)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames / 2; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    // induce stream error in the middle of streaming
    mGetEndpointStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mGetEndpointCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 6;
    mResetStreamCounter = 1;

    // trigger getEndpoint error and wait until it happens.
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetConnection(streamHandle));
    while (mStreamErrorFnCount == 0) {
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    for (; i < totalFrames; ++i) {
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
    UINT32 i;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
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
    UINT32 streamCount = 5;

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;

        for (j = 0; j < streamCount; j++) {
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

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
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
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 3;

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for (k = 0; k < totalFrames; k++) {
            for (j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
        }

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));
        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;

        for (j = 0; j < streamCount; j++) {
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(FUNCTIONALITY_TEST_STRESS_TEST_ITERATION, mCurlDescribeStreamCount);
    EXPECT_EQ(FUNCTIONALITY_TEST_STRESS_TEST_ITERATION, mCurlGetDataEndpointCount);
}

TEST_F(ProducerFunctionalityTest, create_caching_endpoint_repeated_create_stream_put_frame_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(TRUE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(0, mCurlDescribeStreamCount);
    EXPECT_EQ(FUNCTIONALITY_TEST_STRESS_TEST_ITERATION, mCurlGetDataEndpointCount);
}

TEST_F(ProducerFunctionalityTest, create_caching_endpoint_client_repeated_create_stream_put_frame_reset_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(TRUE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));
    }

    EXPECT_EQ(0, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlGetDataEndpointCount);
}

TEST_F(ProducerFunctionalityTest, create_caching_all_repeated_create_stream_put_frame_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(API_CALL_CACHE_TYPE_ALL, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(FUNCTIONALITY_TEST_STRESS_TEST_ITERATION, mCurlDescribeStreamCount);
    EXPECT_EQ(FUNCTIONALITY_TEST_STRESS_TEST_ITERATION, mCurlGetDataEndpointCount);
}

TEST_F(ProducerFunctionalityTest, create_caching_all_client_repeated_create_stream_put_frame_reset_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(API_CALL_CACHE_TYPE_ALL, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {
        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        mFrame.flags = FRAME_FLAG_NONE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
        mFrame.index = 0;

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));
    }

    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlGetDataEndpointCount);
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_free_stream_multi_stream)
{
    UINT32 i, j, k;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    static const UINT32 streamCount = 3;
    TID threadIds[streamCount];

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS,
                      createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION,
                                       FALSE));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for (k = 0; k < totalFrames; k++) {
            for (j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
        }

        // As we are processing multiple streams, we can't sequentially stop the streams
        // as the latter streams will timeout their PutMedia calls
        auto stopRoutine = [](PVOID arg) -> PVOID {
            auto streamHandle = (STREAM_HANDLE) arg;
            EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
            return nullptr;
        };

        // Spin off threads to stop sync
        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(
                    &threadIds[j],
                    stopRoutine,
                    (PVOID) mStreams[j]));
        }

        // Await for every one of the threads
        for (j = 0; j < streamCount; j++) {
            THREAD_JOIN(threadIds[j], NULL);
        }

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
            mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
        }
    }
}

TEST_F(ProducerFunctionalityTest, create_client_repeated_create_stream_put_frame_stop_stream_free_stream)
{
    UINT32 i, j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
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
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 3;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < FUNCTIONALITY_TEST_STRESS_TEST_ITERATION; i++) {

        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION, FALSE));
            EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
        }

        for (k = 0; k < totalFrames; k++) {
            for (j = 0; j < streamCount; j++) {
                EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
            }
            updateFrame();
        }

        for (j = 0; j < streamCount; j++) {
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
    UINT32 totalFrames = 50 * TEST_FPS;
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, FALSE, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
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
    EXPECT_EQ(50, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i, errCount;
    UINT32 totalFrames = 50 * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, FALSE, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    // Aborting an active upload handle will still result in backend sending an INVALID_MKV_DATA
    EXPECT_EQ(1, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    EXPECT_EQ(50, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFrames = 50 * TEST_FPS;
    UINT32 pausePutFrameInterval = 40; // pause putFrame for 5 sec every 40 frames.
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, FALSE, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        updateFrame();
        if (mFrame.index % pausePutFrameInterval == 0) {
            THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    EXPECT_EQ(50, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i, errCount;
    UINT32 totalFrames = 50 * TEST_FPS;
    UINT32 pausePutFrameInterval = 40; // pause putFrame for 5 sec every 40 frames.

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, FALSE, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        if (mFrame.index % pausePutFrameInterval == 0) {
            THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    // There is a chance of INVALID_MKV_DATA received on the old session as the backend closes
    // the connection which will be ignored.
    EXPECT_LE(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    // Last fragment of the previous session before the token rotation wouldn't have
    // a chance to receive the persisted ACK.
    EXPECT_EQ(49, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_verify_eofr_sent)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i, errCount;
    UINT32 totalFrames = 510;
    UINT64 startTime, delay;
    mFrame.duration = 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    mKeyFrameInterval = 10;
    UPLOAD_HANDLE streamUploadHandle = 0;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; i++) {
        startTime = GETTIME();
        // i = 480 will be start of a key frame, if I don't start on key-frame
        // this test fails
        if( (i < 30) || (i >= 480) ) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        }
        updateFrame();

        delay = GETTIME() - startTime;
        if (delay < mFrame.duration) {
            THREAD_SLEEP(mFrame.duration - delay);
        }
        EXPECT_EQ(0, mStreamErrorFnCount);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(0, mStreamErrorFnCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, intermittent_producer_verify_eofr_sent_multi_track)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i, errCount;
    UINT32 totalFrames = 510;
    UINT64 startTime, delay;
    mFrame.duration = 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    mKeyFrameInterval = 10;
    UPLOAD_HANDLE streamUploadHandle = 0;


    PStreamInfo pStreamInfo;

    EXPECT_EQ(STATUS_SUCCESS,
            createRealtimeAudioVideoStreamInfoProvider(TEST_STREAM_NAME,
                                                       TEST_RETENTION_PERIOD,
                                                       TEST_STREAM_BUFFER_DURATION,
                                                       &pStreamInfo));

    EXPECT_EQ(FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR, pStreamInfo->streamCaps.frameOrderingMode);
    EXPECT_EQ(2, pStreamInfo->streamCaps.trackInfoCount);

    mStreamInfo = *pStreamInfo;

    // Test - we don't have real data
    mStreamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    mStreamInfo.streamCaps.fragmentDuration = 1 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_OOM;
    mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];

    mFrame.flags = FRAME_FLAG_KEY_FRAME;

    for (i = 0; i < totalFrames; i++) {
        startTime = GETTIME();
        // i = 480 will be start of a key frame, if I don't start on key-frame
        // this test fails
        if ( (i < 30) || (i >= 480) ) {
            mFrame.trackId = TEST_VIDEO_TRACK_ID;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            mFrame.trackId = TEST_AUDIO_TRACK_ID;
            // ok key frame we close fragment -- so if audio
            // is marked key frame we'll create a fragment with no frames in track1 and
            // throw error
            mFrame.flags = FRAME_FLAG_NONE;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        }
        updateFrame();

        delay = GETTIME() - startTime;
        if (delay < mFrame.duration) {
            THREAD_SLEEP(mFrame.duration - delay);
        }
        EXPECT_EQ(0, mStreamErrorFnCount);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(0, mStreamErrorFnCount);
    MEMFREE(pStreamInfo);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}


TEST_F(ProducerFunctionalityTest, pressure_on_buffer_duration_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 120;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    mCurlEasyPerformInjectionCount = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE, 45 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
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
    // Accounting for re-try
    EXPECT_LE(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_buffer_duration_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 120, errCount;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE, 45 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        handlePressure(&mBufferDurationInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    // Aborting an active upload handle might still result in backend sending an INVALID_MKV_DATA
    EXPECT_LE(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    // In this case the old connection has failed so the number of fragment persisted ACKs will be smaller
    EXPECT_GE(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_storage_fail_new_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 100;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    mCurlEasyPerformInjectionCount = 1;
    BYTE tempBuf[10000];

    mDeviceInfo.storageInfo.storageSize = 1 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE, 45 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, MAX_UINT64, 240 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    mFrame.size = SIZEOF(tempBuf);
    mFrame.frameData = tempBuf;
    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (STATUS_SUCCEEDED(mPutMediaStatus) && mCurlPutMediaCount == 1) {
            // this should fail the new putMedia at token rotation
            mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
            mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
        }

        // Pause initially to allow the first upload handle to proceed
        if (i == TEST_FPS) {
            THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }

        updateFrame();
        handlePressure(&mBufferStorageInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(1, mStreamErrorFnCount);
    // Accounting for re-try
    EXPECT_LE(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pressure_on_storage_fail_old_connection_at_token_rotation)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 100, errCount;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mDeviceInfo.storageInfo.storageSize = 1 * 1024 * 1024;
    mStreamInfo.streamCaps.replayDuration = 240 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE, 45 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 240 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mCurlPutMediaCount == 2) {
            mAbortUploadhandle = 0;
        }

        updateFrame();
        handlePressure(&mBufferStorageInPressure, TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    // Aborting an active upload handle might still result in backend sending an INVALID_MKV_DATA
    EXPECT_LE(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);

    // Accounting for re-try
    EXPECT_LE(totalFragments, mPersistedFragmentCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, offline_upload_limited_buffer_duration)
{
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    UINT32 i;
    STREAM_HANDLE streamHandle;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, 4 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
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
    UINT32 totalFragments = 60;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mDeviceInfo.storageInfo.storageSize = 1 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE, 45 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
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
    UINT32 j, k, clipCount = 5;
    UINT32 fragmentsPerClip = 10;
    UINT32 framesPerClip = fragmentsPerClip * TEST_FPS;
    UINT32 pauseBetweenClipSeconds = 1;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < clipCount; ++j) {
        for (k = 0; k < framesPerClip; ++k) {
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
    UINT32 j, k, clipCount = 5;
    UINT32 fragmentsPerClip = 10;
    UINT32 framesPerClip = fragmentsPerClip * TEST_FPS;
    UINT32 pauseBetweenClipSeconds = 1;
    Frame eofrFrame = EOFR_FRAME_INITIALIZER;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < clipCount; ++j) {
        for (k = 0; k < framesPerClip; ++k) {
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

TEST_F(ProducerFunctionalityTest, high_fragment_rate_file_upload)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalDuration = 10;
    UINT32 totalFrames = totalDuration * TEST_FPS;
    mKeyFrameInterval = 4;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, FUNCTIONALITY_TEST_STOP_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
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
TEST_F(ProducerFunctionalityTest, stream_callbacks_going_off_properly)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 80; // 80s in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    // limit storage size to trigger storage overflow callback.
    mDeviceInfo.storageInfo.storageSize = (UINT64) (1.45 * 1024 * 1024);

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 70 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
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

TEST_F(ProducerFunctionalityTest, connection_stale_callbacks_going_off_properly)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    // block off any acks
    mCurlWriteCallbackPassThrough = TRUE;
    mWriteStatus = STATUS_NOT_IMPLEMENTED;

    mStreamingRotationPeriod = MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    mStreamInfo.streamCaps.connectionStalenessDuration = 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mConnectionStaleFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stream_error_callbacks_going_off_properly)
{
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

    for (i = 0; i < totalFrames; ++i) {
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

TEST_F(ProducerFunctionalityTest, basic_reset_connection)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 20;
    UINT32 totalFrames = totalFragments * TEST_FPS, resetConnectionCount = 2;
    // Reset within the 2-3 seconds
    UINT64 resetConnectionPeriod = (RAND() % 2000 + 1000) * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 resetConnectionTime = GETTIME() + resetConnectionPeriod;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);

        if (GETTIME() > resetConnectionTime && resetConnectionCount > 0) {
            resetConnectionCount--;
            resetConnectionTime = GETTIME() + resetConnectionPeriod;
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetConnection(streamHandle));
        }
    }

    DLOGD("Stopping the stream with stream handle %" PRIu64, (UINT64) streamHandle);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamClosedFnCount);
    EXPECT_LT(0, mFragmentAckReceivedFnCount);
    EXPECT_LT(totalFragments, mPersistedFragmentCount);
    // There should be at least 3 upload handles. In case of any retries we will have more.
    EXPECT_LE(3, mPutStreamFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stream_latency_handling_with_continuous_retry)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 i;
    UINT32 totalFragments = 50; // 50 in total duration
    UINT32 totalFrames = totalFragments * TEST_FPS;

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mStreamingRotationPeriod = 40 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, TRUE);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND, 50 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (i = 0; i < totalFrames; ++i) {
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

TEST_F(ProducerFunctionalityTest, DISABLED_offline_mode_multiple_stream_streaming)
{
    UINT32 j, k;
    UINT32 totalFragments = 10;
    UINT32 totalFrames = totalFragments * TEST_FPS, streamCount = 5;

    mDeviceInfo.storageInfo.storageSize = 4 * 1024 * 1024;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (j = 0; j < streamCount; j++) {
        EXPECT_EQ(STATUS_SUCCESS, createTestStream(j, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        EXPECT_TRUE(mStreams[j] != INVALID_STREAM_HANDLE_VALUE);
    }

    for (k = 0; k < totalFrames; k++) {
        for (j = 0; j < streamCount; j++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[j], &mFrame));
        }
        updateFrame();
    }

    for (j = 0; j < streamCount; j++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[j]));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[j]));
    }

    for (j = 0; j < streamCount; j++) {
        mStreams[j] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(totalFragments * streamCount, mPersistedFragmentCount);
}

TEST_F(ProducerFunctionalityTest, create_abstract_provider_add_stream_callback_free_provider)
{
    PStreamCallbacks pStreamCallbacks;
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     TEST_USER_AGENT,
                                                                     &mCallbacksProvider));
    EXPECT_EQ(STATUS_SUCCESS, createStreamCallbacks(&pStreamCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, addStreamCallbacks(mCallbacksProvider, pStreamCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
}

TEST_F(ProducerFunctionalityTest, stop_sending_frame_in_the_middle_of_streaming_realtime_mode)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS, errCount;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // expecting no error so far
    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    // We need to continue putting frames as the last fragment is not complete and has not received the
    // the final persisted ACK in order to gracefully stop.
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stop_sending_frame_in_the_middle_of_streaming_offline_mode)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS, errCount;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // expecting no error so far
    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, stop_sending_frame_in_the_middle_of_streaming_offline_mode_zero_dts)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS, errCount;
    mFrame.decodingTs = 0;
    mFrame.duration = 0;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        mFrame.decodingTs = 0;
        mFrame.presentationTs += TEST_FRAME_DURATION;
    }

    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // expecting no error so far
    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pause_sending_frame_until_timeout_then_put_non_key_frame)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS, errCount;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // put a non key frame
    mFrame.flags = FRAME_FLAG_NONE;

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    // expecting no error so far
    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, pause_sending_frame_until_timeout_then_put_key_frame)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS, errCount;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    // expecting no error so far
    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, token_rotation_collide_with_curl_timeout_idling)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 3;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // put some frames initially
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
    // trigger curl timeout.
    THREAD_SLEEP(35 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (j = 0; j < totalFrames; ++j) {
        // NOTE: as we have streamed the last bit there will be no more calls to curl
        // The next frame here is a Key-frame which makes the state machine to step out
        // and not yet call put stream state so the fault is not really injected
        // as we are overwriting the fault inject value right after the frame is put.
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mStreamErrorFnCount != 0) {
            mPutMediaStatus = STATUS_SUCCESS;
            mPutMediaCallResult = SERVICE_CALL_RESULT_OK;
        }

        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamErrorFnCount); // putMedia error purposely induced
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, curl_timeout_idling_force_ack)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 3, ackCount, persistedAckCount;
    UINT32 totalFrames = totalFragments * TEST_FPS;
    Frame eofr = EOFR_FRAME_INITIALIZER;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // put some frames initially
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    // Wait for a while to ensure the frames are uploaded
    THREAD_SLEEP(2 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Get the number of ACKs
    ackCount = mFragmentAckReceivedFnCount;
    persistedAckCount = mPersistedFragmentCount;

    // Check the ACK timestamps
    // NOTE: The Buffering ACK should have already been received but Received and Persisted
    // should not yet arrive.
    // The Timestamp of the ACK is the Key frame timestamp in milliseconds
    // hence, we use the total frames and account for the entire fragment (TEST_FPS) to
    // get to the key frame timestamp.
    EXPECT_EQ((totalFrames - TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastBufferingAckTimestamp);
    EXPECT_EQ((totalFrames - 2 * TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastReceivedAckTimestamp);
    EXPECT_EQ((totalFrames - 2 * TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastPersistedAckTimestamp);
    EXPECT_EQ(0, mLastErrorAckTimestamp);

    // Submit an EoFR to force the ack
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &eofr));

    // Wait until the ack comes back
    THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Should be one more as we should have received the persistent ack for the incomplete fragment
    // There should be a received ack and a persisted ack.
    EXPECT_EQ(ackCount + 2, mFragmentAckReceivedFnCount);
    EXPECT_EQ(persistedAckCount + 1, mPersistedFragmentCount);

    // Check the ACK timestamps
    EXPECT_EQ((totalFrames - TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastBufferingAckTimestamp);
    EXPECT_EQ((totalFrames - TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastReceivedAckTimestamp);
    EXPECT_EQ((totalFrames - TEST_FPS) * mFrame.duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, mLastPersistedAckTimestamp);
    EXPECT_EQ(0, mLastErrorAckTimestamp);

    // trigger curl timeout.
    THREAD_SLEEP(32 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, token_rotation_collide_with_curl_timeout_idling_with_non_key_frame)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, TEST_STOP_STREAM_TIMEOUT, FALSE, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // put some frames initially
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    // Call once more to ensure we won't be starting on a key-frame after the timeout
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
    updateFrame();

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;
    // trigger curl timeout.
    THREAD_SLEEP(42 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (j = 0; j < totalFrames; ++j) {
        // NOTE: as we have streamed the last bit there will be no more calls to curl
        // The next frame here is a Key-frame which makes the state machine to step out
        // and not yet call put stream state so the fault is not really injected
        // as we are overwriting the fault inject value right after the frame is put.
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        if (mStreamErrorFnCount != 0) {
            mPutMediaStatus = STATUS_SUCCESS;
            mPutMediaCallResult = SERVICE_CALL_RESULT_OK;
        }

        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_LT(0, mStreamErrorFnCount); // putMedia error purposely induced
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, offline_streaming_single_frame_fragment_large_gop_zero_duration_zero_dts)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 5;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    mFrame.duration = 0;
    mFrame.decodingTs = 0;

    // first batch of frames
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        mFrame.decodingTs = 0;
        mFrame.presentationTs += TEST_FRAME_DURATION;
    }

    // single frame fragment
    mFrame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));

    // large gop
    mFrame.presentationTs += 200 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    // first batch of frames
    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        mFrame.decodingTs = 0;
        mFrame.presentationTs += TEST_FRAME_DURATION;
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments * 2 + 1, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, putFrame_stopSync_reset_then_put_single_frame)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));

    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments + 1, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, putFrame_stopSync_reset_then_putFrame_again)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
        THREAD_SLEEP(mFrame.duration);
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments * 2, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerFunctionalityTest, putFrame_stopSync_timeout_reset_then_putFrame_again)
{
    UINT32 j;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS, oldStreamClosedFnCount, errCount;
    UINT64 stopStreamEndtime;

    mDeviceInfo.clientInfo.stopStreamTimeout = 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT, 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));

    for (j = 0; j < totalFrames; ++j) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
        updateFrame();
    }

    EXPECT_EQ(0, mStreamErrorFnCount);
    errCount = mStreamErrorFnCount;

    oldStreamClosedFnCount = mStreamClosedFnCount;
    stopStreamEndtime = GETTIME() + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    while (oldStreamClosedFnCount == mStreamClosedFnCount && GETTIME() < stopStreamEndtime) {
        THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    EXPECT_GT(mStreamClosedFnCount, oldStreamClosedFnCount);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    // no new error during shutdown
    EXPECT_EQ(errCount, mStreamErrorFnCount);
    EXPECT_LE(totalFragments, mPersistedFragmentCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}


TEST_F(ProducerFunctionalityTest, putFrame_stopSync_reset_then_putFrame_again_multistream)
{
    UINT32 j, i;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2, totalStreams = 3;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, createTestStream(i, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        EXPECT_TRUE(mStreams[i] != INVALID_STREAM_HANDLE_VALUE);
    }

    for (j = 0; j < totalFrames; ++j) {
        for (i = 0; i < totalStreams; i++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[i], &mFrame));
        }
        updateFrame();
    }

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[i]));
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(mStreams[i]));
    }

    for (j = 0; j < totalFrames; ++j) {
        for (i = 0; i < totalStreams; i++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[i], &mFrame));
        }
        updateFrame();
    }

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[i]));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[i]));
        mStreams[i] = INVALID_STREAM_HANDLE_VALUE;
    }

    EXPECT_EQ(0, mStreamErrorFnCount);
    EXPECT_EQ(totalFragments * 2 * totalStreams, mPersistedFragmentCount);
}

TEST_F(ProducerFunctionalityTest, putFrame_stop_reset_then_putFrame_again)
{
    UINT32 j, i;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2, wait, totalIteration = 2;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    for (i = 0; i < totalIteration; ++i) {
        createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

        EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        streamHandle = mStreams[0];
        EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
        wait = RAND() % 100 + 50;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));

        for (j = 0; j < totalFrames; ++j) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &mFrame));
            updateFrame();
        }

        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));

        // may not get all persisted acks for fragments in the first batch because of asycn stop
        EXPECT_GE(mPersistedFragmentCount, totalFragments);
        EXPECT_EQ(0, mStreamErrorFnCount);

        MEMFREE(mProducerCallbacks);
        mProducerCallbacks = NULL;
        mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
        mFrame.presentationTs = 0;
        mFrame.decodingTs = 0;
    }
}

TEST_F(ProducerFunctionalityTest, putFrame_stop_reset_then_putFrame_again_multistream)
{
    UINT32 j, i;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 totalFragments = 2, totalStreams = 3, wait, errorCount;
    UINT32 totalFrames = totalFragments * TEST_FPS;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, createTestStream(i, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));
        EXPECT_TRUE(mStreams[i] != INVALID_STREAM_HANDLE_VALUE);
    }

    for (j = 0; j < totalFrames; ++j) {
        for (i = 0; i < totalStreams; i++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[i], &mFrame));
        }
        updateFrame();
    }

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreams[i]));
        wait = (UINT32) RAND() % 100 + 50;
        THREAD_SLEEP(wait * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(mStreams[i]));
    }

    for (j = 0; j < totalFrames; ++j) {
        for (i = 0; i < totalStreams; i++) {
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[i], &mFrame));
        }
        updateFrame();
    }

    // Shouldn't have any errors
    EXPECT_EQ(0, mStreamErrorFnCount);

    // Store the error count so we can validate. There might be errors due to ACKs still coming in and being rejected
    // after the async version of stop stream called.
    errorCount = mStreamErrorFnCount;

    for (i = 0; i < totalStreams; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[i]));
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[i]));
        mStreams[i] = INVALID_STREAM_HANDLE_VALUE;
    }

    // no error during shutdown
    EXPECT_EQ(errorCount, mStreamErrorFnCount);

    // may not get all persisted acks for fragments in the first batch because of asycn stop
    EXPECT_GE(mPersistedFragmentCount, totalFragments * totalStreams);
}

TEST_F(ProducerFunctionalityTest, dropTailFragPolicyStorageOverflowNoInvalidMkv)
{
    UINT32 i;
    UINT32 totalFrames = (UINT32) ((DOUBLE) 1 * 1024 * 1024 / TEST_FRAME_SIZE * 1.5);

    mDeviceInfo.storageInfo.storageSize = 1 * 1024 * 1024;
    mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_DROP_TAIL_ITEM;
    mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_UNTIL_FRAGMENT_START;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, TEST_STREAM_BUFFER_DURATION));

    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &mFrame));
    updateFrame();

    // short sleep before curl comes to life, otherwise we may push stream start frame out of buffer.
    THREAD_SLEEP(500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    for (i = 0; i < totalFrames; i++) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &mFrame));
        updateFrame();
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[0]));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[0]));
    mStreams[i] = INVALID_STREAM_HANDLE_VALUE;

    // Shouldn't have any errors
    EXPECT_EQ(0, mStreamErrorFnCount);

    // storage callback should go off
    EXPECT_GT(mStorageOverflowCount, 0);
    // should have frame drop
    EXPECT_GT(mDroppedFrameFnCount, 0);
    // should only use one putMedia. Frame drop will not corrupt current putMedia
    EXPECT_EQ(mPutStreamFnCount, 1);

    // should have some persisted ack
    EXPECT_GT(mPersistedFragmentCount, 0);
}

TEST_F(ProducerFunctionalityTest, dropTailFragPolicyBufferOverflowNoInvalidMkv)
{
    UINT32 i;
    UINT32 totalFrames = (UINT32) ((DOUBLE) 5 * HUNDREDS_OF_NANOS_IN_A_SECOND / mFrame.duration * 1.5);

    mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_DROP_TAIL_ITEM;
    mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_UNTIL_FRAGMENT_START;

    createDefaultProducerClient(FALSE, FUNCTIONALITY_TEST_CREATE_STREAM_TIMEOUT);

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &mFrame));
    updateFrame();

    // short sleep before curl comes to life, otherwise we may push stream start frame out of buffer.
    THREAD_SLEEP(500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    for (i = 0; i < totalFrames; i++) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &mFrame));
        updateFrame();
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreams[0]));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreams[0]));
    mStreams[i] = INVALID_STREAM_HANDLE_VALUE;

    // Shouldn't have any errors
    EXPECT_EQ(0, mStreamErrorFnCount);

    // should have frame drop
    EXPECT_GT(mDroppedFrameFnCount, 0);
    // should only use one putMedia. Frame drop will not corrupt current putMedia
    EXPECT_EQ(mPutStreamFnCount, 1);

    // should have some persisted ack
    EXPECT_GT(mPersistedFragmentCount, 0);
}

}
}
}
}
