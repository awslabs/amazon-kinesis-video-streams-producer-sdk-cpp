#include "MkvgenTestFixture.h"

BOOL gTimeCallbackCalled = FALSE;

class MkvgenApiTest : public MkvgenTestBase {
};

TEST_F(MkvgenApiTest, createMkvGenerator_InvalidInput)
{
    PMkvGenerator pMkvGenerator = NULL;
    CHAR contentTypeExceedMaxLen[MAX_CONTENT_TYPE_LEN + 2];
    MEMSET(contentTypeExceedMaxLen, 'a', MAX_CONTENT_TYPE_LEN + 1);
    contentTypeExceedMaxLen[MAX_CONTENT_TYPE_LEN + 1] = '\0';

    BYTE zeroSegmentUUID[MKV_SEGMENT_UUID_LEN];
    MEMSET(zeroSegmentUUID, 0, MKV_SEGMENT_UUID_LEN);

    EXPECT_EQ(STATUS_MKV_INVALID_CONTENT_TYPE_LENGTH, createMkvGenerator(contentTypeExceedMaxLen, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    EXPECT_EQ(STATUS_MKV_INVALID_TIMECODE_SCALE, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE - 1, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    EXPECT_EQ(STATUS_MKV_INVALID_TIMECODE_SCALE, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MAX_TIMECODE_SCALE + 1, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, NULL, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MAX_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MAX_CLUSTER_DURATION + 1, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION - 1, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
    EXPECT_EQ(STATUS_MKV_INVALID_CLUSTER_DURATION, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_FLAG_NONE, MKV_TEST_TIMECODE_SCALE, MAX_CLUSTER_DURATION + 1, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    EXPECT_EQ(STATUS_MKV_INVALID_CLUSTER_DURATION, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_FLAG_NONE, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION - 1, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));

    MEMSET(mTrackInfo.codecId, 'a', MKV_MAX_CODEC_ID_LEN + 1); // codecId is not null terminated
    EXPECT_EQ(STATUS_MKV_INVALID_CODEC_ID_LENGTH, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    STRCPY(mTrackInfo.codecId, MKV_TEST_CODEC_ID);
    MEMSET(mTrackInfo.trackName, 'a', MKV_MAX_TRACK_NAME_LEN + 1); // trackName is not null terminated
    EXPECT_EQ(STATUS_MKV_INVALID_TRACK_NAME_LENGTH, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    STRCPY(mTrackInfo.trackName, MKV_TEST_TRACK_NAME);
    EXPECT_TRUE(STATUS_FAILED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, NULL)));

    // invalid cpd buffer being NULL
    mTrackInfo.codecPrivateDataSize = 1;
    EXPECT_EQ(STATUS_MKV_CODEC_PRIVATE_NULL, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));

    // invalid cpd size
    mTrackInfo.codecPrivateDataSize = MKV_MAX_CODEC_PRIVATE_LEN + 1;
    mTrackInfo.codecPrivateData = (PBYTE) 0x11;
    EXPECT_EQ(STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));
    mTrackInfo.codecPrivateDataSize = 0;
    mTrackInfo.codecPrivateData = NULL;

    EXPECT_EQ(STATUS_MKV_BOTH_ANNEXB_AND_AVCC_SPECIFIED, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS | MKV_GEN_ADAPT_AVCC_NALS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, NULL, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));

    // zero segmentUUID
    EXPECT_EQ(STATUS_MKV_INVALID_SEGMENT_UUID, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, zeroSegmentUUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator));

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, NULL, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, EMPTY_STRING, NULL, 0, &pMkvGenerator)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(pMkvGenerator)));
}

TEST_F(MkvgenApiTest, freeMkvGenerator_Idempotency)
{
    PMkvGenerator pMkvGenerator = NULL;

    // Create a valid generator
    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MIN_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &pMkvGenerator)));

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
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf, MKV_TEST_TRACKID};
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(NULL, &frame, &trackInfo, mBuffer, &size, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, NULL, &trackInfo, mBuffer, &size, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, NULL, mBuffer, &size, NULL)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, &trackInfo, mBuffer, NULL, NULL)));

    // Should succeed in getting the size
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mMkvGenerator, &frame, &trackInfo, NULL, &size, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mMkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

    // Not enough buffer
    size--;
    encodedFrameInfo.streamState = MKV_STATE_START_BLOCK;
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Wrong track
    size++;
    encodedFrameInfo.streamState = MKV_STATE_START_BLOCK;
    trackInfo.trackId++;
    EXPECT_TRUE(STATUS_FAILED(mkvgenPackageFrame(mMkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Make sure it's not set on a failed call
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);
}

TEST_F(MkvgenApiTest, mkvgenValidateFrame_NegativeTest)
{
    PStreamMkvGenerator pStreamGenerator = (PStreamMkvGenerator) mMkvGenerator;
    BYTE frameBuf[10000];
    UINT64 pts, dts, duration;
    MKV_STREAM_STATE streamState;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf, MKV_TEST_TRACKID};

    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(NULL, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, NULL, &trackInfo, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, NULL, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, NULL, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, NULL, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, NULL, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, NULL)));
    trackInfo.trackId += 1;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    trackInfo.trackId -= 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    UINT64 foo = TIMESTAMP_TO_MKV_TIMECODE(MKV_TEST_FRAME_DURATION, MKV_TEST_TIMECODE_SCALE);
    EXPECT_TRUE(duration == TIMESTAMP_TO_MKV_TIMECODE(MKV_TEST_FRAME_DURATION, MKV_TEST_TIMECODE_SCALE * DEFAULT_TIME_UNIT_IN_NANOS));

    frame.presentationTs = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));

    frame.size = 0;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    frame.size = SIZEOF(frameBuf);

    frame.duration = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    frame.duration = MKV_TEST_FRAME_DURATION;

    frame.frameData = NULL;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
    frame.frameData = frameBuf;

    pStreamGenerator->lastClusterPts = 10;
    EXPECT_TRUE(STATUS_FAILED(mkvgenValidateFrame(pStreamGenerator, &frame, &trackInfo, &pts, &dts, &duration, &streamState)));
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

TEST_F(MkvgenApiTest, mkvgenGenerateHeader_TrackNameCodecIdGenerationTest)
{
    PMkvGenerator mkvGenerator = NULL;
    UINT32 size = 0, storedSize, storedSizeNull, storedSizeEmpty;
    PCHAR shortCodecId = (PCHAR) "a", shortTrackName = (PCHAR) "a";
    CHAR longTrackName[MKV_MAX_TRACK_NAME_LEN + 1];
    MEMSET(longTrackName, 'a', MKV_MAX_TRACK_NAME_LEN);
    longTrackName[MKV_MAX_TRACK_NAME_LEN] = '\0';
    CHAR longCodecId[MKV_MAX_CODEC_ID_LEN + 1];
    MEMSET(longCodecId, 'a', MKV_MAX_CODEC_ID_LEN);
    longCodecId[MKV_MAX_CODEC_ID_LEN] = '\0';

    STRCPY(mTrackInfo.codecId, shortCodecId);
    STRCPY(mTrackInfo.trackName, shortTrackName);
    mTrackInfo.trackCustomData.trackVideoConfig.videoWidth = 1280;
    mTrackInfo.trackCustomData.trackVideoConfig.videoHeight = 720;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));
    storedSize = mkvgenGetMkvHeaderSize((PStreamMkvGenerator) mkvGenerator);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
    mkvGenerator = NULL;

    // Same but client id = NULL
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 NULL,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));
    storedSizeNull = mkvgenGetMkvHeaderSize((PStreamMkvGenerator) mkvGenerator);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
    mkvGenerator = NULL;

    // Same but client id = EMPTY
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 EMPTY_STRING,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));
    storedSizeEmpty = mkvgenGetMkvHeaderSize((PStreamMkvGenerator) mkvGenerator);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
    mkvGenerator = NULL;

    // Validate the difference in sizes which should be 2 times (muxer and writer) + delimiter
    EXPECT_EQ(storedSize - 2 * (STRLEN(MKV_TEST_CLIENT_ID) + 1), storedSizeNull);
    EXPECT_EQ(storedSize - 2 * (STRLEN(MKV_TEST_CLIENT_ID) + 1), storedSizeEmpty);
    EXPECT_EQ(storedSizeEmpty, storedSizeNull);

    STRCPY(mTrackInfo.codecId, longCodecId);
    STRCPY(mTrackInfo.trackName, longTrackName);

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));
    size = mkvgenGetMkvHeaderSize((PStreamMkvGenerator) mkvGenerator);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
    mkvGenerator = NULL;

    EXPECT_EQ(STRLEN(longTrackName) - STRLEN(shortTrackName) + STRLEN(longCodecId) - STRLEN(shortCodecId),
              size - storedSize);

    // empty trackname and codec id
    mTrackInfo.codecId[0] = '\0';
    mTrackInfo.trackName[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));
    size = mkvgenGetMkvHeaderSize((PStreamMkvGenerator) mkvGenerator);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
    mkvGenerator = NULL;

    // when track name and codec id are empty, their mkv element wont be generated.
    // 4 accounts for 1 byte of data and 1 byte of ebml length for both track name and codec id.
    EXPECT_EQ(size, storedSize - MKV_TRACK_NAME_BITS_SIZE - MKV_CODEC_ID_BITS_SIZE - 4);
}

TEST_F(MkvgenApiTest, mkvgenGetMkvOverheadSize_PositiveAndNegativeTest)
{
    UINT32 size;
    UINT32 defaultTrackInfoCount = 1;
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(NULL, MKV_STATE_START_BLOCK, &size));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_BLOCK, NULL));
    EXPECT_NE(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(NULL, MKV_STATE_START_BLOCK, NULL));

    EXPECT_EQ(STATUS_SUCCESS, mkvgenGetMkvOverheadSize(mMkvGenerator, MKV_STATE_START_STREAM, &size));
    EXPECT_GE(mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mMkvGenerator), size);

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

TEST_F(MkvgenApiTest, mkvgenContentType_GetContentType)
{
    EXPECT_EQ(MKV_CONTENT_TYPE_NONE, mkvgenGetContentTypeFromContentTypeString(NULL));
    EXPECT_EQ(MKV_CONTENT_TYPE_NONE, mkvgenGetContentTypeFromContentTypeString((PCHAR) ""));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H265, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h265"));
    EXPECT_EQ(MKV_CONTENT_TYPE_X_MKV_VIDEO, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/x-matroska"));
    EXPECT_EQ(MKV_CONTENT_TYPE_X_MKV_AUDIO, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio/x-matroska"));
    EXPECT_EQ(MKV_CONTENT_TYPE_AAC, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio/aac"));
    EXPECT_EQ(MKV_CONTENT_TYPE_ALAW, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio/alaw"));

    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio/aa"));
    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) "udio/aac"));
    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) " audio/aac"));
    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio/aac "));
    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) "audio\aac"));

    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_AAC, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,audio/aac"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_ALAW, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,audio/alaw"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_H265, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,video/h265"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,video/h264"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264, mkvgenGetContentTypeFromContentTypeString((PCHAR) ",video/h264"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_UNKNOWN, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264, video/h265"));
    EXPECT_EQ(MKV_CONTENT_TYPE_UNKNOWN | MKV_CONTENT_TYPE_H265, mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264 ,video/h265"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_H265 | MKV_CONTENT_TYPE_AAC |
              MKV_CONTENT_TYPE_X_MKV_VIDEO | MKV_CONTENT_TYPE_X_MKV_AUDIO,
              mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,video/h265,audio/aac,video/x-matroska,audio/x-matroska"));
    EXPECT_EQ(MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_H265 | MKV_CONTENT_TYPE_AAC |
              MKV_CONTENT_TYPE_X_MKV_VIDEO | MKV_CONTENT_TYPE_X_MKV_AUDIO | MKV_CONTENT_TYPE_UNKNOWN,
              mkvgenGetContentTypeFromContentTypeString((PCHAR) "video/h264,video/h265,audio/aac,blah,video/x-matroska,audio/x-matroska"));
}

TEST_F(MkvgenApiTest, mkvgenSetCodecPrivateData_variations)
{
    BYTE cpd[1000];
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_NE(STATUS_SUCCESS, mkvgenSetCodecPrivateData(NULL, MKV_TEST_TRACKID, cpdSize, cpd));
    EXPECT_NE(STATUS_SUCCESS, mkvgenSetCodecPrivateData(mMkvGenerator, 0, cpdSize, cpd));
    EXPECT_NE(STATUS_SUCCESS, mkvgenSetCodecPrivateData(mMkvGenerator, MKV_TEST_TRACKID, MKV_MAX_CODEC_PRIVATE_LEN + 1, cpd));
    EXPECT_NE(STATUS_SUCCESS, mkvgenSetCodecPrivateData(mMkvGenerator, MKV_TEST_TRACKID, MKV_MAX_CODEC_PRIVATE_LEN, NULL));

    EXPECT_EQ(STATUS_SUCCESS, mkvgenSetCodecPrivateData(mMkvGenerator, MKV_TEST_TRACKID, 0, NULL));
    EXPECT_EQ(STATUS_SUCCESS, mkvgenSetCodecPrivateData(mMkvGenerator, MKV_TEST_TRACKID, cpdSize, cpd));
}

TEST_F(MkvgenApiTest, mkvGenGetAudioConfigFromAmsAcmCpdNegativeCase)
{
    BYTE cpdWrongFormatCode[] = {0x09, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00};
    BYTE cpdTooShort[] = {0x07, 0x00, 0x01, 0x00, 0x40};
    TrackCustomData audioData;

    EXPECT_EQ(STATUS_MKV_INVALID_AMS_ACM_CPD, getAudioConfigFromAmsAcmCpd(cpdWrongFormatCode,
                                                                          SIZEOF(cpdWrongFormatCode),
                                                                          &audioData.trackAudioConfig.samplingFrequency,
                                                                          &audioData.trackAudioConfig.channelConfig,
                                                                          &audioData.trackAudioConfig.bitDepth));

    EXPECT_EQ(STATUS_MKV_INVALID_AMS_ACM_CPD, getAudioConfigFromAmsAcmCpd(cpdTooShort,
                                                                          SIZEOF(cpdTooShort),
                                                                          &audioData.trackAudioConfig.samplingFrequency,
                                                                          &audioData.trackAudioConfig.channelConfig,
                                                                          &audioData.trackAudioConfig.bitDepth));

    EXPECT_EQ(STATUS_NULL_ARG, getAudioConfigFromAmsAcmCpd(cpdTooShort,
                                                           SIZEOF(cpdTooShort),
                                                           NULL,
                                                           NULL,
                                                           NULL));

    EXPECT_EQ(STATUS_NULL_ARG, getAudioConfigFromAmsAcmCpd(NULL,
                                                           SIZEOF(cpdTooShort),
                                                           &audioData.trackAudioConfig.samplingFrequency,
                                                           &audioData.trackAudioConfig.channelConfig,
                                                           &audioData.trackAudioConfig.bitDepth));
}
