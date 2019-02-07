/**
 * Kinesis Video MKV generator statics
 */
#define LOG_CLASS "InputValidator"

#include "Include_i.h"

/**
 * Validates the client callbacks structure
 *
 * @param 1 PDeviceInfo - Device info struct.
 * @param 2 PClientCallbacks - Client callbacks struct.
 *
 * @return Status of the function call.
 */
STATUS validateClientCallbacks(PDeviceInfo pDeviceInfo, PClientCallbacks pClientCallbacks)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pClientCallbacks != NULL && pDeviceInfo != NULL, STATUS_NULL_ARG);
    CHK(pClientCallbacks->version <= CALLBACKS_CURRENT_VERSION, STATUS_INVALID_CALLBACKS_VERSION);

    // Most of the callbacks are optional
    // Check for presence of the Service call callbacks
    CHK(pClientCallbacks->createStreamFn != NULL &&
                pClientCallbacks->describeStreamFn != NULL &&
                pClientCallbacks->getStreamingEndpointFn != NULL &&
                pClientCallbacks->putStreamFn != NULL &&
                pClientCallbacks->getStreamingTokenFn != NULL &&
                pClientCallbacks->createDeviceFn != NULL,
        STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // If we have tags then the tagResource callback should be present
    CHK(pDeviceInfo->tagCount == 0 || pClientCallbacks->tagResourceFn != NULL, STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // Fix-up the basic callbacks by providing the default if none selected
    if (pClientCallbacks->getCurrentTimeFn == NULL) {
        pClientCallbacks->getCurrentTimeFn = defaultGetCurrentTime;
    }

    if (pClientCallbacks->createMutexFn == NULL) {
        pClientCallbacks->createMutexFn = defaultCreateMutex;
    }

    if (pClientCallbacks->lockMutexFn == NULL) {
        pClientCallbacks->lockMutexFn = defaultLockMutex;
    }

    if (pClientCallbacks->unlockMutexFn == NULL) {
        pClientCallbacks->unlockMutexFn = defaultUnlockMutex;
    }

    if (pClientCallbacks->tryLockMutexFn == NULL) {
        pClientCallbacks->tryLockMutexFn = defaultTryLockMutex;
    }

    if (pClientCallbacks->freeMutexFn == NULL) {
        pClientCallbacks->freeMutexFn = defaultFreeMutex;
    }

    if (pClientCallbacks->createConditionVariableFn == NULL) {
        pClientCallbacks->createConditionVariableFn = defaultCreateConditionVariable;
    }

    if (pClientCallbacks->signalConditionVariableFn == NULL) {
        pClientCallbacks->signalConditionVariableFn = defaultSignalConditionVariable;
    }

    if (pClientCallbacks->broadcastConditionVariableFn == NULL) {
        pClientCallbacks->broadcastConditionVariableFn = defaultBroadcastConditionVariable;
    }

    if (pClientCallbacks->waitConditionVariableFn == NULL) {
        pClientCallbacks->waitConditionVariableFn = defaultWaitConditionVariable;
    }

    if (pClientCallbacks->freeConditionVariableFn == NULL) {
        pClientCallbacks->freeConditionVariableFn = defaultFreeConditionVariable;
    }

    if (pClientCallbacks->streamReadyFn == NULL) {
        pClientCallbacks->streamReadyFn = defaultStreamReady;
    }

    if (pClientCallbacks->streamClosedFn == NULL) {
        pClientCallbacks->streamClosedFn = defaultEndOfStream;
    }

    if (pClientCallbacks->clientReadyFn == NULL) {
        pClientCallbacks->clientReadyFn = defaultClientReady;
    }

    if (pClientCallbacks->streamDataAvailableFn == NULL) {
        pClientCallbacks->streamDataAvailableFn = defaultStreamDataAvailable;
    }

    if (pClientCallbacks->getRandomNumberFn == NULL) {
        // Call to seed the number generator
        SRAND((UINT32)pClientCallbacks->getCurrentTimeFn(pClientCallbacks->customData));
        pClientCallbacks->getRandomNumberFn = defaultGetRandomNumber;
    }

    if (pClientCallbacks->logPrintFn != NULL) {
        globalCustomLogPrintFn = pClientCallbacks->logPrintFn;
    }

CleanUp:
    return retStatus;
}

/**
 * Validates the device info structure
 *
 * @param 1 PDeviceInfo - Device info struct.
 *
 * @return Status of the function call.
 */
STATUS validateDeviceInfo(PDeviceInfo pDeviceInfo)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pDeviceInfo != NULL, STATUS_NULL_ARG);
    CHK(pDeviceInfo->version <= DEVICE_INFO_CURRENT_VERSION, STATUS_INVALID_DEVICE_INFO_VERSION);
    CHK(pDeviceInfo->streamCount <= MAX_STREAM_COUNT, STATUS_MAX_STREAM_COUNT);
    CHK(pDeviceInfo->streamCount > 0, STATUS_MIN_STREAM_COUNT);
    CHK(pDeviceInfo->storageInfo.version <= STORAGE_INFO_CURRENT_VERSION, STATUS_INVALID_STORAGE_INFO_VERSION);
    CHK(pDeviceInfo->storageInfo.storageSize >= MIN_STORAGE_ALLOCATION_SIZE &&
                pDeviceInfo->storageInfo.storageSize <= MAX_STORAGE_ALLOCATION_SIZE,
        STATUS_INVALID_STORAGE_SIZE);
    CHK(pDeviceInfo->storageInfo.spillRatio <= 100, STATUS_INVALID_SPILL_RATIO);
    CHK(STRNLEN(pDeviceInfo->storageInfo.rootDirectory, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_INVALID_ROOT_DIRECTORY_LENGTH);
    CHK(STRNLEN(pDeviceInfo->certPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_INVALID_CERT_PATH_LENGTH);
    CHK(STRNLEN(pDeviceInfo->name, MAX_DEVICE_NAME_LEN + 1) <= MAX_DEVICE_NAME_LEN, STATUS_INVALID_DEVICE_NAME_LENGTH);

    // Validate the tags
    CHK_STATUS(validateTags(pDeviceInfo->tagCount, pDeviceInfo->tags));

CleanUp:
    return retStatus;
}

/**
 * Validates the tags
 *
 * @param 1 UINT32 - Number of tags
 * @param 2 PTag - Array of tags
 *
 * @return Status of the function call.
 */
STATUS validateTags(UINT32 tagCount, PTag tags)
{
    UINT32 i;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(tagCount <= MAX_TAG_COUNT, STATUS_MAX_TAG_COUNT);

    // If we have tag count not 0 then tags can't be NULL
    CHK(tagCount == 0 || tags != NULL, STATUS_DEVICE_TAGS_COUNT_NON_ZERO_TAGS_NULL);
    for (i = 0; i < tagCount; i++) {
        // Validate the tag version
        CHK(tags[i].version <= TAG_CURRENT_VERSION, STATUS_INVALID_TAG_VERSION);

        // Validate the tag name
        CHK(STRNLEN(tags[i].name, MAX_TAG_NAME_LEN + 1) <= MAX_TAG_NAME_LEN, STATUS_INVALID_TAG_NAME_LEN);

        // Validate the tag value
        CHK(STRNLEN(tags[i].value, MAX_TAG_VALUE_LEN + 1) <= MAX_TAG_VALUE_LEN, STATUS_INVALID_TAG_VALUE_LEN);
    }

CleanUp:
    return retStatus;
}

/**
 * Validates the stream info structure
 *
 * @param 1 PStreamInfo - Stream info struct.
 * @param 2 PClientCallbacks - Client callbacks struct.
 *
 * @return Status of the function call.
 */
STATUS validateStreamInfo(PStreamInfo pStreamInfo, PClientCallbacks pClientCallbacks)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, j;

    // Validate the stream info struct
    CHK(pStreamInfo != NULL, STATUS_NULL_ARG);
    CHK(pStreamInfo->version <= STREAM_INFO_CURRENT_VERSION, STATUS_INVALID_STREAM_INFO_VERSION);
    CHK(STRNLEN(pStreamInfo->name, MAX_STREAM_NAME_LEN + 1) <= MAX_STREAM_NAME_LEN, STATUS_INVALID_STREAM_NAME_LENGTH);

    // Validate the retention period.
    // NOTE: 0 has is a sentinel value indicating no retention
    CHK(pStreamInfo->retention == RETENTION_PERIOD_SENTINEL ||
        pStreamInfo->retention >= MIN_RETENTION_PERIOD, STATUS_INVALID_RETENTION_PERIOD);

    // Validate the tags
    CHK_STATUS(validateTags(pStreamInfo->tagCount, pStreamInfo->tags));

    // If we have tags then the tagResource callback should be present
    CHK(pStreamInfo->tagCount == 0 || pClientCallbacks->tagResourceFn != NULL, STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // Fix-up the timecode scale if the value is the value of the sentinel
    if (pStreamInfo->streamCaps.timecodeScale == DEFAULT_TIMECODE_SCALE_SENTINEL) {
        pStreamInfo->streamCaps.timecodeScale = DEFAULT_MKV_TIMECODE_SCALE;
    }

    // Fix-up the buffer duration
    pStreamInfo->streamCaps.bufferDuration = MAX(pStreamInfo->streamCaps.bufferDuration, pStreamInfo->streamCaps.replayDuration);
    // Fix-up the frame rate
    pStreamInfo->streamCaps.frameRate = (pStreamInfo->streamCaps.frameRate == 0) ?
                                        DEFAULT_FRAME_RATE : pStreamInfo->streamCaps.frameRate;

    CHK(pStreamInfo->streamCaps.trackInfoCount != 0 && pStreamInfo->streamCaps.trackInfoList != NULL, STATUS_TRACK_INFO_MISSING);
    CHK(pStreamInfo->streamCaps.trackInfoCount <= MAX_SUPPORTED_TRACK_COUNT_PER_STREAM, STATUS_MAX_TRACK_COUNT_EXCEEDED);

    // Ensure uniqueness of the track ID across the track infos. OK to run quadratic algo.
    for (j = 0; j < pStreamInfo->streamCaps.trackInfoCount; j++) {
        for (i = 0; i < j; i++) {
            CHK(pStreamInfo->streamCaps.trackInfoList[j].trackId != pStreamInfo->streamCaps.trackInfoList[i].trackId,
                STATUS_DUPLICATE_TRACK_ID_FOUND);
        }
    }

    // Most the information is either optional or will be validated in the packager

    CHK(!(pStreamInfo->retention == RETENTION_PERIOD_SENTINEL &&
          IS_OFFLINE_STREAMING_MODE(pStreamInfo->streamCaps.streamingType)), STATUS_OFFLINE_MODE_WITH_ZERO_RETENTION);

    if (IS_OFFLINE_STREAMING_MODE(pStreamInfo->streamCaps.streamingType)) {
        pStreamInfo->streamCaps.connectionStalenessDuration = CONNECTION_STALENESS_DETECTION_SENTINEL;
        pStreamInfo->streamCaps.maxLatency = STREAM_LATENCY_PRESSURE_CHECK_SENTINEL;
    }

CleanUp:
    return retStatus;
}