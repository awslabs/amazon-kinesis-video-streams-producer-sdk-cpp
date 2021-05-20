#include "ClientTestFixture.h"

class StreamStateTransitionsTest : public ClientTestBase {
};

TEST_F(StreamStateTransitionsTest, stopStateFromCreate)
{
    CreateStream();
    // Ensure the describe called
    EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));
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

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));
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

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));
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

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingSuccessRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));

    // Validate that we have re-started at the get endpoint
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingSuccessNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_OK));

    // Validate that we have re-started at the get endpoint as we terminated with success
    EXPECT_LE(2, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // we are emulating creating state
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingLimitRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_STREAM_LIMIT));

    // Validate that we have re-started at the get endpoint
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_DEVICE_LIMIT));

    // Validate that we have re-started at the get endpoint
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingLimitNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SERVICE_CALL_STREAM_LIMIT_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_STREAM_LIMIT));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SERVICE_CALL_DEVICE_LIMIT_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_DEVICE_LIMIT));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingNotAuthorizedRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_NOT_AUTHORIZED));

    // Validate that we have re-started at the get token
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_FORBIDDEN));

    // Validate that we have re-started at the get token
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingNotAuthorizedNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_NOT_AUTHORIZED));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_FORBIDDEN));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingResourceInUseRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_IN_USE));

    // Validate that we have re-started at the describe
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // Due to test emulating async resolution
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    // Unknown error - catch all case
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_UNKNOWN));

    // Validate that we have re-started at the describe
    EXPECT_EQ(4, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingResourceInUseNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SERVICE_CALL_RESOURCE_IN_USE_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_IN_USE));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SERVICE_CALL_UNKOWN_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_UNKNOWN));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingInternalRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR));

    // Validate that we have re-started at the get token
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // Due to test emulating async resolution
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingInternalNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_ACK_ERR_ACK_INTERNAL_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingResourceNotFoundRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_NOT_FOUND));

    // Validate that we have re-started at the describe
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // Due to test emulating async resolution
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_DELETED));

    // Validate that we have re-started at the describe
    EXPECT_EQ(4, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingResourceNotFoundNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_NOT_FOUND));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));

    EXPECT_EQ(STATUS_SERVICE_CALL_RESOURCE_DELETED_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_RESOURCE_DELETED));

    // Validate that we have not restarted
    EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingOtherRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // Recover on error
    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_DEVICE_NOT_FOUND));

    // Validate that we have re-started at the describe
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // Due to test emulating async resolution
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));
}

TEST_F(StreamStateTransitionsTest, stopStateFromStreamingOtherNoRecovery)
{
    UINT32 i, remaining;
    BYTE frameData[1000];
    UINT32 frameSize = SIZEOF(frameData);
    UINT64 timestamp;
    Frame frame;
    frame.frameData = frameData;
    frame.size = frameSize;
    frame.trackId = TEST_TRACKID;

    // No Recovery on error
    mStreamInfo.streamCaps.recoverOnError = FALSE;

    // Create and ready a stream
    ReadyStream();

    // Produce a few frames
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;

        // Change the content of the buffer
        *(PUINT32) frameData = i;

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mAckRequired));
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Ensure the pressure notification is called when we are below STORAGE_PRESSURE_NOTIFICATION_THRESHOLD
        remaining = TEST_DEVICE_STORAGE_SIZE - (frameSize * i);
        if ((UINT32) ((DOUBLE) remaining / TEST_DEVICE_STORAGE_SIZE * 100) <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            EXPECT_NE(0, mRemainingSize);
        }

        // Return a put stream result on 5th
        if (i == 5) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_UPLOAD_HANDLE));
        }
    }

    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;

    EXPECT_EQ(STATUS_SERVICE_CALL_DEVICE_NOT_FOND_ERROR, kinesisVideoStreamTerminated(mCallContext.customData, TEST_UPLOAD_HANDLE, SERVICE_CALL_DEVICE_NOT_FOUND));
}
