#include "MkvgenTestFixture.h"

BOOL gTimeCallbackCalled = FALSE;

class MkvgenApiTest : public MkvgenTestBase {
};

TEST_F(MkvgenApiTest, createMkvGenerator_InvalidInput)
{
    PMkvGenerator pMkvGenerator = NULL;
    CHAR contentType[MAX_CONTENT_TYPE_LEN + 2];
    MEMSET(contentType, 'a', MAX_CONTENT_TYPE_LEN + 1);
    contentType[MAX_CONTENT_TYPE_LEN + 1] = '\0';

    CHAR codecId[MKV_MAX_CODEC_ID_LEN + 2];
    MEMSET(codecId, 'a', MKV_MAX_CODEC_ID_LEN + 1);
    codecId[MKV_MAX_CODEC_ID_LEN + 1] = '\0';

    CHAR trackName[MKV_MAX_TRACK_NAME_LEN + 2];
    MEMSET(trackName, 'a', MKV_MAX_TRACK_NAME_LEN + 1);
    trackName[MKV_MAX_TRACK_NAME_LEN + 1] = '\0';

    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(contentType, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(contentType, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE - 1, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(contentType, MKV_TEST_BEHAVIOR_FLAGS, MAX_TIMECODE_SCALE + 1, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MAX_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MAX_CLUSTER_DURATION + 1, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION - 1, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_FLAG_NONE, MKV_TEST_TIMECODE_SCALE, MAX_CLUSTER_DURATION + 1, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_FLAG_NONE, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION - 1, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));

    STRCPY(mTrackInfo.codecId, codecId);
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    STRCPY(mTrackInfo.codecId, MKV_TEST_CODEC_ID);
    STRCPY(mTrackInfo.trackName, trackName);
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    STRCPY(mTrackInfo.trackName, MKV_TEST_TRACK_NAME);
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, NULL)));

    // invalid cpd buffer
    mTrackInfo.codecPrivateDataSize = 1;
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, NULL)));

    // invalid cpd size
    mTrackInfo.codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN + 1;
    mTrackInfo.codecPrivateData = (PBYTE) 0x11;
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, NULL)));
    mTrackInfo.codecPrivateDataSize = 0;
    mTrackInfo.codecPrivateData = NULL;

    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS | MKV_GEN_ADAPT_AVCC_NALS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
}

TEST_F(MkvgenApiTest, freeMkvGenerator_Idempotency)
{
    PMkvGenerator pMkvGenerator = NULL;

    // Create a valid generator
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, &mTrackInfo, mTrackInfoCount, NULL, 0, &pMkvGenerator)));

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));

    // The call is idempotent
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(NULL)));
}

TEST_F(MkvgenApiTest, mkvgenPackageFrame_NegativeTest)
{
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    EncodedFrameInfo encodedFrameInfo;
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf};

    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(NULL, &frame, mBuffer, &size, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, NULL, mBuffer, &size, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, mBuffer, NULL, NULL)));

    // Should succeed in getting the size
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mMkvGenerator, &frame, NULL, &size, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mMkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Not enough buffer
    size = size - 1;
    encodedFrameInfo.streamState = MKV_STATE_START_BLOCK;
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Make sure it's not set on a failed call
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);
}

TEST_F(MkvgenApiTest, mkvgenValidateFrame_NegativeTest)
{
    PStreamMkvGenerator pStreamGenerator = (PStreamMkvGenerator) mMkvGenerator;
    BYTE frameBuf[10000];
    UINT64 pts, dts, duration;
    MKV_STREAM_STATE streamState;
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf, 0};

    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(NULL, &frame, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, NULL, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, NULL, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, NULL, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, NULL, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, NULL)));

    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));
    UINT64 foo = TIMESTAMP_TO_MKV_TIMECODE(MKV_TEST_FRAME_DURATION, MKV_TEST_TIMECODE_SCALE);
    EXPECT_TRUE(duration == TIMESTAMP_TO_MKV_TIMECODE(MKV_TEST_FRAME_DURATION, MKV_TEST_TIMECODE_SCALE * DEFAULT_TIME_UNIT_IN_NANOS));

    frame.presentationTs = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));

    frame.size = 0;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));
    frame.size = SIZEOF(frameBuf);

    frame.duration = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));
    frame.duration = MKV_TEST_FRAME_DURATION;

    frame.frameData = NULL;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));
    frame.frameData = frameBuf;

    pStreamGenerator->lastClusterPts = 10;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &pts, &dts, &duration, &streamState)));
}

TEST_F(MkvgenApiTest, mkvgenValidateFrame_PositiveAndNegativeTest)
{
    UINT64 timestamp;

    EXPECT_TRUE(STATUS_FAILED(mkvgenTimecodeToTimestamp(NULL, 0, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenTimecodeToTimestamp(NULL, 0, &timestamp)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenTimecodeToTimestamp(mMkvGenerator, 0, NULL)));

    EXPECT_EQ(STATUS_SUCCESS, mkvgenTimecodeToTimestamp(mMkvGenerator, 1, &timestamp));
    EXPECT_EQ(MKV_TEST_TIMECODE_SCALE, timestamp);
}

TEST_F(MkvgenApiTest, mkvgenResetGenerator_NegativeTest)
{
    EXPECT_TRUE(STATUS_FAILED(mkvgenResetGenerator(NULL)));
}

TEST_F(MkvgenApiTest, mkvgenGenerateHeader_PositiveAndNegativeTest)
{
    BYTE testBuf[1000];
    UINT32 size, storedSize;
    UINT64 timestamp;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(NULL, testBuf, &size, &timestamp));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, NULL, &timestamp));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, NULL, NULL, &timestamp));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(NULL, NULL, NULL, &timestamp));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(NULL, NULL, NULL, NULL));

    size = 0;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, NULL, &size, &timestamp));
    size = 0;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, NULL, &size, NULL));

    storedSize = size;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, &timestamp));
    EXPECT_EQ(storedSize, size);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, NULL));
    EXPECT_EQ(storedSize, size);

    size = storedSize + 1;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, &timestamp));
    EXPECT_EQ(storedSize, size);

    size = storedSize + 1;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, NULL));
    EXPECT_EQ(storedSize, size);

    size = storedSize - 1;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, &timestamp));

    size = storedSize - 1;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, NULL));

    size = 0;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, NULL));

    size = 0;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateHeader(mMkvGenerator, testBuf, &size, &timestamp));
}

TEST_F(MkvgenApiTest, mkvgenGetMkvOverheadSize_PositiveAndNegativeTest)
{
    UINT32 size;
    UINT32 defaultTrackInfoCount = 1;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(NULL, MKV_STATE_START_BLOCK, &size));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_BLOCK, NULL));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(NULL, MKV_STATE_START_BLOCK, NULL));

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_STREAM, &size));
    EXPECT_GE(GET_MKV_HEADER_OVERHEAD(defaultTrackInfoCount) + MKV_TRACK_VIDEO_BITS_SIZE, size);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_CLUSTER, &size));
    EXPECT_EQ(MKV_CLUSTER_OVERHEAD, size);

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_BLOCK, &size));
    EXPECT_EQ(MKV_SIMPLE_BLOCK_OVERHEAD, size);
}

TEST_F(MkvgenApiTest, mkvgenGetCurrentTimestamps_PositiveAndNegativeTest)
{
    UINT64 clusterTs, streamStartTs, clusterStartDts;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(NULL, &clusterTs, &streamStartTs, &clusterStartDts));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(NULL, NULL, &streamStartTs, &clusterStartDts));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(NULL, NULL, NULL, &clusterStartDts));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(NULL, NULL, NULL, NULL));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(mMkvGenerator, NULL, &streamStartTs, &clusterStartDts));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(mMkvGenerator, &clusterTs, NULL, &clusterStartDts));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(mMkvGenerator, &clusterTs, &streamStartTs, NULL));
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGetCurrentTimestamps(mMkvGenerator, &clusterTs, &streamStartTs, &clusterStartDts));
    EXPECT_EQ(0, clusterTs);
    EXPECT_EQ(0, streamStartTs);
    EXPECT_EQ(0, clusterStartDts);
}

TEST_F(MkvgenApiTest, mkvgenGenerateTag_PositiveAndNegativeTest)
{
    PBYTE tempBuffer = NULL;
    UINT32 size = 100000;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN + 2];
    CHAR tagValue[MKV_MAX_TAG_VALUE_LEN + 2];
    STRCPY(tagName, "TestTagName");
    STRCPY(tagValue, "TestTagValue");
    tempBuffer = (PBYTE) MEMALLOC(size);

    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(NULL, tempBuffer, tagName, tagValue, &size));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, NULL, tagValue, &size));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, NULL, &size));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, tagValue, NULL));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(NULL, tempBuffer, NULL, NULL, NULL));

    // Check for a smaller size returned
    EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, NULL, tagName, tagValue, &size));
    size--;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, tagValue, &size));

    // Larger tagName
    MEMSET(tagName, 'A', SIZEOF(tagName));
    tagName[MKV_MAX_TAG_NAME_LEN + 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, tagValue, &size));

    // Larger tagValue
    STRCPY(tagName, "TestTagName");
    MEMSET(tagValue, 'B', SIZEOF(tagValue));
    tagValue[MKV_MAX_TAG_VALUE_LEN + 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, tagValue, &size));

    // Both are larger
    MEMSET(tagName, 'A', SIZEOF(tagName));
    tagName[MKV_MAX_TAG_NAME_LEN + 1] = '\0';
    MEMSET(tagValue, 'B', SIZEOF(tagValue));
    tagValue[MKV_MAX_TAG_VALUE_LEN + 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, mkvgenGenerateTag(mMkvGenerator, tempBuffer, tagName, tagValue, &size));

    MEMFREE(tempBuffer);
}
