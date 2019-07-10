#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerClientFaultInjectionTest : public ProducerClientTestBase {
};

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedDescribeCall)
{
    createDefaultProducerClient(FALSE);

    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutDescribeCall)
{
    // Set the timeout enough to retry with progressive back-off
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedDescribeCallWithContinuousCallbacks)
{
    createDefaultProducerClient(FALSE, TEST_CREATE_STREAM_TIMEOUT, TRUE);

    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    // Should only attempt a single try even with continuous callbacks as this is a non-recoverable error
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutDescribeCallWithContinuousCallbacksRecovers)
{
    // Set the timeout enough to retry with progressive back-off
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;
    mCurlEasyPerformInjectionCount = SERVICE_CALL_MAX_RETRY_COUNT + 1;

    // Attempt to create a stream
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_LT(SERVICE_CALL_MAX_RETRY_COUNT + 1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedCreateCall)
{
    createDefaultProducerClient(FALSE);

    // Induce create call
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mCreateStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mCreateStreamCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(1, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedCreateCallWithContinuousCallbacksRecovers)
{
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce create call
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mCreateStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mCreateStreamCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // 1 describe, retryCount + 1 create
    mCurlEasyPerformInjectionCount = SERVICE_CALL_MAX_RETRY_COUNT + 1 + 1;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(1, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutCreateCallWithContinuousCallbacksRecovers)
{
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND, TRUE);

    // Induce create call
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mCreateStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mCreateStreamCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    // Attempt to create a stream
    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    // SERVICE_CALL_MAX_RETRY_COUNT + 1 retries and 1 caused by continuous retry logic
    // 1 describe 404, retryCount + 1 create timeout， 1 describe 404
    EXPECT_LT(SERVICE_CALL_MAX_RETRY_COUNT + 3, mCurlCreateStreamCount);
    EXPECT_LE(2, mCurlDescribeStreamCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutCreateCall)
{
    // Set the timeout enough to retry with progressive back-off
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Induce create call
    mDescribeStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mDescribeStreamCallResult = SERVICE_CALL_RESOURCE_NOT_FOUND;

    mCreateStreamStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mCreateStreamCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedGetEndpointCall)
{
    createDefaultProducerClient(FALSE);

    mGetEndpointStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mGetEndpointCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlTagResourceCount);
    EXPECT_EQ(1, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutGetEndpointCall)
{
    // Set the timeout enough to retry with progressive back-off
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    mGetEndpointStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mGetEndpointCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlTagResourceCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedTagCall)
{
    createDefaultProducerClient(FALSE);

    mTagResourceStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mTagResourceCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    // Attempt to create a stream
    EXPECT_NE(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlTagResourceCount);
    EXPECT_EQ(0, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutTagCall)
{
    // Set the timeout enough to retry with progressive back-off
    createDefaultProducerClient(FALSE, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    mTagResourceStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mTagResourceCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    // Inducing tag error should still let the stream creation to go through
    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mCurlTagResourceCount);
    EXPECT_EQ(1, mCurlGetDataEndpointCount);
    EXPECT_EQ(0, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, notAuthorizedPutMediaCall)
{
    createDefaultProducerClient(FALSE);

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_NOT_AUTHORIZED;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    // Induce the putMedia call by putting a frame
    Frame frame;
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    MEMSET(frame.frameData, 0x55, SIZEOF(mFrameBuffer));
    frame.index = 0;
    frame.decodingTs = frame.presentationTs = GETTIME();
    frame.size = SIZEOF(mFrameBuffer);
    frame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &frame));

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(1, mCurlDescribeStreamCount);
    EXPECT_EQ(1, mCurlTagResourceCount);
    EXPECT_EQ(1, mCurlGetDataEndpointCount);
    EXPECT_LE(1, mCurlPutMediaCount);

    freeStreams();
}

TEST_F(ProducerClientFaultInjectionTest, timeoutPutMediaCall)
{
    // Set the timeout enough to retry with progressive back-off more than max retry times
    createDefaultProducerClient(FALSE);

    mPutMediaStatus = STATUS_NOT_IMPLEMENTED; // Non success status
    mPutMediaCallResult = SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;

    EXPECT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    // Induce the putMedia call by putting a frame
    Frame frame;
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    MEMSET(frame.frameData, 0x55, SIZEOF(mFrameBuffer));
    frame.index = 0;
    frame.decodingTs = frame.presentationTs = GETTIME();
    frame.size = SIZEOF(mFrameBuffer);
    frame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreams[0], &frame));

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_LE(1, mCurlDescribeStreamCount);
    EXPECT_LE(1, mCurlTagResourceCount);
    EXPECT_LE(1, mCurlGetDataEndpointCount);
    EXPECT_LE(1, mCurlPutMediaCount);

    freeStreams();
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
