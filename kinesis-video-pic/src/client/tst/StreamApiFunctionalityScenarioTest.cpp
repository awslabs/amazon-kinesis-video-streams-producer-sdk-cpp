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
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
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
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL ) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));

                // fault inject upload handle 1 once.
                if (uploadHandle == 1 && !didSubmitErrorAck) {
                    mockConsumer->submitErrorAck(SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR, &didSubmitErrorAck);
                }
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
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime;
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
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }
    } while (currentTime < testTerminationTime);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationFragmentMultiTrackScenarioEofrPts)
{
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime, trackId;
    BOOL gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0, frameCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE;

    CreateStreamSync();

    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        // Every 5 frames simulate a video frame
        trackId = (frameCount % 5 == 0) ? TEST_VIDEO_TRACK_ID : TEST_AUDIO_TRACK_ID;

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE, trackId)) << "frameCount " << frameCount;
        frameCount++;

        // output the EOFR
        if (frameCount % mMockProducerConfig.mKeyFrameInterval == 0) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            EXPECT_EQ(STATUS_SETTING_KEY_FRAME_FLAG_WHILE_USING_EOFR, putKinesisVideoFrame(mStreamHandle, &eofrFrame));
        }

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }

        // Increment by the frame duration
        incrementTestTimeVal(1000 / mMockProducerConfig.mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    } while (currentTime < testTerminationTime + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationFragmentMultiTrackScenarioEofrDts)
{
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime, trackId;
    BOOL gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0, frameCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE;

    CreateStreamSync();

    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        // Every 5 frames simulate a video frame
        trackId = (frameCount % 5 == 0) ? TEST_VIDEO_TRACK_ID : TEST_AUDIO_TRACK_ID;

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE, trackId)) << "frameCount " << frameCount;
        frameCount++;

        // output the EOFR
        if (frameCount % mMockProducerConfig.mKeyFrameInterval == 0) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            EXPECT_EQ(STATUS_SETTING_KEY_FRAME_FLAG_WHILE_USING_EOFR, putKinesisVideoFrame(mStreamHandle, &eofrFrame));
        }

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }

        // Increment by the frame duration
        incrementTestTimeVal(1000 / mMockProducerConfig.mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    } while (currentTime < testTerminationTime + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationIntermittentMultiTrackScenarioEofrPts)
{
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime, trackId;
    BOOL gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0, frameCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR;

    CreateStreamSync();

    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        // Every 5 frames simulate a video frame
        trackId = (frameCount % 5 == 0) ? TEST_VIDEO_TRACK_ID : TEST_AUDIO_TRACK_ID;

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE, trackId)) << "frameCount " << frameCount;
        frameCount++;

        // output the EOFR
        if (frameCount % mMockProducerConfig.mKeyFrameInterval == 0) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &eofrFrame));
        }

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }

        // Increment by the frame duration
        incrementTestTimeVal(1000 / mMockProducerConfig.mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    } while (currentTime < testTerminationTime + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationIntermittentMultiTrackScenarioEofrDts)
{
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime, trackId;
    BOOL gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0, frameCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR;

    CreateStreamSync();

    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + 2 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        // Every 5 frames simulate a video frame
        trackId = (frameCount % 5 == 0) ? TEST_VIDEO_TRACK_ID : TEST_AUDIO_TRACK_ID;

        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame(FALSE, trackId)) << "frameCount " << frameCount;
        frameCount++;

        // output the EOFR
        if (frameCount % mMockProducerConfig.mKeyFrameInterval == 0) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &eofrFrame));
        }

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }

        // Increment by the frame duration
        incrementTestTimeVal(1000 / mMockProducerConfig.mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    } while (currentTime < testTerminationTime + 1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

TEST_P(StreamApiFunctionalityScenarioTest, TokenRotationBasicMultiTrackPassThroughScenarioEofr)
{
    CreateScenarioTestClient();
    mMockProducerConfig.mSetEOFR = TRUE;

    UINT64 currentTime, testTerminationTime, trackId;
    BOOL didPutFrame, gotStreamData, submittedAck;
    UINT32 tokenRotateCount = 0, frameCount = 0;
    std::vector<UPLOAD_HANDLE> currentUploadHandles;
    MockConsumer *mockConsumer;

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    CreateStreamSync();

    // should make 1 token rotations within this time.
    testTerminationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this)
                          + 1 * MIN_STREAMING_TOKEN_EXPIRATION_DURATION + 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

    do {
        currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        // Every 5 frames simulate a video frame
        trackId = (frameCount % 5 == 0) ? TEST_VIDEO_TRACK_ID : TEST_AUDIO_TRACK_ID;
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.timedPutFrame(currentTime, &didPutFrame, trackId));
        frameCount++;

        mStreamingSession.getActiveUploadHandles(currentUploadHandles);
        for (int i = 0; i < currentUploadHandles.size(); i++) {
            UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
            mockConsumer = mStreamingSession.getConsumer(uploadHandle);

            STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
            VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            if (retStatus == STATUS_END_OF_STREAM) {
                tokenRotateCount++;
            }
            if (mockConsumer != NULL) {
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &submittedAck));
            }
        }
    } while (currentTime < testTerminationTime);

    EXPECT_EQ(tokenRotateCount, 1);

    VerifyStopStreamSyncAndFree();
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, StreamApiFunctionalityScenarioTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE), Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR), Bool(), Values(0, TEST_REPLAY_DURATION)));
