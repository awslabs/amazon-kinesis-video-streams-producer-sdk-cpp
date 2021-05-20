#include "MkvgenTestFixture.h"

class MkvgenApiFunctionalityTest : public MkvgenTestBase {
};

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_ClusterBoundaryKeyFrame)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Iterate with key frames and ensure a new cluster is created
    for (i = 0; i < 10; i++) {
        // Add a key frame
        frame.decodingTs += MKV_TEST_FRAME_DURATION;
        frame.presentationTs += MKV_TEST_FRAME_DURATION;
        frame.flags = FRAME_FLAG_KEY_FRAME;
        size = MKV_TEST_BUFFER_SIZE;

        // Validate that we can call the function with and without the streamState
        if (i % 2 == 0) {
            EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));
        } else {
            EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, NULL)));
        }

        // Validate that it's a cluster block
        EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);
    }

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_LargeFrameTimecode)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE,
                                                    MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                    mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator)));
    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame - this time we should overflow int16 value for the timecode
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_MKV_LARGE_FRAME_TIMECODE, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, NULL));

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvFromJpegAsH264)
{
    PMkvGenerator mkvGenerator;
    CHAR fileName[MAX_PATH_LEN];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf, MKV_TEST_TRACKID};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x01, 0x42, 0x40, 0x15, 0xFF, 0xE1, 0x00, 0x09, 0x67, 0x42, 0x40, 0x28, 0x96, 0x54, 0x0a, 0x0f, 0xc8, 0x01, 0x00, 0x04, 0x68, 0xce, 0x3c, 0x80};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    cpdSize = SIZEOF(cpd);
    STRCPY(mTrackInfo.codecId, (PCHAR) "V_MJPEG");
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    // Pretend it's h264 to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator));
    for (i = 0, index = 0; i < 100; i++) {
        SPRINTF(fileName, (PCHAR) "samples" FPATHSEPARATOR_STR "gif%03d.jpg", i);
        fileSize = MKV_TEST_BUFFER_SIZE;

        if (STATUS_FAILED(readFile(fileName, TRUE, NULL, &fileSize))) {
            break;
        }

        EXPECT_EQ(STATUS_SUCCESS, readFile(fileName, TRUE, frameBuf, &fileSize));

        frame.index = i;
        frame.presentationTs = frameDuration * i;
        frame.decodingTs = frameDuration * i;
        frame.size = (UINT32)fileSize;

        // Add a frame
        packagedSize = size;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mkvBuffer + index, &packagedSize, NULL));
        index += packagedSize;
        size -= packagedSize;


    }

    // Add tags
    for (i = 0; i < MKV_TEST_TAG_COUNT; i++) {
        packagedSize = size;
        SPRINTF(tagName, "testTag_%d", i);
        SPRINTF(tagVal, "testTag_%d_Value", i);
        EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mkvGenerator,
                                                    mkvBuffer + index,
                                                    tagName,
                                                    tagVal,
                                                    &packagedSize));
        index += packagedSize;
        size -= packagedSize;
    }

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_h264_jpg.mkv", TRUE, FALSE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    MEMFREE(frameBuf);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvFromJpegAsFourCc)
{
    PMkvGenerator mkvGenerator;
    CHAR fileName[MAX_PATH_LEN];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf, MKV_TEST_TRACKID};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    cpdSize = SIZEOF(cpd);
    STRCPY(mTrackInfo.codecId, MKV_FOURCC_CODEC_ID);
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    // This is a M-JPG to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_X_MKV_VIDEO_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator));

    for (i = 0, index = 0; i < 100; i++) {
        SPRINTF(fileName, (PCHAR) "samples" FPATHSEPARATOR_STR "gif%03d.jpg", i);
        fileSize = MKV_TEST_BUFFER_SIZE;
        if (STATUS_FAILED(readFile(fileName, TRUE, NULL, &fileSize))) {
            break;
        }

        EXPECT_EQ(STATUS_SUCCESS, readFile(fileName, TRUE, frameBuf, &fileSize));

        frame.index = i;
        frame.presentationTs = frameDuration * i;
        frame.decodingTs = frameDuration * i;
        frame.size = (UINT32)fileSize;

        // Add a frame
        packagedSize = size;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mkvBuffer + index, &packagedSize, NULL));
        index += packagedSize;
        size -= packagedSize;
    }

    // Add tags
    for (i = 0; i < MKV_TEST_TAG_COUNT; i++) {
        packagedSize = size;
        SPRINTF(tagName, (PCHAR) "testTag_%d", i);
        SPRINTF(tagVal, (PCHAR) "testTag_%d_Value", i);
        EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mkvGenerator,
                                                    mkvBuffer + index,
                                                    tagName,
                                                    tagVal,
                                                    &packagedSize));
        index += packagedSize;
        size -= packagedSize;
    }

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_jpeg.mkv", TRUE, FALSE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    MEMFREE(frameBuf);

    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvTagsOnly)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    UINT32 i, index = 0;
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];
    STRCPY(mTrackInfo.codecId, (PCHAR) "V_MJPEG");

    // Pretend it's h264 to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));

    // Add tags
    for (i = 0; i < MKV_TEST_TAG_COUNT; i++) {
        packagedSize = size;
        SPRINTF(tagName, (PCHAR) "testTag_%d", i);
        SPRINTF(tagVal, (PCHAR) "testTag_%d_Value", i);
        EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mkvGenerator,
                                                    mkvBuffer + index,
                                                    tagName,
                                                    tagVal,
                                                    &packagedSize));
        index += packagedSize;
        size -= packagedSize;
    }

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_tags.mkv", TRUE, FALSE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvMixedTags) {
    PMkvGenerator mkvGenerator;
    BYTE frameBuf[100];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, frameDuration, SIZEOF(frameBuf), frameBuf, MKV_TEST_TRACKID};
    UINT32 frameIndex, i, cpdSize, index = 0;
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    cpdSize = SIZEOF(cpd);

    STRCPY(mTrackInfo.codecId, MKV_FOURCC_CODEC_ID);
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_X_MKV_VIDEO_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME | MKV_GEN_KEY_FRAME_PROCESSING,
                                                 MKV_TEST_TIMECODE_SCALE, 2 * HUNDREDS_OF_NANOS_IN_A_SECOND,
                                                 MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator));

    // Generate a key-frame every 10th frame
    for (frameIndex = 0; frameIndex < 100; frameIndex++) {
        frame.index = frameIndex;
        frame.presentationTs = frameDuration * frameIndex;
        frame.decodingTs = frameDuration * frameIndex;
        frame.flags = (frameIndex % 10 == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Insert tags before the cluster
        if ((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) {
            for (i = 0; i < MKV_TEST_TAG_COUNT; i++) {
                packagedSize = size;
                SPRINTF(tagName, (PCHAR) "testTag_%d", i);
                SPRINTF(tagVal, "(PCHAR) testTag_%d_Value", i);
                EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mkvGenerator,
                                                            mkvBuffer + index,
                                                            tagName,
                                                            tagVal,
                                                            &packagedSize));
                index += packagedSize;
                size -= packagedSize;
            }
        }

        // Add a frame
        packagedSize = size;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator,
                                                     &frame,
                                                     &trackInfo,
                                                     mkvBuffer + index,
                                                     &packagedSize,
                                                     NULL));
        index += packagedSize;
        size -= packagedSize;
    }

    // Insert tags after the last cluster
    for (i = 0; i < MKV_TEST_TAG_COUNT; i++) {
        packagedSize = size;
        SPRINTF(tagName, (PCHAR) "testTag_%d", i);
        SPRINTF(tagVal, (PCHAR) "testTag_%d_Value", i);
        EXPECT_EQ(STATUS_SUCCESS, mkvgenGenerateTag(mkvGenerator,
                                                    mkvBuffer + index,
                                                    tagName,
                                                    tagVal,
                                                    &packagedSize));
        index += packagedSize;
        size -= packagedSize;
    }

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_mixed_tags.mkv", TRUE, FALSE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CodecPrivateData)
{
    PMkvGenerator mkvGenerator;
    EncodedFrameInfo encodedFrameInfo;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    UINT32 i, cpdSize;
    UINT32 cpdSizes[3] = {0x80 - 2, 0x4000 - 2, 0x4000 - 1}; // edge cases for 1, 2 and 3 byte sizes
    PBYTE tempBuffer = (PBYTE) MEMCALLOC(MKV_MAX_CODEC_PRIVATE_LEN, 1);
    MEMSET(tempBuffer, 0x12, MKV_MAX_CODEC_PRIVATE_LEN);
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    for (i = 0; i < 3; i++) {
        cpdSize = cpdSizes[i];
        mTrackInfo.codecPrivateDataSize = cpdSize;
        mTrackInfo.codecPrivateData = tempBuffer;
        EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                     MKV_TEST_BEHAVIOR_FLAGS,
                                                     MKV_TEST_TIMECODE_SCALE,
                                                     MKV_TEST_CLUSTER_DURATION,
                                                     MKV_TEST_SEGMENT_UUID,
                                                     &mTrackInfo,
                                                     mTrackInfoCount,
                                                     MKV_TEST_CLIENT_ID,
                                                     NULL,
                                                     0,
                                                     &mkvGenerator));

        // Add a frame
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

        // Validate that it's a start of the stream + cluster
        EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
        EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

        // Add a Non-key frame
        frame.decodingTs += MKV_TEST_FRAME_DURATION;
        frame.presentationTs += MKV_TEST_FRAME_DURATION;
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

        // Validate that it's a simple block
        EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

        // Free the generator
        EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));
    }

    MEMFREE(tempBuffer);
    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_ClusterBoundaryNonKeyFrame)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                    MKV_GEN_IN_STREAM_TIME,
                                                    MKV_TEST_TIMECODE_SCALE,
                                                    MKV_TEST_CLUSTER_DURATION,
                                                    MKV_TEST_SEGMENT_UUID,
                                                    &mTrackInfo,
                                                    mTrackInfoCount,
                                                    MKV_TEST_CLIENT_ID,
                                                    NULL,
                                                    0,
                                                    &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Iterate over the cluster duration without a key frame and ensure we are not adding a new cluster
    for (i = 0; i < 2 * (MKV_TEST_CLUSTER_DURATION / MKV_TEST_FRAME_DURATION); i++) {
        // Add a Non-key frame
        frame.decodingTs += MKV_TEST_FRAME_DURATION;
        frame.presentationTs += MKV_TEST_FRAME_DURATION;
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

        // Validate that it's a simple block
        EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

        // Adding with a buffer
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

        // Validate that it's a simple block
        EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);
    }

    // Add a key frame
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a start of the new cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the new cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a key frame
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, &encodedFrameInfo)));

    // Validate that it's NOT a start of the new cluster
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's NOT a start of the new cluster
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));

    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_TimeCallbackCalls)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    UINT32 retSize;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_KEY_FRAME_PROCESSING, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, MKV_TEST_CLIENT_ID, getTimeCallback,
                                                 MKV_TEST_CUSTOM_DATA, &mkvGenerator));

    // Add a key frame - get size first
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, NULL));

    // Time function should get called to determine the state of the MKV stream
    EXPECT_TRUE(gTimeCallbackCalled);

    // Reset the value to retry
    gTimeCallbackCalled = FALSE;

    // Package the frame in the buffer
    retSize = size;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &retSize, NULL));

    // Time function shouldn't have been called
    EXPECT_TRUE(gTimeCallbackCalled);

    // Validate the returned size
    EXPECT_EQ(size, retSize);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));

    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_TimeCallbackCallsNotCalled)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    UINT32 retSize;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS,
                                                    MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION,
                                                    MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                    MKV_TEST_CLIENT_ID, getTimeCallback,
                                                    MKV_TEST_CUSTOM_DATA, &mkvGenerator)));

    // Add a key frame - get size first
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, NULL, &size, NULL)));

    // Time function shouldn't have been called
    EXPECT_FALSE(gTimeCallbackCalled);

    // Package the frame in the buffer
    retSize = size;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &retSize, NULL)));

    // Time function shouldn't have been called
    EXPECT_FALSE(gTimeCallbackCalled);

    // Validate the returned size
    EXPECT_EQ(size, retSize);

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));

    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGenerator_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    MEMFREE(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGeneratorWithAvccAdaptation_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(size);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf, MKV_TEST_TRACKID};
    UINT32 i, runSize = 100 - SIZEOF(UINT32);
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    // Set the frame buffer
    for (i = 0; i < size; i += 100) {
        // Set every 100 bytes
        putInt32((PINT32) (frame.frameData + i), runSize);
    }

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_AVCC_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    MEMFREE(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGeneratorWithAnnexBAdaptation_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf, MKV_TEST_TRACKID};
    UINT32 i, adaptedSize;
    EncodedFrameInfo encodedFrameInfo;
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    for (i = 0; i < SIZEOF(frameBuf);) {
        frame.frameData[i] = 0;
        frame.frameData[i + 1] = 0;
        frame.frameData[i + 2] = 1;
        i += 100;
    }

    // 1 more byte for every Annex-B NAL
    adaptedSize = SIZEOF(frameBuf) / 100 + SIZEOF(frameBuf);

    // Try for non-video track type. The frame shouldn't get adapted
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(SIZEOF(frameBuf) + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));

    trackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
    // Try for non-video track type. The frame shouldn't get adapted
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &mkvGenerator));

    size = MKV_TEST_BUFFER_SIZE;
    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(adaptedSize + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block

    EXPECT_EQ(adaptedSize + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(adaptedSize + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(adaptedSize + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(adaptedSize + mkvgenGetMkvHeaderOverhead((PStreamMkvGenerator) mkvGenerator), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(mkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenMkvTracksHeaderWithMultipleTrack)
{
    PMkvGenerator pMkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE videoCpd[] = {0x01, 0x4d, 0x00, 0x20, 0xff, 0xe1, 0x00, 0x23, 0x27, 0x4d, 0x00, 0x20, 0x89, 0x8b, 0x60, 0x28, 0x02,
                        0xdd, 0x80, 0x9e, 0x00, 0x00, 0x4e, 0x20, 0x00, 0x0f, 0x42, 0x41, 0xc0, 0xc0, 0x01, 0x77, 0x00, 0x00,
                        0x5d, 0xc1, 0x7b, 0xdf, 0x07, 0xc2, 0x21, 0x1b, 0x80, 0x01, 0x00, 0x04, 0x28, 0xee, 0x1f, 0x20};
    UINT32 videoCpdSize = SIZEOF(videoCpd);
    BYTE audioCpd[] = {0x12, 0x10, 0x56, 0xe5, 0x00};
    UINT32 audioCpdSize = SIZEOF(audioCpd);
    TrackInfo trackInfo;
    trackInfo.trackId = MKV_TEST_TRACKID;

    TrackInfo testTrackInfo[2];
    // video track;
    testTrackInfo[0].trackId = MKV_TEST_TRACKID;
    STRCPY(testTrackInfo[0].codecId, MKV_TEST_CODEC_ID);
    STRCPY(testTrackInfo[0].trackName, MKV_TEST_TRACK_NAME);
    testTrackInfo[0].codecPrivateDataSize = videoCpdSize;
    testTrackInfo[0].codecPrivateData = videoCpd;
    testTrackInfo[0].trackCustomData.trackVideoConfig.videoWidth = 1280;
    testTrackInfo[0].trackCustomData.trackVideoConfig.videoHeight = 720;
    testTrackInfo[0].trackType = MKV_TRACK_INFO_TYPE_VIDEO;

    // audio track;
    testTrackInfo[1].trackId = MKV_TEST_TRACKID + 1;
    STRCPY(testTrackInfo[1].codecId, MKV_TEST_CODEC_ID);
    STRCPY(testTrackInfo[1].trackName, MKV_TEST_TRACK_NAME);
    testTrackInfo[1].codecPrivateDataSize = audioCpdSize;
    testTrackInfo[1].codecPrivateData = audioCpd;
    testTrackInfo[1].trackCustomData.trackAudioConfig.channelConfig = 2;
    testTrackInfo[1].trackCustomData.trackAudioConfig.samplingFrequency = 44100;
    testTrackInfo[1].trackCustomData.trackAudioConfig.bitDepth = 0;
    testTrackInfo[1].trackType = MKV_TRACK_INFO_TYPE_AUDIO;

    UINT32 testTrackInfoCount = 2;
    BYTE frameBuf[10000];
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf, MKV_TEST_TRACKID};
    EncodedFrameInfo encodedFrameInfo;
    PBYTE pCurrentPnt = NULL;
    BYTE tracksElementId[] = {0x16, 0x54, 0xae, 0x6b};
    UINT32 mkvTracksElementSize = 0;
    UINT32 encodedCpdLen = 0;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_AAC_MULTI_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 testTrackInfo,
                                                 testTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));
    UINT32 encodedLen;
    mkvgenEbmlEncodeSegmentInfo((PStreamMkvGenerator) pMkvGenerator, NULL, 0, &encodedLen);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, &trackInfo, mBuffer, &size, &encodedFrameInfo));

    // Jump to beginning of Tracks element
    pCurrentPnt = mBuffer + MKV_EBML_SEGMENT_SIZE + encodedLen;

    // Verify that the next 4 byte is Tracks element's id
    EXPECT_EQ(0, MEMCMP(pCurrentPnt, tracksElementId, SIZEOF(tracksElementId)));

    // Calculated size of the mkv Tracks element
    mkvTracksElementSize = mkvgenGetMkvTrackHeaderSize(testTrackInfo, testTrackInfoCount) - MKV_TRACKS_ELEM_BITS_SIZE;

    // Jump to the actual size of the mkv Tracks element stored in buffer
    pCurrentPnt += SIZEOF(mkvTracksElementSize);

    // Verify that the actual value matches calculation
    EXPECT_EQ(mkvTracksElementSize, getInt32(*((PINT32) pCurrentPnt)) & 0x0fffffff);

    // Jump over the size bits (allocated 4 byte)
    pCurrentPnt += 4;

    // Calculate the EBML encoded cpd size for video cpd
    mkvgenEbmlEncodeNumber(videoCpdSize,
                           NULL,
                           0,
                           &encodedCpdLen);

    // Jump to the beginning of video cpd bits. mkvgenGetMkvTrackHeaderSize includes MKV_TRACKS_ELEM_BITS_SIZE but we just
    // want the track entry size, therefore subtract MKV_TRACKS_ELEM_BITS_SIZE
    pCurrentPnt += (mkvgenGetMkvTrackHeaderSize(testTrackInfo, 1) - MKV_TRACKS_ELEM_BITS_SIZE - videoCpdSize);

    // Verify that the bits match
    EXPECT_EQ(0, MEMCMP(pCurrentPnt, videoCpd, videoCpdSize));

    // Jump to the beginning of audio cpd bits
    pCurrentPnt += videoCpdSize;

    mkvgenEbmlEncodeNumber(audioCpdSize,
                           NULL,
                           0,
                           &encodedCpdLen);

    // Jump to the beginning of audio cpd bits. mkvgenGetMkvTrackHeaderSize includes MKV_TRACKS_ELEM_BITS_SIZE but we just
    // want the track entry size, therefore subtract MKV_TRACKS_ELEM_BITS_SIZE
    pCurrentPnt += (mkvgenGetMkvTrackHeaderSize(testTrackInfo + 1, 1) - MKV_TRACKS_ELEM_BITS_SIZE - audioCpdSize);

    // Verify that the bits match
    EXPECT_EQ(0, MEMCMP(pCurrentPnt, audioCpd, audioCpdSize));
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenExtractCpd_Variations)
{
    BYTE frameBuf[10000];
    UINT32 size = SIZEOF(frameBuf);
    Frame frame = {FRAME_CURRENT_VERSION, 0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf) / 2, frameBuf, MKV_TEST_TRACKID};
    EncodedFrameInfo encodedFrameInfo;
    PMkvGenerator pMkvGenerator;
    PStreamMkvGenerator pStreamMkvGenerator;
    PTrackInfo pTrackInfo;

    BYTE cpdH264[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x34, 0xAC, 0x2B, 0x40, 0x1E, 0x00, 0x78, 0xD8, 0x08,
                      0x80, 0x00, 0x01, 0xF4, 0x00, 0x00, 0xEA, 0x60, 0x47, 0xA5, 0x50, 0x00, 0x00, 0x00, 0x01, 0x68,
                      0xEE, 0x3C, 0xB0};
    BYTE cpdH264Aud[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x34, 0xAC, 0x2B, 0x40, 0x1E, 0x00, 0x78, 0xD8, 0x08,
                         0x80, 0x00, 0x01, 0xF4, 0x00, 0x00, 0xEA, 0x60, 0x47, 0xA5, 0x50, 0x00, 0x00, 0x00, 0x01, 0x68,
                         0xEE, 0x3C, 0xB0};
    BYTE cpdH264_2[] =  {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
                         0xa9, 0x50, 0x14, 0x07, 0xb4, 0x20, 0x00, 0x00,
                         0x7d, 0x00, 0x00, 0x1d, 0x4c, 0x00, 0x80, 0x00,
                         0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80};
    BYTE cpdH265[] = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                      0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x3c, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                      0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                      0x3c, 0xa0, 0x18, 0x20, 0x28, 0x71, 0x31, 0x39, 0x6b, 0xb9, 0x32, 0x4b, 0xb9, 0x48, 0x28, 0x10,
                      0x10, 0x17, 0x68, 0x50, 0x94, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20};
    BYTE cpdH265Aud[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                         0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x3c, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                         0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                         0x3c, 0xa0, 0x18, 0x20, 0x28, 0x71, 0x31, 0x39, 0x6b, 0xb9, 0x32, 0x4b, 0xb9, 0x48, 0x28, 0x10,
                         0x10, 0x17, 0x68, 0x50, 0x94, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20};

    // I-frame from a real-life encoder output which is actually invalid Annex-B format (shortened after a few bytes of the actual frame)
    BYTE cpdH264AudSeiExtra0[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1E, 0xAC,
                        0x1B, 0x1A, 0x80, 0xB0, 0x3D, 0xFF, 0xFF, 0x00, 0x28, 0x00, 0x21, 0x6E, 0x0C, 0x0C, 0x0C, 0x80,
                        0x00, 0x01, 0xF4, 0x00, 0x00, 0x75, 0x30, 0x74, 0x30, 0x07, 0xD0, 0x00, 0x01, 0x31, 0x2D, 0x5D,
                        0xE5, 0xC6, 0x86, 0x00, 0xFA, 0x00, 0x00, 0x26, 0x25, 0xAB, 0xBC, 0xB8, 0x50, 0x00, 0x00, 0x00,
                        0x00, 0x01, 0x68, 0xEE, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x0D, 0xBC, 0xFF,
                        0x87, 0x49, 0xB5, 0x16, 0x3C, 0xFF, 0x87, 0x49, 0xB5, 0x16, 0x40, 0x01, 0x04, 0x00, 0x78, 0x08,
                        0x10, 0x06, 0x01, 0xC4, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x65, 0xB8, 0x00, 0x02, 0x00, 0x00,
                        0x03, 0x02, 0x7F, 0xEC, 0x0E, 0xD0, 0xE1, 0xA7, 0x9D, 0xA3, 0x7C, 0x49, 0x42, 0xC2, 0x23, 0x59,};

    BYTE cpdH265WithIdr_N_Lp[] = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60,
                                  0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                                  0x5a, 0x95, 0x98, 0x09, 0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x01,
                                  0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
                                  0x00, 0x5a, 0xa0, 0x05, 0x02, 0x01, 0xe1, 0x65, 0x95, 0x9a, 0x49, 0x32,
                                  0xbf, 0xfc, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x03, 0x00, 0x04,
                                  0x00, 0x00, 0x03, 0x00, 0x64, 0x20, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01,
                                  0xc1, 0x72, 0xb4, 0x62, 0x40, 0x00, 0x00, 0x00, 0x01, 0x28, 0x01, 0xaf,
                                  0x3c, 0x40, 0xeb, 0x4d, 0x2b, 0x2c, 0x9a, 0x4a, 0x2f, 0x09, 0x56, 0xe0,
                                  0xdd, 0x7b, 0x56, 0xe5, 0x00, 0xc5, 0xb4, 0xbc, 0x5c, 0x27, 0x8a, 0xc0,
                                  0x7e, 0xaa, 0x7d, 0xc6, 0xbf, 0xec, 0xc1, 0xb0, 0x8a, 0xfd, 0x07, 0x3a,};

    BYTE annexBStart[] = {0x00, 0x00, 0x00, 0x01};

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264, SIZEOF(cpdH264));
    MEMCPY(frame.frameData + SIZEOF(cpdH264), annexBStart, SIZEOF(annexBStart));

    // Set an IDR NALu header
    frame.frameData[SIZEOF(cpdH264) + SIZEOF(annexBStart)] = 0x65;

    //
    // Non-key frame scenario
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Non-H264 frame scenario
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator((PCHAR) "abc",
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Add a Non-key frame
    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // No adaptation specified
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Add a Key frame
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // CPD already specified
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize = 10;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_EQ(10, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    // Reset to 0 so we won't teardown non-allocated memory
    pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize = 0;
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Invalid CPD - no I-frame bits
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    size = SIZEOF(frameBuf);
    frame.size = SIZEOF(cpdH264);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));
    frame.size = SIZEOF(frameBuf) / 2;

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Invalid CPD
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Invalid - H265 with invalid H264 content type
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH265, SIZEOF(cpdH265));
    MEMCPY(frame.frameData + SIZEOF(cpdH265), annexBStart, SIZEOF(annexBStart));

    // Set the NALu header
    frame.frameData[SIZEOF(cpdH265) + SIZEOF(annexBStart)] = 0x26;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // NOTE: Should have failed to extract the CPD due to the wrong NALu IDR slice compared to the content type
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H264
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264, SIZEOF(cpdH264));
    MEMCPY(frame.frameData + SIZEOF(cpdH264), annexBStart, SIZEOF(annexBStart));

    // Set an IDR NALu header
    frame.frameData[SIZEOF(cpdH264) + SIZEOF(annexBStart)] = 0x65;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(3840, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(1920, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H264 with AUD
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264Aud, SIZEOF(cpdH264Aud));
    MEMCPY(frame.frameData + SIZEOF(cpdH264Aud), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264Aud) + SIZEOF(annexBStart)] = 0x65;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(3840, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(1920, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H265 with content type of H265
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H265_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH265, SIZEOF(cpdH265));
    MEMCPY(frame.frameData + SIZEOF(cpdH265), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH265) + SIZEOF(annexBStart)] = 0x26;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(176, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(144, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H265 with content type of H265 with AUD
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H265_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH265Aud, SIZEOF(cpdH265Aud));
    MEMCPY(frame.frameData + SIZEOF(cpdH265Aud), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH265Aud) + SIZEOF(annexBStart)] = 0x26;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(176, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(144, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H264 with existing CPD
    //
    mTrackInfo.codecPrivateDataSize = SIZEOF(cpdH264);
    mTrackInfo.codecPrivateData = cpdH264;
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    EXPECT_EQ(SIZEOF(cpdH264), pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(3840, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(1920, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264_2, SIZEOF(cpdH264_2));
    MEMCPY(frame.frameData + SIZEOF(cpdH264_2), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264_2) + SIZEOF(annexBStart)] = 0x65;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure the data hasn't changed
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(3840, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(1920, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H264 set CPD after a couple of frames
    //
    mTrackInfo.codecPrivateDataSize = 0;
    mTrackInfo.codecPrivateData = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer without CPD
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure the data hasn't changed
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer with CPD but non-key-frame
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264_2, SIZEOF(cpdH264_2));
    MEMCPY(frame.frameData + SIZEOF(cpdH264_2), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264_2) + SIZEOF(annexBStart)] = 0x65;
    frame.flags = FRAME_FLAG_NONE;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure the data hasn't changed
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer with CPD and different track
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264_2, SIZEOF(cpdH264_2));
    MEMCPY(frame.frameData + SIZEOF(cpdH264_2), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264_2) + SIZEOF(annexBStart)] = 0x65;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    pTrackInfo->trackId++;

    size = SIZEOF(frameBuf);
    EXPECT_NE(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));
    pTrackInfo->trackId--;

    // Ensure the data hasn't changed
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer with CPD
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264_2, SIZEOF(cpdH264_2));
    MEMCPY(frame.frameData + SIZEOF(cpdH264_2), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264_2) + SIZEOF(annexBStart)] = 0x65;
    frame.flags = FRAME_FLAG_KEY_FRAME;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure the data hasn't changed
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(640, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(480, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer with a different CPD - ensure it does't change
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    MEMCPY(frame.frameData, cpdH264, SIZEOF(cpdH264));
    MEMCPY(frame.frameData + SIZEOF(cpdH264), annexBStart, SIZEOF(annexBStart));
    frame.frameData[SIZEOF(cpdH264) + SIZEOF(annexBStart)] = 0x65;
    frame.flags = FRAME_FLAG_KEY_FRAME;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure the data hasn't changed
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(640, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(480, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // InValid H264 getting fixed up with content type of H264
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    frame.size = SIZEOF(cpdH264AudSeiExtra0);
    MEMCPY(frame.frameData, cpdH264AudSeiExtra0, frame.size);

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(704, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(480, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // InValid H264 getting fixed up with content type of H264 with CPD adaptation
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS | MKV_GEN_ADAPT_ANNEXB_CPD_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    frame.size = SIZEOF(cpdH264AudSeiExtra0);
    MEMCPY(frame.frameData, cpdH264AudSeiExtra0, frame.size);

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(704, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(480, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));

    //
    // Valid H265 with IDR_N_LP in Annex-B format
    //
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H265_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS | MKV_GEN_ADAPT_ANNEXB_CPD_NALS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &mTrackInfo,
                                                 mTrackInfoCount,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    pTrackInfo = &pStreamMkvGenerator->trackInfoList[0];

    // Ensure we have no width/height or CPD
    EXPECT_EQ(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_EQ((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_WIDTH, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(MKV_TEST_DEFAULT_TRACK_HEIGHT, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Set the frame buffer
    frame.size = SIZEOF(cpdH265WithIdr_N_Lp);
    MEMCPY(frame.frameData, cpdH265WithIdr_N_Lp, frame.size);
    frame.flags = FRAME_FLAG_KEY_FRAME;

    size = SIZEOF(frameBuf);
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, pTrackInfo, mBuffer, &size, &encodedFrameInfo));

    // Ensure we have no width/height or CPD
    EXPECT_NE(0, pStreamMkvGenerator->trackInfoList[0].codecPrivateDataSize);
    EXPECT_NE((PBYTE) NULL, pStreamMkvGenerator->trackInfoList[0].codecPrivateData);
    EXPECT_EQ(640, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoWidth);
    EXPECT_EQ(480, pStreamMkvGenerator->trackInfoList[0].trackCustomData.trackVideoConfig.videoHeight);

    // Free the generator
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenExtractCpd_G711Alaw)
{
    BYTE alawCpd[] = {0x06, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00};
    TrackInfo trackInfo;
    PMkvGenerator pMkvGenerator;
    PStreamMkvGenerator pStreamMkvGenerator;

    trackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo.version = TRACK_INFO_CURRENT_VERSION;
    trackInfo.codecPrivateDataSize = SIZEOF(alawCpd);
    trackInfo.codecPrivateData = alawCpd;
    trackInfo.trackId = 1;
    STRCPY(trackInfo.codecId, (PCHAR) "audio/teststream");
    STRCPY(trackInfo.trackName, MKV_TEST_TRACK_NAME);

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_ALAW_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &trackInfo,
                                                 1,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    EXPECT_TRUE((DOUBLE) 8000 == pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(1, pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(16, pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.bitDepth);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenExtractCpd_G711Mulaw)
{
    BYTE alawCpd[] = {0x07, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00};
    TrackInfo trackInfo;
    PMkvGenerator pMkvGenerator;
    PStreamMkvGenerator pStreamMkvGenerator;

    trackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    trackInfo.version = TRACK_INFO_CURRENT_VERSION;
    trackInfo.codecPrivateDataSize = SIZEOF(alawCpd);
    trackInfo.codecPrivateData = alawCpd;
    trackInfo.trackId = 1;
    STRCPY(trackInfo.codecId, (PCHAR) "audio/teststream");
    STRCPY(trackInfo.trackName, MKV_TEST_TRACK_NAME);

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_MULAW_CONTENT_TYPE,
                                                 MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID,
                                                 &trackInfo,
                                                 1,
                                                 MKV_TEST_CLIENT_ID,
                                                 NULL,
                                                 0,
                                                 &pMkvGenerator));

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    EXPECT_TRUE((DOUBLE) 8000 == pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.samplingFrequency);
    EXPECT_EQ(1, pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.channelConfig);
    EXPECT_EQ(8, pStreamMkvGenerator->trackInfoList->trackCustomData.trackAudioConfig.bitDepth);
    EXPECT_EQ(STATUS_SUCCESS, freeMkvGenerator(pMkvGenerator));
}
