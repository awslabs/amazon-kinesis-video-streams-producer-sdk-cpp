#include "ClientTestFixture.h"

class StreamApiServiceCallsTest : public ClientTestBase {
};

TEST_F(StreamApiServiceCallsTest, putFrame_DescribeStreamCreating)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    // Move to ready state
    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_CREATING;
    mStreamDescription.creationTime = GETTIME();
    // Reset the stream name
    mStreamName[0] = '\0';
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    EXPECT_EQ(2, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    EXPECT_EQ(4, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    EXPECT_EQ(5, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    // Should fail after the retries
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_NE(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
}
