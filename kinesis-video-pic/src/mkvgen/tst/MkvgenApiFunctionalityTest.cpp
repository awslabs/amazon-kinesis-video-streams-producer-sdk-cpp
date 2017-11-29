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
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
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
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MIN_TIMECODE_SCALE,
                                                    MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
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
    BYTE frameBuf[MKV_TEST_BUFFER_SIZE];
    CHAR fileName[256];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x01, 0x42, 0x40, 0x15, 0xFF, 0xE1, 0x00, 0x09, 0x67, 0x42, 0x40, 0x28, 0x96, 0x54, 0x0a, 0x0f, 0xc8, 0x01, 0x00, 0x04, 0x68, 0xce, 0x3c, 0x80};

    cpdSize = SIZEOF(cpd);
    // Pretend it's h264 to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_H264_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
            2 * HUNDREDS_OF_NANOS_IN_A_SECOND, "V_MJPEG", MKV_TEST_TRACK_NAME, cpd, cpdSize, NULL, 0, &mkvGenerator));

    for (i = 0, index = 0; i < 100; i++) {
        sprintf(fileName, "samples/gif%03d.jpg", i);
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile("test_h264_jpg.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CreateStoreMkvFromJpegAsFourCc)
{
    PMkvGenerator mkvGenerator;
    BYTE frameBuf[MKV_TEST_BUFFER_SIZE];
    CHAR fileName[256];
    UINT32 size = 5000000, packagedSize;
    PBYTE mkvBuffer = (PBYTE) MEMCALLOC(size, 1);
    UINT64 frameDuration = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, frameDuration, 0, frameBuf};
    UINT32 i, cpdSize, index;
    UINT64 fileSize;
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    cpdSize = SIZEOF(cpd);

    // This is a M-JPG to enforce the video width and height inclusion in the track info
    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_X_MKV_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
                                                 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, MKV_FOURCC_CODEC_ID,
                                                 MKV_TEST_TRACK_NAME, cpd, cpdSize, NULL, 0, &mkvGenerator));

    for (i = 0, index = 0; i < 100; i++) {
        sprintf(fileName, "samples/gif%03d.jpg", i);
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

    EXPECT_EQ(STATUS_SUCCESS, writeFile("test_jpeg.mkv", TRUE, mkvBuffer, index));
    MEMFREE(mkvBuffer);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_CodecPrivateData)
{
    PMkvGenerator mkvGenerator;
    EncodedFrameInfo encodedFrameInfo;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[100000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i, cpdSize;
    UINT32 cpdSizes[3] = {0x80 - 2, 0x4000 - 2, 0x4000 - 1}; // edge cases for 1, 2 and 3 byte sizes
    PBYTE tempBuffer = (PBYTE) MEMCALLOC(MKV_MAX_CODEC_PRIVATE_LEN, 1);
    MEMSET(tempBuffer, 0x12, MKV_MAX_CODEC_PRIVATE_LEN);

    for (i = 0; i < 3; i++) {
        cpdSize = cpdSizes[i];
        EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
                MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, tempBuffer, cpdSize, NULL, 0, &mkvGenerator));

        // Add a frame
        size = MKV_TEST_BUFFER_SIZE;
        EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

        // Validate that it's a start of the stream + cluster
        EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD + MKV_CODEC_PRIVATE_DATA_ELEM_SIZE + i + 1 + cpdSize, size);
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
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_ClusterBoundaryNonKeyFrame)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_IN_STREAM_TIME, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator)));

    // Add a Non-key frame
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);

    // Add a Non-key frame with a buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo)));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
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
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_TimeCallbackCalls)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    UINT32 retSize;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_GEN_KEY_FRAME_PROCESSING, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, getTimeCallback, MKV_TEST_CUSTOM_DATA, &mkvGenerator)));

    // Add a key frame - get size first
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, NULL, &size, NULL)));

    // Time function should get called to determine the state of the MKV stream
    EXPECT_TRUE(gTimeCallbackCalled);

    // Reset the value to retry
    gTimeCallbackCalled = FALSE;

    // Package the frame in the buffer
    retSize = size;
    EXPECT_TRUE(STATUS_SUCCEEDED(mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &retSize, NULL)));

    // Time function shouldn't have been called
    EXPECT_TRUE(gTimeCallbackCalled);

    // Validate the returned size
    EXPECT_EQ(size, retSize);

    // Free the generator
    EXPECT_TRUE(STATUS_SUCCEEDED(freeMkvGenerator(mkvGenerator)));
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenPackageFrame_TimeCallbackCallsNotCalled)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    UINT32 retSize;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_KEY_FRAME, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};

    EXPECT_TRUE(STATUS_SUCCEEDED(createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
            MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, getTimeCallback, MKV_TEST_CUSTOM_DATA, &mkvGenerator)));

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
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGenerator_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i;
    EncodedFrameInfo encodedFrameInfo;

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
                                                    MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
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
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
}

TEST_F(MkvgenApiFunctionalityTest, mkvgenResetGeneratorWithAvccAdaptation_Variations)
{
    PMkvGenerator mkvGenerator;
    UINT32 size = MKV_TEST_BUFFER_SIZE;
    BYTE frameBuf[10000];
    Frame frame = {0, FRAME_FLAG_NONE, 0, 0, MKV_TEST_FRAME_DURATION, 10000, frameBuf};
    UINT32 i, runSize = 100 - SIZEOF(UINT32);
    EncodedFrameInfo encodedFrameInfo;

    // Set the buffer info
    for (i = 0; i < SIZEOF(frameBuf);) {
        // Set every 100 bytes
        putInt32((PINT32) (frame.frameData + i), runSize);
        i += 100;
    }

    EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS | MKV_GEN_ADAPT_AVCC_NALS, MKV_TEST_TIMECODE_SCALE,
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
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
    EXPECT_EQ(10000 + MKV_HEADER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
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
                                                 MKV_TEST_CLUSTER_DURATION, MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mkvGenerator));

    // Add a Non-key frame
    EXPECT_EQ(STATUS_SUCCESS, mkvgenPackageFrame(mkvGenerator, &frame, mBuffer, &size, &encodedFrameInfo));

    // Validate that it's a start of the stream + cluster
    EXPECT_EQ(adaptedSize + MKV_HEADER_OVERHEAD, size);
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
    EXPECT_EQ(adaptedSize + MKV_HEADER_OVERHEAD, size);
    EXPECT_EQ(MKV_STATE_START_STREAM, encodedFrameInfo.streamState);
}