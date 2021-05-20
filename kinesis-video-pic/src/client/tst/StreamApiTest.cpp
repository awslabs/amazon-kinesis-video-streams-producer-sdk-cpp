#include "ClientTestFixture.h"

class StreamApiTest : public ClientTestBase {
};

TEST_F(StreamApiTest, createKinesisVideoStream_NullInput)
{
    STREAM_HANDLE streamHandle;

    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(INVALID_CLIENT_HANDLE_VALUE, &mStreamInfo, &streamHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(mClientHandle, NULL, &streamHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(mClientHandle, &mStreamInfo, NULL)));
}

TEST_F(StreamApiTest, createKinesisVideoStreamSync_NullInput)
{
    STREAM_HANDLE streamHandle;

    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStreamSync(INVALID_CLIENT_HANDLE_VALUE, &mStreamInfo, &streamHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStreamSync(mClientHandle, NULL, &streamHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, NULL)));
}

TEST_F(StreamApiTest, createKinesisVideoStream_MaxStreams)
{
    STREAM_HANDLE streamHandle;
    UINT32 i;

    mStreamInfo.name[0] = '\0';
    for (i = 0; i < MAX_TEST_STREAM_COUNT; i++) {
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    }

    // Ensure it fails now
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_EmptyName)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.name[0] = '\0';
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
}

TEST_F(StreamApiTest, createKinesisVideoStream_InvalidNalAdaptation)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_ANNEXB_NALS | NAL_ADAPTATION_AVCC_NALS;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_InvalidRetentionPeriod)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.retention = 1;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.retention = 1 * HUNDREDS_OF_NANOS_IN_AN_HOUR - 1;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_BufferReplayVariations)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.name[0] = '\0';
    mStreamInfo.streamCaps.bufferDuration = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    mStreamInfo.streamCaps.bufferDuration = 0;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = TEST_BUFFER_DURATION + 1;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    mStreamInfo.streamCaps.bufferDuration = TEST_REPLAY_DURATION;
    mStreamInfo.streamCaps.replayDuration = TEST_REPLAY_DURATION;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_DuplicateNames)
{
    STREAM_HANDLE streamHandle;

    STRCPY(mStreamInfo.name, "ABC");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "DEF");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "GHI");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    STRCPY(mStreamInfo.name, "ABC");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "DEF");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "GHI");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    STRCPY(mStreamInfo.name, "aBC");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "def");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "GHi");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    STRCPY(mStreamInfo.name, "ABC ");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, " DEF");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, " GHI ");
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_ZeroFrameRate)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.streamCaps.frameRate = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_ZeroTimecodeScale)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.name[0] = '\0';
    mStreamInfo.streamCaps.timecodeScale = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.timecodeScale = MAX_TIMECODE_SCALE;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.timecodeScale = MAX_TIMECODE_SCALE + 1;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_CodecPrivateData)
{
    STREAM_HANDLE streamHandle;
    BYTE cpd[100];
    PBYTE tempBuff = (PBYTE) MEMCALLOC(1, MKV_MAX_CODEC_PRIVATE_LEN + 1);

    mStreamInfo.name[0] = '\0';
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateData = cpd;
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateDataSize = SIZEOF(cpd);
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateData = NULL;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateData = tempBuff;
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACK_INDEX].codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN + 1;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    MEMFREE(tempBuff);
}

TEST_F(StreamApiTest, createKinesisVideoStream_InvalidInput)
{
    STREAM_HANDLE streamHandle;

    // Set various callbacks to null and check the behavior
    mStreamInfo.version = STREAM_INFO_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    mStreamInfo.version = STREAM_INFO_CURRENT_VERSION;

    MEMSET(mStreamInfo.name, 'a', (MAX_STREAM_NAME_LEN + 1) * SIZEOF(CHAR));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    STRCPY(mStreamInfo.name, TEST_STREAM_NAME);
}

TEST_F(StreamApiTest, createKinesisVideoStream_InvalidTrackInfoInput)
{
    STREAM_HANDLE streamHandle;

    // Set various callbacks to null and check the behavior
    mStreamInfo.streamCaps.trackInfoCount = MAX_SUPPORTED_TRACK_COUNT_PER_STREAM + 1;
    EXPECT_EQ(STATUS_MAX_TRACK_COUNT_EXCEEDED, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.trackInfoCount = 0;
    EXPECT_EQ(STATUS_TRACK_INFO_MISSING, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.trackInfoCount = 1;
    mStreamInfo.streamCaps.trackInfoList = NULL;
    EXPECT_EQ(STATUS_TRACK_INFO_MISSING, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    // Check for duplicate tracks
    mStreamInfo.streamCaps.trackInfoCount = MAX_SUPPORTED_TRACK_COUNT_PER_STREAM;
    TrackInfo trackInfo[MAX_SUPPORTED_TRACK_COUNT_PER_STREAM];
    mStreamInfo.streamCaps.trackInfoList = trackInfo;

    // All same ids
    for (UINT32 i = 0; i < MAX_SUPPORTED_TRACK_COUNT_PER_STREAM; i++) {
        trackInfo[i].trackId = TEST_TRACKID;
        trackInfo[i].codecPrivateDataSize = 0;
        trackInfo[i].codecPrivateData = NULL;
        STRCPY(trackInfo[i].codecId, TEST_CODEC_ID);
        STRCPY(trackInfo[i].trackName, TEST_TRACK_NAME);
        trackInfo[i].trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        trackInfo[i].trackCustomData.trackVideoConfig.videoWidth = 1280;
        trackInfo[i].trackCustomData.trackVideoConfig.videoHeight = 720;
    }
    EXPECT_EQ(STATUS_DUPLICATE_TRACK_ID_FOUND, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    // Last one is the same as the first one
    for (UINT32 i = 0; i < MAX_SUPPORTED_TRACK_COUNT_PER_STREAM; i++) {
        trackInfo[i].trackId = i;
        trackInfo[i].codecPrivateDataSize = 0;
        trackInfo[i].codecPrivateData = NULL;
        STRCPY(trackInfo[i].codecId, TEST_CODEC_ID);
        STRCPY(trackInfo[i].trackName, TEST_TRACK_NAME);
        trackInfo[i].trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        trackInfo[i].trackCustomData.trackVideoConfig.videoWidth = 1280;
        trackInfo[i].trackCustomData.trackVideoConfig.videoHeight = 720;
    }
    trackInfo[MAX_SUPPORTED_TRACK_COUNT_PER_STREAM - 1].trackId = 0;
    EXPECT_EQ(STATUS_DUPLICATE_TRACK_ID_FOUND, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, freeKinesisVideoStream_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoStream(&streamHandle)));
    EXPECT_TRUE(STATUS_FAILED(freeKinesisVideoStream(NULL)));
}

TEST_F(StreamApiTest, stopKinesisVideoStream_Invalid)
{
    EXPECT_NE(STATUS_SUCCESS, stopKinesisVideoStream(INVALID_STREAM_HANDLE_VALUE));
}

TEST_F(StreamApiTest, stopKinesisVideoStreamSync_Invalid)
{
    EXPECT_NE(STATUS_SUCCESS, stopKinesisVideoStreamSync(INVALID_STREAM_HANDLE_VALUE));
}

TEST_F(StreamApiTest, kinesisVideoPutFrame_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    Frame frame;

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(putKinesisVideoFrame(streamHandle, &frame)));
    EXPECT_TRUE(STATUS_FAILED(putKinesisVideoFrame(mStreamHandle, NULL)));
}

TEST_F(StreamApiTest, kinesisVideoPutFrame_InvalidTrackId)
{
    BYTE tempBuffer[10000];
    UINT64 timestamp = 100;
    Frame frame;

    // Create and ready a stream
    ReadyStream();

    frame.index = 0;
    frame.decodingTs = timestamp;
    frame.presentationTs = timestamp;
    frame.duration = TEST_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.trackId = TEST_INVALID_TRACK_ID; // invalid trackId
    frame.frameData = tempBuffer;
    frame.flags = FRAME_FLAG_KEY_FRAME;

    EXPECT_EQ(STATUS_MKV_TRACK_INFO_NOT_FOUND, putKinesisVideoFrame(mStreamHandle, &frame));
}

TEST_F(StreamApiTest, insertKinesisVideoTag_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN + 2];
    MEMSET(tagName, 'a', MKV_MAX_TAG_NAME_LEN + 1);
    tagName[MKV_MAX_TAG_NAME_LEN + 1] = '\0';

    CHAR tagValue[MKV_MAX_TAG_VALUE_LEN + 2];
    MEMSET(tagValue, 'b', MKV_MAX_TAG_VALUE_LEN + 1);
    tagValue[MKV_MAX_TAG_VALUE_LEN + 1] = '\0';

    // Create a stream
    CreateStream();

    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, NULL, (PCHAR) "tagValue", TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, NULL, (PCHAR) "tagValue", FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "", (PCHAR) "tagValue", TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "", (PCHAR) "tagValue", FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", NULL, TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", NULL, FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, NULL, NULL, TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, NULL, NULL, FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle, NULL, NULL, TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle, NULL, NULL, FALSE));

    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, (PCHAR) "tagValue", TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, (PCHAR) "tagValue", FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", tagValue, TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", tagValue, FALSE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, tagValue, TRUE));
    EXPECT_NE(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, tagValue, FALSE));

    // Validate the negative case with state
    EXPECT_EQ(STATUS_INVALID_STREAM_STATE, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", TRUE));
    EXPECT_EQ(STATUS_INVALID_STREAM_STATE, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", FALSE));
}


TEST_F(StreamApiTest, insertKinesisVideoTag_Invalid_Name)
{
    // Create and ready stream
    ReadyStream();

    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS", (PCHAR) "Tag Value", TRUE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS ", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS ", (PCHAR) "Tag Value", TRUE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWSTag", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWSTag", (PCHAR) "Tag Value", TRUE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS:", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_INVALID_METADATA_NAME, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "AWS:", (PCHAR) "Tag Value", TRUE));

    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "aWS", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "aWS", (PCHAR) "Tag Value", TRUE));
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "aws", (PCHAR) "Tag Value", FALSE));
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "aws", (PCHAR) "Tag Value", TRUE));
}

TEST_F(StreamApiTest, insertKinesisVideoTag_Stream_State_Error) {
    // Create the stream which is not yet in ready state
    CreateStream();

    // Should throw stream state error
    EXPECT_EQ(STATUS_INVALID_STREAM_STATE, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", TRUE));
    EXPECT_EQ(STATUS_INVALID_STREAM_STATE, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", FALSE));
}

TEST_F(StreamApiTest, insertKinesisVideoTag_Non_Persistent_Count) {
    UINT32 i;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN + 1];
    CHAR tagValue[MKV_MAX_TAG_VALUE_LEN + 1];

    // Create and ready a stream
    ReadyStream();

    for (i = 0; i < MAX_FRAGMENT_METADATA_COUNT; i++) {
        SPRINTF(tagName, "tagName_%d", i);
        SPRINTF(tagValue, "tagValue_%d", i);
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, tagValue, FALSE));
    }

    // Adding one more will cause a limit error
    EXPECT_EQ(STATUS_MAX_FRAGMENT_METADATA_COUNT, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", FALSE));
}

TEST_F(StreamApiTest, insertKinesisVideoTag_Persistent_Count) {
    UINT32 i;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN + 1];
    CHAR tagValue[MKV_MAX_TAG_VALUE_LEN + 1];

    // Create and ready a stream
    ReadyStream();

    for (i = 0; i < MAX_FRAGMENT_METADATA_COUNT; i++) {
        SPRINTF(tagName, "tagName_%d", i);
        SPRINTF(tagValue, "tagValue_%d", i);
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, tagValue, TRUE));
    }

    // Adding one more will cause a limit error
    EXPECT_EQ(STATUS_MAX_FRAGMENT_METADATA_COUNT, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", TRUE));
}

TEST_F(StreamApiTest, insertKinesisVideoTag_Mixed_Count) {
    UINT32 i;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN + 1];
    CHAR tagValue[MKV_MAX_TAG_VALUE_LEN + 1];

    // Create and ready a stream
    ReadyStream();

    for (i = 0; i < MAX_FRAGMENT_METADATA_COUNT; i++) {
        SPRINTF(tagName, "tagName_%d", i);
        SPRINTF(tagValue, "tagValue_%d", i);
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, tagName, tagValue, i % 2 == 0));
    }

    // Adding one more will cause a limit error
    EXPECT_EQ(STATUS_MAX_FRAGMENT_METADATA_COUNT, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", TRUE));
    EXPECT_EQ(STATUS_MAX_FRAGMENT_METADATA_COUNT, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "tagName", (PCHAR) "tagValue", FALSE));
}

TEST_F(StreamApiTest, kinesisVideoGetData_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 bufferSize = 100000, fillSize;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(
            getKinesisVideoStreamData(streamHandle, TEST_UPLOAD_HANDLE, pBuffer, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, INVALID_UPLOAD_HANDLE_VALUE, pBuffer, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, TEST_UPLOAD_HANDLE, NULL, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, TEST_UPLOAD_HANDLE, pBuffer, 0, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, TEST_UPLOAD_HANDLE, pBuffer, bufferSize, NULL)));

    MEMFREE(pBuffer);
}

TEST_F(StreamApiTest, kinesisVideoStreamFormatChanged_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 bufferSize = MKV_MAX_CODEC_PRIVATE_LEN;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);
    UINT32 trackId = TEST_TRACKID;

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(streamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer, trackId)));
    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(mStreamHandle, 1, NULL, trackId)));
    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN + 1, pBuffer, trackId)));

    MEMFREE(pBuffer);
}

TEST_F(StreamApiTest, kinesisVideoStreamFormatChanged_Valid)
{
    UINT32 bufferSize = MKV_MAX_CODEC_PRIVATE_LEN;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);
    UINT32 trackId = TEST_TRACKID;

    // Create a stream
    CreateStream();

    // First time it will not free the previous allocation
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN / 2, pBuffer, trackId));

    // Second and consecutive times it will
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer, trackId));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer, trackId));

    // Ensure we can Zero it out
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 0, NULL, trackId));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 0, pBuffer, trackId));

    // Set it again a couple of times to free the previous allocations
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 1, pBuffer, trackId));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer, trackId));

    MEMFREE(pBuffer);
}

TEST_F(StreamApiTest, kinesisVideoStreamFormatChanged_Multitrack_free)
{
    UINT32 cpdSize = 1;
    PBYTE pCpd = (PBYTE) MEMALLOC(cpdSize);
    UINT32 cpd2Size = MKV_MAX_CODEC_PRIVATE_LEN;
    PBYTE pCpd2 = (PBYTE) MEMALLOC(cpd2Size);

    PTrackInfo trackInfos = (PTrackInfo) MEMALLOC(2 * SIZEOF(TrackInfo));

    // Set the on-the-stack CPD to some values
    MEMSET(pCpd, 0xab, cpdSize);
    MEMSET(pCpd2, 0x55, cpd2Size);

    trackInfos[0].trackId = TEST_TRACKID;
    trackInfos[0].trackType = MKV_TRACK_INFO_TYPE_VIDEO;
    trackInfos[0].codecPrivateDataSize = cpdSize;
    trackInfos[0].codecPrivateData = pCpd;
    STRCPY(trackInfos[0].codecId, (PCHAR) "TestCodec1");
    STRCPY(trackInfos[0].trackName, (PCHAR) "TestTrack1");
    trackInfos[0].trackCustomData.trackVideoConfig.videoWidth = 1280;
    trackInfos[0].trackCustomData.trackVideoConfig.videoHeight = 720;

    trackInfos[1].trackId = TEST_TRACKID + 1;
    trackInfos[1].trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfos[1].codecPrivateDataSize = cpd2Size;
    trackInfos[1].codecPrivateData = pCpd2;
    STRCPY(trackInfos[1].codecId, (PCHAR) "TestCodec2");
    STRCPY(trackInfos[1].trackName, (PCHAR) "TestTrack2");
    trackInfos[1].trackCustomData.trackAudioConfig.channelConfig = 5;
    trackInfos[1].trackCustomData.trackAudioConfig.samplingFrequency = 44000;

    mStreamInfo.streamCaps.trackInfoCount = 2;
    mStreamInfo.streamCaps.trackInfoList = trackInfos;

    mStreamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_ANNEXB_CPD_NALS;

    // Create a stream
    CreateStream();

    // Free the codecs which were used in the stream creation track infos
    MEMFREE(pCpd);
    MEMFREE(pCpd2);
    MEMFREE(trackInfos);

    // Set the CPD for track 1
    cpdSize = 3000;
    pCpd = (PBYTE) MEMALLOC(cpdSize);
    MEMSET(pCpd, 1, cpdSize);
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, cpdSize, pCpd, TEST_TRACKID));

    // Free the CPD
    MEMFREE(pCpd);

    // Set the CPD for track 2
    cpdSize = 5000;
    pCpd = (PBYTE) MEMALLOC(cpdSize);
    MEMSET(pCpd, 2, cpdSize);
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, cpdSize, pCpd, TEST_TRACKID + 1));

    // Free the cpd
    MEMFREE(pCpd);

    // Set the CPD for track 1
    cpdSize = 300;
    pCpd = (PBYTE) MEMALLOC(cpdSize);
    MEMSET(pCpd, 3, cpdSize);
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, cpdSize, pCpd, TEST_TRACKID));

    // Free the CPD
    MEMFREE(pCpd);

    // Set the CPD for track 2
    cpdSize = 50;
    pCpd = (PBYTE) MEMALLOC(cpdSize);
    MEMSET(pCpd, 4, cpdSize);
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, cpdSize, pCpd, TEST_TRACKID + 1));

    // Free the cpd
    MEMFREE(pCpd);
}

PVOID streamStopNotifier(PVOID arg)
{
    STREAM_HANDLE streamHandle = (STREAM_HANDLE) arg;
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    DLOGI("Indicating stream closed");
    // Need to interlock - this is for test only as test accesses the internal objects and methods directly
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);
    MUTEX_LOCK(pKinesisVideoStream->base.lock);
    notifyStreamClosed(pKinesisVideoStream, 0);
    MUTEX_UNLOCK(pKinesisVideoStream->base.lock);
    return NULL;
}

TEST_F(StreamApiTest, kinesisVideoStreamCreateSync_Valid)
{
    freeKinesisVideoStream(&mStreamHandle);

    // Create synchronously
    CreateStreamSync();

    // Produce a frame to enforce awaiting for the stop stream
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = 0;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    // Spin up a thread to act as a delayed notification
    TID tid;
    EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&tid, streamStopNotifier, (PVOID) mStreamHandle));
    EXPECT_EQ(STATUS_SUCCESS, THREAD_DETACH(tid));

    // Stop synchronously
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreamHandle));
}

TEST_F(StreamApiTest, kinesisVideoStreamCreateSync_Valid_Timeout)
{
    CLIENT_HANDLE clientHandle;

    // Create a client with appropriate timeout so we don't block on test.
    mClientSyncMode = TRUE;
    mDeviceInfo.clientInfo.createStreamTimeout = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));

    // Create synchronously
    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, createKinesisVideoStreamSync(clientHandle, &mStreamInfo, &mStreamHandle));

    EXPECT_FALSE(IS_VALID_STREAM_HANDLE(mStreamHandle));

    // Stop synchronously - will fail as we should have invalid handle
    EXPECT_NE(STATUS_SUCCESS, stopKinesisVideoStreamSync(mStreamHandle));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamApiTest, kinesisVideoStreamCreateSyncStopSync_Valid_Timeout)
{
    CLIENT_HANDLE clientHandle;

    // Create a client with appropriate timeout so we don't block on test.
    mClientSyncMode = TRUE;
    mDeviceInfo.clientInfo.stopStreamTimeout = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));

    // Create synchronously
    CreateStreamSync(clientHandle);

    // Produce a frame to enforce awaiting for the stop stream
    BYTE temp[100];
    Frame frame;
    frame.trackId = 1;
    frame.size = SIZEOF(temp);
    frame.duration = 0;
    frame.index = 0;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.presentationTs = 0;
    frame.decodingTs = 0;
    frame.frameData = temp;
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

    // Stop synchronously
    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, stopKinesisVideoStreamSync(mStreamHandle));

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}
