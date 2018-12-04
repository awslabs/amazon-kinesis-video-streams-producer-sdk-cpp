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

TEST_F(StreamApiTest, createKinesisVideoStream_MaxStreams)
{
    STREAM_HANDLE streamHandle;
    UINT32 i;
    
    mStreamInfo.name[0] = '\0';
    for (i = 0; i < MAX_TEST_STREAM_COUNT; i++) {
        THREAD_SLEEP(1000);
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

    THREAD_SLEEP(1000);

    mStreamInfo.streamCaps.bufferDuration = 0;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(1000);

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = TEST_BUFFER_DURATION + 1;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(1000);

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    THREAD_SLEEP(1000);

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
    mStreamInfo.streamCaps.codecPrivateData = cpd;
    mStreamInfo.streamCaps.codecPrivateDataSize = SIZEOF(cpd);
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.codecPrivateData = NULL;
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));

    mStreamInfo.streamCaps.codecPrivateData = tempBuff;
    mStreamInfo.streamCaps.codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    mStreamInfo.streamCaps.codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN + 1;
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

TEST_F(StreamApiTest, kinesisVideoPutFrame_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    Frame frame;

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(putKinesisVideoFrame(streamHandle, &frame)));
    EXPECT_TRUE(STATUS_FAILED(putKinesisVideoFrame(mStreamHandle, NULL)));
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
    UINT64 clientStreamHandle = 0;
    UINT32 bufferSize = 100000, fillSize;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(
            getKinesisVideoStreamData(streamHandle, &clientStreamHandle, pBuffer, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, NULL, pBuffer, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, NULL, bufferSize, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, pBuffer, 0, &fillSize)));
    EXPECT_TRUE(STATUS_FAILED(getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, pBuffer, bufferSize, NULL)));

    MEMFREE(pBuffer);
}

TEST_F(StreamApiTest, kinesisVideoStreamFormatChanged_NULL_Invalid)
{
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UINT32 bufferSize = MKV_MAX_CODEC_PRIVATE_LEN;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);

    // Create a stream
    CreateStream();

    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(streamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer)));
    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(mStreamHandle, 1, NULL)));
    EXPECT_TRUE(STATUS_FAILED(kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN + 1, pBuffer)));

    MEMFREE(pBuffer);
}

TEST_F(StreamApiTest, kinesisVideoStreamFormatChanged_Valid)
{
    UINT32 bufferSize = MKV_MAX_CODEC_PRIVATE_LEN;
    PBYTE pBuffer = (PBYTE) MEMALLOC(bufferSize);

    // Create a stream
    CreateStream();

    // First time it will not free the previous allocation
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN / 2, pBuffer));

    // Second and consecutive times it will
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer));

    // Ensure we can Zero it out
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 0, NULL));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 0, pBuffer));

    // Set it again a couple of times to free the previous allocations
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, 1, pBuffer));
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(mStreamHandle, MKV_MAX_CODEC_PRIVATE_LEN, pBuffer));

    MEMFREE(pBuffer);
}
