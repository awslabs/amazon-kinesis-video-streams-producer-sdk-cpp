#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class StreamStoppingFunctionalityTest : public ClientTestBase,
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

TEST_P(StreamStoppingFunctionalityTest, CreateSyncStreamWithTwoUploadHandlesStopSyncFreeSuccess) {

    UINT64 currentTime, testTerminationTime;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
            + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 15 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            //GetStreamedData and SubmitAck
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
        }

    } while (currentTime < testTerminationTime);

    EXPECT_EQ(tokenRotateCount, 2);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamStoppingFunctionalityTest, CreateSyncResetConnectionSuccess)
{
    PStateMachineState pState;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    // stream state should be STREAM_STATE_READY at this point.
    PKinesisVideoStream pKinesisVideoStream = fromStreamHandle(mStreamHandle);

    // kinesisVideoStreamTerminated should succeed but have no effect on stream state because stream state is STREAM_STATE_READY
    kinesisVideoStreamTerminated(mStreamHandle, INVALID_UPLOAD_HANDLE_VALUE, SERVICE_CALL_RESULT_OK);

    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(pKinesisVideoStream->base.pStateMachine, &pState));
    EXPECT_TRUE(pState->state == STREAM_STATE_READY);
}

TEST_P(StreamStoppingFunctionalityTest, CreateSyncStreamHighDensityStopSyncTimeoutFreeSuccess) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, testTerminationTime;

    mDeviceInfo.clientInfo.stopStreamTimeout = 3 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Set slow consumer
    mMockConsumerConfig.mUploadSpeedBytesPerSecond = 100;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 35 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    } while (currentTime < testTerminationTime);

    VerifyStopStreamSyncAndFree(TRUE);
}

TEST_P(StreamStoppingFunctionalityTest, CreateSyncStreamHighDensityStopSyncStreamOutRest)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime;
    StreamMetrics streamMetrics;
    streamMetrics.version = STREAM_METRICS_CURRENT_VERSION;

    // 120s stop stream timeout
    mDeviceInfo.clientInfo.stopStreamTimeout = 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // 10 mbps producer
    mMockProducerConfig.mFrameSizeByte = 10 * 1024 * 1024 / mMockProducerConfig.mFps;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // stream for 9 seconds. Since producer is 10mbps and consumer is 1mbps, at stream stop there should be
    // around 80mb of data left to stream out.
    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 9 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    } while (currentTime < streamStopTime);

    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoStreamMetrics(mStreamHandle, &streamMetrics));
    DLOGD("stream metric at stream stop: currentViewSize %llu byte, overallViewSize %llu byte", streamMetrics.currentViewSize, streamMetrics.overallViewSize);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamStoppingFunctionalityTest, CreateSyncStreamStopSyncFree)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 30 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    } while (currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamStoppingFunctionalityTest, CreateSyncStreamStopSyncErrorAckWhileStreamingRemainingBits)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck, submittedErrorAck = FALSE;
    UINT64 currentTime, streamStopTime;

    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION +
            5 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < streamStopTime);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));

    // submit internal error ack to the first handle after stop
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL && !submittedErrorAck) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR, &submittedErrorAck));
            }
        }
    } while (!submittedErrorAck);

    // remaining buffer should be streamed out successfully and stream closed callback called.
    consumeStream((STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS) * HUNDREDS_OF_NANOS_IN_A_SECOND);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    EXPECT_EQ(0, ATOMIC_LOAD(&mDroppedFrameReportFuncCount));
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamStoppingFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
