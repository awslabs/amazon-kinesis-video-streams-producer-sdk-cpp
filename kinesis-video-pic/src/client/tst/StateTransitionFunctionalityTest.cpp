#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class StateTransitionFunctionalityTest : public ClientTestBase,
                                         public WithParamInterface< ::std::tuple<STREAMING_TYPE, uint64_t, bool, uint64_t> >{
protected:
    void SetUp() {
        ClientTestBase::SetUp();

        STREAMING_TYPE streamingType;
        bool enableAck;
        uint64_t retention, replayDuration;
        std::tie(streamingType, retention, enableAck, replayDuration) = GetParam();
        mStreamInfo.retention = (UINT64) retention;
        mStreamInfo.streamCaps.streamingType = streamingType;
        mStreamInfo.streamCaps.fragmentAcks = enableAck;
        mStreamInfo.streamCaps.replayDuration = (UINT64) replayDuration;
    }
};

//Create stream, fault inject retriable error the first call for Describe, Create, Tag, Get Endpoint, Get Token, ensure retry
TEST_P(StateTransitionFunctionalityTest, ControlPlaneServiceCallRetryOnRetriableError)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    Tag tags[1];
    PCHAR tagName = (PCHAR) "foo";
    PCHAR tagValue = (PCHAR) "vfoo";
    tags[0].version = TAG_CURRENT_VERSION;
    tags[0].name = tagName;
    tags[0].value = tagValue;
    mStreamInfo.tagCount = 1;
    mStreamInfo.tags = tags;

    CreateStream();

    setupStreamDescription();
    // submit retriable error
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, &mStreamDescription));
    EXPECT_EQ(2, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // check retry happened
    // submit not found result to move to create state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));

    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAM_ARN));
    EXPECT_EQ(2, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));

    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN));
    EXPECT_EQ(2, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));

    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_UNKNOWN,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           TEST_AUTH_EXPIRATION));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_RESULT_OK,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           TEST_AUTH_EXPIRATION));
}

//Create stream, fault inject non-retriable error the first call for Describe, Create, Tag, Get Endpoint, Get Token, ensure error returned
TEST_P(StateTransitionFunctionalityTest, ControlPlaneServiceCallReturnNonRetriableError)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    Tag tags[1];
    PCHAR tagName = (PCHAR) "foo";
    PCHAR tagValue = (PCHAR) "vfoo";
    tags[0].version = TAG_CURRENT_VERSION;
    tags[0].name = tagName;
    tags[0].value = tagValue;
    mStreamInfo.tagCount = 1;
    mStreamInfo.tags = tags;

    CreateStream();

    setupStreamDescription();
    // submit non-retriable error
    EXPECT_EQ(STATUS_SERVICE_CALL_INVALID_ARG_ERROR, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_INVALID_ARG, &mStreamDescription));
    // submit not found result to move to create state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));

    EXPECT_EQ(STATUS_SERVICE_CALL_INVALID_ARG_ERROR, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_INVALID_ARG, TEST_STREAM_ARN));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));

    EXPECT_EQ(STATUS_SERVICE_CALL_INVALID_ARG_ERROR, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_INVALID_ARG));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));

    EXPECT_EQ(STATUS_SERVICE_CALL_INVALID_ARG_ERROR, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_INVALID_ARG, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

    EXPECT_EQ(STATUS_SERVICE_CALL_INVALID_ARG_ERROR, getStreamingTokenResultEvent(mCallContext.customData,
                                                                                  SERVICE_CALL_INVALID_ARG,
                                                                                  (PBYTE) TEST_STREAMING_TOKEN,
                                                                                  SIZEOF(TEST_STREAMING_TOKEN),
                                                                                  TEST_AUTH_EXPIRATION));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_RESULT_OK,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           TEST_AUTH_EXPIRATION));
}

TEST_P(StateTransitionFunctionalityTest, ControlPlaneServiceCallReturnNotAuthorizedError)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    Tag tags[1];
    PCHAR tagName = (PCHAR) "foo";
    PCHAR tagValue = (PCHAR) "vfoo";
    tags[0].version = TAG_CURRENT_VERSION;
    tags[0].name = tagName;
    tags[0].value = tagValue;
    mStreamInfo.tagCount = 1;
    mStreamInfo.tags = tags;

    CreateStream();

    setupStreamDescription();
    // submit auth error which should be retriable
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_NOT_AUTHORIZED, &mStreamDescription));
    EXPECT_EQ(2, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // check retry happened
    // submit not found result to move to create state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));

    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_NOT_AUTHORIZED, TEST_STREAM_ARN));
    EXPECT_EQ(2, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));

    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_NOT_AUTHORIZED));
    EXPECT_EQ(2, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));

    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_NOT_AUTHORIZED, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

    EXPECT_EQ(STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_NOT_AUTHORIZED,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           TEST_AUTH_EXPIRATION));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_RESULT_OK,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           TEST_AUTH_EXPIRATION));
}

// Create stream, fault inject all calls for Describe, Create, Tag, Get Endpoint, Get Token, ensure fails at the end
TEST_P(StateTransitionFunctionalityTest, ControlPlaneServiceCallExhaustRetry)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    Tag tags[1];
    PCHAR tagName = (PCHAR) "foo";
    PCHAR tagValue = (PCHAR) "vfoo";
    tags[0].version = TAG_CURRENT_VERSION;
    tags[0].name = tagName;
    tags[0].value = tagValue;
    mStreamInfo.tagCount = 1;
    mStreamInfo.tags = tags;
    UINT32 i;

    setupStreamDescription();

    // exhaust retry for describeStreamResultEvent
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));
    for(i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(i + 1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, &mStreamDescription));
    }
    EXPECT_EQ(STATUS_DESCRIBE_STREAM_CALL_FAILED, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, &mStreamDescription));
    freeKinesisVideoStream(&mStreamHandle);

    // exhaust retry for createStreamResultEvent
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));
    // move to create state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));
    for(i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(i + 1, ATOMIC_LOAD(&mCreateStreamFuncCount));
        EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAM_ARN));
    }
    EXPECT_EQ(STATUS_CREATE_STREAM_CALL_FAILED, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAM_ARN));
    freeKinesisVideoStream(&mStreamHandle);

    // exhaust retry for tagResourceResultEvent
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));
    // move to tag state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));

    // tagStream is special because it is not critical to streaming. After retries are exhausted it will move to the next
    // state and return success.
    for(i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(i + 1, ATOMIC_LOAD(&mTagResourceFuncCount));
        EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN));
    }
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount)); // moved to getStreamingEndPointState
    ATOMIC_STORE(&mGetStreamingEndpointFuncCount, 0);
    freeKinesisVideoStream(&mStreamHandle);

    // exhaust retry for getStreamingEndpointResultEvent
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));
    // move to get endpoint state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));

    for(i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(i + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAMING_ENDPOINT));
    }
    EXPECT_EQ(STATUS_GET_STREAMING_ENDPOINT_CALL_FAILED, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_UNKNOWN, TEST_STREAMING_ENDPOINT));
    freeKinesisVideoStream(&mStreamHandle);

    // exhaust retry for getStreamingTokenResultEvent
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));
    // move to get token state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, &mStreamDescription));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

    for(i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(i + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
        EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                               SERVICE_CALL_UNKNOWN,
                                                               (PBYTE) TEST_STREAMING_TOKEN,
                                                               SIZEOF(TEST_STREAMING_TOKEN),
                                                               TEST_AUTH_EXPIRATION));
    }
    EXPECT_EQ(STATUS_GET_STREAMING_TOKEN_CALL_FAILED, getStreamingTokenResultEvent(mCallContext.customData,
                                                                                   SERVICE_CALL_UNKNOWN,
                                                                                   (PBYTE) TEST_STREAMING_TOKEN,
                                                                                   SIZEOF(TEST_STREAMING_TOKEN),
                                                                                   TEST_AUTH_EXPIRATION));
    freeKinesisVideoStream(&mStreamHandle);
}

// check that kinesisVideoStreamTerminated with certain service call results should move to getEndpoint state
TEST_P(StateTransitionFunctionalityTest, StreamTerminatedAndGoToGetEndpointState)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UINT32 oldGetStreamingEndpointFuncCount, oldDescribeFuncCount, oldGetTokenFuncCount;
    initDefaultProducer();
    std::vector<SERVICE_CALL_RESULT> getEndpointStateResults = {SERVICE_CALL_RESULT_OK,
                                                                SERVICE_CALL_DEVICE_LIMIT,
                                                                SERVICE_CALL_STREAM_LIMIT,
                                                                SERVICE_CALL_BAD_REQUEST,
                                                                SERVICE_CALL_REQUEST_TIMEOUT,
                                                                SERVICE_CALL_GATEWAY_TIMEOUT,
                                                                SERVICE_CALL_NETWORK_READ_TIMEOUT,
                                                                SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT,
                                                                SERVICE_CALL_NOT_AUTHORIZED,
                                                                SERVICE_CALL_FORBIDDEN};

    mStreamInfo.streamCaps.recoverOnError = TRUE;

    for (int i = 0; i < getEndpointStateResults.size(); i++) {
        SERVICE_CALL_RESULT service_call_result = getEndpointStateResults[i];
        CreateStreamSync();
        MockProducer producer(mMockProducerConfig, mStreamHandle);

        // move to putStreamState
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size());
        oldGetStreamingEndpointFuncCount = ATOMIC_LOAD(&mGetStreamingEndpointFuncCount);
        oldGetTokenFuncCount = ATOMIC_LOAD(&mGetStreamingTokenFuncCount);
        oldDescribeFuncCount = ATOMIC_LOAD(&mDescribeStreamFuncCount);

        // Submit a terminate event
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle, currentUploadHandles[0], service_call_result));

        STATUS retStatus = serviceCallResultCheck(service_call_result);

        switch (retStatus) {
            case STATUS_SUCCESS:
                // Should start from get endpoint
                EXPECT_EQ(oldGetStreamingEndpointFuncCount + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
                EXPECT_EQ(oldGetTokenFuncCount + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
                EXPECT_EQ(oldDescribeFuncCount, ATOMIC_LOAD(&mDescribeStreamFuncCount));

                break;

            case STATUS_SERVICE_CALL_TIMEOUT_ERROR:
                // Should start from ready
                EXPECT_EQ(oldGetStreamingEndpointFuncCount, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
                EXPECT_EQ(oldGetTokenFuncCount, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
                EXPECT_EQ(oldDescribeFuncCount, ATOMIC_LOAD(&mDescribeStreamFuncCount));

                break;

            case STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR:
                // Should start from token
                EXPECT_EQ(oldGetStreamingEndpointFuncCount, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
                EXPECT_EQ(oldGetTokenFuncCount + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
                EXPECT_EQ(oldDescribeFuncCount, ATOMIC_LOAD(&mDescribeStreamFuncCount));

                break;

            case STATUS_SERVICE_CALL_DEVICE_LIMIT_ERROR:
            case STATUS_SERVICE_CALL_STREAM_LIMIT_ERROR:
                // Should start from get endpoint
                EXPECT_EQ(oldGetStreamingEndpointFuncCount + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
                EXPECT_EQ(oldGetTokenFuncCount + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
                EXPECT_EQ(oldDescribeFuncCount, ATOMIC_LOAD(&mDescribeStreamFuncCount));

                break;

            default:
                // Should start from describe
                EXPECT_EQ(oldGetStreamingEndpointFuncCount + 1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
                EXPECT_EQ(oldGetTokenFuncCount + 1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
                EXPECT_EQ(oldDescribeFuncCount + 1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
        }

        mStreamingSession.clearSessions(); // remove current session for next iteration.
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    }
}

// check that kinesisVideoStreamTerminated with certain service call results should move to describeStream state
TEST_P(StateTransitionFunctionalityTest, StreamTerminatedAndGoToDescribeState)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UINT32 oldDescribeStreamFuncCount;
    initDefaultProducer();
    std::vector<SERVICE_CALL_RESULT> describeStreamStateResults = {SERVICE_CALL_RESOURCE_IN_USE,
                                                                   SERVICE_CALL_INTERNAL_ERROR,
                                                                   SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR,
                                                                   SERVICE_CALL_RESOURCE_NOT_FOUND,
                                                                   SERVICE_CALL_RESOURCE_DELETED};

    mStreamInfo.streamCaps.recoverOnError = TRUE;

    for (int i = 0; i < describeStreamStateResults.size(); i++) {
            SERVICE_CALL_RESULT service_call_result = describeStreamStateResults[i];
        CreateStreamSync();
        MockProducer producer(mMockProducerConfig, mStreamHandle);

        // move to putStreamState
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size());
        oldDescribeStreamFuncCount = ATOMIC_LOAD(&mDescribeStreamFuncCount);
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle, currentUploadHandles[0], service_call_result));
        // describeStream has been called once, that means stream state moved to describeStream
        EXPECT_EQ(ATOMIC_LOAD(&mDescribeStreamFuncCount), oldDescribeStreamFuncCount + 1);
        mStreamingSession.clearSessions(); // remove current session for next iteration.
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    }
}

// check that kinesisVideoStreamTerminated with certain service call results should move to getStreamingToken state
TEST_P(StateTransitionFunctionalityTest, StreamTerminatedAndGoToGetTokenState)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UINT32 oldGetStreamingTokenFuncCount;
    initDefaultProducer();
    std::vector<SERVICE_CALL_RESULT> getStreamingTokenStateResults = {SERVICE_CALL_NOT_AUTHORIZED,
                                                                      SERVICE_CALL_FORBIDDEN};

    mStreamInfo.streamCaps.recoverOnError = TRUE;

    for (int i = 0; i < getStreamingTokenStateResults.size(); i++) {
        SERVICE_CALL_RESULT service_call_result = getStreamingTokenStateResults[i];
        CreateStreamSync();
        MockProducer producer(mMockProducerConfig, mStreamHandle);

        // move to putStreamState
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size());
        oldGetStreamingTokenFuncCount = ATOMIC_LOAD(&mGetStreamingTokenFuncCount);
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle, currentUploadHandles[0], service_call_result));
        // getStreamingToken has been called once, that means stream state moved to getStreamingToken
        EXPECT_EQ(ATOMIC_LOAD(&mGetStreamingTokenFuncCount), oldGetStreamingTokenFuncCount + 1);
        mStreamingSession.clearSessions(); // remove current session for next iteration.
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    }
}

// check that kinesisVideoStreamTerminated with certain service call results should move to new state
TEST_P(StateTransitionFunctionalityTest, StreamTerminatedAndGoToNewState)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    mStreamInfo.streamCaps.recoverOnError = TRUE; // otherwise kinesisVideoStreamTerminated simply returns error status
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UINT32 oldDescribeStreamFuncCount;
    initDefaultProducer();
    std::vector<SERVICE_CALL_RESULT> newStateResults = {SERVICE_CALL_UNKNOWN,
                                                        SERVICE_CALL_INVALID_ARG,
                                                        SERVICE_CALL_DEVICE_NOT_FOUND,
                                                        SERVICE_CALL_INTERNAL_ERROR};

    for (int i = 0; i < newStateResults.size(); i++) {
        SERVICE_CALL_RESULT service_call_result = newStateResults[i];
        CreateStreamSync();
        MockProducer producer(mMockProducerConfig, mStreamHandle);

        // move to putStreamState
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size());
        oldDescribeStreamFuncCount = ATOMIC_LOAD(&mDescribeStreamFuncCount);
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle, currentUploadHandles[0], service_call_result));
        // after stream state is moved to new, it automatically move to describeStream state
        EXPECT_EQ(ATOMIC_LOAD(&mDescribeStreamFuncCount), oldDescribeStreamFuncCount + 1);
        mStreamingSession.clearSessions(); // remove current session for next iteration.
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    }
}

// Create stream, stream and leave large buffer, StopSync, inject a fault and terminate the upload handle
// check the behavior for re-streaming with no putFrame.
TEST_P(StateTransitionFunctionalityTest, FaultInjectUploadHandleAfterStopBeforeTokenRotation) {
    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    BOOL submittedErrorAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    mStreamInfo.streamCaps.recoverOnError = TRUE;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // putFrame for 10 seconds, should finish putting at least 1 fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

    // stop while having large buffer not sent
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));

    //give 10 seconds to getStreamData and submit error ack on the first fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size()); // test should finish before token rotation

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, &submittedErrorAck));
            }
        }
    } while (currentTime < stopTime && !submittedErrorAck);

    EXPECT_EQ(TRUE, submittedErrorAck);

    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(2, currentUploadHandles.size()); // a new handle should be created

    VerifyStopStreamSyncAndFree();
}

TEST_P(StateTransitionFunctionalityTest, FaultInjectUploadHandleAfterStopDuringTokenRotation) {
    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    BOOL submittedErrorAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    mStreamInfo.streamCaps.recoverOnError = TRUE;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // keep running putFrame until token rotation
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

    // stop while having large buffer not sent
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));

    //give 10 seconds to getStreamData and submit error ack on upload handle 0 (first upload handle)
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL && uploadHandle == 0 && !submittedErrorAck){
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, &submittedErrorAck));
            }
        }
    } while (currentTime < stopTime && !submittedErrorAck);

    // should've submitted error ack to upload handle 0
    EXPECT_EQ(TRUE, submittedErrorAck);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StateTransitionFunctionalityTest, basicResetConnectionTest) {
    CreateScenarioTestClient();
    BOOL didPutFrame = FALSE, gotStreamData = FALSE, submittedAck = FALSE;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime, resetConnectionTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 30 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    resetConnectionTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 15 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
            }
        }

        if (IS_VALID_TIMESTAMP(resetConnectionTime) && currentTime > resetConnectionTime) {
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetConnection(mStreamHandle));
            resetConnectionTime = INVALID_TIMESTAMP_VALUE;
        }

    } while (currentTime < stopTime);

    VerifyStopStreamSyncAndFree();
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StateTransitionFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
