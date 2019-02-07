#include "MkvgenTestFixture.h"

class MkvgenApiFunctionalityTest : public MkvgenTestBase {
};

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_ClusterBoundaryKeyFrame)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

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
            EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));
        } else {
            EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, NULL)));
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
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE,
                                                    MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                    mTrackInfoCount, NULL, 0, &mkvGenerator)));
    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame - this time we should overflow int16 value for the timecode
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_MKV_LARGE_FRAME_TIMECODE, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, NULL));

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
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x01, 0x42, 0x40, 0x15, 0xFF, 0xE1, 0x00, 0x09, 0x67, 0x42, 0x40, 0x28, 0x96, 0x54, 0x0a, 0x0f, 0xc8, 0x01, 0x00, 0x04, 0x68, 0xce, 0x3c, 0x80};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];

    cpdSize = SIZEOF(cpd);
    STRCPY(mTrackInfo.codecId, (PCHAR) "V_MJPEG");
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    // Pretend it's h264 to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, NULL, 0, &mkvGenerator));
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
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mkvBuffer + index, &packagedSize, NULL));
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_h264_jpg.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvFromJpegAsFourCc)
{
    PMkvGenerator mkvGenerator;
    CHAR fileName[MAX_PATH_LEN];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];

    cpdSize = SIZEOF(cpd);
    STRCPY(mTrackInfo.codecId, MKV_FOURCC_CODEC_ID);
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    // This is a M-JPG to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_X_MKV_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, NULL, 0, &mkvGenerator));

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
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mkvBuffer + index, &packagedSize, NULL));
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_jpeg.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
    MEMFREE(frameBuf);
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_tags.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvMixedTags) {
    PMkvGenerator mkvGenerator;
    BYTE frameBuf[100];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, frameDuration, SIZEOF(frameBuf), frameBuf};
    UINT32 frameIndex, i, cpdSize, index = 0;
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHAR tagName[MKV_MAX_TAG_NAME_LEN];
    CHAR tagVal[MKV_MAX_TAG_VALUE_LEN];

    cpdSize = SIZEOF(cpd);

    STRCPY(mTrackInfo.codecId, MKV_FOURCC_CODEC_ID);
    mTrackInfo.codecPrivateDataSize = cpdSize;
    mTrackInfo.codecPrivateData = cpd;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_X_MKV_CONTENT_TYPE,
                                                 MKV_GEN_IN_STREAM_TIME | MKV_GEN_KEY_FRAME_PROCESSING,
                                                 MKV_TEST_TIMECODE_SCALE, 2 * HUNDREDS_OF_NANOS_IN_A_SECOND,
                                                 MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                 NULL, 0, &mkvGenerator));

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

            // Make sure adding a non-key frame fails on non start
            if (frame.index != 0) {
                frame.flags = FRAME_FLAG_NONE;
                packagedSize = size;
                EXPECT_NE(STATUS_SUCCESS,
                          mkvgenPackageFrame(mkvGenerator, &frame, mkvBuffer + index, &packagedSize, NULL));
                frame.flags = FRAME_FLAG_KEY_FRAME;
            }
        }

        // Add a frame
        packagedSize = size;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mkvBuffer + index, &packagedSize, NULL));
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_mixed_tags.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CodecPrivateData)
{
    PMkvGenerator mkvGenerator;
    EncodedFrameInfo encodedFrameInfo;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i, cpdSize;
    UINT32 cpdSizes[3] = {0x80 - 2, 0x4000 - 2, 0x4000 - 1}; // edge cases for 1, 2 and 3 byte sizes
    PBYTE tempBuffer = (PBYTE) MEMCALLOC(MKV_MAX_CODEC_PRIVATE_LEN, 1);
    MEMSET(tempBuffer, 0x12, MKV_MAX_CODEC_PRIVATE_LEN);

    for (i = 0; i < 3; i++) {
        cpdSize = cpdSizes[i];
        mTrackInfo.codecPrivateDataSize = cpdSize;
        mTrackInfo.codecPrivateData = tempBuffer;
        EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
                MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, NULL, 0, &mkvGenerator));

        // Add a frame
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

        // Validate that it's a start of the stream + cluster
        EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
        EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

        // Add a Non-key frame
        frame.decodingTs += MKV_TEST_FRAME_DURATION;
        frame.presentationTs += MKV_TEST_FRAME_DURATION;
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

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
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Iterate over the cluster duration without a key frame and ensure we are not adding a new cluster
    for (i = 0; i < 2 * (MKV_TEST_CLUSTER_DURATION / MKV_TEST_FRAME_DURATION); i++) {
        // Add a Non-key frame
        frame.decodingTs += MKV_TEST_FRAME_DURATION;
        frame.presentationTs += MKV_TEST_FRAME_DURATION;
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

        // Validate that it's a simple block
        EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

        // Adding with a buffer
        EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

        // Validate that it's a simple block
        EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
        EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);
    }

    // Add a key frame
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a start of the new cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the new cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a key frame
    frame.flags = FRAME_FLAG_KEY_FRAME;
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Validate that it's NOT a start of the new cluster
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Adding with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

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
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_KEY_FRAME_PROCESSING, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, getTimeCallback,
                                                 MKV_TEST_CUSTOM_DATA, &mkvGenerator));

    // Add a key frame - get size first
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, NULL));

    // Time function should get called to determine the state of the MKV stream
    EXPECT_TRUE(gTimeCallbackCalled);

    // Reset the value to retry
    gTimeCallbackCalled = FALSE;

    // Package the frame in the buffer
    retSize = size;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &retSize, NULL));

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
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS,
                                                    MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION,
                                                    MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                    getTimeCallback, MKV_TEST_CUSTOM_DATA, &mkvGenerator)));

    // Add a key frame - get size first
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, NULL)));

    // Time function shouldn't have been called
    EXPECT_FALSE(gTimeCallbackCalled);

    // Package the frame in the buffer
    retSize = size;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &retSize, NULL)));

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
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS,
                                                 MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION,
                                                 MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount,
                                                 NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGeneratorWithAvccAdaptation_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    PBYTE frameBuf = (PBYTE) MEMALLOC(size);
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i, runSize = 100 - SIZEOF(UINT32);
    EncodedFrameInfo encodedFrameInfo;

    // Set the buffer info
    for (i = 0; i < size; i += 100) {
        // Set every 100 bytes
        putInt32((PINT32) (frame.frameData + i), runSize);
    }

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_AVCC_NALS, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(10000 + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(10000 + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    MEMFREE(frameBuf);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGeneratorWithAnnexBAdaptation_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i, adaptedSize;
    EncodedFrameInfo encodedFrameInfo;

    // Set the buffer info
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuf));
    for (i = 0; i < SIZEOF(frameBuf);) {
        frame.frameData[i] = 0;
        frame.frameData[i + 1] = 0;
        frame.frameData[i + 2] = 1;
        i += 100;
    }

    // 1 more byte for every Annex-B NAL
    adaptedSize = SIZEOF(frameBuf) / 100 + SIZEOF(frameBuf);

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_ANNEXB_NALS, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, &mTrackInfo,
                                                 mTrackInfoCount, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(adaptedSize + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a simple block

    EXPECT_EQ(adaptedSize + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Add a key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_KEY_FRAME;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a cluster
    EXPECT_EQ(adaptedSize + MKV_CLUSTER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_CLUSTER, encodedFrameInfo.streamState);

    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    frame.flags = FRAME_FLAG_NONE;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));
    // Validate that it's a simple block
    EXPECT_EQ(adaptedSize + MKV_SIMPLE_BLOCK_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_BLOCK, encodedFrameInfo.streamState);

    // Reset and add a non-key frame and ensure we have a stream start
    EXPECT_EQ(STATUS_SUCCESS, mkvgenResetGenerator(mkvGenerator));
    // Add a Non-key frame
    frame.decodingTs += MKV_TEST_FRAME_DURATION;
    frame.presentationTs += MKV_TEST_FRAME_DURATION;
    size = MKV_TEST_BUFFER_SIZE;
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(adaptedSize + mkvgenGetMkvHeaderOverhead(&mTrackInfo, mTrackInfoCount), size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
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

    TrackInfo testTrackInfo[2];
    // video track;
    testTrackInfo[0].trackId = 0;
    STRCPY(testTrackInfo[0].codecId, MKV_TEST_CODEC_ID);
    STRCPY(testTrackInfo[0].trackName, MKV_TEST_TRACK_NAME);
    testTrackInfo[0].codecPrivateDataSize = videoCpdSize;
    testTrackInfo[0].codecPrivateData = videoCpd;
    testTrackInfo[0].trackCustomData.trackVideoConfig.videoWidth = 1280;
    testTrackInfo[0].trackCustomData.trackVideoConfig.videoHeight = 720;
    testTrackInfo[0].trackType = MKV_TRACK_INFO_TYPE_VIDEO;

    // audio track;
    testTrackInfo[1].trackId = 1;
    STRCPY(testTrackInfo[1].codecId, MKV_TEST_CODEC_ID);
    STRCPY(testTrackInfo[1].trackName, MKV_TEST_TRACK_NAME);
    testTrackInfo[1].codecPrivateDataSize = audioCpdSize;
    testTrackInfo[1].codecPrivateData = audioCpd;
    testTrackInfo[1].trackCustomData.trackAudioConfig.channelConfig = 2;
    testTrackInfo[1].trackCustomData.trackAudioConfig.samplingFrequency = 44100;
    testTrackInfo[1].trackType = MKV_TRACK_INFO_TYPE_AUDIO;

    UINT32 testTrackInfoCount = 2;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, SIZEOF(frameBuf), frameBuf};
    EncodedFrameInfo encodedFrameInfo;
    PBYTE pCurrentPnt = NULL;
    BYTE tracksElementId[] = {0x16, 0x54, 0xae, 0x6b};
    UINT32 mkvTracksElementSize = 0;
    PStreamMkvGenerator pStreamMkvGenerator = NULL;
    UINT32 encodedCpdLen = 0;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_SEGMENT_UUID, testTrackInfo,
                                                 testTrackInfoCount, NULL, 0, &pMkvGenerator));
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(pMkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Jump to beginning of Tracks element
    pCurrentPnt = mBuffer + (MKV_EBML_SEGMENT_SIZE + MKV_SEGMENT_INFO_BITS_SIZE);

    // Verify that the next 4 byte is Tracks element's id
    EXPECT_EQ(0, MEMCMP(pCurrentPnt, tracksElementId, SIZEOF(tracksElementId)));

    // Calculated size of the mkv Tracks element
    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    mkvTracksElementSize = mkvgenGetMkvTrackHeaderSize(testTrackInfo, testTrackInfoCount) - MKV_TRACKS_ELEM_BITS_SIZE;

    // Jump to the acutal size of the mkv Tracks element stored in buffer
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
}
