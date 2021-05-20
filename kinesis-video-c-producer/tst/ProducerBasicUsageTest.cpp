#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerBasicUsageTest : public ProducerClientTestBase {
};

TEST_F(ProducerBasicUsageTest, videoOnlyUsage)
{
    STATUS retStatus = STATUS_SUCCESS;
    Frame frame;
    BYTE frameBuffer[500];
    UINT32 frameSize = SIZEOF(frameBuffer), frameIndex = 0;

    // Check if it's run with the env vars set if not bail out
    if (!mAccessKeyIdSet) {
        return;
    }

    createDefaultProducerClient(FALSE, 30 * HUNDREDS_OF_NANOS_IN_A_SECOND);
    ASSERT_EQ(STATUS_SUCCESS, createTestStream(0, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

    STREAM_HANDLE streamHandle = mStreams[0];

    UINT64 streamStopTime = GETTIME() + TEST_EXECUTION_DURATION;

    // setup dummy frame
    MEMSET(frameBuffer, 0x00, frameSize);
    frame.frameData = frameBuffer;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.flags = frameIndex % TEST_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
    frame.size = frameSize;
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND / TEST_KEY_FRAME_INTERVAL;
    frame.index = frameIndex++;
    frame.decodingTs = 0;
    frame.presentationTs = frame.decodingTs;

    while (GETTIME() < streamStopTime) {
        CHK_STATUS(putKinesisVideoFrame(streamHandle, &frame));

        THREAD_SLEEP(frame.duration);
        frame.index = frameIndex++;
        frame.flags = frameIndex % TEST_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        frame.decodingTs += frame.duration;
        frame.presentationTs = frame.decodingTs;
    }

    CHK_STATUS(stopKinesisVideoStreamSync(streamHandle));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        DLOGE("Failed with status 0x%08x\n", retStatus);
    }

    EXPECT_EQ(STATUS_SUCCESS, retStatus);
}

}
}
}
}