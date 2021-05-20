#include "ClientTestFixture.h"

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

STATUS MockProducer::putFrame(BOOL isEofr, UINT64 trackId) {
    STATUS retStatus = STATUS_SUCCESS;

    mFrame.trackId = trackId;

    if (isEofr) {
        Frame eofr = EOFR_FRAME_INITIALIZER;
        mIndex += (mKeyFrameInterval - (mIndex % mKeyFrameInterval)); // next frame must have key frame flag after eofr.
        retStatus = putKinesisVideoFrame(mStreamHandle, &eofr);
    } else {
        mFrame.flags = (mIndex % mKeyFrameInterval == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        mFrame.decodingTs = mTimestamp;
        mFrame.presentationTs = mTimestamp;
        mFrame.index = mIndex;

        // Set the body of the frame so it's easier to track
        MEMSET(mFrame.frameData, mFrame.index, mFrame.size);

        retStatus = putKinesisVideoFrame(mStreamHandle, &mFrame);

        mIndex++;
        mTimestamp += mFrame.duration;
    }

    return retStatus;
}

STATUS MockProducer::timedPutFrame(UINT64 currentTime, PBOOL pDidPutFrame, UINT64 trackId) {
    STATUS putStatus = STATUS_SUCCESS, eofrPutStatus = STATUS_SUCCESS;
    *pDidPutFrame = FALSE;

    if (!mIsLive || currentTime >= mNextPutFrameTime) {
        *pDidPutFrame = TRUE;
        putStatus = putFrame(FALSE, trackId);

        // If do EOFR and next frame is key frame, then put EOFR frame
        if (mSetEOFR && (mIndex % mKeyFrameInterval == 0)) {
            Frame eofrFrame = EOFR_FRAME_INITIALIZER;
            eofrPutStatus = putKinesisVideoFrame(mStreamHandle, &eofrFrame);
        }

        if (mIsLive) {
            mNextPutFrameTime = currentTime + mFrame.duration;
        }
    }

    if (STATUS_FAILED(putStatus)) {
        return putStatus;
    } else if (STATUS_FAILED(eofrPutStatus)) {
        return eofrPutStatus;
    } else {
        return STATUS_SUCCESS;
    }
}

PFrame MockProducer::getCurrentFrame() {
    return &mFrame;
}
