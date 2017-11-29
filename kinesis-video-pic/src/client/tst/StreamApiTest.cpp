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
        EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    }

    // Ensure it fails now
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
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
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

    mStreamInfo.streamCaps.bufferDuration = 0;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = TEST_BUFFER_DURATION + 1;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

    mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

    mStreamInfo.streamCaps.bufferDuration = TEST_REPLAY_DURATION;
    mStreamInfo.streamCaps.replayDuration = TEST_REPLAY_DURATION;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
}

TEST_F(StreamApiTest, createKinesisVideoStream_DuplicateNames)
{
    STREAM_HANDLE streamHandle;

    STRCPY(mStreamInfo.name, "ABC");
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    STRCPY(mStreamInfo.name, "DEF");
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    STRCPY(mStreamInfo.name, "GHI");
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
    STRCPY(mStreamInfo.name, "ABC");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "DEF");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
    STRCPY(mStreamInfo.name, "GHI");
    EXPECT_EQ(STATUS_DUPLICATE_STREAM_NAME, createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle));
}

TEST_F(StreamApiTest, createKinesisVideoStream_ZeroFrameRate)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.streamCaps.frameRate = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));
}

TEST_F(StreamApiTest, createKinesisVideoStream_ZeroTimecodeScale)
{
    STREAM_HANDLE streamHandle;

    mStreamInfo.name[0] = '\0';
    mStreamInfo.streamCaps.timecodeScale = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

    mStreamInfo.streamCaps.timecodeScale = MAX_TIMECODE_SCALE;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

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
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoStream(mClientHandle, &mStreamInfo, &streamHandle)));

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

    MEMSET(mStreamInfo.name, 'a', MAX_STREAM_NAME_LEN * SIZEOF(CHAR));
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
