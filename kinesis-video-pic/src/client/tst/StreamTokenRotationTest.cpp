#include "ClientTestFixture.h"

class StreamTokenRotationTest : public ClientTestBase {
};

UINT64 gPresetTimeValue = 0;
UINT32 gPutStreamFuncCount = 0;
CHAR gStreamingToken[100];

#define UPDATE_TEST_STREAMING_TOKEN(x) (sprintf(gStreamingToken, "%s%d", "StreamingToken_", (x)))

UINT64 getCurrentTimePreset(UINT64 customData)
{
    UNUSED_PARAM(customData);
    DLOGV("TID 0x%016llx getCurrentTimePreset called.", GETTID());

    return gPresetTimeValue;
}

STATUS testPutStream(UINT64 customData,
                     PCHAR streamName,
                     PCHAR containerType,
                     UINT64 streamStartTime,
                     BOOL absoluteFragmentTimestamp,
                     BOOL ackRequired,
                     PCHAR streamingEndpoint,
                     PServiceCallContext pCallbackContext)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(containerType);
    UNUSED_PARAM(streamStartTime);
    UNUSED_PARAM(absoluteFragmentTimestamp);
    UNUSED_PARAM(ackRequired);
    UNUSED_PARAM(streamingEndpoint);
    DLOGV("TID 0x%016llx testPutStream called.", GETTID());

    gPutStreamFuncCount++;

    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pCallbackContext->version);
    if (pCallbackContext->pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pCallbackContext->version);
        if (pCallbackContext->pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(0, STRCMP(gStreamingToken, (PCHAR) pCallbackContext->pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

TEST_F(StreamTokenRotationTest, basicTokenRotationNonPersistAwait)
{
    UINT32 i, filledSize, rotation, lastRotation;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[5000];
    UINT64 timestamp, uploadHandle = TEST_UPLOAD_HANDLE, newUploadHandle = TEST_UPLOAD_HANDLE;
    Frame frame;
    STATUS status;
    UINT64 runDuration = 3 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + TEST_LONG_FRAME_DURATION;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    BOOL iterate = TRUE;

    // Reset the values for repeated playback of the tests
    gPresetTimeValue = 0;
    gPutStreamFuncCount = 0;

    // Set the specialized callback functions
    mClientCallbacks.getCurrentTimeFn = getCurrentTimePreset;
    mClientCallbacks.putStreamFn = testPutStream;

    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Set the frame buffer to a pattern
    MEMSET(tempBuffer, 0x55, SIZEOF(tempBuffer));

    // Create and ready a stream
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &streamHandle));

    // Move to ready state
    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    mStreamDescription.creationTime = GETTIME();

    lastRotation = 0;
    rotation = 1;

    UPDATE_TEST_STREAMING_TOKEN(rotation);

    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_INVALID_TOKEN_EXPIRATION, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) gStreamingToken, SIZEOF(gStreamingToken), MIN_STREAMING_TOKEN_EXPIRATION_DURATION - 1));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) gStreamingToken, SIZEOF(gStreamingToken), MIN_STREAMING_TOKEN_EXPIRATION_DURATION));

    // Produce and consume the buffer
    for (i = 0, timestamp = 0; timestamp <= runDuration; timestamp += TEST_LONG_FRAME_DURATION, i++) {

        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;
        frame.size = SIZEOF(tempBuffer);
        frame.trackId = TEST_TRACKID;
        frame.frameData = tempBuffer;

        // Update the timer
        gPresetTimeValue = timestamp;

        // Key frame every 5th
        frame.flags = i % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // Check for rotation
        if (timestamp > (UINT64)(rotation * MIN_STREAMING_TOKEN_EXPIRATION_DURATION - STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD + TEST_LONG_FRAME_DURATION)) {
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
            EXPECT_EQ(rotation, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
            EXPECT_EQ(rotation, gPutStreamFuncCount);

            EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData,
                                                                      SERVICE_CALL_RESULT_OK,
                                                                      TEST_STREAMING_ENDPOINT));
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
            EXPECT_EQ(rotation, gPutStreamFuncCount);
            EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                                   SERVICE_CALL_RESULT_OK,
                                                                   (PBYTE) gStreamingToken,
                                                                   SIZEOF(gStreamingToken),
                                                                   (rotation + 1) * MIN_STREAMING_TOKEN_EXPIRATION_DURATION));

            // Validate the put stream count
            EXPECT_EQ(rotation + 1, gPutStreamFuncCount);

            rotation++;
            UPDATE_TEST_STREAMING_TOKEN(rotation);
        }

        // Return a put stream result after some iterations
        if (lastRotation != rotation && (i % 5 == 0)) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, newUploadHandle++));
            lastRotation = rotation;
        }

        // consume after every 10th
        if (i != 0 && (i % 10 == 0)) {
            iterate = TRUE;
            while (iterate) {
                status = getKinesisVideoStreamData(streamHandle, uploadHandle, getDataBuffer, SIZEOF(getDataBuffer),
                                                   &filledSize);
                EXPECT_TRUE(status == STATUS_SUCCESS ||
                            status == STATUS_END_OF_STREAM ||
                            status == STATUS_NO_MORE_DATA_AVAILABLE);
                switch (status) {
                    case STATUS_SUCCESS:
                        break;
                    case STATUS_END_OF_STREAM:
                        uploadHandle++;
                        break;
                    case STATUS_NO_MORE_DATA_AVAILABLE:
                        iterate = FALSE;
                        break;
                    default:
                        break;
                }
            }
        }
    }


    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamErrorReportFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, mStatus);
    EXPECT_TRUE(uploadHandle > TEST_UPLOAD_HANDLE); //upload handle rotated at least once.

    if (IS_VALID_CLIENT_HANDLE(clientHandle)) {
        freeKinesisVideoClient(&clientHandle);
    }
}

TEST_F(StreamTokenRotationTest, rotationWithAwaitingCheck)
{
    UINT32 i, filledSize, rotation, lastRotation;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[5000];
    PBYTE emptyTagValue = (PBYTE) MEMALLOC(MKV_TAG_STRING_BITS_SIZE);
    UINT64 timestamp, uploadHandle = TEST_UPLOAD_HANDLE, newUploadHandle = TEST_UPLOAD_HANDLE;
    Frame frame;
    STATUS status;
    UINT64 runDuration = 3 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + TEST_LONG_FRAME_DURATION;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    BOOL sendAck = FALSE;
    FragmentAck fragmentAck;
    PUploadHandleInfo pUploadHandleInfo = NULL;
    BOOL iterate;

    // Reset the values for repeated playback of the tests
    gPresetTimeValue = 0;
    gPutStreamFuncCount = 0;

    mStreamInfo.streamCaps.recoverOnError = TRUE;

    // Set the ACK values
    fragmentAck.version = FRAGMENT_ACK_CURRENT_VERSION;
    fragmentAck.ackType = FRAGMENT_ACK_TYPE_PERSISTED;
    fragmentAck.result = SERVICE_CALL_RESULT_OK;
    STRCPY(fragmentAck.sequenceNumber, "SequenceNumber");
    fragmentAck.timestamp = 0;

    // Set the specialized callback functions
    mClientCallbacks.getCurrentTimeFn = getCurrentTimePreset;
    mClientCallbacks.putStreamFn = testPutStream;

    // Set the retention period to enforce the awaiting for last persist ACK
    mStreamInfo.retention = 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR;

    // Set to absolute timestamps to ensure deterministic ACKs
    mStreamInfo.streamCaps.absoluteFragmentTimes = TRUE;

    // Set the frame buffer to a pattern
    MEMSET(tempBuffer, 0x55, SIZEOF(tempBuffer));

    // Copy over empty MkvTagStringBits and fix up ebml size for empty tag string
    MEMCPY(emptyTagValue, gMkvTagStringBits, gMkvTagStringBitsSize);
    emptyTagValue[2] = 0x01;

    // Create and ready a stream
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &streamHandle));

    // Move to ready state
    mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
    STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
    STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
    STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
    STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
    mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    mStreamDescription.creationTime = GETTIME();

    lastRotation = 0;
    rotation = 1;

    UPDATE_TEST_STREAMING_TOKEN(rotation);

    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_INVALID_TOKEN_EXPIRATION, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) gStreamingToken, SIZEOF(gStreamingToken), MIN_STREAMING_TOKEN_EXPIRATION_DURATION - 1));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) gStreamingToken, SIZEOF(gStreamingToken), MIN_STREAMING_TOKEN_EXPIRATION_DURATION));

    // Produce and consume the buffer
    for (i = 0, timestamp = 0; timestamp <= runDuration; timestamp += TEST_LONG_FRAME_DURATION, i++) {

        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.duration = TEST_LONG_FRAME_DURATION;
        frame.size = SIZEOF(tempBuffer);
        frame.frameData = tempBuffer;
        frame.trackId = TEST_TRACKID;

        // Update the timer
        gPresetTimeValue = timestamp;

        // Key frame every 5th
        frame.flags = i % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // Check for rotation
        if ((INT64) timestamp > rotation * MIN_STREAMING_TOKEN_EXPIRATION_DURATION - STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD + TEST_LONG_FRAME_DURATION) {
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
            EXPECT_EQ(rotation, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
            EXPECT_EQ(rotation, gPutStreamFuncCount);

            EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData,
                                                                      SERVICE_CALL_RESULT_OK,
                                                                      TEST_STREAMING_ENDPOINT));
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
            EXPECT_EQ(rotation + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
            EXPECT_EQ(rotation, gPutStreamFuncCount);
            EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                                   SERVICE_CALL_RESULT_OK,
                                                                   (PBYTE) gStreamingToken,
                                                                   SIZEOF(gStreamingToken),
                                                                   (rotation + 1) * MIN_STREAMING_TOKEN_EXPIRATION_DURATION));

            // Validate the put stream count
            EXPECT_EQ(rotation + 1, gPutStreamFuncCount);

            rotation++;
            UPDATE_TEST_STREAMING_TOKEN(rotation);
        }

        // Return a put stream result after some iterations
        if (lastRotation != rotation && (i % 5 == 0)) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, newUploadHandle++));

            lastRotation = rotation;
        }

        // consume after every 10th
        if (i != 0 && (i % 10 == 0)) {
            iterate = TRUE;
            while (iterate) {
                status = getKinesisVideoStreamData(streamHandle, uploadHandle, getDataBuffer, SIZEOF(getDataBuffer),
                                                   &filledSize);
                EXPECT_TRUE(status == STATUS_SUCCESS ||
                            status == STATUS_END_OF_STREAM ||
                            status == STATUS_NO_MORE_DATA_AVAILABLE ||
                            status == STATUS_AWAITING_PERSISTED_ACK);
                switch (status) {
                    case STATUS_SUCCESS:
                        break;
                    case STATUS_END_OF_STREAM:
                        uploadHandle++;
                        break;
                    case STATUS_NO_MORE_DATA_AVAILABLE:
                        iterate = FALSE;
                        break;
                    case STATUS_AWAITING_PERSISTED_ACK:
                        // The last bits should be EOS tag.
                        // We can check the EOS tag by checking the value part of the
                        // tag which should be empty
                        EXPECT_EQ(0, MEMCMP(emptyTagValue, getDataBuffer + filledSize - gMkvTagStringBitsSize,
                                            gMkvTagStringBitsSize));

                        // Send the ACK
                        pUploadHandleInfo = getStreamUploadInfo(FROM_STREAM_HANDLE(streamHandle), uploadHandle);
                        fragmentAck.timestamp = pUploadHandleInfo->lastFragmentTs / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
                        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFragmentAck(streamHandle, uploadHandle, &fragmentAck));
                        break;
                    default:
                        break;
                }
            }
        }
    }

    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamErrorReportFuncCount));
    EXPECT_TRUE(uploadHandle > TEST_UPLOAD_HANDLE); //upload handle rotated at least once.

    if (IS_VALID_CLIENT_HANDLE(clientHandle)) {
        freeKinesisVideoClient(&clientHandle);
    }
    MEMFREE(emptyTagValue);
}
