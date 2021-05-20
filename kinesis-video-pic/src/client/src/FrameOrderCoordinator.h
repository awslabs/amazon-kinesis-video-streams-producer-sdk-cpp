/*******************************************
PutFrameCoordinator internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_PUT_FRAME_COORDINATOR_INCLUDE_I__
#define __KINESIS_VIDEO_PUT_FRAME_COORDINATOR_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __FrameOrderTrackData FrameOrderTrackData;
struct __FrameOrderTrackData {
    PTrackInfo pTrackInfo;

    // current number of frames in the queue
    volatile UINT32 frameCount;

    PStackQueue frameQueue;
};
typedef struct __FrameOrderTrackData* PFrameOrderTrackData;

typedef STATUS (*FrameTimestampCompareFunc)(PFrameOrderTrackData, PFrameOrderTrackData, UINT64, PBOOL);

typedef struct __FrameOrderCoordinator FrameOrderCoordinator;
struct __FrameOrderCoordinator {
    // whether any eofr frame has been put
    volatile BOOL eofrPut;

    FrameOrderTrackData putFrameTrackDataList[MAX_SUPPORTED_TRACK_COUNT_PER_STREAM];

    UINT32 putFrameTrackDataListCount;

    // indicate whether any frame with FRAME_FLAG_KEY_FRAME set was received
    volatile BOOL keyFrameDetected;

    // number of tracks that has frame in the queue
    volatile UINT32 trackWithFrame;

    MUTEX lock;

    PStreamInfo pStreamInfo;

    FrameTimestampCompareFunc frameTimestampCompareFn;
};
typedef struct __FrameOrderCoordinator* PFrameOrderCoordinator;

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////
STATUS createFrameOrderCoordinator(PKinesisVideoStream, PFrameOrderCoordinator*);
STATUS freeFrameOrderCoordinator(PKinesisVideoStream, PFrameOrderCoordinator*);
STATUS frameOrderCoordinatorPutFrame(PKinesisVideoStream, PFrame);
STATUS frameOrderCoordinatorFlush(PKinesisVideoStream);

/**
 *
 * Normally, if the frame timestamps are different, frameOrderCoordinatorAudioVideoCompareFn would return -1 if frame 1's
 * timestamp is smaller and 1 otherwise. But if two frames' timestamp are equal after applying mkv timecode scale, and
 * one of them has key frame flag, then whichever frame that is put next must have the key frame flag, and the other one
 * should not have key frame flag, otherwise, previous cluster's ending frame would have the same timestamp as the next cluster.
 * The compare function would set the key frame flag correctly in such cases.
 *
 * @UINT64 - IN - timestamp of frame 1.
 * @PFrameOrderTrackData - IN - PFrameOrderTrackData that frame 1 belongs to.
 * @UINT64 - IN - timestamp of frame 2.
 * @PFrameOrderTrackData - IN - PFrameOrderTrackData that frame 2 belongs to.
 * @UINT64 - IN - custom data. In this case, the mkv timecode scale.
 * @PBOOL - OUT - compare result. If TRUE, first frame go first, else second frame go first.
 *
 * @return - STATUS - status code of the operation.
 */
STATUS audioVideoFrameTimestampComparator(PFrameOrderTrackData, PFrameOrderTrackData, UINT64, PBOOL);

#ifdef __cplusplus
}
#endif
#endif /*__KINESIS_VIDEO_PUT_FRAME_COORDINATOR_INCLUDE_I__*/
