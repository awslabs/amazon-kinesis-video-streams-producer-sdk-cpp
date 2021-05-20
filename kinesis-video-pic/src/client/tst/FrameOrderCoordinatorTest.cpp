#include "ClientTestFixture.h"

class FrameOrderCoordinatorTest : public ClientTestBase {
};

TEST_F(FrameOrderCoordinatorTest, avTsComparatorVariations)
{
    FrameOrderTrackData first, second;
    StreamInfo streamInfo;
    TrackInfo firstTrackInfo, secondTrackInfo;
    PStackQueue pFirstFrameQueue, pSecondFrameQueue;
    Frame firstFrame, secondFrame;
    BOOL isFirst;
    BYTE frameBuf[100];
    UINT64 curTime;

    // Negative tests
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(NULL, &second, (UINT64) &mStreamInfo, &isFirst));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, NULL, (UINT64) &mStreamInfo, &isFirst));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(NULL, NULL, (UINT64) &mStreamInfo, &isFirst));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &mStreamInfo, NULL));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, 0, &isFirst));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(NULL, NULL, (UINT64) &mStreamInfo, NULL));
    EXPECT_NE(STATUS_SUCCESS, audioVideoFrameTimestampComparator(NULL, NULL, 0, NULL));

    // Pass-through
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &mStreamInfo, &isFirst));

    // Create a test stackqueue to be used
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pFirstFrameQueue));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pSecondFrameQueue));

    // Copy the streaminfo and modify some fields
    streamInfo = mStreamInfo;
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR;

    // Fix-up timecode scale as the input validator in client would use 0 as sentinel value and set the internal value to default
    streamInfo.streamCaps.timecodeScale = DEFAULT_MKV_TIMECODE_SCALE;

    firstTrackInfo = mTrackInfo;
    secondTrackInfo = mTrackInfo;

    // Init the params
    first.frameCount = 10;
    first.frameQueue = pFirstFrameQueue;
    first.pTrackInfo = &firstTrackInfo;

    // Copy the data into the second
    second = first;
    second.frameQueue = pSecondFrameQueue;
    second.pTrackInfo = &secondTrackInfo;

    // Check with no frames
    EXPECT_EQ(STATUS_NOT_FOUND, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));

    // Add a single frame to first
    firstFrame.version = FRAME_CURRENT_VERSION;
    firstFrame.flags = FRAME_FLAG_NONE;
    curTime = GETTIME();
    firstFrame.decodingTs = curTime + 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    firstFrame.presentationTs = curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    firstFrame.index = 0;
    firstFrame.trackId = TEST_VIDEO_TRACK_ID;
    firstFrame.frameData = frameBuf;
    firstFrame.size = ARRAY_SIZE(frameBuf);

    secondFrame = firstFrame;
    secondFrame.trackId = TEST_AUDIO_TRACK_ID;
    firstFrame.index = 1;

    EXPECT_EQ(STATUS_SUCCESS, stackQueueEnqueue(pFirstFrameQueue, (UINT64) &firstFrame));

    // Should still be missing frame
    EXPECT_EQ(STATUS_NOT_FOUND, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));

    // Add second frame
    EXPECT_EQ(STATUS_SUCCESS, stackQueueEnqueue(pSecondFrameQueue, (UINT64) &secondFrame));

    // Should succeed
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);

    // Change the presentation ts to lower value but still first goes first
    secondFrame.presentationTs = curTime + 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);

    // Use the non-EoFR version
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);

    // Use pts values
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_FALSE(isFirst);

    // Check no swapping with same track types
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);
    EXPECT_EQ(curTime + 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, firstFrame.decodingTs);
    EXPECT_EQ(curTime + 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, secondFrame.decodingTs);

    // Check the second pts increases with different track types
    secondTrackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    secondFrame.presentationTs = firstFrame.presentationTs;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);
    EXPECT_EQ(firstFrame.decodingTs, secondFrame.decodingTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, firstFrame.presentationTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND + 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, secondFrame.presentationTs);

    // Check the second pts increases with different track types and both key frames
    secondTrackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    secondFrame.presentationTs = firstFrame.presentationTs;
    firstFrame.flags = FRAME_FLAG_KEY_FRAME;
    secondFrame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);
    EXPECT_EQ(firstFrame.decodingTs, secondFrame.decodingTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, firstFrame.presentationTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND + 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, secondFrame.presentationTs);

    // Check the second pts increases with different track types and first key frame and the second none
    secondTrackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    secondFrame.presentationTs = firstFrame.presentationTs;
    firstFrame.flags = FRAME_FLAG_KEY_FRAME;
    secondFrame.flags = FRAME_FLAG_NONE;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);
    EXPECT_EQ(firstFrame.decodingTs, secondFrame.decodingTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, firstFrame.presentationTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND + 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, secondFrame.presentationTs);

    // Check the first pts increases with different track types and first non-key frame and the second key frame
    secondTrackInfo.trackType = MKV_TRACK_INFO_TYPE_AUDIO;
    secondFrame.presentationTs = firstFrame.presentationTs;
    firstFrame.flags = FRAME_FLAG_NONE;
    secondFrame.flags = FRAME_FLAG_KEY_FRAME;
    EXPECT_EQ(STATUS_SUCCESS, audioVideoFrameTimestampComparator(&first, &second, (UINT64) &streamInfo, &isFirst));
    EXPECT_TRUE(isFirst);
    EXPECT_EQ(firstFrame.decodingTs, secondFrame.decodingTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, secondFrame.presentationTs);
    EXPECT_EQ(curTime + 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND + 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, firstFrame.presentationTs);

    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pFirstFrameQueue));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pSecondFrameQueue));
}
