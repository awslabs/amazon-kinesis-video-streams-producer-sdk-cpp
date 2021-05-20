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
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }

        if (IS_VALID_TIMESTAMP(resetConnectionTime) && currentTime >= resetConnectionTime) {
            // reset connection
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle,
                                                                   INVALID_UPLOAD_HANDLE_VALUE,
                                                                   SERVICE_CALL_RESULT_OK));
            resetConnectionTime = INVALID_TIMESTAMP_VALUE; // reset connection only once.
        }

    } while (currentTime < streamStopTime);

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
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            }
        }

        if (IS_VALID_TIMESTAMP(resetConnectionTime) && currentTime >= resetConnectionTime) {
            // reset connection
            EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamTerminated(mStreamHandle,
                                                                   INVALID_UPLOAD_HANDLE_VALUE,
                                                                   SERVICE_CALL_RESULT_OK));
            // restore slow speed after reset so all subsequent uploads are at normal speed
            mMockConsumerConfig.mUploadSpeedBytesPerSecond = 1000000;
            // restore speed for current active upload handles
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                mockConsumer->mUploadSpeed = 1000000;
            }
            resetConnectionTime = INVALID_TIMESTAMP_VALUE; // reset connection only once.
        }

    } while (currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

// Create stream, stream, last persisted ACK is before the rollback duration, the received ACK is
// after the rollback duration, inject a fault indicating not-dead host and terminate session.
// Make sure the rollback is to the received ACK + next fragment
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamRollbackToLastReceivedAckEnsureRecovery) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    UPLOAD_HANDLE errorHandle = INVALID_UPLOAD_HANDLE_VALUE;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, streamStopTime, rollbackTime, lastReceivedAckTime = 0, lastPersistedAckTime = 0;
    STATUS retStatus;

    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();

    mDeviceInfo.clientInfo.stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateScenarioTestClient();

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
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            }
        } else {
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);
                retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
                if (mockConsumer != NULL) {
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
    } while (currentTime < streamStopTime);

    VerifyStopStreamSyncAndFree();
}

/*
 * contentView: Frag1 | Frag2 | Frag3 ...
 * Send a non recoverable error ack to Frag1. Make sure that Frag1 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorThrowAwayBadFragmentNearTail) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfThreeFragments, totalByteSent = 0, retrievedDataSize = 0, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed.
    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfThreeFragments = mMockProducerConfig.mFrameSizeByte * 3 * mMockProducerConfig.mKeyFrameInterval;

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
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
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

/*
 * contentView: Frag1 | Frag2 | Frag3 ...
 * Send a non recoverable error ack to Frag2 with a timestamp. Make sure that ONLY Frag2 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorWithTimestampThrowAwayBadFragmentMiddle) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime, ackTime = 0, duration;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfThreeFragments, totalByteSent = 0, retrievedDataSize = 0, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed.
    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();
    PASS_TEST_FOR_OFFLINE_ZERO_REPLAY_DURATION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfThreeFragments = mMockProducerConfig.mFrameSizeByte * 3 * mMockProducerConfig.mKeyFrameInterval;

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    duration = (UINT64) 1000 / mMockProducerConfig.mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval * totalFragmentPut; i++) {
        if (i < mMockProducerConfig.mKeyFrameInterval) {
            ackTime += duration;
        }

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream 3 fragments without submitting any acks
    while (totalByteSent < sizeOfThreeFragments) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (gotStreamData) {
                totalByteSent += retrievedDataSize;
            }
        }
    }

    // submit a fatal error ack to current upload handle at timestamp corresponding to the second fragment
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, ackTime / HUNDREDS_OF_NANOS_IN_A_MILLISECOND, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

/*
 * contentView: Frag1 | Frag2 | Frag3 ...
 * Send a non recoverable error ack on the 4th fragment ingestion without a timestamp.
 * Make sure that the earlier fragments are not streamed in the new connection.
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorWithoutTimestampThrowAwayBadFragmentMiddle) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfThreeFragments, totalByteSent = 0, retrievedDataSize = 0, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed.
    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();
    PASS_TEST_FOR_OFFLINE_ZERO_REPLAY_DURATION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfThreeFragments = mMockProducerConfig.mFrameSizeByte * 3 * mMockProducerConfig.mKeyFrameInterval;

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval * totalFragmentPut; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream 3 fragments without submitting any acks
    while (totalByteSent < sizeOfThreeFragments) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (gotStreamData) {
                totalByteSent += retrievedDataSize;
            }
        }
    }

    // submit a fatal error ack to current upload handle at timestamp corresponding to the second fragment
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, INVALID_TIMESTAMP_VALUE, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the entire duration of the streamed upload handle
    EXPECT_EQ(ackReceived, (totalFragmentPut - 4) * 3);
}

/*
 * contentView: Frag1 | Frag2 | Frag3 ...
 * Send the first 3 fragments and issue a persistent ACKs for the first one. Send a non recoverable error ack
 * without a timestamp. Make sure that the first fragment is rolled back to and streamed whioe
 * the fragments from the persistent ACK till current are skipped in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorWithoutTimestampThrowAwayBadFromPersist) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus;
    UINT32 i, totalFragmentPut = 10, sizeOfThreeFragments, totalByteSent = 0, retrievedDataSize = 0, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    // need ack to count number of fragments streamed.
    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();
    PASS_TEST_FOR_OFFLINE_ZERO_REPLAY_DURATION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();
    sizeOfThreeFragments = mMockProducerConfig.mFrameSizeByte * 3 * mMockProducerConfig.mKeyFrameInterval;

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // need ack to count number of fragments streamed. No ack case should also work if with ack works.
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval * totalFragmentPut; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream 3 fragments with submitting acks only for the first one
    while (totalByteSent < sizeOfThreeFragments) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (gotStreamData) {
                totalByteSent += retrievedDataSize;
            }
        }
    }

    // Only ACK the first fragment
    if (mockConsumer != NULL) {
        mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK, FRAGMENT_ACK_TYPE_BUFFERING, 0, &submittedAck);
        if (submittedAck) {
            ackReceived++;
        }
        mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK, FRAGMENT_ACK_TYPE_RECEIVED, 0, &submittedAck);
        if (submittedAck) {
            ackReceived++;
        }
        mockConsumer->submitNormalAck(SERVICE_CALL_RESULT_OK, FRAGMENT_ACK_TYPE_PERSISTED, 0, &submittedAck);
        if (submittedAck) {
            ackReceived++;
        }
    }

    // submit a fatal error ack to current upload handle at timestamp corresponding to the second fragment
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED, INVALID_TIMESTAMP_VALUE, &submittedAck));
    EXPECT_EQ(TRUE, submittedAck);
    errorHandle = mockConsumer->mUploadHandle;

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedDataSize);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the entire duration of the streamed upload handle back until the lsat
    // persisted ack which would be included in the rollback.
    EXPECT_EQ(ackReceived, (totalFragmentPut - 3) * 3);
}

/*
 * contentView: Frag1_Frame1 Frag1_Frame2 Frag1_Frame3 ...
 * Send a non recoverable error ack to Frag1 while it is not completed. Make sure that Frag1 is not streamed in the new connection
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamFatalErrorThrowAwayBadFragmentAtHeadPartiallyStreamed) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, totalFragmentPut = 10, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
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
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

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
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, totalFragmentPut = 10, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;
    CreateScenarioTestClient();

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
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
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    // Verify that we didnt get any ack for the bad fragment
    EXPECT_EQ(ackReceived, (totalFragmentPut - 1) * 3);
}

/*
 * contentView: Frag1_Frame1 Frag1_Frame2 Frag1_Frame3 ... Frag2_fram1,.... FragN_frameN
 * Timeout the upload handle with current view non-zero. Ensure immediate auto-recovery with rollback
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamTimeoutWithBuffer) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, totalFragmentPut = 10, ackReceived = 0;
    UPLOAD_HANDLE errorHandle;

    PASS_TEST_FOR_OFFLINE();

    // reduce the frame size so that it generates fragments faster
    CreateScenarioTestClient();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // put all frames for the first few fragments
    for (i = 0; i < 5 * mMockProducerConfig.mKeyFrameInterval; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    // stream out a few fragments but still have some in the buffer
    for (i = 0; i < 3 * mMockProducerConfig.mKeyFrameInterval; i++) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int j = 0; j < currentUploadHandles.size(); j++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[j];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    }

    // submit a timeout to current upload handle
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());

    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitConnectionError(SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT));
    errorHandle = mockConsumer->mUploadHandle;

    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    // loop until the error handle get end-of-stream
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(retStatus, gotStreamData, mockConsumer->mUploadHandle, &currentTime, &mockConsumer);
    } while (!gotStreamData);

    // put in more fragments. At the end we are expecting totalFragmentPut * 3 acks.
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval * (totalFragmentPut -1 ); i++) {
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
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

/*
 * contentView: Frag1_Frame1 Frag1_Frame2 Frag1_Frame3 ... Frag2_fram1,.... FragN_frameN
 * Timeout the upload handle with current view zero. Ensure no immediate auto-recovery
 */
TEST_P(StreamRecoveryFunctionalityTest, CreateStreamThenStreamTimeoutWithoutBuffer) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus = STATUS_SUCCESS;
    BYTE dataBuf[TEST_DEFAULT_PRODUCER_CONFIG_FRAME_SIZE + 1000]; // Should be over the default frame size
    UINT32 i, ackReceived = 0, dataBufSize = SIZEOF(dataBuf), retrievedSize, overhead, numFragments = 5;
    UPLOAD_HANDLE errorHandle;
    PKinesisVideoStream pKinesisVideoStream;
    PStreamMkvGenerator pStreamMkvGenerator;

    PASS_TEST_FOR_OFFLINE();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;

    // reduce the frame size so that it generates fragments faster
    CreateScenarioTestClient();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // put all frames for the first few fragments
    for (i = 0; i < numFragments * mMockProducerConfig.mKeyFrameInterval; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }

    // Put one more frame to ensure we are on a non-key frame
    mockProducer.putFrame();

    // stream out everything currently in buffer
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }
    } while (retStatus != STATUS_NO_MORE_DATA_AVAILABLE);

    // submit a timeout to current upload handle
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());

    // Expected call counts
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // NOTE: Describe is made to fail a few times
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));

    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitConnectionError(SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT));
    errorHandle = mockConsumer->mUploadHandle;

    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // No recovery
    EXPECT_EQ(1, ATOMIC_LOAD(&mPutStreamFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamReadyFuncCount));

    // Put some non-key frames until the next key frame - stream should have recovered
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval - 1; i++) {
        mockProducer.putFrame();
        // Recovery till ready state
        EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));
        EXPECT_EQ(3, ATOMIC_LOAD(&mDescribeStreamFuncCount)); // remains the same
        EXPECT_EQ(0, ATOMIC_LOAD(&mCreateStreamFuncCount));
        EXPECT_EQ(0, ATOMIC_LOAD(&mTagResourceFuncCount));
        EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount)); // remains the same
        EXPECT_EQ(2, ATOMIC_LOAD(&mStreamReadyFuncCount));
    }

    // Next one should be a key frame.
    // In this case it shouldn't make a stream start as we have a rollback
    // and the persisted ACK has not been submitted so the stream start
    // should be a key frame from past.
    mockProducer.putFrame();
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    // Put some non-key frames to complete the fragment
    for (i = 0; i < mMockProducerConfig.mKeyFrameInterval - 1; i++) {
        mockProducer.putFrame();
    }

    // check that a new upload handle is created and the error handle is gone.
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_TRUE(currentUploadHandles[0] != errorHandle);

    // Validate that the new upload handle has a rolled back proper start stream
    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoStreamData(mStreamHandle,
                                                        currentUploadHandles[0], dataBuf,
                                                        dataBufSize, &retrievedSize));
    EXPECT_EQ(dataBufSize, retrievedSize);

    pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    // Get the overhead size
    overhead = mkvgenGetFrameOverhead(pStreamMkvGenerator, MKV_STATE_START_STREAM);

    // Check the content of the buffer
    // NOTE: We had submitted ACKs for all of the fragments before the termination
    // so the entire buffer is trimmed. We will be skipping over the frames
    // until the next key frame which will become the stream start
    for (i = 0; i < TEST_DEFAULT_PRODUCER_CONFIG_FRAME_SIZE; i++) {
        if (mStreamInfo.streamCaps.replayDuration == 0) {
            EXPECT_EQ((numFragments + 1) * mMockProducerConfig.mKeyFrameInterval, dataBuf[i + overhead])
                                << "Failed on " << i;
        } else {
            EXPECT_EQ(0, dataBuf[i + overhead])
                                << "Failed on " << i;
        }
    }

    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, stopStreamSyncRoutine, (PVOID) this));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // stream out whats left in the buffer
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck);
                if (submittedAck) {
                    ackReceived++;
                }
            }
        }
    } while (currentTime < stopTime && !currentUploadHandles.empty());

    THREAD_JOIN(thread, NULL);

    EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
    EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
    EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

TEST_P(StreamRecoveryFunctionalityTest, streamStartViewDroppedBeforeFullyConsumedRecoverable)
{
    UINT64 currentTime, testTerminationTime;
    CreateScenarioTestClient();
    BOOL didPutFrame, gotStreamData = FALSE, submittedAck;
    UINT32 tokenRotateCount = 0, i;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    STATUS status = STATUS_SUCCESS;

    CreateScenarioTestClient();
    /* data buffer need to be less than frame size so a single frame needs two getStreamData calls to be consumed */
    mMockProducerConfig.mFrameSizeByte = 50000;
    mMockConsumerConfig.mDataBufferSizeByte = 30000;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    PASS_TEST_FOR_OFFLINE()

    /* content view can contain only 40 frames */
    mStreamInfo.streamCaps.frameRate = 20;
    mStreamInfo.streamCaps.bufferDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;

    CreateStreamSync();
    /* default fps is 20 */
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    /* fill the content view */
    for(i = 0; i < 40; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    /* should only be 1 because we didnt hit token rotation */
    EXPECT_EQ(1, currentUploadHandles.size());

    UPLOAD_HANDLE uploadHandle = currentUploadHandles[0];
    mockConsumer = mStreamingSession.getConsumer(uploadHandle);

    /* do ONE successful getStreamData */
    while (!gotStreamData) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        status = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
        VerifyGetStreamDataResult(status, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
    }

    /* put 40 frames again. The first 40 frames are now all dropped */
    for(i = 0; i < 40; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE));
    }

    status = STATUS_SUCCESS;
    gotStreamData = FALSE;

    /* do another getStreamData */
    while (!gotStreamData) {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        status = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
    }

    EXPECT_EQ(STATUS_SUCCESS, status);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamRecoveryFunctionalityTest, FragmentMetadataStartStreamFailRecovery) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer = nullptr;
    BOOL gotStreamData = FALSE, submittedAck, firstChunk = TRUE;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS retStatus = STATUS_SUCCESS;
    BYTE dataBuf[TEST_DEFAULT_PRODUCER_CONFIG_FRAME_SIZE + 1000];
    BYTE storedDataBuf[SIZEOF(dataBuf)];
    UINT32 i, ackReceived = 0, dataBufSize = SIZEOF(dataBuf), retrievedSize,
            storedRetrievedSize, overhead, numFragments = 1;
    UPLOAD_HANDLE errorHandle;
    PKinesisVideoStream pKinesisVideoStream;
    PStreamMkvGenerator pStreamMkvGenerator;

    PASS_TEST_FOR_OFFLINE_ZERO_REPLAY_DURATION();

    // reduce the frame size so that it generates fragments faster
    mMockProducerConfig.mFrameSizeByte = 500;

    // reduce the frame size so that it generates fragments faster
    CreateScenarioTestClient();

    CreateStreamSync();
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // Start with some metadata - important that it's before frames
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "TestName", (PCHAR) "TestValue", TRUE));

    // put all frames for the first few fragments
    for (i = 0; i < numFragments * mMockProducerConfig.mKeyFrameInterval; i++) {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }

    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());

    // stream out everything currently in buffer
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        UPLOAD_HANDLE uploadHandle = currentUploadHandles[0];
        mockConsumer = mStreamingSession.getConsumer(uploadHandle);
        retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData, &retrievedSize);
        VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);

        // Store the first chunk for later comparison
        if (firstChunk) {
            storedRetrievedSize = MIN(dataBufSize, retrievedSize);
            MEMCPY(storedDataBuf, mockConsumer->mDataBuffer, storedRetrievedSize);
            firstChunk = FALSE;
        }
    } while (retStatus != STATUS_NO_MORE_DATA_AVAILABLE);

    // submit an error ACK and ensure we roll back
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    mockConsumer = mStreamingSession.getConsumer(currentUploadHandles[0]);
    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->submitConnectionError(SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT));
    errorHandle = mockConsumer->mUploadHandle;

    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    mockProducer.putFrame();
    EXPECT_EQ(2, ATOMIC_LOAD(&mPutStreamFuncCount));

    // Should have two
    mStreamingSession.getActiveUploadHandles(currentUploadHandles);
    EXPECT_EQ(1, currentUploadHandles.size());
    EXPECT_NE(errorHandle, currentUploadHandles[0]);

    // Get the data with the new handle
    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoStreamData(mStreamHandle,
                                                        currentUploadHandles[0], dataBuf,
                                                        dataBufSize, &retrievedSize));
    EXPECT_EQ(storedRetrievedSize, retrievedSize);

    EXPECT_EQ(0, MEMCMP(dataBuf, storedDataBuf, storedRetrievedSize));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamRecoveryFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
