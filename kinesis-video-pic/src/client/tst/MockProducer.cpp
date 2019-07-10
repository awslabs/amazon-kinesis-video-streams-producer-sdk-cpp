#include "MockProducer.h"

MockProducer::MockProducer(MockProducerConfig config,
             STREAM_HANDLE mStreamHandle)
        : mFps(config.mFps),
          mKeyFrameInterval(config.mKeyFrameInterval),
          mIsLive(config.mIsLive),
          mSetEOFR(config.mSetEOFR),
          mStreamHandle(mStreamHandle),
          mNextPutFrameTime(0),
          mIndex(0),
          mTimestamp(0) {
    mFrame.frameData = (PBYTE) MEMALLOC(config.mFrameSizeByte);
    MEMSET(mFrame.frameData, 0x00, config.mFrameSizeByte);
    mFrame.size = config.mFrameSizeByte;
    mFrame.flags = FRAME_FLAG_NONE;
    mFrame.trackId = config.mFrameTrackId;
    mFrame.duration = (UINT64) 1000 / mFps * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
}

STATUS MockProducer::putFrame(BOOL eofr) {
    STATUS retStatus = STATUS_SUCCESS;

    if (eofr) {
        Frame eofr = EOFR_FRAME_INITIALIZER;
        mIndex += (mKeyFrameInterval - (mIndex % mKeyFrameInterval)); // next frame must have key frame flag after eofr.
        retStatus = putKinesisVideoFrame(mStreamHandle, &eofr);
    } else {
        mFrame.flags = (mIndex % mKeyFrameInterval == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        mFrame.decodingTs = mTimestamp;
        mFrame.presentationTs = mTimestamp;
        mFrame.index = mIndex;

        retStatus = putKinesisVideoFrame(mStreamHandle, &mFrame);

        mIndex++;
        mTimestamp += mFrame.duration;
    }

    return retStatus;
}

STATUS MockProducer::timedPutFrame(UINT64 currentTime, PBOOL pDidPutFrame) {
    STATUS retStatus = STATUS_SUCCESS;
    *pDidPutFrame = FALSE;

    if (!mIsLive || currentTime >= mNextPutFrameTime) {
        *pDidPutFrame = TRUE;
        retStatus = putFrame();

        // If do EOFR and next frame is key frame, then put EOFR frame
        if (mSetEOFR && (mIndex % mKeyFrameInterval == 0)) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            retStatus = putKinesisVideoFrame(mStreamHandle, &eofrFrame);
        }

        if (mIsLive) {
            mNextPutFrameTime = currentTime + mFrame.duration;
        }
    }

    return retStatus;
}