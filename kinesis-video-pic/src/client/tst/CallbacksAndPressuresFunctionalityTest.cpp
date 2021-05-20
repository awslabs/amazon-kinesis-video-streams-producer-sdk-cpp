#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class CallbacksAndPressuresFunctionalityTest : public ClientTestBase,
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

TEST_P(CallbacksAndPressuresFunctionalityTest, CreateStreamLatencyPressureCallbackCalledSuccess) {
    BOOL didPutFrame;
    UINT64 currentTime, streamStopTime;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Set the stream latency pressure triggered at 2 seconds delay
    mStreamInfo.streamCaps.maxLatency = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateStreamSync();

    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamLatencyPressureFuncCount));
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 4 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));
    } while (currentTime < streamStopTime);

    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE) {
        EXPECT_TRUE(ATOMIC_LOAD(&mStreamLatencyPressureFuncCount) == 0);
    } else {
        EXPECT_TRUE(ATOMIC_LOAD(&mStreamLatencyPressureFuncCount) > 0);
    }
}

TEST_P(CallbacksAndPressuresFunctionalityTest, CreateStreamDelayACKsStaleCallbackCalledSuccess) {
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT64 currentTime, streamStopTime, streamStartTime;

    CreateScenarioTestClient();
    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Set the connection staleness triggered at 2 seconds delay
    mStreamInfo.streamCaps.connectionStalenessDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateStreamSync();

    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamConnectionStaleFuncCount));
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    streamStopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 4 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamStartTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame));

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (mockConsumer != NULL) {
                if (currentTime > streamStartTime + 3 * HUNDREDS_OF_NANOS_IN_A_SECOND) {
                    EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
                }
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            }
        }
    } while (currentTime < streamStopTime);

    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE || mStreamInfo.streamCaps.fragmentAcks == FALSE) {
        EXPECT_TRUE(ATOMIC_LOAD(&mStreamConnectionStaleFuncCount) == 0);
    } else {
        EXPECT_TRUE(ATOMIC_LOAD(&mStreamConnectionStaleFuncCount) > 0);
    }
}

TEST_P(CallbacksAndPressuresFunctionalityTest, BiggerBufferDurationThanStorageCheckStoragePressureCallbackAndOOMError)
{
    mDeviceInfo.storageInfo.storageSize = 5 * 1024 * 1024;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    // in offline mode, putFrame never fails with OOM but blocks instead. Therefore pass test for offline mode.
    PASS_TEST_FOR_OFFLINE();

    mStreamInfo.streamCaps.bufferDuration = 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateStreamSync();

    STATUS retStatus = STATUS_SUCCESS;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // keep putting frame until fails.
    do {
        retStatus = mockProducer.putFrame();
    } while (STATUS_SUCCEEDED(retStatus));

    EXPECT_TRUE(ATOMIC_LOAD(&mStorageOverflowPressureFuncCount) > 0);
    EXPECT_EQ(STATUS_STORE_OUT_OF_MEMORY, retStatus);
}

// Similar to the previous test but sets the eviction policy instead of failing with OOM
TEST_P(CallbacksAndPressuresFunctionalityTest, BiggerBufferDurationThanStorageCheckStoragePressureCallbackAndNoOOMError)
{
    UINT64 timestamp = 0;
    mDeviceInfo.storageInfo.storageSize = 5 * 1024 * 1024;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    // in offline mode, putFrame never fails with OOM but blocks instead. Therefore pass test for offline mode.
    PASS_TEST_FOR_OFFLINE();

    mStreamInfo.streamCaps.bufferDuration = 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_DROP_TAIL_ITEM;
    CreateStreamSync();

    STATUS retStatus = STATUS_SUCCESS;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    // keep putting frame until we get a few dropped frames.
    do {
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
        if (ATOMIC_LOAD(&mDroppedFrameReportFuncCount) != 0) {
            // Check the time stamps of the dropped frames
            EXPECT_EQ(timestamp, mFrameTime);
            timestamp += mockProducer.getCurrentFrame()->duration;
        }
    } while (ATOMIC_LOAD(&mDroppedFrameReportFuncCount) < 100);

    EXPECT_TRUE(ATOMIC_LOAD(&mStorageOverflowPressureFuncCount) > 0);
    EXPECT_EQ(STATUS_SUCCESS, retStatus);
}

// Create Offline stream. Put frame until blocked on availability. StopSync. Send ACK to enable availability. Ensure unblocked put Frame call returns an error.
TEST_P(CallbacksAndPressuresFunctionalityTest, CheckBlockedOfflinePutFrameReturnsErrorAfterStop)
{
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;
    BOOL gotStreamData, submittedAck;
    UINT64 currentTime, stopTime;
    TID thread;
    STATUS *pRetValue;
    STREAM_HANDLE streamHandle;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();
    PASS_TEST_FOR_REALTIME();

    // use a small buffer size so it's easily filled
    mDeviceInfo.storageInfo.storageSize = 5 * 1024 * 1024;
    CreateScenarioTestClient();

    mMockProducerConfig.mIsLive = FALSE;
    // give it a big buffer duration so producer should hit storage limit first.
    mStreamInfo.streamCaps.bufferDuration = 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    CreateStreamSync();

    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, defaultProducerRoutine, (PVOID) this));
    // Let producer run for 5 seconds. Producer thread should get blocked on space within 5 seconds.
    THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    MUTEX_LOCK(mTestClientMutex);
    streamHandle = mStreamHandle;
    MUTEX_UNLOCK(mTestClientMutex);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(streamHandle));

    // Start consuming until a persisted ack is submitted
    stopTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 1 * HUNDREDS_OF_NANOS_IN_A_MINUTE;
    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);
            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
        }

        if (submittedAck && mFragmentAck.ackType == FRAGMENT_ACK_TYPE_PERSISTED) {
            break;
        }
    } while (currentTime < stopTime);

    // The persisted ack should unblock the producer.
    THREAD_JOIN(thread, (PVOID *) &pRetValue);
    // Check that the last putFrame in producer gets the correct error code.
    EXPECT_EQ(STATUS_BLOCKING_PUT_INTERRUPTED_STREAM_TERMINATED, mStatus);
}


INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, CallbacksAndPressuresFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
