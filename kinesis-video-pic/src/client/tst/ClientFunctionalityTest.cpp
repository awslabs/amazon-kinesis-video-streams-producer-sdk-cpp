#include "ClientTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class ClientFunctionalityTest : public ClientTestBase,
                                public WithParamInterface< ::std::tuple<STREAMING_TYPE, uint64_t, bool, uint64_t, DEVICE_STORAGE_TYPE> >{
protected:
    void SetUp() {
        ClientTestBase::SetUp();

        STREAMING_TYPE streamingType;
        bool enableAck;
        uint64_t retention, replayDuration;
        DEVICE_STORAGE_TYPE storageType;
        std::tie(streamingType, retention, enableAck, replayDuration, storageType) = GetParam();
        mStreamInfo.retention = (UINT64) retention;
        mStreamInfo.streamCaps.streamingType = streamingType;
        mStreamInfo.streamCaps.fragmentAcks = enableAck;
        mStreamInfo.streamCaps.replayDuration = (UINT64) replayDuration;
        mDeviceInfo.storageInfo.storageType = storageType;
    }
};

TEST_P(ClientFunctionalityTest, CreateAwaitReadyAndFreeClient)
{
    CreateScenarioTestClient();
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, CreateSyncAndFree)
{
    TID thread;
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, CreateClientRoutineSync, (PVOID) this));

    // Satisfy the create device callback after the create device call
    UINT64 iterateTime = GETTIME() + HUNDREDS_OF_NANOS_IN_A_SECOND;

    while (ATOMIC_LOAD(&mCreateDeviceDoneFuncCount) != 2 && GETTIME() <= iterateTime) {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    EXPECT_NE(0, ATOMIC_LOAD(&mCreateDeviceDoneFuncCount));

    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    THREAD_JOIN(thread, NULL);

    // Ensure the client ready is called
    EXPECT_TRUE(ATOMIC_LOAD_BOOL(&mClientReady));
    EXPECT_EQ(mClientHandle, mReturnedClientHandle);
}

TEST_P(ClientFunctionalityTest, CreateAndFree)
{
    // Free the existing client
    if (IS_VALID_CLIENT_HANDLE(mClientHandle)) {
        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    }

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &mClientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
}

TEST_P(ClientFunctionalityTest, CreateClientCreateStreamFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, CreateStream());

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, CreateClientCreateStreamStopStreamFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, CreateStream());

    EXPECT_EQ(0, ATOMIC_LOAD(&mClientShutdownFuncCount));
    EXPECT_FALSE(ATOMIC_LOAD_BOOL(&mClientShutdown));
    EXPECT_EQ(0, ATOMIC_LOAD(&mStreamShutdownFuncCount));
    EXPECT_FALSE(ATOMIC_LOAD_BOOL(&mStreamShutdown));

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
    EXPECT_EQ(1, ATOMIC_LOAD(&mClientShutdownFuncCount));
    EXPECT_TRUE(ATOMIC_LOAD_BOOL(&mClientShutdown));
    EXPECT_EQ(1, ATOMIC_LOAD(&mStreamShutdownFuncCount));
    EXPECT_TRUE(ATOMIC_LOAD_BOOL(&mStreamShutdown));
}

TEST_P(ClientFunctionalityTest, createClientCreateStreamSyncFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, CreateStreamSync());

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, CreateClientCreateStreamSyncStopStreamFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, CreateStreamSync());
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create Producer, Create Streams, Await Ready, Put Frame, Free Producer
TEST_P(ClientFunctionalityTest, CreateClientCreateStreamPutFrameFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, ReadyStream());

    // mockProducer need to be destructed before client in case of DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC
    // because freeKinesisVideoClient also frees mockProducer MEMALLOC
    {
        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

        //Stream is ready by now, so putFrame should succeed
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }


    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create Producer, Create Streams, Await Ready, Put Frame, Stop Stream, Free Producer
TEST_P(ClientFunctionalityTest, CreateClientCreateStreamPutFrameStopStreamFreeClient)
{
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    EXPECT_EQ(STATUS_SUCCESS, ReadyStream());

    // mockProducer need to be destructed before client in case of DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC
    // because freeKinesisVideoClient also frees mockProducer MEMALLOC
    {
        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

        //Stream is ready by now, so putFrame should succeed
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
    EXPECT_TRUE(ATOMIC_LOAD(&mDroppedFrameReportFuncCount) > 0); // the frame put should be dropped.
}

//Create Producer, Create Streams Sync, Await Ready, Put Frame, Free Producer
TEST_P(ClientFunctionalityTest, CreateClientCreateStreamSyncPutFrameFreeClient)
{
    TID thread;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // disable auto submission so that CreateStreamSync will block
    mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;
    mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, CreateStreamSyncRoutine, (PVOID) this));

    // wait until stream has been created so that we can submit describeStreamResult
    UINT64 iterateTime = GETTIME() + HUNDREDS_OF_NANOS_IN_A_SECOND;

    while (ATOMIC_LOAD(&mDescribeStreamDoneFuncCount) == 0 && GETTIME() <= iterateTime) {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    EXPECT_NE(0, ATOMIC_LOAD(&mDescribeStreamDoneFuncCount));

    // enable auto submission so that submitting describeStreamResult will trigger stream ready, and unblock
    // CreateStreamSyncRoutine
    mSubmitServiceCallResultMode = STOP_AT_GET_STREAMING_TOKEN;
    setupStreamDescription();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    THREAD_JOIN(thread, NULL);
    EXPECT_TRUE(ATOMIC_LOAD(&mStreamReadyFuncCount) > 0);

    // mockProducer need to be destructed before client in case of DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC
    // because freeKinesisVideoClient also frees mockProducer MEMALLOC
    {
        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

        //Stream is ready by now, so putFrame should succeed
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create Producer, Create Streams Sync, Await Ready, Put Frame, Stop Stream, Free Producer
TEST_P(ClientFunctionalityTest, CreateClientCreateStreamSyncPutFrameStopStreamFreeClient)
{
    TID thread;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    // disable auto submission so that CreateStreamSync will block
    mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;
    mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, CreateStreamSyncRoutine, (PVOID) this));

    // wait until stream has been created so that we can submit describeStreamResult
    UINT64 iterateTime = GETTIME() + HUNDREDS_OF_NANOS_IN_A_SECOND;

    while (ATOMIC_LOAD(&mDescribeStreamDoneFuncCount) == 0 && GETTIME() <= iterateTime) {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    EXPECT_NE(0, ATOMIC_LOAD(&mDescribeStreamDoneFuncCount));

    // enable auto submission so that submitting describeStreamResult will trigger stream ready, and unblock
    // CreateStreamSyncRoutine
    mSubmitServiceCallResultMode = STOP_AT_GET_STREAMING_TOKEN;
    setupStreamDescription();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));
    THREAD_JOIN(thread, NULL);
    EXPECT_TRUE(ATOMIC_LOAD(&mStreamReadyFuncCount) > 0);

    // mockProducer need to be destructed before client in case of DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC
    // because freeKinesisVideoClient also frees mockProducer MEMALLOC
    {
        MockProducer mockProducer(mMockProducerConfig, mStreamHandle);

        //Stream is ready by now, so putFrame should succeed
        EXPECT_EQ(STATUS_SUCCESS, mockProducer.putFrame());
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStream(mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create producer, create streams sync, create same stream and fail, free client
TEST_P(ClientFunctionalityTest, CreateClientCreateStreamSyncCreateSameStreamAndFailFreeClient)
{
    TID thread;
    CreateScenarioTestClient();

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, CreateStreamSyncRoutine, (PVOID) this));

    // wait until stream has been created so that we can submit describeStreamResult
    UINT64 iterateTime = GETTIME() + HUNDREDS_OF_NANOS_IN_A_SECOND;

    while (ATOMIC_LOAD(&mDescribeStreamDoneFuncCount) == 0 && GETTIME() <= iterateTime) {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    EXPECT_NE(0, ATOMIC_LOAD(&mDescribeStreamDoneFuncCount));

    // this should unblock CreateStreamSyncRoutine
    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;
    setupStreamDescription();
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

    THREAD_JOIN(thread, NULL);
    EXPECT_TRUE(ATOMIC_LOAD(&mStreamReadyFuncCount) > 0);

    // creating the same stream again should fail
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create Stream, call streamformatchanged twice with audio and video. make sure proper mkv header is generated.
TEST_P(ClientFunctionalityTest, StreamFormatChangedAudioVideoCorrectMkvHeader)
{
    CreateScenarioTestClient();

    TrackInfo trackInfo[2];
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_VIDEO;

    trackInfo[1].trackId = 2;
    trackInfo[1].codecPrivateDataSize = 0;
    trackInfo[1].codecPrivateData = NULL;
    STRNCPY(trackInfo[1].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[1].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[1].trackType = MKV_TRACK_INFO_TYPE_AUDIO;

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 2;

    BYTE audioCpd[] = {0x11, 0x90, 0x56, 0xe5, 0x00};
    BYTE videoCpd[] = {0x01, 0x4d, 0x00, 0x20, 0xff, 0xe1, 0x00, 0x22, 0x27, 0x4d, 0x00, 0x20, 0x89,
                       0x8b, 0x60, 0x28, 0x02, 0xdd, 0x80, 0x88, 0x00, 0x01, 0x38, 0x80, 0x00, 0x3d,
                       0x09, 0x07, 0x03, 0x00, 0x05, 0xdc, 0x00, 0x01, 0x77, 0x05, 0xef, 0x7c, 0x1f,
                       0x08, 0x84, 0x6e, 0x01, 0x00, 0x04, 0x28, 0xee, 0x1f, 0x20};

    STRNCPY(mStreamInfo.streamCaps.contentType, TEST_AUDIO_VIDEO_CONTENT_TYPE, MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, SIZEOF(audioCpd), audioCpd, 2));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, SIZEOF(videoCpd), videoCpd, 1));

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_EQ(1280, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(720, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    EXPECT_TRUE(48000 == pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(2, pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

//Create Stream, call streamformatchanged twice with audio and video. make sure proper mkv header is generated.
TEST_P(ClientFunctionalityTest, StreamFormatChangedPcmAudioVideoCorrectMkvHeader)
{
    CreateScenarioTestClient();

    TrackInfo trackInfo[2];
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_VIDEO;

    trackInfo[1].trackId = 2;
    trackInfo[1].codecPrivateDataSize = 0;
    trackInfo[1].codecPrivateData = NULL;
    STRNCPY(trackInfo[1].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[1].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[1].trackType = MKV_TRACK_INFO_TYPE_AUDIO;

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 2;

    BYTE audioCpd[] = {0x06, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00};
    BYTE videoCpd[] = {0x01, 0x4d, 0x00, 0x20, 0xff, 0xe1, 0x00, 0x22, 0x27, 0x4d, 0x00, 0x20, 0x89,
                       0x8b, 0x60, 0x28, 0x02, 0xdd, 0x80, 0x88, 0x00, 0x01, 0x38, 0x80, 0x00, 0x3d,
                       0x09, 0x07, 0x03, 0x00, 0x05, 0xdc, 0x00, 0x01, 0x77, 0x05, 0xef, 0x7c, 0x1f,
                       0x08, 0x84, 0x6e, 0x01, 0x00, 0x04, 0x28, 0xee, 0x1f, 0x20};

    STRNCPY(mStreamInfo.streamCaps.contentType, "video/h264,audio/alaw", MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, SIZEOF(audioCpd), audioCpd, 2));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, SIZEOF(videoCpd), videoCpd, 1));

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_EQ(1280, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(720, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    EXPECT_TRUE(8000 == pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(1, pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(16, pStreamMkvGenerator->trackInfoList[1].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, StreamFormatChangedPcmAudioDirectCpdPassingCorrectTrackAudioConfig)
{
    CreateScenarioTestClient();

    TrackInfo trackInfo[1];
    BYTE audioCpd[] = {0x06, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00};
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo[0].codecPrivateData = audioCpd;
    trackInfo[0].codecPrivateDataSize = SIZEOF(audioCpd);

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 1;

    STRNCPY(mStreamInfo.streamCaps.contentType, "audio/alaw", MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_TRUE(8000 == pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(1, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(16, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, StreamFormatChangedGeneratedPcmAlawAudioDirectCpdPassingCorrectTrackAudioConfig)
{
    CreateScenarioTestClient();
    BYTE cpd[KVS_PCM_CPD_SIZE_BYTE];
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGeneratePcmCpd(KVS_PCM_FORMAT_CODE_ALAW,
            8000,
            2,
            cpd,
            cpdSize));

    TrackInfo trackInfo[1];
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo[0].codecPrivateData = cpd;
    trackInfo[0].codecPrivateDataSize = cpdSize;

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 1;

    STRNCPY(mStreamInfo.streamCaps.contentType, "audio/alaw", MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_EQ(8000, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(2, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, StreamFormatChangedGeneratedPcmMlawAudioDirectCpdPassingCorrectTrackAudioConfig)
{
    CreateScenarioTestClient();
    BYTE cpd[KVS_PCM_CPD_SIZE_BYTE];
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGeneratePcmCpd(KVS_PCM_FORMAT_CODE_MULAW,
                                                   8000,
                                                   2,
                                                   cpd,
                                                   cpdSize));

    TrackInfo trackInfo[1];
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo[0].codecPrivateData = cpd;
    trackInfo[0].codecPrivateDataSize = cpdSize;

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 1;

    STRNCPY(mStreamInfo.streamCaps.contentType, "audio/mulaw", MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_EQ(8000, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(2, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

TEST_P(ClientFunctionalityTest, StreamFormatChangedGeneratedAacAudioDirectCpdPassingCorrectTrackAudioConfig)
{
    CreateScenarioTestClient();
    BYTE cpd[KVS_AAC_CPD_SIZE_BYTE];
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateAacCpd(AAC_MAIN,
                                                   16000,
                                                   2,
                                                   cpd,
                                                   cpdSize));

    TrackInfo trackInfo[1];
    trackInfo[0].trackId = 1;
    trackInfo[0].codecPrivateDataSize = 0;
    trackInfo[0].codecPrivateData = NULL;
    STRNCPY(trackInfo[0].codecId, TEST_AUDIO_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
    STRNCPY(trackInfo[0].trackName, TEST_AUDIO_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
    trackInfo[0].trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo[0].codecPrivateData = cpd;
    trackInfo[0].codecPrivateDataSize = cpdSize;

    mStreamInfo.streamCaps.trackInfoList = trackInfo;
    mStreamInfo.streamCaps.trackInfoCount = 1;

    STRNCPY(mStreamInfo.streamCaps.contentType, "audio/aac", MAX_CONTENT_TYPE_LEN);

    PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE();

    CreateStreamSync();

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    EXPECT_TRUE(pKinesisVideoStream != NULL);
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pKinesisVideoStream->pMkvGenerator;
    EXPECT_TRUE(pStreamMkvGenerator != NULL);

    EXPECT_EQ(16000, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(2, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackAudioConfig.bitDepth);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));
    EXPECT_TRUE(!IS_VALID_CLIENT_HANDLE(mClientHandle));
}

INSTANTIATE_TEST_CASE_P(PermutatedStreamInfo, ClientFunctionalityTest,
                        Combine(Values(STREAMING_TYPE_REALTIME, STREAMING_TYPE_OFFLINE),
                                Values(0, 10 * HUNDREDS_OF_NANOS_IN_AN_HOUR),
                                Bool(),
                                Values(0, TEST_REPLAY_DURATION),
                                Values(DEVICE_STORAGE_TYPE_IN_MEM, DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC)));
