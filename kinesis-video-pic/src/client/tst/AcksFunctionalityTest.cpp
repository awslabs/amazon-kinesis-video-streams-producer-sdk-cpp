#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class AcksFunctionalityTest : public ClientTestBase,
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

//Submit various types of error ACKs, Ensure the rollback is done from the ACK error time.
TEST_P(AcksFunctionalityTest, CheckRollbackFromErrorAckTime)
{
    CreateScenarioTestClient();
    BOOL submittedErrorAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime, currentIndex;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    PViewItem pViewItem;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    //putFrame for 10 seconds, should finish putting at least 1 fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

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
                // right now, resetting current is irregardless of the service error code. There just testing SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR is enough
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, &submittedErrorAck));
            }
        }
    } while (currentTime < stopTime && !submittedErrorAck);


    EXPECT_EQ(TRUE, submittedErrorAck);

    // check current matches error ack timestamp
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(pKinesisVideoStream->pView, &currentIndex));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(pKinesisVideoStream->pView, currentIndex, &pViewItem));
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    // mFragmentAck is the error ack that was submitted. Verify that error ack's timestamp equals current's timestamp.
    EXPECT_EQ(pViewItem->ackTimestamp, mFragmentAck.timestamp);
}

TEST_P(AcksFunctionalityTest, CreateStreamSubmitPersistedAckBeforeReceivedAckSuccess) {
    CreateScenarioTestClient();
    BOOL submittedAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    //putFrame for 10 seconds, should finish putting at least 1 fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size()); // test should finish before token rotation

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            // sending acks but get order reversed for persisted ack and received ack
            if (mockConsumer != NULL && mockConsumer->mAckQueue.size() > 0) {
                switch (mockConsumer->mAckQueue.top().mFragmentAck.ackType) {
                    case FRAGMENT_ACK_TYPE_PERSISTED:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                FRAGMENT_ACK_TYPE_RECEIVED,
                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                &submittedAck));
                        break;
                    case FRAGMENT_ACK_TYPE_RECEIVED:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                FRAGMENT_ACK_TYPE_PERSISTED,
                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                &submittedAck));
                        break;
                    default:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                mockConsumer->mAckQueue.top().mFragmentAck.ackType,
                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                &submittedAck));
                }
                mockConsumer->mAckQueue.pop();
            }
        }
    } while (currentTime < stopTime && !submittedAck);
    VerifyStopStreamSyncAndFree();
}

TEST_P(AcksFunctionalityTest, CreateStreamSubmitReceivedAckBeforeBufferingAckSuccess) {
    CreateScenarioTestClient();
    BOOL submittedAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    //putFrame for 10 seconds, should finish putting at least 1 fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size()); // test should finish before token rotation

        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            // sending acks but get order reversed for buffering ack and received ack
            if (mockConsumer != NULL && mockConsumer->mAckQueue.size() > 0) {
                switch (mockConsumer->mAckQueue.top().mFragmentAck.ackType) {
                    case FRAGMENT_ACK_TYPE_RECEIVED:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                                                                FRAGMENT_ACK_TYPE_BUFFERING,
                                                                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                                                                &submittedAck));
                        break;
                    case FRAGMENT_ACK_TYPE_BUFFERING:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                                                                FRAGMENT_ACK_TYPE_RECEIVED,
                                                                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                                                                &submittedAck));
                        break;
                    default:
                        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK,
                                                                                mockConsumer->mAckQueue.top().mFragmentAck.ackType,
                                                                                mockConsumer->mAckQueue.top().mFragmentAck.timestamp,
                                                                                &submittedAck));
                }
                mockConsumer->mAckQueue.pop();
            }
        }
    } while (currentTime < stopTime && !submittedAck);
    VerifyStopStreamSyncAndFree();
}

TEST_P(AcksFunctionalityTest, CreateStreamSubmitACKsOutsideOfViewRangeFail) {
    CreateScenarioTestClient();
    BOOL submittedAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    //putFrame for 10 seconds, should finish putting at least 1 fragment
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

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
                // send one persisted ack falling in future timestamp, should fail as out of content view
                EXPECT_EQ(STATUS_ACK_TIMESTAMP_NOT_IN_VIEW_WINDOW, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK, FRAGMENT_ACK_TYPE_PERSISTED, stopTime + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND, &submittedAck));
            }
        }
    } while (currentTime < stopTime && !submittedAck);
}

TEST_P(AcksFunctionalityTest, CreateStreamSubmitACKsTerminatedUploadHandle) {
    CreateScenarioTestClient();
    BOOL submittedAck = FALSE, didPutFrame, gotStreamData;
    MockConsumer *mockConsumer;
    UINT64 stopTime, currentTime, startTime;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    STATUS retStatus;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // putFrame for a few seconds, should finish putting at least 2 fragment
    startTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
    stopTime = startTime + 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < stopTime);

    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    UPLOAD_HANDLE storedHandle = currentUploadHandles[0];

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(1, currentUploadHandles.size());

        UPLOAD_HANDLE uploadHandle = currentUploadHandles[0];
        mockConsumer = mStreamingSession.getConsumer(uploadHandle);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);

        // send one persisted ack
        EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK, FRAGMENT_ACK_TYPE_PERSISTED, 0, &submittedAck));
    } while (currentTime < stopTime && !submittedAck);

    // Submit terminated
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle, storedHandle, SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT));

    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(2, currentUploadHandles.size());

    // This should be benign and shouldn't affect anything
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_OK, 0, &submittedAck));

    // Continue with the new handle
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        EXPECT_EQ(2, currentUploadHandles.size());

        UPLOAD_HANDLE uploadHandle = currentUploadHandles[1];
        mockConsumer = mStreamingSession.getConsumer(uploadHandle);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
    } while (currentTime < stopTime && !submittedAck);

    // Validate that no errors actually been reported as the reported error was on an already closed handle
    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamErrorReportFuncCount));
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, AcksFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
