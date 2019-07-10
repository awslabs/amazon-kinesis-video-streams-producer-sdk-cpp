#ifndef __MOCK_PRODUCER_H__
#define __MOCK_PRODUCER_H__

#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/client/Include.h>

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
    MockProducer(MockProducerConfig config,
                 STREAM_HANDLE mStreamHandle);

    ~MockProducer() {
        MEMFREE(mFrame.frameData);
    }

    /**
     * call putKinesisVideoFrame immediately with a time aligned frame.
     *
     * @return STATUS code of putKinesisVideoFrame
     */
    STATUS putFrame(BOOL eofr = FALSE);

    /**
     * time based putFrame
     *
     * @param 1 UINT64 - Current time.
     * @param 2 PBOOL - Whether putFrame was called.
     *
     * @return STATUS code of putKinesisVideoFrame if it happened, otherwise STATUS_SUCCESS
     */
    STATUS timedPutFrame(UINT64 currentTime, PBOOL pDidPutFrame);
};


#endif //__MOCK_PRODUCER_H__
