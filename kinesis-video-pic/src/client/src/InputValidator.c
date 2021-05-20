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
    CHK(pClientCallbacks->createStreamFn != NULL && pClientCallbacks->describeStreamFn != NULL && pClientCallbacks->getStreamingEndpointFn != NULL &&
            pClientCallbacks->putStreamFn != NULL && pClientCallbacks->getStreamingTokenFn != NULL && pClientCallbacks->createDeviceFn != NULL,
        STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // If we have tags then the tagResource callback should be present
    CHK(pDeviceInfo->tagCount == 0 || pClientCallbacks->tagResourceFn != NULL, STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // Fix-up the basic callbacks by providing the default if none selected
    if (pClientCallbacks->getCurrentTimeFn == NULL) {
        pClientCallbacks->getCurrentTimeFn = kinesisVideoStreamDefaultGetCurrentTime;
    }

    if (pClientCallbacks->createMutexFn == NULL) {
        pClientCallbacks->createMutexFn = kinesisVideoStreamDefaultCreateMutex;
    }

    if (pClientCallbacks->lockMutexFn == NULL) {
        pClientCallbacks->lockMutexFn = kinesisVideoStreamDefaultLockMutex;
    }

    if (pClientCallbacks->unlockMutexFn == NULL) {
        pClientCallbacks->unlockMutexFn = kinesisVideoStreamDefaultUnlockMutex;
    }

    if (pClientCallbacks->tryLockMutexFn == NULL) {
        pClientCallbacks->tryLockMutexFn = kinesisVideoStreamDefaultTryLockMutex;
    }

    if (pClientCallbacks->freeMutexFn == NULL) {
        pClientCallbacks->freeMutexFn = kinesisVideoStreamDefaultFreeMutex;
    }

    if (pClientCallbacks->createConditionVariableFn == NULL) {
        pClientCallbacks->createConditionVariableFn = kinesisVideoStreamDefaultCreateConditionVariable;
    }

    if (pClientCallbacks->signalConditionVariableFn == NULL) {
        pClientCallbacks->signalConditionVariableFn = kinesisVideoStreamDefaultSignalConditionVariable;
    }

    if (pClientCallbacks->broadcastConditionVariableFn == NULL) {
        pClientCallbacks->broadcastConditionVariableFn = kinesisVideoStreamDefaultBroadcastConditionVariable;
    }

    if (pClientCallbacks->waitConditionVariableFn == NULL) {
        pClientCallbacks->waitConditionVariableFn = kinesisVideoStreamDefaultWaitConditionVariable;
    }

    if (pClientCallbacks->freeConditionVariableFn == NULL) {
        pClientCallbacks->freeConditionVariableFn = kinesisVideoStreamDefaultFreeConditionVariable;
    }

    if (pClientCallbacks->streamReadyFn == NULL) {
        pClientCallbacks->streamReadyFn = kinesisVideoStreamDefaultStreamReady;
    }

    if (pClientCallbacks->streamClosedFn == NULL) {
        pClientCallbacks->streamClosedFn = kinesisVideoStreamDefaultEndOfStream;
    }

    if (pClientCallbacks->clientReadyFn == NULL) {
        pClientCallbacks->clientReadyFn = kinesisVideoStreamDefaultClientReady;
    }

    if (pClientCallbacks->streamDataAvailableFn == NULL) {
        pClientCallbacks->streamDataAvailableFn = kinesisVideoStreamDefaultStreamDataAvailable;
    }

    if (pClientCallbacks->clientShutdownFn == NULL) {
        pClientCallbacks->clientShutdownFn = kinesisVideoStreamDefaultClientShutdown;
    }

    if (pClientCallbacks->streamShutdownFn == NULL) {
        pClientCallbacks->streamShutdownFn = kinesisVideoStreamDefaultStreamShutdown;
    }

    if (pClientCallbacks->getRandomNumberFn == NULL) {
        // Call to seed the number generator
        SRAND((UINT32) pClientCallbacks->getCurrentTimeFn(pClientCallbacks->customData));
        pClientCallbacks->getRandomNumberFn = kinesisVideoStreamDefaultGetRandomNumber;
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
    CHK(pDeviceInfo->storageInfo.storageSize >= MIN_STORAGE_ALLOCATION_SIZE && pDeviceInfo->storageInfo.storageSize <= MAX_STORAGE_ALLOCATION_SIZE,
        STATUS_INVALID_STORAGE_SIZE);
    CHK(pDeviceInfo->storageInfo.spillRatio <= 100, STATUS_INVALID_SPILL_RATIO);
    CHK(STRNLEN(pDeviceInfo->storageInfo.rootDirectory, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_INVALID_ROOT_DIRECTORY_LENGTH);
    CHK(STRNLEN(pDeviceInfo->name, MAX_DEVICE_NAME_LEN + 1) <= MAX_DEVICE_NAME_LEN, STATUS_INVALID_DEVICE_NAME_LENGTH);

    // Validate the tags
    CHK_STATUS(validateClientTags(pDeviceInfo->tagCount, pDeviceInfo->tags));

    // If we are v1 then we validate the client info
    if (pDeviceInfo->version == 1) {
        CHK(STRNLEN(pDeviceInfo->clientId, MAX_CLIENT_ID_STRING_LENGTH + 1) <= MAX_CLIENT_ID_STRING_LENGTH, STATUS_INVALID_CLIENT_ID_STRING_LENGTH);
        CHK_STATUS(validateClientInfo(&pDeviceInfo->clientInfo));
    }

CleanUp:
    return retStatus;
}

/**
 * Validates the client info structure
 *
 * @param 1 PClientInfo - Client info struct.
 *
 * @return Status of the function call.
 */
STATUS validateClientInfo(PClientInfo pClientInfo)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pClientInfo != NULL, STATUS_NULL_ARG);
    CHK(pClientInfo->version <= CLIENT_INFO_CURRENT_VERSION, STATUS_INVALID_CLIENT_INFO_VERSION);

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
STATUS validateClientTags(UINT32 tagCount, PTag tags)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK_STATUS(validateTags(tagCount, tags));

CleanUp:

    // Translate the errors
    switch (retStatus) {
        case STATUS_UTIL_MAX_TAG_COUNT:
            retStatus = STATUS_MAX_TAG_COUNT;
            break;
        case STATUS_UTIL_INVALID_TAG_VERSION:
            retStatus = STATUS_INVALID_TAG_VERSION;
            break;
        case STATUS_UTIL_TAGS_COUNT_NON_ZERO_TAGS_NULL:
            retStatus = STATUS_DEVICE_TAGS_COUNT_NON_ZERO_TAGS_NULL;
            break;
        case STATUS_UTIL_INVALID_TAG_NAME_LEN:
            retStatus = STATUS_INVALID_TAG_NAME_LEN;
            break;
        case STATUS_UTIL_INVALID_TAG_VALUE_LEN:
            retStatus = STATUS_INVALID_TAG_VALUE_LEN;
            break;
    }

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
    CHK(pStreamInfo->retention == RETENTION_PERIOD_SENTINEL || pStreamInfo->retention >= MIN_RETENTION_PERIOD, STATUS_INVALID_RETENTION_PERIOD);

    // Validate the tags
    CHK_STATUS(validateClientTags(pStreamInfo->tagCount, pStreamInfo->tags));

    // If we have tags then the tagResource callback should be present
    CHK(pStreamInfo->tagCount == 0 || pClientCallbacks->tagResourceFn != NULL, STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // Fix-up the timecode scale if the value is the value of the sentinel
    if (pStreamInfo->streamCaps.timecodeScale == DEFAULT_TIMECODE_SCALE_SENTINEL) {
        pStreamInfo->streamCaps.timecodeScale = DEFAULT_MKV_TIMECODE_SCALE;
    }

    // Fix-up the replay duration
    pStreamInfo->streamCaps.replayDuration = MIN(pStreamInfo->streamCaps.bufferDuration, pStreamInfo->streamCaps.replayDuration);
    // Fix-up the frame rate
    pStreamInfo->streamCaps.frameRate = (pStreamInfo->streamCaps.frameRate == 0) ? DEFAULT_FRAME_RATE : pStreamInfo->streamCaps.frameRate;

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

    if (IS_OFFLINE_STREAMING_MODE(pStreamInfo->streamCaps.streamingType)) {
        CHK(pStreamInfo->retention != RETENTION_PERIOD_SENTINEL && pStreamInfo->streamCaps.fragmentAcks, STATUS_OFFLINE_MODE_WITH_ZERO_RETENTION);
        pStreamInfo->streamCaps.connectionStalenessDuration = CONNECTION_STALENESS_DETECTION_SENTINEL;
        pStreamInfo->streamCaps.maxLatency = STREAM_LATENCY_PRESSURE_CHECK_SENTINEL;
    }

CleanUp:
    return retStatus;
}

VOID fixupDeviceInfo(PDeviceInfo pClientDeviceInfo, PDeviceInfo pDeviceInfo)
{
    PClientInfo pOrigClientInfo = NULL;
    switch (pDeviceInfo->version) {
        case 1:
            // Copy the individual fields for V1
            MEMCPY(pClientDeviceInfo->clientId, pDeviceInfo->clientId, MAX_CLIENT_ID_STRING_LENGTH + 1);
            pOrigClientInfo = &pDeviceInfo->clientInfo;

            // Explicit fall-through
        case 0:
            // Copy and fixup individual fields
            pClientDeviceInfo->version = DEVICE_INFO_CURRENT_VERSION;
            MEMCPY(pClientDeviceInfo->name, pDeviceInfo->name, MAX_DEVICE_NAME_LEN + 1);
            pClientDeviceInfo->tagCount = pDeviceInfo->tagCount;
            pClientDeviceInfo->tags = pDeviceInfo->tags;
            pClientDeviceInfo->storageInfo = pDeviceInfo->storageInfo;
            pClientDeviceInfo->streamCount = pDeviceInfo->streamCount;

            break;

        default:
            DLOGW("Invalid DeviceInfo version");
    }

    fixupClientInfo(&pClientDeviceInfo->clientInfo, pOrigClientInfo);
}

/**
 * The structure has been sanitized prior this call. We are setting defaults if needed
 */
VOID fixupClientInfo(PClientInfo pClientInfo, PClientInfo pOrigClientInfo)
{
    // Assuming pClientInfo had been zeroed already
    if (pOrigClientInfo != NULL) {
        pClientInfo->version = pOrigClientInfo->version;
        pClientInfo->createClientTimeout = pOrigClientInfo->createClientTimeout;
        pClientInfo->createStreamTimeout = pOrigClientInfo->createStreamTimeout;
        pClientInfo->stopStreamTimeout = pOrigClientInfo->stopStreamTimeout;
        pClientInfo->offlineBufferAvailabilityTimeout = pOrigClientInfo->offlineBufferAvailabilityTimeout;
        pClientInfo->loggerLogLevel = pOrigClientInfo->loggerLogLevel;
        pClientInfo->logMetric = pOrigClientInfo->logMetric;
        pClientInfo->automaticStreamingFlags = AUTOMATIC_STREAMING_INTERMITTENT_PRODUCER;
        pClientInfo->reservedCallbackPeriod = INTERMITTENT_PRODUCER_PERIOD_SENTINEL_VALUE;

        switch (pOrigClientInfo->version) {
            case 2:
                // Copy individual fields and skip to V1
                pClientInfo->automaticStreamingFlags = pOrigClientInfo->automaticStreamingFlags;
                pClientInfo->reservedCallbackPeriod = pOrigClientInfo->reservedCallbackPeriod;

                // explicit fall-through
            case 1:
                // Copy individual fields and skip to V0
                pClientInfo->metricLoggingPeriod = pOrigClientInfo->metricLoggingPeriod;
                break;
            default:
                DLOGW("Invalid ClientInfo version");
        }
    }

    if (pClientInfo->reservedCallbackPeriod == INTERMITTENT_PRODUCER_PERIOD_SENTINEL_VALUE) {
        pClientInfo->reservedCallbackPeriod = INTERMITTENT_PRODUCER_PERIOD_DEFAULT;
    }

    // Set the defaults if older version or if the sentinel values have been specified
    if (pClientInfo->createClientTimeout == 0 || !IS_VALID_TIMESTAMP(pClientInfo->createClientTimeout)) {
        pClientInfo->createClientTimeout = CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    }

    if (pClientInfo->createStreamTimeout == 0 || !IS_VALID_TIMESTAMP(pClientInfo->createStreamTimeout)) {
        pClientInfo->createStreamTimeout = STREAM_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    }

    if (pClientInfo->stopStreamTimeout == 0 || !IS_VALID_TIMESTAMP(pClientInfo->stopStreamTimeout)) {
        pClientInfo->stopStreamTimeout = STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND;
    }

    if (pClientInfo->offlineBufferAvailabilityTimeout == 0 || !IS_VALID_TIMESTAMP(pClientInfo->offlineBufferAvailabilityTimeout)) {
        pClientInfo->offlineBufferAvailabilityTimeout = MAX_BLOCKING_PUT_WAIT;
    }

    if (pClientInfo->loggerLogLevel == 0 || pClientInfo->loggerLogLevel > LOG_LEVEL_SILENT) {
        pClientInfo->loggerLogLevel = LOG_LEVEL_WARN;
    }

    // logMetric and metricLoggingPeriod do not need to be fixed
}

/**
 * Returns the size of the device info structure in bytes.
 * The pointer has been sanitized.
 */
SIZE_T sizeOfDeviceInfo(PDeviceInfo pDeviceInfo)
{
    SIZE_T size = 0;

    // Version dependent and assumes later versions are supersets.
    switch (pDeviceInfo->version) {
        case 0:
            size = SIZEOF(DeviceInfo_V0);
            break;

        case 1:
            size = SIZEOF(DeviceInfo);
            break;
    }

    return size;
}

/**
 * The structure has been sanitized prior this call. We are setting defaults if needed
 */
VOID fixupStreamDescription(PStreamDescription pStreamDescription)
{
    // Set the defaults if older version
    // Version dependent and assumes later versions are supersets.
    switch (pStreamDescription->version) {
        case 0:
            pStreamDescription->kmsKeyId[0] = '\0';
            pStreamDescription->retention = 0;
            break;

        case 1:
            // No-op - the latest version
            break;
    }

    // Fix-up the version
    pStreamDescription->version = STREAM_DESCRIPTION_CURRENT_VERSION;
}

/**
 * The structure has been sanitized prior this call. We are setting defaults if needed
 */
VOID fixupStreamInfo(PStreamInfo pStreamInfo)
{
    // Set the defaults if older version
    // Version dependent and assumes later versions are supersets.
    switch (pStreamInfo->version) {
        case 0:
            pStreamInfo->streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;

            if (pStreamInfo->streamCaps.trackInfoCount == 2 &&
                ((pStreamInfo->streamCaps.trackInfoList[0].trackType == MKV_TRACK_INFO_TYPE_VIDEO &&
                  pStreamInfo->streamCaps.trackInfoList[1].trackType == MKV_TRACK_INFO_TYPE_AUDIO) ||
                 (pStreamInfo->streamCaps.trackInfoList[0].trackType == MKV_TRACK_INFO_TYPE_AUDIO &&
                  pStreamInfo->streamCaps.trackInfoList[1].trackType == MKV_TRACK_INFO_TYPE_VIDEO))) {
                // TODO change back to FRAME_ORDERING_MODE_MULTI_TRACK_AV once backend is fixed.
                pStreamInfo->streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE;
            }

            // Explicit fall-through
        case 1:
            pStreamInfo->streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_OOM;
            pStreamInfo->streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;
            break;

        case 2:
            // No-op - the latest version
            break;
    }

    // Fix-up the version
    pStreamInfo->version = STREAM_INFO_CURRENT_VERSION;
}

VOID fixupTrackInfo(PTrackInfo pTrackInfo, UINT32 trackInfoCount)
{
    // NO OP
}

VOID fixupFrame(PFrame pFrame)
{
    // NO OP
}

/**
 * Returns the size of the device info structure in bytes.
 * The pointer has been sanitized.
 */
SIZE_T sizeOfStreamDescription(PStreamDescription pStreamDescription)
{
    SIZE_T size = 0;

    // Version dependent and assumes later versions are supersets.
    switch (pStreamDescription->version) {
        case 0:
            size = SIZEOF(StreamDescription_V0);
            break;

        case 1:
            size = SIZEOF(StreamDescription);
            break;
    }

    return size;
}
