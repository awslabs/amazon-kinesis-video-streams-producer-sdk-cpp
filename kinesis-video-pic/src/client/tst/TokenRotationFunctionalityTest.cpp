#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class TokenRotationFunctionalityTest : public ClientTestBase,
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

TEST_P(TokenRotationFunctionalityTest, CreateSyncStreamWithResultEventAfterGracePeriodStopFreeSuccess) {

    UINT64 currentTime, testTerminationTime,  startTestTime, stopPutFrameTime,
            putStreamEventResultTime = INVALID_TIMESTAMP_VALUE;

    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    STATUS retStatus;

    UINT32 currentputStreamFuncCount = 0;

    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    mSubmitServiceCallResultMode = STOP_AT_GET_STREAMING_TOKEN;

    UPLOAD_HANDLE localUploadHandle = mStreamingSession.addNewConsumerSession(mMockConsumerConfig,
                                                                              mStreamHandle);
    EXPECT_EQ(STATUS_SUCCESS,
              putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, localUploadHandle));

    currentputStreamFuncCount = ATOMIC_LOAD(&mPutStreamFuncCount);

    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
            + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    startTestTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

    stopPutFrameTime = startTestTime + MIN_STREAMING_TOKEN_EXPIRATION_DURATION;

    do {

        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        if (currentTime < stopPutFrameTime) {
            EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
        }

        if (ATOMIC_LOAD(&mPutStreamFuncCount) > currentputStreamFuncCount) {

            putStreamEventResultTime =
                    currentTime + (STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD - 1 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            currentputStreamFuncCount++;
        }

        if (IS_VALID_TIMESTAMP(putStreamEventResultTime) && currentTime >= putStreamEventResultTime) {

            UPLOAD_HANDLE localUploadHandle = mStreamingSession.addNewConsumerSession(mMockConsumerConfig,
                                                                                      mStreamHandle);
            EXPECT_EQ(STATUS_SUCCESS,
                      putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, localUploadHandle));

            putStreamEventResultTime = INVALID_TIMESTAMP_VALUE;

        }
        mStreamingSession.getActiveUploadHandles(currentUploadHandles);

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];

            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS,
                      mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
        }

    } while (currentTime < testTerminationTime);

    EXPECT_TRUE(mStreamingSession.mActiveUploadHandles.size() == 0);

    VerifyStopStreamSyncAndFree();
}

TEST_P(TokenRotationFunctionalityTest, CreateSyncStreamWithLargeBufferMultipleSessionsStopSyncFreeSuccess) {
    UINT64 currentTime, testTerminationTime, startTestTime, endPutFrameTime;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    STATUS retStatus;

    startTestTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

    mDeviceInfo.storageInfo.storageSize = ((UINT64) 200 * 1024 * 1024);

    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    endPutFrameTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
            + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

    } while (currentTime < endPutFrameTime);

    do {

        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];

            if (currentTime >
                    (startTestTime + MIN_STREAMING_TOKEN_EXPIRATION_DURATION
                            - STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD) &&
                    currentTime < (startTestTime + MIN_STREAMING_TOKEN_EXPIRATION_DURATION)) {
                DLOGV("Within Token expiration grace period");
            }

            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
        }

    } while (currentTime < testTerminationTime);

    EXPECT_TRUE(mStreamingSession.mActiveUploadHandles.size() == 0);

    VerifyStopStreamSyncAndFree();
}

TEST_P(TokenRotationFunctionalityTest, CreateSyncStreamForMultipleRotationsStopSyncFreeSuccess) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, testTerminationTime;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateScenarioTestClient();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // Run for duration for 5 token rotations
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + MIN_STREAMING_TOKEN_EXPIRATION_DURATION * 5;

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

    VerifyStopStreamSyncAndFree();
}

TEST_P(TokenRotationFunctionalityTest, CreateSyncStreamNewPutStreamResultComesFastButExistingSessionFailsRollsbackRestartsStopSyncFreeSuccess) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, testTerminationTime;
    UINT64 newUploadHandle = 2;
    BOOL eventReplied[3] = {FALSE, FALSE, FALSE};

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateScenarioTestClient();
    mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;

    ReadyStream();

    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + MIN_STREAMING_TOKEN_EXPIRATION_DURATION * 5;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        if (!eventReplied[ATOMIC_LOAD(&mPutStreamFuncCount)]) {
            UPLOAD_HANDLE localUploadHandle = mStreamingSession.addNewConsumerSession(mMockConsumerConfig,
                    mStreamHandle);
            UINT32 putStreamFuncCount = ATOMIC_LOAD(&mPutStreamFuncCount);
            switch (putStreamFuncCount) {
                case 1:
                    putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, localUploadHandle);
                    break;
                case 2:
                    newUploadHandle = localUploadHandle;
                    putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, localUploadHandle);
                    break;
            }
            eventReplied[ATOMIC_LOAD(&mPutStreamFuncCount)] = TRUE;
        }

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            if (ATOMIC_LOAD(&mPutStreamFuncCount) == 1 || uploadHandle == newUploadHandle) {
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            } else {
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                EXPECT_EQ(STATUS_SUCCESS,
                          mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR, &submittedAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            }
        }
    } while (currentTime < testTerminationTime);

    VerifyStopStreamSyncAndFree();
}

//CreateSync, Stream, at token rotation, getStreamingTokenResultEvent takes long time to return (after previous handle has hit eos.)
TEST_P(TokenRotationFunctionalityTest, CreateSyncStreamAtTokenRotationLongDelayForGetStreamingTokenResult)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck, gotStatusAwaitingAck = FALSE;
    UINT64 currentTime, streamStopTime;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateScenarioTestClient();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) +
                     MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
        if (mSubmitServiceCallResultMode != DISABLE_AUTO_SUBMIT && didPutFrame) {
            // Let first putFrame trigger putStreamResult to create consumer, then disable auto submitting getStreamTokenResult
            mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;
        }
        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    } while (currentTime < streamStopTime);

    // at this point, earliest upload handle should have reached end-of-stream
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_TRUE(currentUploadHandles.empty());

    // enable pic event chaining.
    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData,
                                                           SERVICE_CALL_RESULT_OK,
                                                           (PBYTE) TEST_STREAMING_TOKEN,
                                                           SIZEOF(TEST_STREAMING_TOKEN),
                                                           currentTime + HUNDREDS_OF_NANOS_IN_A_SECOND +
                                                           MIN_STREAMING_TOKEN_EXPIRATION_DURATION));
    mockProducer.putFrame(); // put a frame to trigger putStreamResult

    // should stream out remaining buffer successfully.
    VerifyStopStreamSyncAndFree();
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, TokenRotationFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
