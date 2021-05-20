#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class IntermittentProducerFunctionalityTest : public ClientTestBase,
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

TEST_P(IntermittentProducerFunctionalityTest, CreateSyncStreamWithLargeBufferAwaitForLastAckStopSyncFreeSuccess) {
    UINT64 currentTime, testTerminationTime, endPutFrameTime;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    STATUS retStatus;

    int intermittentProducerSessionCount = 0;

    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    do {

        CreateStreamSync();

        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

        endPutFrameTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

        do {
            currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

            EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        } while (currentTime < endPutFrameTime);


        testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
                + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

        do {
            currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

            mStreamingSession.getActiveUploadHandles(currentUploadHandles);

            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                //GetStreamedData and SubmitAck
                retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
                if (retStatus == STATUS_END_OF_STREAM) {
                    tokenRotateCount++;
                }
            }

        } while (currentTime < testTerminationTime );

        EXPECT_TRUE(mStreamingSession.mActiveUploadHandles.size() == 0);

        VerifyStopStreamSyncAndFree();

        mStreamingSession.clearSessions();

    } while (intermittentProducerSessionCount++ < 2);

}

TEST_P(IntermittentProducerFunctionalityTest, CreateSyncStreamStopSyncFreeRepeatSuccess) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, testTerminationTime;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    for (int iter = 0; iter < (int) mRepeatTime; iter++) {
        CreateStreamSync();
        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
        testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND;

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
        mStreamingSession.clearSessions();
    }
}

// CreateSync, StopSync, Free, Repeat
TEST_P(IntermittentProducerFunctionalityTest, RepeatedCreateSyncStopSyncFree)
{
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    UINT32 repeatTime = 10, i;

    for(i = 0; i < repeatTime; i ++) {
        CreateStreamSync();
        stopKinesisVideoStreamSync(mStreamHandle);
        freeKinesisVideoStream(&mStreamHandle);
    }
}


INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, IntermittentProducerFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
