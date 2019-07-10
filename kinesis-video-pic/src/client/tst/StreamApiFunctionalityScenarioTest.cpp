#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class StreamApiFunctionalityScenarioTest : public ClientTestBase,
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

/*
 * Testing basic token rotation. The test was given enough time for 1 token rotations plus 2 seconds extra.
 * When the given time ran out, we should have observed 3 token rotations taken place. if timedGetStreamData returns
 * STATUS_END_OF_STREAM, it means the previous session has ended, thus one token rotation has taken place.
 */
TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationBasicScenario)
{
    UINT64 currentTime, testTerminationTime;
    CreateScenarioTestClient();
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
                          + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
        }
    } while (currentTime < testTerminationTime);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationBasicScenarioFaultInjected)
{
    UINT64 currentTime, testTerminationTime;
    CreateScenarioTestClient();
    BOOL didPutFrame, gotStreamData, submittedAck, didSubmitErrorAck = FALSE;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
                          + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }

            // fault inject upload handle 1 once.
            if (uploadHandle == 1 && !didSubmitErrorAck) {
                mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, &didSubmitErrorAck);
            }
        }
    } while (currentTime < testTerminationTime);

    switch (mStreamInfo.streamCaps.streamingType) {
        case STREAMING_TYPE_REALTIME:
            EXPECT_TRUE(tokenRotateCount >= 2);
            break;
        case STREAMING_TYPE_OFFLINE:
            // in offline mode we expect one more rotation because in offline mode, at error, producer rollback all the
            // way to the tail.
            EXPECT_TRUE(tokenRotateCount >= 3);
            break;
        default:
            break;
    }

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationBasicScenarioEOFR)
{
    mMockProducerConfig.mSetEOFR = TRUE;

    CreateScenarioTestClient();

    UINT64 currentTime, testTerminationTime;
    BOOL didPutFrame, gotStreamData, submittedAck, didSubmitErrorAck = FALSE;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
                          + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
        }
    } while (currentTime < testTerminationTime);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}



INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamApiFunctionalityScenarioTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));