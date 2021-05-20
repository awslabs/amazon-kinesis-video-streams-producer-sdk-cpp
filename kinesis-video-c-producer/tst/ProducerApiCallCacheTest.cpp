#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerApiCallCacheTest : public ProducerClientTestBase {
};

TEST_F(ProducerApiCallCacheTest, basicValidateNoCaching)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    // Fault-inject putmedia call to make sure the describe gets called again
    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 2;

    createDefaultProducerClient(API_CALL_CACHE_TYPE_NONE, 0, TEST_STOP_STREAM_TIMEOUT, TRUE);
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND, 50 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Make sure the true APIs are called
    EXPECT_EQ(0, mCreateStreamFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_EQ(1, mTagResourceFnCount);
    EXPECT_EQ(1, mGetStreamingEndpointFnCount);

    // The number of times the Curl hook was called
    EXPECT_EQ(3, mEasyPerformFnCount);

    // Make the transition to streaming
    Frame frame;
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    MEMSET(frame.frameData, 0x55, mFrameSize);
    frame.index = 0;
    frame.decodingTs = frame.presentationTs = GETTIME();
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &frame));

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mBufferDurationInPressure);
    EXPECT_LE(2, mStreamReadyFnCount);

    // The number of times the Curl hook was called
    EXPECT_EQ(12, mEasyPerformFnCount);

    EXPECT_EQ(0, mCreateStreamFnCount);
    EXPECT_EQ(3, mDescribeStreamFnCount);
    EXPECT_EQ(3, mTagResourceFnCount);
    EXPECT_EQ(3, mGetStreamingEndpointFnCount);
    EXPECT_EQ(3, mCurlPutMediaCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerApiCallCacheTest, basicValidateEndpointOnlyCaching)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    createDefaultProducerClient(API_CALL_CACHE_TYPE_ENDPOINT_ONLY, 0, TEST_STOP_STREAM_TIMEOUT, TRUE);
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND, 50 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Make sure the true APIs are called
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_EQ(1, mTagResourceFnCount);
    EXPECT_EQ(1, mGetStreamingEndpointFnCount);

    // The number of times the Curl hook was called - only once should be called
    EXPECT_EQ(1, mEasyPerformFnCount);

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mBufferDurationInPressure);
    EXPECT_EQ(0, mDroppedFrameFnCount);
    EXPECT_EQ(1, mStreamReadyFnCount);
    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

TEST_F(ProducerApiCallCacheTest, basicValidateCachingAllApis)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    // Fault-inject putmedia call to make sure the describe gets called again
    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_UNKNOWN;
    mCurlEasyPerformInjectionCount = 2;

    createDefaultProducerClient(API_CALL_CACHE_TYPE_ALL, 0, TEST_STOP_STREAM_TIMEOUT, TRUE);
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 40 * HUNDREDS_OF_NANOS_IN_A_SECOND, 50 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    streamHandle = mStreams[0];
    EXPECT_TRUE(streamHandle != INVALID_STREAM_HANDLE_VALUE);

    // Make sure the true APIs are called
    EXPECT_EQ(0, mCreateStreamFnCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_EQ(1, mTagResourceFnCount);
    EXPECT_EQ(1, mGetStreamingEndpointFnCount);

    // The number of times the Curl hook was called
    EXPECT_EQ(3, mEasyPerformFnCount);

    // Make the transition to streaming
    Frame frame;
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    MEMSET(frame.frameData, 0x55, mFrameSize);
    frame.index = 0;
    frame.decodingTs = frame.presentationTs = GETTIME();
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &frame));

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(0, mBufferDurationInPressure);
    EXPECT_LE(2, mStreamReadyFnCount);

    // The number of times the Curl hook was called
    // 1 describe, 1 get endpoint, 1 tag and 3 put media calls
    EXPECT_EQ(6, mEasyPerformFnCount);

    EXPECT_EQ(0, mCreateStreamFnCount);
    EXPECT_EQ(3, mDescribeStreamFnCount);
    EXPECT_EQ(3, mTagResourceFnCount);
    EXPECT_EQ(3, mGetStreamingEndpointFnCount);
    EXPECT_EQ(3, mCurlPutMediaCount);

    mStreams[0] = INVALID_STREAM_HANDLE_VALUE;
}

}
}
}
}