#include "ClientTestFixture.h"

class StreamStateTransitionsTest : public ClientTestBase {
};

TEST_F(StreamStateTransitionsTest, stopStateFromCreate)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, SERVICE_CALL_RESULT_OK));
}

TEST_F(StreamStateTransitionsTest, stopStateFromDescribe)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    mStreamDescription.creationTime = GETTIME();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, SERVICE_CALL_RESULT_OK));
}

TEST_F(StreamStateTransitionsTest, stopStateFromGetEndpoint)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    mStreamDescription.creationTime = GETTIME();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, SERVICE_CALL_RESULT_OK));
}

TEST_F(StreamStateTransitionsTest, stopStateFromGetToken)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    mStreamDescription.creationTime = GETTIME();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN, SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, SERVICE_CALL_RESULT_OK));
}