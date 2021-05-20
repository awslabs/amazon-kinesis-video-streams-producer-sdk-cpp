#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class IntermittentProducerAutomaticStreamingTest : public ClientTestBase,
                                    public WithParamInterface< ::std::tuple<UINT64, UINT32> > {
protected:
        StreamInfo mStreamInfo2;
        TrackInfo mTrackInfo2;

    void SetUp() {
        ClientTestBase::SetUpWithoutClientCreation();
        UINT64 callbackInterval;
        UINT32 clientInfoVersion;
        std::tie(callbackInterval, clientInfoVersion) = GetParam();
        mDeviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION;
        mDeviceInfo.clientInfo.reservedCallbackPeriod = (UINT64) callbackInterval * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
        mDeviceInfo.clientInfo.automaticStreamingFlags = AUTOMATIC_STREAMING_INTERMITTENT_PRODUCER;
        mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;
        mStreamInfo.streamCaps.absoluteFragmentTimes = TRUE;
    }

    void initStreamInfo2() {
        // Initialize stream info
        mStreamInfo2.version = STREAM_INFO_CURRENT_VERSION;
        mStreamInfo2.tagCount = 0;
        mStreamInfo2.retention = 0;
        mStreamInfo2.tags = NULL;
        STRNCPY(mStreamInfo2.name, "test_abc_xyz", 13);
        mStreamInfo2.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
        STRNCPY(mStreamInfo2.streamCaps.contentType, TEST_CONTENT_TYPE, MAX_CONTENT_TYPE_LEN);
        mStreamInfo2.streamCaps.adaptive = FALSE;
        mStreamInfo2.streamCaps.maxLatency = STREAM_LATENCY_PRESSURE_CHECK_SENTINEL;
        mStreamInfo2.streamCaps.fragmentDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
        mStreamInfo2.streamCaps.keyFrameFragmentation = TRUE;
        mStreamInfo2.streamCaps.frameTimecodes = TRUE;
        mStreamInfo2.streamCaps.absoluteFragmentTimes = TRUE;
        mStreamInfo2.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
        mStreamInfo2.streamCaps.fragmentAcks = TRUE;
        mStreamInfo2.streamCaps.recoverOnError = FALSE;
        mStreamInfo2.streamCaps.recalculateMetrics = TRUE;
        mStreamInfo2.streamCaps.avgBandwidthBps = 15 * 1000000;
        mStreamInfo2.streamCaps.frameRate = 24;
        mStreamInfo2.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
        mStreamInfo2.streamCaps.replayDuration = TEST_REPLAY_DURATION;
        mStreamInfo2.streamCaps.timecodeScale = 0;
        mStreamInfo2.streamCaps.trackInfoCount = 1;
        mStreamInfo2.streamCaps.segmentUuid = TEST_SEGMENT_UUID;
        mStreamInfo2.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;
        mStreamInfo2.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_OOM;
        mStreamInfo2.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;

        mTrackInfo2.trackId = TEST_TRACKID;
        mTrackInfo2.codecPrivateDataSize = 0;
        mTrackInfo2.codecPrivateData = NULL;
        STRNCPY(mTrackInfo2.codecId, TEST_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
        STRNCPY(mTrackInfo2.trackName, TEST_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
        mTrackInfo2.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        mTrackInfo2.trackCustomData.trackVideoConfig.videoWidth = 1280;
        mTrackInfo2.trackCustomData.trackVideoConfig.videoHeight = 720;

        mStreamInfo2.streamCaps.trackInfoList = &mTrackInfo2;
    }

};

STATUS timerCallbackPreHook(UINT64 hookCustomData)
{
    STATUS retStatus = STATUS_SUCCESS;
    IntermittentProducerAutomaticStreamingTest* pTest = (IntermittentProducerAutomaticStreamingTest*) hookCustomData;
    CHECK(pTest != NULL);
    ATOMIC_INCREMENT(&pTest->mTimerCallbackFuncCount);
    return retStatus;
};

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateTimerInvokedBeforeTime) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);


    EXPECT_EQ(0, ATOMIC_LOAD(&mTimerCallbackFuncCount));

    // Create synchronously
    CreateStreamSync();

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(1.2 * INTERMITTENT_PRODUCER_TIMER_START_DELAY);

    EXPECT_EQ(1, ATOMIC_LOAD(&mTimerCallbackFuncCount));
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateTimerInvokedAfterFirstPeriod) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

// Create synchronously
    CreateStreamSync();

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY + 1.20 * mDeviceInfo.clientInfo.reservedCallbackPeriod);

    EXPECT_EQ(2, ATOMIC_LOAD(&mTimerCallbackFuncCount));
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateTimerContinueInvocationsAfterStopStream) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

// Create synchronously
    CreateStreamSync();

    mStreamUploadHandle = 0;

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, notifyStreamClosed(FROM_STREAM_HANDLE(mStreamHandle), mStreamUploadHandle));

    THREAD_SLEEP(2 *  HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));

    int beforeCount = ATOMIC_LOAD(&mTimerCallbackFuncCount);
    int m = 3;

    THREAD_SLEEP(m * mDeviceInfo.clientInfo.reservedCallbackPeriod);

    int afterCount = ATOMIC_LOAD(&mTimerCallbackFuncCount);
    // make sure call back is still firing after this stream is closed out
    EXPECT_TRUE(afterCount > beforeCount);

    // make sure call back isn't over-firing
    EXPECT_TRUE(afterCount <= beforeCount + m + 1);
}



TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateTimerNoMoreInvocationsAfterFreeClient) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // Create synchronously
    CreateStreamSync();

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    freeKinesisVideoClient(&mClientHandle);

    int val = ATOMIC_LOAD(&mTimerCallbackFuncCount);

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY + 5 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    EXPECT_EQ(val, ATOMIC_LOAD(&mTimerCallbackFuncCount));
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateTimerNoInvocationsWithAutoStreamingOFF) {

    mDeviceInfo.clientInfo.automaticStreamingFlags = AUTOMATIC_STREAMING_ALWAYS_CONTINUOUS;

    // Validate timerqueue is never constructed
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());
    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);


    // Create synchronously
    CreateStreamSync();

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY + 2 * mDeviceInfo.clientInfo.reservedCallbackPeriod);

    EXPECT_TRUE(!IS_VALID_TIMER_QUEUE_HANDLE(client->timerQueueHandle));
    EXPECT_EQ(0, ATOMIC_LOAD(&mTimerCallbackFuncCount));
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateLastUpdateTimeOfStreamUpdated) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // Create synchronously
    CreateStreamSync();
    PKinesisVideoStream stream = FROM_STREAM_HANDLE(mStreamHandle);

    // Lock the Stream because PIC can be reading/writing this value as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    EXPECT_EQ(stream->lastPutFrameTimestamp, INVALID_TIMESTAMP_VALUE);
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

// Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = TEST_FRAME_DURATION;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Lock the Stream because PIC can be reading/writing this value as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    UINT64 firstFrameTs = stream->lastPutFrameTimestamp;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

    UINT64 waitTime = 0;
    EXPECT_TRUE(IS_VALID_TIMESTAMP(firstFrameTs));

    EXPECT_EQ(0, ATOMIC_LOAD(&mTimerCallbackFuncCount));

    while(ATOMIC_LOAD(&mTimerCallbackFuncCount) < 1 && waitTime < 1.5*INTERMITTENT_PRODUCER_TIMER_START_DELAY) {
        THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        waitTime += 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    }

    int callbackInvCount = ATOMIC_LOAD(&mTimerCallbackFuncCount);
    // sleep for one invocation, verify we haven't updated the lastPutFrameTimestamp
    THREAD_SLEEP(1.20 * mDeviceInfo.clientInfo.reservedCallbackPeriod);
    // Verify callback executed 1 additional time and lastPutFrameTimestamp did NOT get updated
    EXPECT_TRUE(callbackInvCount + 1 == ATOMIC_LOAD(&mTimerCallbackFuncCount));
    // Lock the Stream because PIC can be reading/writing this value as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    EXPECT_TRUE(firstFrameTs == stream->lastPutFrameTimestamp);
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

    // Sleep long enough to reach timeout
    THREAD_SLEEP(INTERMITTENT_PRODUCER_MAX_TIMEOUT);
    // Lock the Stream because PIC can be reading/writing this value as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    UINT64 eoFrFrameTs = stream->lastPutFrameTimestamp;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

    EXPECT_TRUE(eoFrFrameTs > firstFrameTs);
}

TEST_P(IntermittentProducerAutomaticStreamingTest, MultiTrackVerifyNoInvocationsWithSingleTrackProducer) {
    // Create new client so param value of callbackPeriod can be applied
    mStreamInfo.streamCaps.absoluteFragmentTimes = FALSE;
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // setup multi-track
    // Create multi-track configuration
    TrackInfo tracks[2];
    // Copy the forward the default track info to 0th and 1st index and modify
    tracks[0] = mTrackInfo;
    tracks[1] = mTrackInfo;
    tracks[0].trackId = TEST_VIDEO_TRACK_ID;
    tracks[1].trackId = TEST_AUDIO_TRACK_ID;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = tracks;

    // This will fail for FRAME_ORDERING_MODE_MULTI_TRACK_AV, we will get STATUS_END_OF_FRAGMENT_FRAME_INVALID_STATE
    // when we try to submit a frame
    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR;

    // Create synchronously
    CreateStreamSync();
    PKinesisVideoStream stream = FROM_STREAM_HANDLE(mStreamHandle);
    STREAM_HANDLE streamHandle = mStreamHandle;

    // Lock the Stream because PIC can be reading/writing this value as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    EXPECT_EQ(stream->lastPutFrameTimestamp, INVALID_TIMESTAMP_VALUE);
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.size = SIZEOF(temp);
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;
    frame.frameData = temp;

    int frameCount = 0;
    UINT64 startTime;
    UINT64 ts;

    UINT64 expectedFramesInTimeout = INTERMITTENT_PRODUCER_MAX_TIMEOUT/HUNDREDS_OF_NANOS_IN_A_SECOND;
    UINT64 track2FrameMax = expectedFramesInTimeout/4;

    UINT64 expectedCalls = 1.25*INTERMITTENT_PRODUCER_MAX_TIMEOUT / mDeviceInfo.clientInfo.reservedCallbackPeriod;

    for ( ts = 0; ts < 1.5*INTERMITTENT_PRODUCER_MAX_TIMEOUT; ts+= HUNDREDS_OF_NANOS_IN_A_SECOND ) {
        startTime = GETTIME();
        frame.presentationTs = ts;
        frame.decodingTs = ts;
        frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;
        frame.index = frameCount;
        frame.flags = frameCount % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        frame.trackId = 1;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // only put frames in track 2 for first 5s
        if ( frameCount <= track2FrameMax ) {
            frame.trackId = 2;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
        }

        frameCount++;
        UINT64 diff = GETTIME()-startTime;
        if ( diff < HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND - diff);
        }
    }

    EXPECT_TRUE(ATOMIC_LOAD(&mTimerCallbackFuncCount) >= expectedCalls);

    // Lock the Stream because PIC can be reading/writing these values as well
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
    EXPECT_TRUE(stream->eofrFrame);
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateNoConsecutiveEOFR) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;
    frame.frameData = temp;
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

    // Create synchronously
    CreateStreamSync();
    PKinesisVideoStream stream = FROM_STREAM_HANDLE(mStreamHandle);
    STREAM_HANDLE streamHandle = mStreamHandle;

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY);

    UINT64 ts, startTime;
    int frameCountS1 = 0;

    /*
     *      |
     *      |
     *  S1  |*****                               *************
     *      -----------------------------------------------------
     *    t=0    5                              55          60
     *
     *    `*` denotes frames are being produced, gaps mean no frames.
     *    s1 produces frames from t=0 to t=5, then stops for 50s, at this point
     *    we assume INTERMITTENT_PRODUCER_MAX_TIMEOUT = 20s, so by this time
     *    the callback should have fired with an EOFR once, but we wait an additional
     *    time period to make sure we don't accidentally send eofr twice (this would result in
     *    an error in put frame).
     */


    for (ts = 0; ts < 60 * HUNDREDS_OF_NANOS_IN_A_SECOND; ts += HUNDREDS_OF_NANOS_IN_A_SECOND) {
        startTime = GETTIME();
        frame.presentationTs = ts;
        frame.decodingTs = ts;
        frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

        if ( ts < 5 * HUNDREDS_OF_NANOS_IN_A_SECOND || ts > 55 * HUNDREDS_OF_NANOS_IN_A_SECOND) {
            frame.flags = frameCountS1 % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
            frame.index = frameCountS1++;

            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
            EXPECT_EQ(0, ATOMIC_LOAD(&mStreamErrorReportFuncCount));
        }

        UINT64 diff = GETTIME()-startTime;
        if ( diff < HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND - diff);
        }
    }
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateErrorOnForceConsecutiveEOFR) {
    // Create new client so param value of callbackPeriod can be applied
    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;
    frame.frameData = temp;
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

    // Create synchronously
    CreateStreamSync();
    PKinesisVideoStream stream = FROM_STREAM_HANDLE(mStreamHandle);
    STREAM_HANDLE streamHandle = mStreamHandle;

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY);

    UINT64 ts, startTime;
    int frameCountS1 = 0;

    /*
     *      |
     *      |
     *  S1  |*****                        +++++++++
     *      --------------------------------------------
     *    t=0    5                        35      40
     *
     *    '*' denotes frames are being produced, gaps mean no frames.
     *    '+' denotes we manually submit eofr, expect error
     *    s1 produces frames from t=0 to t=5, then stops for 30s, at this point
     *    we assume INTERMITTENT_PRODUCER_MAX_TIMEOUT = 20s, so by this time
     *    the callback should have fired with an EOFR once, now we manually
     *    fire eofr frames and expect error
     */

    Frame eofrFrame = EOFR_FRAME_INITIALIZER;

    for (ts = 0; ts < 2 * INTERMITTENT_PRODUCER_MAX_TIMEOUT; ts += HUNDREDS_OF_NANOS_IN_A_SECOND) {
        startTime = GETTIME();
        frame.presentationTs = ts;
        frame.decodingTs = ts;
        frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

        if ( ts < 5 * HUNDREDS_OF_NANOS_IN_A_SECOND) {
            frame.flags = frameCountS1 % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
            frame.index = frameCountS1++;

            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
            EXPECT_EQ(0, ATOMIC_LOAD(&mStreamErrorReportFuncCount));
        }


        if ( ts >= 35 * HUNDREDS_OF_NANOS_IN_A_SECOND) {
            // Lock the Stream because PIC can be reading/writing this value as well
            client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream->base.lock);
            EXPECT_TRUE(stream->eofrFrame);
            client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream->base.lock);

            // attempt to put another eofr, expect this failure
            EXPECT_EQ(STATUS_MULTIPLE_CONSECUTIVE_EOFR, putKinesisVideoFrame(streamHandle, &eofrFrame));
        }

        UINT64 diff = GETTIME()-startTime;
        if ( diff < HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND - diff);
        }
    }
}

TEST_P(IntermittentProducerAutomaticStreamingTest, ValidateMultiStream) {
    // Create new client so param value of callbackPeriod can be applied
    mClientCallbacks.describeStreamFn = describeStreamMultiStreamFunc;
    mClientCallbacks.getStreamingEndpointFn = getStreamingEndpointMultiStreamFunc;
    mClientCallbacks.putStreamFn = putStreamMultiStreamFunc;
    mClientCallbacks.getStreamingTokenFn = getStreamingTokenMultiStreamFunc;

    mClientCallbacks.streamReadyFn = NULL;


    ASSERT_EQ(STATUS_SUCCESS, CreateClient());

    PKinesisVideoClient client = FROM_CLIENT_HANDLE(mClientHandle);
    // Lock client before setting hook custom data and callback because PIC reads these values
    client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, client->base.lock);
    client->hookCustomData = (UINT64) this;
    client->timerCallbackPreHookFunc = timerCallbackPreHook;
    client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, client->base.lock);

    // Create 2 streams synchronously
    // the callbacks all reset mStreamHandle, so creating sh2 first

    // Produce a frame
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;
    frame.frameData = temp;
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

    // DO NOT use mStreamHandle for this test... somewhere when calling PutFrame
    // Using the other handle that value will get overwritten.
    // For multi-stream case we should always use locally created stream handles
    STREAM_HANDLE sh1 = INVALID_STREAM_HANDLE_VALUE;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &sh1));
    PKinesisVideoStream stream1 = FROM_STREAM_HANDLE(sh1);

    STREAM_HANDLE sh2 = INVALID_STREAM_HANDLE_VALUE;
    initStreamInfo2();
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo2, &sh2));
    PKinesisVideoStream stream2 = FROM_STREAM_HANDLE(sh2);

    THREAD_SLEEP(INTERMITTENT_PRODUCER_TIMER_START_DELAY);

    EXPECT_NE(sh1, sh2);

    UINT64 ts, startTime;
    int frameCountS1 = 0, frameCountS2 = 0;

    /*
     *      |
     *  S2  |****************                                  ******************
     *      |
     *      |
     *      |
     *  S1  |*****                                ******************************
     *      -----------------------------------------------------------------------------------
     *    t=0    5          15                    35          45                60
     *
     *    `*` denotes frames are being produced, gaps mean no frames.
     *    stream1 produces frames from t=0 to t=5, then stops for 30s, at this point
     *    we assume INTERMITTENT_PRODUCER_MAX_TIMEOUT = 20s, so by this time
     *    the callback should have fired with an Eofr for s1 only, s2 should still have NO eofr
     *    s2 is producing frames until t = 15, at t=45, 30s has passed so we need to validate
     *    that eofr frame has been sent, but at t = 45, s1 is getting frames and we validate there
     *    is NO eofr.  Then both streams keep producing streams until t = 60s, no eofrs on either stream
     */


    for (ts = 0; ts < 3 * INTERMITTENT_PRODUCER_MAX_TIMEOUT; ts += HUNDREDS_OF_NANOS_IN_A_SECOND) {
        startTime = GETTIME();
        frame.presentationTs = ts;
        frame.decodingTs = ts;
        frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND;

        if ( ts < 5 * HUNDREDS_OF_NANOS_IN_A_SECOND || ts > 35 * HUNDREDS_OF_NANOS_IN_A_SECOND) {
            frame.flags = frameCountS1 % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
            frame.index = frameCountS1++;

            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(sh1, &frame));

            if ( (ts > 40 * HUNDREDS_OF_NANOS_IN_A_SECOND) ) {
                // We should no longer have eofr
                // Lock the Stream because PIC can be reading/writing this value as well
                client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream1->base.lock);
                EXPECT_FALSE(stream1->eofrFrame);
                client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream1->base.lock);
            }
        }

        // expect s1 to be in eofr state
        if ( ts < 35 * HUNDREDS_OF_NANOS_IN_A_SECOND && ts > 30 * HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            // Lock the Stream because PIC can be reading/writing this value as well
            client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream1->base.lock);
            EXPECT_TRUE(stream1->eofrFrame);
            client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream1->base.lock);
        }

        if ( ts < 15 * HUNDREDS_OF_NANOS_IN_A_SECOND || (ts > 45 * HUNDREDS_OF_NANOS_IN_A_SECOND)) {
            frame.flags = frameCountS2 % 5 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
            frame.index = frameCountS2++;

            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(sh2, &frame));

            if ( (ts > 50 * HUNDREDS_OF_NANOS_IN_A_SECOND) ) {
                // Lock the Stream because PIC can be reading/writing this value as well
                client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream2->base.lock);
                // We should no longer have eofr
                EXPECT_FALSE(stream2->eofrFrame);
                client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream2->base.lock);
            }
        }

        // expect s2 to be in eofr state
        if ( ts < 45 * HUNDREDS_OF_NANOS_IN_A_SECOND && ts > 40 * HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            // Lock the Stream because PIC can be reading/writing this value as well
            client->clientCallbacks.lockMutexFn(client->clientCallbacks.customData, stream2->base.lock);
            EXPECT_TRUE(stream2->eofrFrame);
            client->clientCallbacks.unlockMutexFn(client->clientCallbacks.customData, stream2->base.lock);
        }

        UINT64 diff = GETTIME()-startTime;
        if ( diff < HUNDREDS_OF_NANOS_IN_A_SECOND ) {
            THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND - diff);
        }
    }
}



INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, IntermittentProducerAutomaticStreamingTest,
        Combine(Values(1000,2000,3000), Values(0, CLIENT_INFO_CURRENT_VERSION)));
