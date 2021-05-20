/**
 * Kinesis Video Producer Response functionality
 */
#define LOG_CLASS "FrameOrderCoordinator"

#include "Include_i.h"

STATUS createFrameOrderCoordinator(PKinesisVideoStream pKinesisVideoStream, PFrameOrderCoordinator* ppFrameOrderCoordinator)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PFrameOrderCoordinator pFrameOrderCoordinator = NULL;
    UINT32 i = 0;

    CHK(pKinesisVideoStream != NULL && ppFrameOrderCoordinator != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Allocate the entire structure
    pFrameOrderCoordinator = (PFrameOrderCoordinator) MEMCALLOC(1, SIZEOF(FrameOrderCoordinator));
    CHK(pFrameOrderCoordinator != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pFrameOrderCoordinator->pStreamInfo = &pKinesisVideoStream->streamInfo;
    pFrameOrderCoordinator->eofrPut = FALSE;
    pFrameOrderCoordinator->keyFrameDetected = FALSE;
    pFrameOrderCoordinator->putFrameTrackDataListCount = pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount;
    pFrameOrderCoordinator->lock = pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, TRUE);
    switch (pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode) {
        case FRAME_ORDERING_MODE_MULTI_TRACK_AV:
        case FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE:
        case FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE:
        case FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR:
        case FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR:
            pFrameOrderCoordinator->frameTimestampCompareFn = audioVideoFrameTimestampComparator;
            break;
        case FRAME_ORDER_MODE_PASS_THROUGH:
            pFrameOrderCoordinator->frameTimestampCompareFn = NULL;
            break;
    }

    for (i = 0; i < pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount; ++i) {
        pFrameOrderCoordinator->putFrameTrackDataList[i].pTrackInfo = &pKinesisVideoStream->streamInfo.streamCaps.trackInfoList[i];
        pFrameOrderCoordinator->putFrameTrackDataList[i].frameCount = 0;
        CHK_STATUS(stackQueueCreate(&pFrameOrderCoordinator->putFrameTrackDataList[i].frameQueue));
    }

CleanUp:

    if (!STATUS_SUCCEEDED(retStatus)) {
        MEMFREE(pFrameOrderCoordinator);
        pFrameOrderCoordinator = NULL;
    }

    if (pFrameOrderCoordinator != NULL) {
        *ppFrameOrderCoordinator = pFrameOrderCoordinator;
    }

    return retStatus;
}

STATUS freeFrameOrderCoordinator(PKinesisVideoStream pKinesisVideoStream, PFrameOrderCoordinator* ppFrameOrderCoordinator)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i = 0;
    PFrameOrderCoordinator pFrameOrderCoordinator = NULL;
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(ppFrameOrderCoordinator != NULL, STATUS_NULL_ARG);
    pFrameOrderCoordinator = *ppFrameOrderCoordinator;
    CHK(pFrameOrderCoordinator != NULL, retStatus);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);

    // free the queue and its frames.
    for (i = 0; i < pFrameOrderCoordinator->putFrameTrackDataListCount; ++i) {
        CHK_STATUS(stackQueueClear(pFrameOrderCoordinator->putFrameTrackDataList[i].frameQueue, TRUE));
        CHK_STATUS(stackQueueFree(pFrameOrderCoordinator->putFrameTrackDataList[i].frameQueue));
    }

    // unlock and free the lock
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);
    pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);

    // free the entire struct
    MEMFREE(pFrameOrderCoordinator);

    *ppFrameOrderCoordinator = NULL;

CleanUp:

    return retStatus;
}

STATUS audioVideoFrameTimestampComparator(PFrameOrderTrackData pFrameOrderTrackData1, PFrameOrderTrackData pFrameOrderTrackData2, UINT64 customData,
                                          PBOOL pFirstFrameFirst)
{
    PStreamInfo pStreamInfo = (PStreamInfo) customData;
    UINT64 mkvTimestamp1, mkvTimestamp2, item;
    BOOL firstFrameFirst;
    PFrame pFirst, pSecond, pSwap;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pFirstFrameFirst != NULL && pFrameOrderTrackData1 != NULL && pFrameOrderTrackData2 != NULL && pStreamInfo != NULL, STATUS_NULL_ARG);

    CHK(pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE ||
            pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE ||
            pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR ||
            pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR,
        retStatus);

    CHK_STATUS(stackQueuePeek(pFrameOrderTrackData1->frameQueue, &item));
    pFirst = (PFrame) item;
    CHK_STATUS(stackQueuePeek(pFrameOrderTrackData2->frameQueue, &item));
    pSecond = (PFrame) item;

    if (pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE ||
        pStreamInfo->streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR) {
        mkvTimestamp1 = pFirst->decodingTs;
        mkvTimestamp2 = pSecond->decodingTs;
    } else {
        mkvTimestamp1 = pFirst->presentationTs;
        mkvTimestamp2 = pSecond->presentationTs;
    }

    mkvTimestamp1 /= pStreamInfo->streamCaps.timecodeScale;
    mkvTimestamp2 /= pStreamInfo->streamCaps.timecodeScale;

    firstFrameFirst = mkvTimestamp1 <= mkvTimestamp2;

    // if two frames from different tracks have same mkv timestamps, and the latter frame has a KEY_FRAME_FLAG set,
    // then move KEY_FRAME_FLAG to the earlier frame. Because we cant have frame before key frame that has the same
    // mkv timestamp as the key frame.
    // In case of the same MKV timestamps with the same track we won't do any auto-adjustment
    // and will let the call fail later with the same timestamps in the main PutFrame API call
    if (mkvTimestamp1 == mkvTimestamp2 &&
        pFrameOrderTrackData1->pTrackInfo->trackType != pFrameOrderTrackData2->pTrackInfo->trackType) {

        // If the first frame does not have key frame flag but the second does the we swap
        if (!CHECK_FRAME_FLAG_KEY_FRAME(pFirst->flags) && CHECK_FRAME_FLAG_KEY_FRAME(pSecond->flags)) {
            pSwap = pFirst;
            pFirst = pSecond;
            pSecond = pSwap;
        }

        // add 1 unit of mkvTimecodeScale to the latter frame if it has key frame flag so that backend dont complain
        // about fragment overlap. No need to change dts as it determines frame order and mkv contains pts only.
        pSecond->presentationTs += pStreamInfo->streamCaps.timecodeScale;
    }

CleanUp:

    if (pFirstFrameFirst != NULL) {
        *pFirstFrameFirst = firstFrameFirst;
    }

    return retStatus;
}

STATUS getEarliestTrack(PFrameOrderCoordinator pFrameOrderCoordinator, PFrameOrderTrackData* ppEarliestPutFrameTrackData)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 minIndex = 0, i;
    PFrameOrderTrackData pPutFrameTrackData = NULL;
    BOOL firstFrameFirst;

    CHK(pFrameOrderCoordinator != NULL && ppEarliestPutFrameTrackData != NULL, STATUS_NULL_ARG);
    CHK(pFrameOrderCoordinator->trackWithFrame > 0, retStatus);

    // find the first track that has frame.
    for (; minIndex < pFrameOrderCoordinator->putFrameTrackDataListCount && pFrameOrderCoordinator->putFrameTrackDataList[minIndex].frameCount == 0;
         ++minIndex)
        ;

    // Assumes the track found previously is the earliest, compare its first frame with rest of
    // of the tracks that has frame.
    for (i = minIndex + 1; i < pFrameOrderCoordinator->putFrameTrackDataListCount; ++i) {
        if (pFrameOrderCoordinator->putFrameTrackDataList[i].frameCount != 0) {
            CHK_STATUS(pFrameOrderCoordinator->frameTimestampCompareFn(&pFrameOrderCoordinator->putFrameTrackDataList[minIndex],
                                                                       &pFrameOrderCoordinator->putFrameTrackDataList[i],
                                                                       (UINT64) pFrameOrderCoordinator->pStreamInfo, &firstFrameFirst));

            if (!firstFrameFirst) {
                minIndex = i;
            }
        }
    }

    pPutFrameTrackData = &pFrameOrderCoordinator->putFrameTrackDataList[minIndex];

CleanUp:

    if (ppEarliestPutFrameTrackData != NULL) {
        *ppEarliestPutFrameTrackData = pPutFrameTrackData;
    }

    return retStatus;
}

/**
 * Compares each track by their first frame and submit the first frame from the earliest track.
 * @param pKinesisVideoStream
 * @return - STATUS - status of the operation
 */
STATUS putEarliestFrame(PKinesisVideoStream pKinesisVideoStream)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 item;
    PFrameOrderTrackData earliestPutFrameTrackData = NULL;
    PFrame pFrame = NULL;
    PFrameOrderCoordinator pFrameOrderCoordinator = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);
    pFrameOrderCoordinator = pKinesisVideoStream->pFrameOrderCoordinator;

    // return early if there is no track that has frame.
    CHK(pFrameOrderCoordinator->trackWithFrame > 0, retStatus);

    CHK_STATUS(getEarliestTrack(pFrameOrderCoordinator, &earliestPutFrameTrackData));
    CHK(earliestPutFrameTrackData != NULL, retStatus);

    CHK_STATUS(stackQueueDequeue(earliestPutFrameTrackData->frameQueue, &item));
    // bookkeeping
    earliestPutFrameTrackData->frameCount--;
    if (earliestPutFrameTrackData->frameCount == 0) {
        pFrameOrderCoordinator->trackWithFrame--;
    }

    pFrame = (PFrame) item;
    // if eofr frame has been put, next frame must be key frame.
    if (pFrameOrderCoordinator->eofrPut) {
        pFrameOrderCoordinator->eofrPut = FALSE;
        SET_FRAME_FLAG_KEY_FRAME(pFrame->flags);
    }

    CHK_STATUS(putFrame(pKinesisVideoStream, pFrame));

CleanUp:

    if (pFrame != NULL) {
        MEMFREE(pFrame);
    }

    return retStatus;
}

STATUS frameOrderCoordinatorPutFrame(PKinesisVideoStream pKinesisVideoStream, PFrame pUserFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PFrame pFrame;
    UINT32 overallSize, i = 0;
    BOOL locked = FALSE;
    PFrameOrderTrackData pPutFrameTrackData = NULL;
    PFrameOrderCoordinator pFrameOrderCoordinator = NULL;

    CHK(pKinesisVideoStream != NULL && pUserFrame != NULL, STATUS_NULL_ARG);
    // route traffic to putFrame if in FRAME_ORDER_MODE_PASS_THROUGH mode.
    if (pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode == FRAME_ORDER_MODE_PASS_THROUGH) {
        retStatus = putFrame(pKinesisVideoStream, pUserFrame);
        CHK(FALSE, retStatus);
    }

    pFrameOrderCoordinator = pKinesisVideoStream->pFrameOrderCoordinator;
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);
    locked = TRUE;

    for (i = 0; i < pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount; ++i) {
        if (pFrameOrderCoordinator->putFrameTrackDataList[i].pTrackInfo->trackId == pUserFrame->trackId) {
            pPutFrameTrackData = &pFrameOrderCoordinator->putFrameTrackDataList[i];
            break;
        }
    }

    CHK(pPutFrameTrackData != NULL || CHECK_FRAME_FLAG_END_OF_FRAGMENT(pUserFrame->flags), STATUS_MKV_TRACK_INFO_NOT_FOUND);
    if (!CHECK_FRAME_FLAG_END_OF_FRAGMENT(pUserFrame->flags)) {
        CHK(pPutFrameTrackData->frameCount <= DEFAULT_MAX_FRAME_QUEUE_SIZE_PER_TRACK, STATUS_MAX_FRAME_TIMESTAMP_DELTA_BETWEEN_TRACKS_EXCEEDED);
    }

    if (!pFrameOrderCoordinator->keyFrameDetected && CHECK_FRAME_FLAG_KEY_FRAME(pUserFrame->flags)) {
        pFrameOrderCoordinator->keyFrameDetected = TRUE;
    }

    // In case of AV sync modes which explicitly set EoFR after each fragment, the fragmentation
    // should not be driven by the key-frame flags but rather EoFR.
    if ((pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE ||
         pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE) &&
        pFrameOrderCoordinator->keyFrameDetected && CHECK_FRAME_FLAG_END_OF_FRAGMENT(pUserFrame->flags)) {
        CHK(FALSE, STATUS_SETTING_KEY_FRAME_FLAG_WHILE_USING_EOFR);
    }

    if (CHECK_FRAME_FLAG_END_OF_FRAGMENT(pUserFrame->flags)) {
        CHK_STATUS(frameOrderCoordinatorFlush(pKinesisVideoStream));
        CHK_STATUS(putFrame(pKinesisVideoStream, pUserFrame));
        // Only set the eofr flag in fragment EoFR modes
        if (pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE ||
            pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode == FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE_EOFR) {
            pFrameOrderCoordinator->eofrPut = TRUE;
        }

        CHK(FALSE, retStatus);
    }

    overallSize = SIZEOF(Frame) + pUserFrame->size;
    CHK(NULL != (pFrame = (PFrame) MEMALLOC(overallSize)), STATUS_NOT_ENOUGH_MEMORY);

    *pFrame = *pUserFrame;
    pFrame->frameData = (PBYTE)(pFrame + 1);
    MEMCPY(pFrame->frameData, pUserFrame->frameData, pUserFrame->size);

    CHK_STATUS(stackQueueEnqueue(pPutFrameTrackData->frameQueue, (UINT64) pFrame));

    if (pPutFrameTrackData->frameCount == 0) {
        pFrameOrderCoordinator->trackWithFrame++;
    }
    pPutFrameTrackData->frameCount++;

    // only put earliest frame when all tracks have frame
    while (pFrameOrderCoordinator->trackWithFrame == pFrameOrderCoordinator->putFrameTrackDataListCount) {
        CHK_STATUS(putEarliestFrame(pKinesisVideoStream));
    }

CleanUp:

    if (STATUS_FAILED(retStatus) && pKinesisVideoStream != NULL) {
        pKinesisVideoStream->diagnostics.putFrameErrors++;
    }

    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);
    }

    return retStatus;
}

STATUS frameOrderCoordinatorFlush(PKinesisVideoStream pKinesisVideoStream)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;
    PFrameOrderCoordinator pFrameOrderCoordinator = NULL;

    // no effect if in FRAME_ORDER_MODE_PASS_THROUGH mode.
    CHK(pKinesisVideoStream->streamInfo.streamCaps.frameOrderingMode != FRAME_ORDER_MODE_PASS_THROUGH, retStatus);
    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);
    pFrameOrderCoordinator = pKinesisVideoStream->pFrameOrderCoordinator;
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);
    locked = TRUE;

    // put remaining frames order by their timestamp
    while (pFrameOrderCoordinator->trackWithFrame > 0) {
        CHK_STATUS(putEarliestFrame(pKinesisVideoStream));
    }

CleanUp:

    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pFrameOrderCoordinator->lock);
    }

    return retStatus;
}
