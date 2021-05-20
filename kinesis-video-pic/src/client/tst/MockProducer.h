#ifndef __MOCK_PRODUCER_H__
#define __MOCK_PRODUCER_H__

typedef struct {
    UINT32 mFps;
    UINT32 mKeyFrameInterval;
    UINT32 mFrameSizeByte;
    UINT64 mFrameTrackId;
    BOOL mIsLive;
    BOOL mSetEOFR;
} MockProducerConfig;

class MockProducer {
    UINT32 mFps;
    UINT32 mKeyFrameInterval;
    UINT64 mNextPutFrameTime; // Mark time at which putFrame can happen. In hundreds of nanos.
    STREAM_HANDLE mStreamHandle;
    BOOL mIsLive;
    Frame mFrame;
    UINT32 mIndex;
    UINT64 mTimestamp; // Used to assigned to Frame pts and dts. In hundreds of nanos.
    BOOL mSetEOFR;

  public:
    MockProducer(MockProducerConfig config, STREAM_HANDLE mStreamHandle);

    ~MockProducer()
    {
        MEMFREE(mFrame.frameData);
    }

    /**
     * call putKinesisVideoFrame immediately with a time aligned frame.
     *
     * @return STATUS code of putKinesisVideoFrame
     */
    STATUS putFrame(BOOL eofr = FALSE, UINT64 trackId = TEST_VIDEO_TRACK_ID);

    /**
     * time based putFrame
     *
     * @param 1 UINT64 - Current time.
     * @param 2 PBOOL - Whether putFrame was called.
     * @param 3 UINT64 - Track id for the frame to set, Defaults to video
     *
     * @return STATUS code of putKinesisVideoFrame if it happened, otherwise STATUS_SUCCESS
     */
    STATUS timedPutFrame(UINT64 currentTime, PBOOL pDidPutFrame, UINT64 trackId = TEST_VIDEO_TRACK_ID);

    /**
     * Returns the current frame which can be used in the tests
     *
     * @return Current frame pointer
     */
    PFrame getCurrentFrame();
};

#endif //__MOCK_PRODUCER_H__
