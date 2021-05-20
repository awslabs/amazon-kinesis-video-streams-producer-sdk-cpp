#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class StreamFunctionalityTest : public ClientTestBase,
                                public WithParamInterface< ::std::tuple<STREAMING_TYPE, uint64_t, bool, uint64_t> >
{
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

TEST_P(StreamFunctionalityTest, CreateFreeSuccess)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStream();
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateAwaitReadyStopFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    ReadyStream();
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateSyncFreeSuccess)
{
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}


TEST_P(StreamFunctionalityTest, CreateSyncStopSyncFreePutFrameFail) {
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    stopKinesisVideoStream(mStreamHandle);
    freeKinesisVideoStream(&mStreamHandle);

    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    EXPECT_NE(STATUS_SUCCESS, mockProducer.putFrame());
}

TEST_P(StreamFunctionalityTest, CreateAwaitReadyPutFrameFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    ReadyStream();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Put a single frame
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateSyncPutFrameFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Put a single frame
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateSyncPutMultipleFramesFree)
{
    PStateMachineState pState;
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Disable auto-stepping in the fixtures
    mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;

    // Put a couple of frames
    for (UINT32 i = 0; i < 50; i++) {
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    }

    // We shouldn't have sent the PutStreamResult yet
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(pKinesisVideoStream->base.pStateMachine, &pState));
    EXPECT_EQ(STREAM_STATE_PUT_STREAM, pState->state);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateSyncStopFreeSuccess) {
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateSyncPutFrameStopSyncFreeSuccess) {
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    VerifyStopStreamSyncAndFree();
}

// stopStreamSync should timeout since putStreamResult is never submitted.
TEST_P(StreamFunctionalityTest, CreateAwaitPutFrameStopSyncFreeSuccess) {
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    // the test is meant to timeout, use a shorter timeout to reduce test running time.
    mDeviceInfo.clientInfo.stopStreamTimeout = TEST_STOP_STREAM_TIMEOUT_SHORT;
    CreateScenarioTestClient();
    ReadyStream();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    VerifyStopStreamSyncAndFree(TRUE);
}


TEST_P(StreamFunctionalityTest, CreateAwaitReadyFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    ReadyStream();
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamFunctionalityTest, CreateAwaitReadyStopSyncFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    ReadyStream();
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamClosedFuncCount));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

// Create (Sync/Await), Put Frame, Stop, Free
TEST_P(StreamFunctionalityTest, CreateSyncPutFrameStopFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Put a single frame
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

// CreateSync, putFrame, EOFR
TEST_P(StreamFunctionalityTest, CreateSyncPutFrameEoFR)
{
    CreateScenarioTestClient();
    BOOL submittedAck = FALSE, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Put a single frame
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    // Put a eofr frame
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame(TRUE));

    //give 5 seconds to get the single frame fragment
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
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }
    } while ((currentTime < stopTime) && !submittedAck);

    // should have submitted ack because a complete fragment has been consumed.
    if (mStreamInfo.streamCaps.fragmentAcks) {
        EXPECT_EQ(TRUE, submittedAck);
    }
}

TEST_P(StreamFunctionalityTest, CreateSyncPutFrameEoFRFirst)
{
    CreateScenarioTestClient();
    std::vector<UPLOAD_HANDLE> currentUploadHandles;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Put a eofr frame, this should fail we
    // should not allow first frame to be eofr
    EXPECT_NE(STATUS_SUCCESS, producer.putFrame(TRUE));

    // Produce some frames
    for (UINT32 i = 0; i < 2 * TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE; i++) {
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    }

    // Produce a few EoFRs
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame(TRUE));
    EXPECT_EQ(STATUS_MULTIPLE_CONSECUTIVE_EOFR, producer.putFrame(TRUE));
    EXPECT_EQ(STATUS_MULTIPLE_CONSECUTIVE_EOFR, producer.putFrame(TRUE));

    // Produce some more frames
    for (UINT32 i = 0; i < 2 * TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE; i++) {
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    }

    // Ensure the skipped frame count is zero -- the first eofr is NOT skipped
    // even if skipNonKeyFrames is true
    StreamMetrics streamMetrics;
    streamMetrics.version = STREAM_METRICS_CURRENT_VERSION;
    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoStreamMetrics(mStreamHandle, &streamMetrics));
    EXPECT_EQ(0, streamMetrics.skippedFrames);
}

TEST_P(StreamFunctionalityTest, CreateSyncPutFrameEoFRFirstForceNotSkipping)
{
    CreateScenarioTestClient();
    std::vector<UPLOAD_HANDLE> currentUploadHandles;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    initDefaultProducer();
    MockProducer producer(mMockProducerConfig, mStreamHandle);

    // Minor brain surgery - cast to internal struct and modify skip variable directly to force the EoFR
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    pKinesisVideoStream->skipNonKeyFrames = FALSE;

    // Put a eofr frame - ensure fails
    EXPECT_NE(STATUS_SUCCESS, producer.putFrame(TRUE));

    // Produce some frames
    for (UINT32 i = 0; i < 2 * TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE; i++) {
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    }

    // Produce a few EoFRs
    EXPECT_EQ(STATUS_SUCCESS, producer.putFrame(TRUE));
    EXPECT_EQ(STATUS_MULTIPLE_CONSECUTIVE_EOFR, producer.putFrame(TRUE));
    EXPECT_EQ(STATUS_MULTIPLE_CONSECUTIVE_EOFR, producer.putFrame(TRUE));

    // Produce some more frames
    for (UINT32 i = 0; i < 2 * TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE; i++) {
        EXPECT_EQ(STATUS_SUCCESS, producer.putFrame());
    }

    // Ensure the skipped frame count is zero
    StreamMetrics streamMetrics;
    streamMetrics.version = STREAM_METRICS_CURRENT_VERSION;
    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoStreamMetrics(mStreamHandle, &streamMetrics));
    EXPECT_EQ(0, streamMetrics.skippedFrames);
    // There should be no mem leaks
}

// Validate StreamDescription_V0 structure handling
TEST_P(StreamFunctionalityTest, StreamDescription_V0_Test)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create a stream
    CreateStream();

    // Ensure the describe called
    EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));

    StreamDescription_V0 streamDescription_v0;
    streamDescription_v0.version = 0;
    STRNCPY(streamDescription_v0.deviceName, TEST_DEVICE_NAME, MAX_DEVICE_NAME_LEN);
    STRNCPY(streamDescription_v0.streamName, TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    STRNCPY(streamDescription_v0.contentType, TEST_CONTENT_TYPE, MAX_CONTENT_TYPE_LEN);
    STRNCPY(streamDescription_v0.streamArn, TEST_STREAM_ARN, MAX_ARN_LEN);
    STRNCPY(streamDescription_v0.updateVersion, TEST_UPDATE_VERSION, MAX_UPDATE_VERSION_LEN);
    streamDescription_v0.streamStatus = STREAM_STATUS_ACTIVE;
    // creation time should be sometime in the past.
    streamDescription_v0.creationTime = getCurrentTimeFunc((UINT64) this) - 6 * HUNDREDS_OF_NANOS_IN_AN_HOUR;

    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PStreamDescription) &streamDescription_v0));

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);

    EXPECT_EQ(0, pKinesisVideoStream->retention);

    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN, SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
