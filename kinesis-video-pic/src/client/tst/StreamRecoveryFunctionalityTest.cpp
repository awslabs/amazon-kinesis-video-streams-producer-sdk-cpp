#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class StreamRecoveryFunctionalityTest : public ClientTestBase,
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

TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamResetConnectionEnsureRecovery)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime, resetConnectionTime;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 50 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    resetConnectionTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 30 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
        }

        if (IS_VALID_TIMESTAMP(resetConnectionTime) && currentTime >= resetConnectionTime) {
            // reset connection
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle,
                                                                   INVALID_UPLOAD_HANDLE_VALUE,
                                                                   SERVICE_CALL_RESULT_OK));
            resetConnectionTime = INVALID_TIMESTAMP_VALUE; // reset connection only once.
        }

    } while(currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamResetConnectionAfterTokenRotationEnsureRecovery) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime, resetConnectionTime;

    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    mMockConsumerConfig.mUploadSpeedBytesPerSecond = 100;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 50 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    resetConnectionTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 30 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
        if (!IS_VALID_TIMESTAMP(resetConnectionTime)) {
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            }
        }

        if (IS_VALID_TIMESTAMP(resetConnectionTime) && currentTime >= resetConnectionTime) {
            // reset connection
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle,
                                                                   INVALID_UPLOAD_HANDLE_VALUE,
                                                                   SERVICE_CALL_RESULT_OK));
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                mockConsumer->mUploadSpeed = 1000000;
            }
            resetConnectionTime = INVALID_TIMESTAMP_VALUE; // reset connection only once.
        }

    } while(currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

// Create stream, stream, last persisted ACK is before the rollback duration, the received ACK is
// after the rollback duration, inject a fault indicating not-dead host and terminate session.
// Make sure the rollback is to the received ACK + next fragment
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamRollbackToLastReceivedAckEnsureRecovery) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UPLOAD_HANDLE errorHandle;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime, rollbackTime, lastReceivedAckTime, lastPersistedAckTime;
    STATUS retStatus;

    if (mStreamInfo.streamCaps.fragmentAcks == FALSE) {
        return;
    }
    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    DLOGD("mStreamInfo.streamCaps.streamingType %d mStreamInfo.retention %ld", mStreamInfo.streamCaps.streamingType, mStreamInfo.retention);

    mStreamInfo.streamCaps.replayDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mMockConsumerConfig.mPersistAckDelayMs = 5000;
    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);
    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    rollbackTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 9 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        if (!IS_VALID_TIMESTAMP(rollbackTime)) {
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);

                // First getStreamData should trigger rollback
                if (IS_VALID_TIMESTAMP(lastReceivedAckTime) && uploadHandle != errorHandle) {
                    UINT32 actualDataSize;
                    retStatus = getKinesisVideoStreamData(mStreamHandle, mockConsumer->mUploadHandle,
                            mockConsumer->mDataBuffer, 1, &actualDataSize);
                    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_REALTIME) {
                        // Rollback to last received ack in realtime mode
                        EXPECT_EQ(
                                lastReceivedAckTime * HUNDREDS_OF_NANOS_IN_A_MILLISECOND,
                                FROM_STREAM_HANDLE(mStreamHandle)->curViewItem.viewItem.ackTimestamp);
                    } else {
                        // Rollback to the tail in offline mode
                        EXPECT_EQ(
                                lastPersistedAckTime * HUNDREDS_OF_NANOS_IN_A_MILLISECOND
                                + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND *
                                mMockProducerConfig.mKeyFrameInterval / mMockProducerConfig.mFps,
                                FROM_STREAM_HANDLE(mStreamHandle)->curViewItem.viewItem.ackTimestamp);
                    }
                    lastReceivedAckTime = INVALID_TIMESTAMP_VALUE; // only check when rollback happens
                } else {
                    retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                }

                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            }
        } else {
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    if (mFragmentAck.ackType == FRAGMENT_ACK_TYPE_RECEIVED) {
                        lastReceivedAckTime = mFragmentAck.timestamp;
                    } else if (mFragmentAck.ackType == FRAGMENT_ACK_TYPE_PERSISTED) {
                        lastPersistedAckTime = mFragmentAck.timestamp;
                    }
                }
            }
        }

        if (IS_VALID_TIMESTAMP(rollbackTime) && currentTime >= rollbackTime) {
            // send error ack to trigger rollback
            submittedAck = FALSE;
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, lastReceivedAckTime, &submittedAck));
                if (submittedAck) {
                    rollbackTime = INVALID_TIMESTAMP_VALUE; // rollback only once.
                    errorHandle = uploadHandle;
                }
            }
        }
    } while(currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

/*
 * contentView: Frag1 | Frag2 | Frag3 ...
 * Send a non recoverable error ack to Frag1. Make sure that Frag1 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorThrowAwayBadFragmentNearTail) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfThreeFragments, totalByteSent = 0, retrievedDataSize = 0, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed.
    if (mStreamInfo.streamCaps.fragmentAcks == FALSE || mStreamInfo.retention == 0) {
        return;
    }

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfThreeFragments = mMockProducerConfig.mFrameSizeByte * 3 * mMockProducerConfig.mKeyFrameInterval;
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    for(i = 0; i < mMockProducerConfig.mKeyFrameInterval * totalFragmentPut; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream 3 fragments without submitting any acks
    while (totalByteSent < sizeOfThreeFragments) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            if (gotStreamData) {
                totalByteSent += retrievedDataSize;
            }
        }
    }

    // submit a fatal error ack to current upload handle
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime);
    } while(!gotStreamData);

    // check that a new upload handle is created and the error handle is gone.
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_TRUE(currentUploadHandles[0] != errorHandle);
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, stopStreamSyncRoutine, (PVOID) this));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // stream out whats left in the buffer
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
            if (submittedAck) {
                ackReceived++;
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, mStreamClosed);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

/*
 * contentView: Frag1_Frame1 Frag1_Frame2 Frag1_Frame3 ...
 * Send a non recoverable error ack to Frag1 while it is not completed. Make sure that Frag1 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorThrowAwayBadFragmentAtHeadPartiallyStreamed) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfOneFragment, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    if (mStreamInfo.streamCaps.fragmentAcks == FALSE || mStreamInfo.retention == 0) {
        return;
    }

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfOneFragment = mMockProducerConfig.mFrameSizeByte * mMockProducerConfig.mKeyFrameInterval;
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // put some frames but not all for the first fragment
    for(i = 0; i < mMockProducerConfig.mKeyFrameInterval - 5; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream out everything currently in buffer
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
        }
    } while (retStatus != STATUS_NO_MORE_DATA_AVAILABLE);

    // submit a fatal error ack to current upload handle
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    // Mock consumer hasnt queue up any acks at this point. Manually override error ack submission by specifying the
    // timestamp for the first fragment.
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, 0, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime);
    } while(!gotStreamData);

    // finishing putting all frames for the first fragment.
    for(i = 0; i < 5; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // put in more fragments. At the end we are expecting totalFragmentPut * 3 acks.
    for(i = 0; i < mMockProducerConfig.mKeyFrameInterval * (totalFragmentPut-1); i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // check that a new upload handle is created and the error handle is gone.
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_TRUE(currentUploadHandles[0] != errorHandle);
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, stopStreamSyncRoutine, (PVOID) this));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // stream out whats left in the buffer
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
            if (submittedAck) {
                ackReceived++;
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, mStreamClosed);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

/*
 * contentView: Frag1_Frame1 Frag1_Frame2 Frag1_Frame3 ...
 * Send a non recoverable error ack to Frag1 while Frag1 is completed (next frame will be the start of Frag2).
 * Make sure that Frag1 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorThrowAwayBadFragmentAtHeadFullyStreamed) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfOneFragment, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    if (mStreamInfo.streamCaps.fragmentAcks == FALSE || mStreamInfo.retention == 0) {
        return;
    }

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfOneFragment = mMockProducerConfig.mFrameSizeByte * mMockProducerConfig.mKeyFrameInterval;
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // put all frames for the first fragment
    for(i = 0; i < mMockProducerConfig.mKeyFrameInterval; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream out everything currently in buffer
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
        }
    } while (retStatus != STATUS_NO_MORE_DATA_AVAILABLE);

    // submit a fatal error ack to current upload handle
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    // Mock consumer hasnt queue up any acks at this point. Manually override error ack submission by specifying the
    // timestamp for the first fragment.
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, 0, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime);
    } while(!gotStreamData);

    // put in more fragments. At the end we are expecting totalFragmentPut * 3 acks.
    for(i = 0; i < mMockProducerConfig.mKeyFrameInterval * (totalFragmentPut-1); i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // check that a new upload handle is created and the error handle is gone.
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_TRUE(currentUploadHandles[0] != errorHandle);
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, stopStreamSyncRoutine, (PVOID) this));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // stream out whats left in the buffer
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime);
            mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
            if (submittedAck) {
                ackReceived++;
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, mStreamClosed);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamRecoveryFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
