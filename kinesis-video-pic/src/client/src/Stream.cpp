/**
 * Implementation of a data stream
 */

#define LOG_CLASS "Stream"

#include "Include_i.h"

 /**
  * Static definitions of the states
  */
extern StateMachineState STREAM_STATE_MACHINE_STATES[];
extern UINT32 STREAM_STATE_MACHINE_STATE_COUNT;

/**
 * Creates a new Kinesis Video stream
 */
STATUS createStream(PKinesisVideoClient pKinesisVideoClient, PStreamInfo pStreamInfo, PKinesisVideoStream* ppKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = NULL;
    PContentView pView = NULL;
    PStackQueue pStackQueue = NULL;
    PMkvGenerator pMkvGenerator = NULL;
    PStateMachine pStateMachine = NULL;
    UINT32 allocationSize, maxViewItems, i;
    PBYTE pCurPnt = NULL;
    BOOL locked = FALSE;
    BOOL tearDownOnError = TRUE;
    CHAR tempStreamName[MAX_STREAM_NAME_LEN];
    UINT32 trackInfoSize;

    CHK(pKinesisVideoClient != NULL && ppKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // Set the return stream pointer first
    *ppKinesisVideoStream = NULL;

    // Validate the input structs
    CHK_STATUS(validateStreamInfo(pStreamInfo, &pKinesisVideoClient->clientCallbacks));

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    locked = TRUE;

    // Check for the stream count
    CHK(pKinesisVideoClient->streamCount < pKinesisVideoClient->deviceInfo.streamCount, STATUS_MAX_STREAM_COUNT);

    // Fix-up the stream name if not specified
    if (pStreamInfo->name[0] == '\0') {
        createRandomName(tempStreamName,
            DEFAULT_STREAM_NAME_LEN,
            pKinesisVideoClient->clientCallbacks.getRandomNumberFn,
            pKinesisVideoClient->clientCallbacks.customData);
    }
    else {
        // Copy the stream name otherwise
        // NOTE: Stream name length has already been validated
        STRCPY(tempStreamName, pStreamInfo->name);
    }

    // Check if a stream by that name already exists
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            CHK(0 != STRCMP(pKinesisVideoClient->streams[i]->streamInfo.name, tempStreamName), STATUS_DUPLICATE_STREAM_NAME);
        }
    }

    // Space for track info bits
    trackInfoSize = SIZEOF(TrackInfo) * pStreamInfo->streamCaps.trackInfoCount;

    // Allocate the main struct
    // NOTE: The calloc will Zero the fields
    // We are allocating more than the structure size to accommodate for
    // the variable size buffers which will come at the end and for the tags
    allocationSize = SIZEOF(KinesisVideoStream) + pStreamInfo->tagCount * TAG_FULL_LENGTH + trackInfoSize + MKV_SEGMENT_UUID_LEN;
    pKinesisVideoStream = (PKinesisVideoStream) MEMCALLOC(1, allocationSize);
    CHK(pKinesisVideoStream != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the stream id first available slot so during the teardown it will clean it up.
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL == pKinesisVideoClient->streams[i]) {
            // Found an empty slot
            pKinesisVideoStream->streamId = i;
            break;
        }
    }

    // set the maximum frame size observed to 0
    pKinesisVideoStream->maxFrameSizeSeen = 0;

    // Set the back reference
    pKinesisVideoStream->pKinesisVideoClient = pKinesisVideoClient;

    // Set the basic info
    pKinesisVideoStream->base.identifier = KINESIS_VIDEO_OBJECT_IDENTIFIER_STREAM;
    pKinesisVideoStream->base.version = STREAM_CURRENT_VERSION;

    // We can now unlock the client lock so we won't block it
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    locked = FALSE;

    // Set the initial state and the stream status
    pKinesisVideoStream->streamState = STREAM_STATE_NONE;
    pKinesisVideoStream->streamStatus = STREAM_STATUS_CREATING;

    // Stream is not stopped
    pKinesisVideoStream->streamStopped = FALSE;

    // Set the stream start timestamps and index
    pKinesisVideoStream->newSessionTimestamp = INVALID_TIMESTAMP_VALUE;
    pKinesisVideoStream->newSessionIndex = INVALID_VIEW_INDEX_VALUE;

    // Not in a grace period
    pKinesisVideoStream->gracePeriod = FALSE;

    // Shouldn't reset the generator on next key frame
    pKinesisVideoStream->resetGeneratorOnKeyFrame = FALSE;

    // Shouldn't reset the generator so set invalid time
    pKinesisVideoStream->resetGeneratorTime = INVALID_TIMESTAMP_VALUE;

    // No connections have been dropped as this is a new stream
    pKinesisVideoStream->connectionState = UPLOAD_CONNECTION_STATE_OK;

    // Set the last frame EoFr indicator
    pKinesisVideoStream->eofrFrame = FALSE;

    // Set the initial diagnostics information from the defaults
    pKinesisVideoStream->diagnostics.currentFrameRate = pStreamInfo->streamCaps.frameRate;
    pKinesisVideoStream->diagnostics.currentTransferRate = pStreamInfo->streamCaps.avgBandwidthBps;
    pKinesisVideoStream->diagnostics.accumulatedByteCount = 0;
    pKinesisVideoStream->diagnostics.lastFrameRateTimestamp = pKinesisVideoStream->diagnostics.lastTransferRateTimestamp = 0;

    // Set the trackers
    pKinesisVideoStream->eosTracker.size = 0;
    pKinesisVideoStream->eosTracker.offset = 0;
    pKinesisVideoStream->eosTracker.send = FALSE;
    pKinesisVideoStream->eosTracker.data = NULL;
    pKinesisVideoStream->metadataTracker.size = 0;
    pKinesisVideoStream->metadataTracker.offset = 0;
    pKinesisVideoStream->metadataTracker.send = FALSE;
    pKinesisVideoStream->metadataTracker.data = NULL;

    // Reset the current view item
    MEMSET(&pKinesisVideoStream->curViewItem, 0x00, SIZEOF(CurrentViewItem));
    pKinesisVideoStream->curViewItem.viewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;

    // Copy the structures in their entirety
    MEMCPY(&pKinesisVideoStream->streamInfo, pStreamInfo, SIZEOF(StreamInfo));

    // Fix-up the stream name if not specified
    if (pKinesisVideoStream->streamInfo.name[0] == '\0') {
        STRCPY(pKinesisVideoStream->streamInfo.name, tempStreamName);
    }

    // Create the stream lock
    pKinesisVideoStream->base.lock = pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, TRUE);

    // Create the buffer availability lock for the condition variable
    pKinesisVideoStream->bufferAvailabilityLock = pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, FALSE);

    // Create the buffer availability condition variable
    pKinesisVideoStream->bufferAvailabilityCondition = pKinesisVideoClient->clientCallbacks.createConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData);

    // Fix-up the buffer duration first
    pKinesisVideoStream->streamInfo.streamCaps.bufferDuration = calculateViewBufferDuration(&pKinesisVideoStream->streamInfo);

    // Set the tags pointer to point after the KinesisVideoStream struct
    pKinesisVideoStream->streamInfo.tags = (PTag)((PBYTE) (pKinesisVideoStream + 1));

    // Copy the tags if any. First, set the pointer to the end of the tags to iterate
    pCurPnt = (PBYTE) (pKinesisVideoStream->streamInfo.tags + pKinesisVideoStream->streamInfo.tagCount);
    for (i = 0; i < pKinesisVideoStream->streamInfo.tagCount; i++) {
        pKinesisVideoStream->streamInfo.tags[i].version = pStreamInfo->tags[i].version;
        // Fix-up the pointers first
        pKinesisVideoStream->streamInfo.tags[i].name = (PCHAR) pCurPnt;
        pCurPnt += (MAX_TAG_NAME_LEN + 1) * SIZEOF(CHAR);

        pKinesisVideoStream->streamInfo.tags[i].value = (PCHAR) pCurPnt;
        pCurPnt += (MAX_TAG_VALUE_LEN + 1) * SIZEOF(CHAR);

        // Copy the strings
        STRNCPY(pKinesisVideoStream->streamInfo.tags[i].name, pStreamInfo->tags[i].name, MAX_TAG_NAME_LEN);
        pKinesisVideoStream->streamInfo.tags[i].name[MAX_TAG_NAME_LEN] = '\0';
        STRNCPY(pKinesisVideoStream->streamInfo.tags[i].value, pStreamInfo->tags[i].value, MAX_TAG_VALUE_LEN);
        pKinesisVideoStream->streamInfo.tags[i].value[MAX_TAG_VALUE_LEN] = '\0';
    }

    // Fix-up/store the segment Uid to make it random if NULL
    pKinesisVideoStream->streamInfo.streamCaps.segmentUuid = pCurPnt;
    if (pStreamInfo->streamCaps.segmentUuid == NULL) {
        for (i = 0; i < MKV_SEGMENT_UUID_LEN; i++) {
            pKinesisVideoStream->streamInfo.streamCaps.segmentUuid[i] =
                    ((BYTE) (pKinesisVideoClient->clientCallbacks.getRandomNumberFn(pKinesisVideoClient->clientCallbacks.customData) % 0x100));
        }
    } else {
        MEMCPY(pKinesisVideoStream->streamInfo.streamCaps.segmentUuid, pStreamInfo->streamCaps.segmentUuid, MKV_SEGMENT_UUID_LEN);
    }

    // Advance the current pointer
    pCurPnt += MKV_SEGMENT_UUID_LEN;

    // Copy the structures in their entirety
    // NOTE: This will copy the raw pointers, however, we will only use it in the duration of the call.
    MEMCPY(pCurPnt, pStreamInfo->streamCaps.trackInfoList, trackInfoSize);
    pKinesisVideoStream->streamInfo.streamCaps.trackInfoList = (PTrackInfo) pCurPnt;

    // Move pCurPnt to the end of pKinesisVideoStream->streamInfo.streamCaps.trackInfoList
    pCurPnt = (PBYTE) (pKinesisVideoStream->streamInfo.streamCaps.trackInfoList + pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount);

    // Calculate the max items in the view
    maxViewItems = calculateViewItemCount(&pKinesisVideoStream->streamInfo);

    // Create the view
    CHK_STATUS(createContentView(maxViewItems,
                                 pKinesisVideoStream->streamInfo.streamCaps.bufferDuration,
                                 viewItemRemoved,
                                 TO_CUSTOM_DATA(pKinesisVideoStream),
                                 &pView));
    pKinesisVideoStream->pView = pView;

    // Create an MKV generator
    CHK_STATUS(createPackager(pKinesisVideoStream, &pMkvGenerator));

    // Set the generator object
    pKinesisVideoStream->pMkvGenerator = pMkvGenerator;

    // Package EOS metadata and store
    CHK_STATUS(generateEosMetadata(pKinesisVideoStream));

    // Create the state machine
    CHK_STATUS(createStateMachine(STREAM_STATE_MACHINE_STATES,
                                  STREAM_STATE_MACHINE_STATE_COUNT,
                                  TO_CUSTOM_DATA(pKinesisVideoStream),
                                  pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                                  pKinesisVideoClient->clientCallbacks.customData,
                                  &pStateMachine));
    pKinesisVideoStream->base.pStateMachine = pStateMachine;

    // Create the stream upload handle queue
    CHK_STATUS(stackQueueCreate(&pStackQueue));
    pKinesisVideoStream->pUploadInfoQueue = pStackQueue;

    // Create the metadata queue
    CHK_STATUS(stackQueueCreate(&pStackQueue));
    pKinesisVideoStream->pMetadataQueue = pStackQueue;

    // Set the call result to unknown to start
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Set the new object in the parent object, set the ID and increment the current count
    // NOTE: Make sure we set the stream in the client object before setting the return value and
    // no tear-down flag is set.
    pKinesisVideoClient->streams[pKinesisVideoStream->streamId] = pKinesisVideoStream;
    pKinesisVideoClient->streamCount++;

    // Reset the ACK parser
    CHK_STATUS(resetAckParserState(pKinesisVideoStream));

    // Assign the created object
    *ppKinesisVideoStream = pKinesisVideoStream;

    // Setting this value will ensure we won't tear down the newly created object after this on failure
    tearDownOnError = FALSE;

    // Set the initial state to new
    pKinesisVideoStream->streamState = STREAM_STATE_NEW;

    // Call to transition the state machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (STATUS_FAILED(retStatus) && tearDownOnError) {
        freeStream(pKinesisVideoStream);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the client object
 */
STATUS freeStream(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    UINT32 i;

    // Call is idempotent
    CHK(pKinesisVideoStream != NULL, retStatus);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    CHK(pKinesisVideoClient != NULL, STATUS_CLIENT_FREED_BEFORE_STREAM);

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);

    // Stop the processing
    stopStream(pKinesisVideoStream);

    // Release the underlying objects
    freeContentView(pKinesisVideoStream->pView);
    freeMkvGenerator(pKinesisVideoStream->pMkvGenerator);
    freeStateMachine(pKinesisVideoStream->base.pStateMachine);

    // Free the upload handle info queue
    freeStackQueue(pKinesisVideoStream->pUploadInfoQueue);

    // Free the metadata queue
    freeStackQueue(pKinesisVideoStream->pMetadataQueue);

    // Free the eosTracker and eofrTracker data if any
    freeMetadataTracker(&pKinesisVideoStream->eosTracker);
    freeMetadataTracker(&pKinesisVideoStream->metadataTracker);

    // Lock the client to update the streams
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    // Remove from the parent object by setting the ref to NULL
    pKinesisVideoClient->streams[pKinesisVideoStream->streamId] = NULL;
    pKinesisVideoClient->streamCount--;

    // Unlock and free the availability lock
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->bufferAvailabilityLock);
    pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->bufferAvailabilityLock);

    // Free the condition variable
    pKinesisVideoClient->clientCallbacks.freeConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->bufferAvailabilityCondition);

    // Release the client lock
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    // Unlock and free the lock
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);

    // Release the object
    MEMFREE(pKinesisVideoStream);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeStackQueue(PStackQueue pStackQueue)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 count = 0;
    UINT64 item;

    // The call is idempotent
    CHK(pStackQueue != NULL, retStatus);

    // Get the count of the items and iterate through
    stackQueueGetCount(pStackQueue, &count);
    while (count-- != 0) {
        // Dequeue the current item
        stackQueueDequeue(pStackQueue, &item);

        // The item is the allocation so free it
        MEMFREE((PVOID) item);
    }

    // Free the object itself
    stackQueueFree(pStackQueue);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Stops the stream
 */
STATUS stopStream(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient;
    UINT64 duration, viewByteSize;
    UINT32 i, sessionCount;
    BOOL streamLocked = FALSE, clientLocked = FALSE, notSent = FALSE;
    PUploadHandleInfo pUploadHandleInfo = NULL;
    UINT64 item;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // It's a no-op if stop has been called
    CHK(!pKinesisVideoStream->streamStopped, retStatus);

    // Set the stream end flag
    pKinesisVideoStream->streamStopped = TRUE;

    // Notify in case of an OFFLINE stream
    if (IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType)) {
        pKinesisVideoClient->clientCallbacks.signalConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                       pKinesisVideoStream->bufferAvailabilityCondition);
    }

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the duration from current point to the head
    // Get the size of the allocation from current point to the head
    getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize);

    // Pulse the data available callback so the callee will know of a state change
    CHK_STATUS(stackQueueGetCount(pKinesisVideoStream->pUploadInfoQueue, &sessionCount));

    // Check if we have any content left and whether we have any metadata left to be send
    CHK_STATUS(checkForNotSentMetadata(pKinesisVideoStream, &notSent));

    // Package the frames if needed to be sent
    if (notSent) {
        CHK_STATUS(packageNotSentMetadata(pKinesisVideoStream));
    }

    for (i = 0; i < sessionCount; i++) {
        CHK_STATUS(stackQueueGetAt(pKinesisVideoStream->pUploadInfoQueue, i, &item));
        pUploadHandleInfo = (PUploadHandleInfo) item;
        CHK(pUploadHandleInfo != NULL, STATUS_INTERNAL_ERROR);

        // Call the notification callback to unblock potentially blocked listener for the first non terminated upload handle
        // The unblocked handle will unblock subsequent handles once it's done. Therefore break the loop
        if (pUploadHandleInfo->state != UPLOAD_HANDLE_STATE_TERMINATED) {
            // If we have sent all the data but still have metadata then we will
            // "mock" some duration and set the size to the not sent metadata size
            // in order not to close the network upload handler.
            if (notSent && (duration == 0)) {
                duration = 1;
                viewByteSize = pKinesisVideoStream->metadataTracker.size + pKinesisVideoStream->eosTracker.size;
            }

            CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                    pKinesisVideoClient->clientCallbacks.customData,
                    TO_STREAM_HANDLE(pKinesisVideoStream),
                    pKinesisVideoStream->streamInfo.name,
                    pUploadHandleInfo->handle,
                    duration,
                    viewByteSize + pKinesisVideoStream->curViewItem.viewItem.length -
                    pKinesisVideoStream->curViewItem.offset));
            break;
        }
    }

    // Check if we need to call stream closed callback
    if (!notSent &&
        viewByteSize == 0 &&
        sessionCount == 0 && // If we have active handle, then eventually one of the handle will call streamClosedFn
        !pKinesisVideoStream->metadataTracker.send &&
        !pKinesisVideoStream->eosTracker.send) {
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamClosedFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                       TO_STREAM_HANDLE(pKinesisVideoStream),
                                                                       pUploadHandleInfo == NULL
                                                                       ? INVALID_UPLOAD_HANDLE_VALUE
                                                                       : pUploadHandleInfo->handle));
    }

    // Unlock the client as we no longer need it locked
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Puts a frame into the stream
 */
STATUS putFrame(PKinesisVideoStream pKinesisVideoStream, PFrame pFrame) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    ALLOCATION_HANDLE allocHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT64 remainingSize = 0, remainingDuration = 0, thresholdPercent = 0, duration = 0, viewByteSize = 0, allocSize = 0;
    PBYTE pAlloc = NULL;
    UINT32 packagedSize = 0, packagedMetadataSize = 0, overallSize = 0, itemFlags = ITEM_FLAG_NONE;
    BOOL streamLocked = FALSE, clientLocked = FALSE, freeOnError = TRUE, contains = FALSE;
    EncodedFrameInfo encodedFrameInfo;
    MKV_STREAM_STATE generatorState = MKV_STATE_START_BLOCK;
    UINT64 currentTime = INVALID_TIMESTAMP_VALUE;
    DOUBLE frameRate, deltaInSeconds;
    PViewItem pViewItem = NULL;
    PUploadHandleInfo pUploadHandleInfo;
    UINT32 i;
    BOOL trackInfoFound = FALSE;
    UINT64 windowDuration, currentDuration;

    CHK(pKinesisVideoStream != NULL && pFrame != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lookup the track that pFrame belong to
    for(i = 0; i < pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount; ++i) {
        if (pKinesisVideoStream->streamInfo.streamCaps.trackInfoList[i].trackId == pFrame->trackId) {
            trackInfoFound = TRUE;
            break;
        }
    }

    CHK (trackInfoFound, STATUS_MKV_TRACK_INFO_NOT_FOUND);

    // Check if the stream has been stopped
    CHK(!pKinesisVideoStream->streamStopped, STATUS_STREAM_HAS_BEEN_STOPPED);

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                     pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Check if we are in the right state only if we are not in a rotation state
    if (pKinesisVideoStream->streamState != STREAM_STATE_READY) {
        CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                           STREAM_STATE_READY |
                                           STREAM_STATE_PUT_STREAM |
                                           STREAM_STATE_TAG_STREAM |
                                           STREAM_STATE_STREAMING |
                                           STREAM_STATE_GET_ENDPOINT |
                                           STREAM_STATE_GET_TOKEN |
                                           STREAM_STATE_STOPPED));
    }

    // Validate that we are not seeing EoFr explicit marker in a non-key-frame fragmented stream
    if (!pKinesisVideoStream->streamInfo.streamCaps.keyFrameFragmentation) {
        CHK(!CHECK_FRAME_FLAG_END_OF_FRAGMENT(pFrame->flags), STATUS_END_OF_FRAGMENT_FRAME_INVALID_STATE);
    }

    // Need to check whether the streaming token has expired.
    // If we have the streaming token and it's in the grace period
    // then we need to go back to the get streaming end point and
    // get the streaming end point and the new streaming token.
    CHK_STATUS(checkStreamingTokenExpiration(pKinesisVideoStream));

    // Check if we have passed the delay for resetting generator.
    if (IS_VALID_TIMESTAMP(pKinesisVideoStream->resetGeneratorTime)) {
        currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
        if (currentTime >= pKinesisVideoStream->resetGeneratorTime) {
            pKinesisVideoStream->resetGeneratorTime = INVALID_TIMESTAMP_VALUE;
            pKinesisVideoStream->resetGeneratorOnKeyFrame = TRUE;
        }
    }

    // NOTE: If the connection has been reset we need to start from a new header
    if (pKinesisVideoStream->streamState == STREAM_STATE_NEW) {
        // Step the state machine once to get out of the Ready state
        CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));
    }

    // if we need to reset the generator on the next key frame (during the rotation only)
    if (pKinesisVideoStream->resetGeneratorOnKeyFrame && CHECK_FRAME_FLAG_KEY_FRAME(pFrame->flags)) {
        CHK_STATUS(mkvgenResetGenerator(pKinesisVideoStream->pMkvGenerator));
        pKinesisVideoStream->resetGeneratorOnKeyFrame = FALSE;
    }

    // Package and store the frame.
    // If the frame is a special End-of-Fragment indicator
    // then we need to package the not yet sent metadata with EoFr metadata
    // and indicate the new cluster boundary

    // Currently, we are storing every frame in a separate allocation
    // We will need to optimize for storing perhaps an entire cluster but
    // Not right now as we need to deal with mapping/unmapping and allocating
    // the cluster storage up-front which might be more hassle than needed.

    if (CHECK_FRAME_FLAG_END_OF_FRAGMENT(pFrame->flags)) {
        // We will append the EoFr tag and package the tags
        CHK_STATUS(appendValidatedMetadata(pKinesisVideoStream,
                                           (PCHAR) EOFR_METADATA_NAME,
                                           (PCHAR) "",
                                           FALSE,
                                           pKinesisVideoStream->eosTracker.size));

        // Package the not-applied metadata as the frame bits
        CHK_STATUS(packageStreamMetadata(pKinesisVideoStream,
                                         MKV_STATE_START_CLUSTER,
                                         TRUE,
                                         NULL,
                                         &packagedSize));
    } else {
        // Get the size of the packaged frame
        CHK_STATUS(mkvgenPackageFrame(pKinesisVideoStream->pMkvGenerator,
                                      pFrame,
                                      NULL,
                                      &packagedSize,
                                      &encodedFrameInfo));

        // Preserve the current stream state as it might change after we apply the metadata
        generatorState = encodedFrameInfo.streamState;

        // Need to handle the metadata if any.
        // NOTE: The accumulated metadata will be inserted before the fragment start due to MKV limitation
        // as tags are level 1 and will break MKV clusters.
        if (generatorState == MKV_STATE_START_STREAM || generatorState == MKV_STATE_START_CLUSTER) {
            // Calculate the size of the metadata first
            CHK_STATUS(packageStreamMetadata(pKinesisVideoStream, generatorState, FALSE, NULL, &packagedMetadataSize));
        }
    }

    // Overall frame allocation size
    overallSize = packagedSize + packagedMetadataSize;

    pKinesisVideoStream->maxFrameSizeSeen = MAX(pKinesisVideoStream->maxFrameSizeSeen, overallSize);

    // Might need to block on the availability in the OFFLINE mode
    CHK_STATUS(waitForAvailability(pKinesisVideoStream, overallSize));

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                     pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Allocate storage for the frame
    CHK_STATUS(heapAlloc(pKinesisVideoClient->pHeap, overallSize, &allocHandle));

    // Ensure we have space and if not then bail
    CHK(IS_VALID_ALLOCATION_HANDLE(allocHandle), STATUS_STORE_OUT_OF_MEMORY);

    // Map the storage
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, allocHandle, (PVOID *) &pAlloc, &allocSize));

    // Validate we had allocated enough storage just in case
    CHK(overallSize <= allocSize, STATUS_ALLOCATION_SIZE_SMALLER_THAN_REQUESTED);

    // Store the metadata at the beginning of the allocation
    if (packagedMetadataSize != 0) {
        // Packaging the metadata if the packaged size is not zero
        CHK_STATUS(packageStreamMetadata(pKinesisVideoStream, generatorState, FALSE, pAlloc, &packagedMetadataSize));
    }

    // Check if we are packaging special EoFr
    if (CHECK_FRAME_FLAG_END_OF_FRAGMENT(pFrame->flags)) {
        // Package the existing metadata that's not yet sent
        CHK_STATUS(packageStreamMetadata(pKinesisVideoStream,
                                         MKV_STATE_START_CLUSTER,
                                         TRUE,
                                         pAlloc + packagedMetadataSize,
                                         &packagedSize));

        // Synthesize the encodedFrameInfo
        CHK_STATUS(mkvgenGetCurrentTimestamps(pKinesisVideoStream->pMkvGenerator,
                                   &encodedFrameInfo.streamStartTs,
                                   &encodedFrameInfo.clusterPts,
                                   &encodedFrameInfo.clusterDts));
        encodedFrameInfo.dataOffset = 0;
        encodedFrameInfo.streamState = MKV_STATE_START_BLOCK;

        // Pre-set the item flags to differentiate EoFr in EoS case
        SET_ITEM_FRAGMENT_END(itemFlags);

        // Synthesize the frame timestamps based on the last content view entry
        CHK_STATUS(contentViewGetHead(pKinesisVideoStream->pView, &pViewItem));
        encodedFrameInfo.frameDts = pViewItem->timestamp + pViewItem->duration - encodedFrameInfo.clusterDts;
        encodedFrameInfo.framePts = pViewItem->ackTimestamp + pViewItem->duration - encodedFrameInfo.clusterPts;
        encodedFrameInfo.duration = 0;

        // Set the EoFr flag so we won't append not-sent metadata/EOS on StreamStop
        pKinesisVideoStream->eofrFrame = TRUE;
    } else {
        // Actually package the bits in the storage
        CHK_STATUS(mkvgenPackageFrame(pKinesisVideoStream->pMkvGenerator,
                                      pFrame,
                                      pAlloc + packagedMetadataSize,
                                      &packagedSize,
                                      &encodedFrameInfo));
    }

    // Unmap the storage for the frame
    CHK_STATUS(heapUnmap(pKinesisVideoClient->pHeap, ((PVOID) pAlloc)));

    // Check for storage pressures. No need for offline mode as the media pipeline will be blocked when there
    // is not enough storage
    if (!IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType) &&
        pKinesisVideoClient->clientCallbacks.storageOverflowPressureFn != NULL) {
        remainingSize = pKinesisVideoClient->pHeap->heapLimit - pKinesisVideoClient->pHeap->heapSize;
        thresholdPercent = (UINT32) (((DOUBLE) remainingSize / pKinesisVideoClient->pHeap->heapLimit) * 100);

        if (thresholdPercent <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD) {
            // Notify the client app about buffer pressure
            CHK_STATUS(pKinesisVideoClient->clientCallbacks.storageOverflowPressureFn(
                    pKinesisVideoClient->clientCallbacks.customData,
                    remainingSize));
        }
    }

    // Only report buffer duration overflow if retention is non-zero, since if retention is zero, there will be no
    // persisted ack and buffer will drop off tail all the time.
    if (pKinesisVideoStream->streamInfo.retention != RETENTION_PERIOD_SENTINEL) {
        CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &currentDuration, &windowDuration));

        // Check for buffer duration pressure. Note that streamCaps.bufferDuration will never be 0.
        remainingDuration = pKinesisVideoStream->streamInfo.streamCaps.bufferDuration - windowDuration;
        thresholdPercent = (UINT32) (((DOUBLE) remainingDuration / pKinesisVideoStream->streamInfo.streamCaps.bufferDuration) * 100);
        if (thresholdPercent <= BUFFER_DURATION_PRESSURE_NOTIFICATION_THRESHOLD &&
            pKinesisVideoClient->clientCallbacks.bufferDurationOverflowPressureFn != NULL) {
            // Notify the client app about buffer pressure
            CHK_STATUS(pKinesisVideoClient->clientCallbacks.bufferDurationOverflowPressureFn(
                    pKinesisVideoClient->clientCallbacks.customData,
                    TO_STREAM_HANDLE(pKinesisVideoStream),
                    remainingDuration));
        }
    }

    // Generate the view flags
    switch (generatorState) {
        case MKV_STATE_START_STREAM:
            SET_ITEM_STREAM_START(itemFlags);
            // fall-through
        case MKV_STATE_START_CLUSTER:
            SET_ITEM_FRAGMENT_START(itemFlags);

            // Clear the EoFr flag
            pKinesisVideoStream->eofrFrame = FALSE;
            break;
        case MKV_STATE_START_BLOCK:
            break;
    }

    // Put the frame into the view.
    // NOTE: For the timestamp we will specify the cluster timestamp + frame timestamp which
    // will be useful later to find the start of the fragment corresponding to the ACK timecode.
    // We will also use the a non simple block state as a fragment start indicator - an IDR frame.
    // NOTE: We are using the decoding timestamp as our item timestamp.
    // Although, the intra-cluster pts might vary from dts, the cluster start frame pts
    // will be equal to dts which is important when processing ACKs.
    // NOTE: The size of the actual data might be smaller than the allocated size due to the fact that
    // calculating the packaged metadata and frame might have preserved the MKV header twice as the calculation
    // of the size doesn't mutate the state of the generator. We will use the actual packaged size.
    CHK_STATUS(contentViewAddItem(pKinesisVideoStream->pView,
                                  encodedFrameInfo.clusterDts + encodedFrameInfo.frameDts,
                                  encodedFrameInfo.clusterPts + encodedFrameInfo.framePts,
                                  encodedFrameInfo.duration,
                                  allocHandle,
                                  encodedFrameInfo.dataOffset,
                                  packagedSize + packagedMetadataSize,
                                  itemFlags));


    // From now on we don't need to free the allocation as it's in the view already and will be collected
    freeOnError = FALSE;

    if (CHECK_ITEM_STREAM_START(itemFlags)) {
        // Store the stream start timestamp for ACK timecode adjustment for relative cluster timecode streams
        pKinesisVideoStream->newSessionTimestamp = encodedFrameInfo.streamStartTs;
        CHK_STATUS(contentViewGetHead(pKinesisVideoStream->pView, &pViewItem));
        pKinesisVideoStream->newSessionIndex = pViewItem->index;
    }

    // We need to check for the latency pressure. If the view head is ahead of the current
    // for more than the specified max latency then we need to call the optional user callback.
    // NOTE: A special sentinel value is used to determine whether the latency is specified.
    if (pKinesisVideoStream->streamInfo.streamCaps.maxLatency != STREAM_LATENCY_PRESSURE_CHECK_SENTINEL &&
        pKinesisVideoClient->clientCallbacks.streamLatencyPressureFn != NULL) {
        // Get the window duration from the view
        CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &duration, NULL));

        // Check for the breach and invoke the user provided callback
        if (duration > pKinesisVideoStream->streamInfo.streamCaps.maxLatency) {
            CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamLatencyPressureFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                duration));
        }
    }

    // Notify about data is available
    pUploadHandleInfo = getStreamUploadInfoWithState(pKinesisVideoStream, UPLOAD_HANDLE_STATE_READY |
            UPLOAD_HANDLE_STATE_STREAMING);
    if (NULL != pUploadHandleInfo && IS_VALID_UPLOAD_HANDLE(pUploadHandleInfo->handle)) {
        // Get the duration and the size
        CHK_STATUS(getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize));

        // Call the notification callback
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                pKinesisVideoStream->streamInfo.name,
                pUploadHandleInfo->handle,
                duration,
                viewByteSize));
    }

    // Recalculate frame rate if enabled
    if (pKinesisVideoStream->streamInfo.streamCaps.recalculateMetrics) {
        // Calculate the current frame rate only after the first iteration
        currentTime = IS_VALID_TIMESTAMP(currentTime) ?
                      currentTime :
                      pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
        if (!CHECK_ITEM_STREAM_START(itemFlags)) {
            // Calculate the delta time in seconds
            deltaInSeconds = (DOUBLE) (currentTime - pKinesisVideoStream->diagnostics.lastFrameRateTimestamp) /
                             HUNDREDS_OF_NANOS_IN_A_SECOND;

            if (deltaInSeconds != 0) {
                frameRate = 1 / deltaInSeconds;

                // Update the current frame rate
                pKinesisVideoStream->diagnostics.currentFrameRate = EMA_ACCUMULATOR_GET_NEXT(
                        pKinesisVideoStream->diagnostics.currentFrameRate, frameRate);
            }
        }

        // Store the last frame timestamp
        pKinesisVideoStream->diagnostics.lastFrameRateTimestamp = currentTime;
    }

    // Unlock the client as we no longer need it locked
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    // We need to see whether we need to remove the allocation on error. Otherwise, we will leak
    if (STATUS_FAILED(retStatus) && IS_VALID_ALLOCATION_HANDLE(allocHandle) && freeOnError) {
        // Lock the client if it's not locked
        if (!clientLocked) {
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
            clientLocked = TRUE;
        }

        // Free the actual allocation as we will leak otherwise.
        heapFree(pKinesisVideoClient->pHeap, allocHandle);
    }

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Fills the caller buffer with the stream data
 */
STATUS getStreamData(PKinesisVideoStream pKinesisVideoStream, UPLOAD_HANDLE uploadHandle, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pFillSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, stalenessCheckStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PViewItem pViewItem = NULL;
    PBYTE pAlloc = NULL;
    UINT32 size = 0, remainingSize = bufferSize, uploadHandleCount;
    UINT64 allocSize, currentTime, duration, viewByteSize;
    PBYTE pCurPnt = pBuffer;
    BOOL streamLocked = FALSE, clientLocked = FALSE, rollbackToLastAck, restarted = FALSE, eosSent = FALSE;
    DOUBLE transferRate, deltaInSeconds;
    PUploadHandleInfo pUploadHandleInfo = NULL, pNextUploadHandleInfo = NULL;

    CHK(pKinesisVideoStream != NULL &&
        pKinesisVideoStream->pKinesisVideoClient != NULL &&
        pBuffer != NULL &&
        pFillSize != NULL,
        STATUS_NULL_ARG);
    CHK(bufferSize != 0 && IS_VALID_UPLOAD_HANDLE(uploadHandle), STATUS_INVALID_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // If the state of the connection is IN_USE
    // and we are not in grace period
    // and we are not in a retry state on rotation
    // then we need to rollback the current view pointer
    if ((pKinesisVideoStream->connectionState & UPLOAD_CONNECTION_STATE_IN_USE) != UPLOAD_CONNECTION_STATE_NONE) {
        if (IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType)) {
            // In case of offline mode, we just need to set the current to the tail.
            CHK_STATUS(contentViewGetTail(pKinesisVideoStream->pView, &pViewItem));
            CHK_STATUS(contentViewSetCurrentIndex(pKinesisVideoStream->pView, pViewItem->index));
        } else {
            // Calculate the rollback duration based on the stream replay duration and the latest ACKs received.
            // If the acks are not enabled or we have potentially dead host situation then we rollback to the replay duration.
            // NOTE: The received ACK should be sequential and not out-of-order.
            rollbackToLastAck = pKinesisVideoStream->streamInfo.streamCaps.fragmentAcks &&
                                CONNECTION_DROPPED_HOST_ALIVE(pKinesisVideoStream->connectionDroppedResult);

            CHK_STATUS(contentViewRollbackCurrent(pKinesisVideoStream->pView,
                                                  pKinesisVideoStream->streamInfo.streamCaps.replayDuration,
                                                  TRUE,
                                                  rollbackToLastAck));
        }

        // Fix-up the current element
        CHK_STATUS(streamStartFixupOnReconnect(pKinesisVideoStream));

        restarted = TRUE;
    }

    // Reset the connection dropped indicator
    pKinesisVideoStream->connectionState = UPLOAD_CONNECTION_STATE_OK;

    // Set the size first
    *pFillSize = 0;

    // Get the latest upload handle
    pUploadHandleInfo = getStreamUploadInfo(pKinesisVideoStream, uploadHandle);

    // Should indicate an abort for an invalid handle or handle that is not in the queue
    CHK(pUploadHandleInfo != NULL, STATUS_UPLOAD_HANDLE_ABORTED);

    switch (pUploadHandleInfo->state) {
        case UPLOAD_HANDLE_STATE_READY:
            // if we've created a new handle but has nothing to send
            if (pKinesisVideoStream->streamStopped) {
                // Get the duration and the size
                CHK_STATUS(getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize));

                if (viewByteSize == 0) {
                    pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;
                    CHK(FALSE, STATUS_END_OF_STREAM);
                }
            }

            if (IS_VALID_TIMESTAMP(pKinesisVideoStream->newSessionTimestamp)) {
                // set upload handle start timestamp to map ack timestamp to view item timestamp in case
                // of using relative timestamp mode.
                pUploadHandleInfo->timestamp = pKinesisVideoStream->newSessionTimestamp;

                // Zero the sentinels
                pKinesisVideoStream->newSessionTimestamp = INVALID_TIMESTAMP_VALUE;
                pKinesisVideoStream->newSessionIndex = INVALID_VIEW_INDEX_VALUE;
            }
            pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_STREAMING;

            break;
        case UPLOAD_HANDLE_STATE_AWAITING_ACK:
            // handle case where new handle got last ack already
            if (pUploadHandleInfo->lastPersistedAckTs == pUploadHandleInfo->lastFragmentTs) {
                pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;

                CHK(FALSE, STATUS_END_OF_STREAM);
            }
            CHK(FALSE, STATUS_AWAITING_PERSISTED_ACK);

        case UPLOAD_HANDLE_STATE_ACK_RECEIVED:
            // Set the state to terminated and early return with the EOS with the right handle
            pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;

            DLOGI("Indicating an EOS after last persisted ACK is received for stream upload handle %" PRIu64, uploadHandle);
            CHK(FALSE, STATUS_END_OF_STREAM);

        case UPLOAD_HANDLE_STATE_TERMINATED:
            // This path get invoked if a connection get terminated by calling reset connection
            DLOGW("Indicating an abort for a terminated stream upload handle %" PRIu64, uploadHandle);
            CHK(FALSE, STATUS_UPLOAD_HANDLE_ABORTED);

        case UPLOAD_HANDLE_STATE_ERROR:
            DLOGW("Indicating an abort for a errorred stream upload handle %" PRIu64, uploadHandle);
            CHK(FALSE, STATUS_UPLOAD_HANDLE_ABORTED);
        default:
            // no-op for other UPLOAD_HANDLE states
            break;
    }

    // Continue filling the buffer from the point we left off
    do {
        // First and foremost, we need to check whether we need to
        // send the packaged not-yet sent metadata, then
        // we need to check whether we need to send the packaged EOS metadata
        // Next, we need to check whether we have an allocation handle.
        // This could happen in case when we start getting the data out or
        // when the last time we got the data the item fell off the view window.
        if (IS_UPLOAD_HANDLE_IN_SENDING_EOS_STATE(pUploadHandleInfo) && pKinesisVideoStream->metadataTracker.send) {
            // Check if we have finished sending the packaged metadata and reset the values
            if (pKinesisVideoStream->metadataTracker.offset == pKinesisVideoStream->metadataTracker.size) {
                pKinesisVideoStream->metadataTracker.offset = 0;
                pKinesisVideoStream->metadataTracker.send = FALSE;

                // Set to send the EoS
                pKinesisVideoStream->eosTracker.send = TRUE;
            } else {
                // Copy as much as we can
                size = MIN(remainingSize, pKinesisVideoStream->metadataTracker.size - pKinesisVideoStream->metadataTracker.offset);
                MEMCPY(pCurPnt, pKinesisVideoStream->metadataTracker.data + pKinesisVideoStream->metadataTracker.offset, size);

                // Set the values for the next iteration.
                pKinesisVideoStream->metadataTracker.offset += size;
                pCurPnt += size;
                remainingSize -= size;
                *pFillSize += size;
            }
        } else if (IS_UPLOAD_HANDLE_IN_SENDING_EOS_STATE(pUploadHandleInfo) && pKinesisVideoStream->eosTracker.send) {
            if (pKinesisVideoStream->eosTracker.offset == pKinesisVideoStream->eosTracker.size) {
                pKinesisVideoStream->eosTracker.offset = 0;
                pKinesisVideoStream->eosTracker.send = FALSE;

                // Calculate the state for the upload handle based on whether we need to await for the last ACK
                if (WAIT_FOR_PERSISTED_ACK(pKinesisVideoStream)) {
                    if (pUploadHandleInfo->lastPersistedAckTs == pUploadHandleInfo->lastFragmentTs) {
                        pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;

                        CHK(FALSE, STATUS_END_OF_STREAM);
                    }
                    pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_AWAITING_ACK;
                    DLOGI("Handle %" PRIu64 " waiting for last persisted ack with ts %" PRIu64, pUploadHandleInfo->handle, pUploadHandleInfo->lastFragmentTs);

                    // Will terminate later after the ACK is received
                    CHK(FALSE, STATUS_AWAITING_PERSISTED_ACK);
                } else {
                    pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;

                    // The client should terminate and close the stream after finalizing any remaining transfer.
                    CHK(FALSE, STATUS_END_OF_STREAM);
                }
            }

            // Copy as much as we can
            size = MIN(remainingSize, pKinesisVideoStream->eosTracker.size - pKinesisVideoStream->eosTracker.offset);
            MEMCPY(pCurPnt, pKinesisVideoStream->eosTracker.data + pKinesisVideoStream->eosTracker.offset, size);
            // Set the values
            pKinesisVideoStream->eosTracker.offset += size;
            pCurPnt += size;
            remainingSize -= size;
            *pFillSize += size;
        } else if (!IS_VALID_ALLOCATION_HANDLE(pKinesisVideoStream->curViewItem.viewItem.handle)) {
            // Reset the current view item
            pKinesisVideoStream->curViewItem.offset = pKinesisVideoStream->curViewItem.viewItem.length = 0;

            // Fix-up the current item as it might be a stream start
            CHK_STATUS(resetCurrentViewItemStreamStart(pKinesisVideoStream));

            // Need to find the next key frame boundary in case of the current rolling out of the window
            CHK_STATUS(getNextBoundaryViewItem(pKinesisVideoStream, &pViewItem));

            if (pViewItem != NULL) {
                // Reset the item ACK flags as this might be replay after rollback
                CLEAR_ITEM_BUFFERING_ACK(pViewItem->flags);
                CLEAR_ITEM_RECEIVED_ACK(pViewItem->flags);

                // Use this as a current for the next iteration
                pKinesisVideoStream->curViewItem.viewItem = *pViewItem;
            } else {
                // Couldn't find any boundary items, default to empty - early return
                CHK(FALSE, STATUS_NO_MORE_DATA_AVAILABLE);
            }
        } else if (pKinesisVideoStream->curViewItem.offset == pKinesisVideoStream->curViewItem.viewItem.length) {
            // Check if the current was a special EoFr in which case we don't need to send EoS
            eosSent = CHECK_ITEM_FRAGMENT_END(pKinesisVideoStream->curViewItem.viewItem.flags) ? TRUE : FALSE;

            // Fix-up the current item as it might be a stream start
            CHK_STATUS(resetCurrentViewItemStreamStart(pKinesisVideoStream));

            // Second, we need to check whether the existing view item has been exhausted
            retStatus = contentViewGetNext(pKinesisVideoStream->pView, &pViewItem);
            CHK(retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS || retStatus == STATUS_SUCCESS, retStatus);

            // Special handling for the stopped stream.
            if (retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS) {
                // This requires user to call stop stream as soon as they are done.
                // Need to kick off the EOS sequence
                if (!pKinesisVideoStream->streamStopped) {
                    // Early return
                    CHK(FALSE, retStatus);
                }

                pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATING;
                pKinesisVideoStream->eosTracker.send = TRUE;
                pKinesisVideoStream->eosTracker.offset = eosSent ? pKinesisVideoStream->eosTracker.size : 0;

                // If we have eosSent then we can't append the last metadata afterwards as it will break the MKV cluster
                if (eosSent) {
                    pKinesisVideoStream->metadataTracker.send = FALSE;
                }
            } else {

                // Reset the item ACK flags as this might be replay after rollback
                CLEAR_ITEM_BUFFERING_ACK(pViewItem->flags);
                CLEAR_ITEM_RECEIVED_ACK(pViewItem->flags);

                // Store it in the current view
                pKinesisVideoStream->curViewItem.viewItem = *pViewItem;
                pKinesisVideoStream->curViewItem.offset = 0;

                // Check if we are finishing the previous stream and we have a boundary item
                if (CHECK_ITEM_STREAM_START(pKinesisVideoStream->curViewItem.viewItem.flags)) {

                    // Set the stream handle as we have exhausted the previous
                    pUploadHandleInfo->state = WAIT_FOR_PERSISTED_ACK(pKinesisVideoStream)
                                               ? UPLOAD_HANDLE_STATE_TERMINATING : UPLOAD_HANDLE_STATE_TERMINATED;

                    // Indicate to send EOS in the next iteration
                    pKinesisVideoStream->eosTracker.send = TRUE;
                    pKinesisVideoStream->eosTracker.offset = eosSent ? pKinesisVideoStream->eosTracker.size : 0;

                    // If we have eosSent then we can't append the last metadata afterwards as it will break the MKV cluster
                    if (eosSent) {
                        pKinesisVideoStream->metadataTracker.send = FALSE;
                    }
                }
            }
        } else {
            // Now, we can stream enough data out if we don't have a zero item
            CHK(pKinesisVideoStream->curViewItem.offset != pKinesisVideoStream->curViewItem.viewItem.length, STATUS_NO_MORE_DATA_AVAILABLE);

            // Set the key frame indicator on fragment or session start to track the persist ACKs
            if (CHECK_ITEM_FRAGMENT_START(pKinesisVideoStream->curViewItem.viewItem.flags) ||
                CHECK_ITEM_STREAM_START(pKinesisVideoStream->curViewItem.viewItem.flags)) {
                pUploadHandleInfo->lastFragmentTs = pKinesisVideoStream->curViewItem.viewItem.ackTimestamp;
            }

            // Lock the client
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
            clientLocked = TRUE;

            // Fill the rest of the buffer of the current view item first
            // Map the storage
            CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, pKinesisVideoStream->curViewItem.viewItem.handle, (PVOID *) &pAlloc, &allocSize));
            CHK(allocSize < MAX_UINT32, STATUS_INVALID_ALLOCATION_SIZE);
            size = (UINT32) allocSize;

            // Validate we had allocated enough storage just in case
            CHK(pKinesisVideoStream->curViewItem.viewItem.length - pKinesisVideoStream->curViewItem.offset <= size, STATUS_VIEW_ITEM_SIZE_GREATER_THAN_ALLOCATION);

            // Copy as much as we can
            size = MIN(remainingSize, pKinesisVideoStream->curViewItem.viewItem.length - pKinesisVideoStream->curViewItem.offset);
            MEMCPY(pCurPnt, pAlloc + pKinesisVideoStream->curViewItem.offset, size);

            // Unmap the storage for the frame
            CHK_STATUS(heapUnmap(pKinesisVideoClient->pHeap, ((PVOID) pAlloc)));

            // Unlock the client
            pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                               pKinesisVideoClient->base.lock);
            clientLocked = FALSE;

            // Set the values
            pKinesisVideoStream->curViewItem.offset += size;
            pCurPnt += size;
            remainingSize -= size;
            *pFillSize += size;
        }
    } while (remainingSize != 0);

CleanUp:

    // Run staleness detection if we have ACKs enabled and if we have retrieved any data
    if (pFillSize != NULL && *pFillSize != 0) {
        stalenessCheckStatus = checkForConnectionStaleness(pKinesisVideoStream, &pKinesisVideoStream->curViewItem.viewItem);
    }

    if (retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS) {
        // Replace it with a client side error
        retStatus = STATUS_NO_MORE_DATA_AVAILABLE;
    }

    // Calculate the metrics for transfer rate on successful call and when we do have some data.
    if (pKinesisVideoStream->streamInfo.streamCaps.recalculateMetrics &&
        IS_VALID_GET_STREAM_DATA_STATUS(retStatus) &&
        pFillSize != NULL &&
        *pFillSize != 0) {
        // Calculate the current transfer rate only after the first iteration
        currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
        pKinesisVideoStream->diagnostics.accumulatedByteCount += *pFillSize;
        if (!restarted) {
            // Calculate the delta time in seconds
            deltaInSeconds = (DOUBLE) (currentTime - pKinesisVideoStream->diagnostics.lastTransferRateTimestamp) / HUNDREDS_OF_NANOS_IN_A_SECOND;

            if (deltaInSeconds > TRANSFER_RATE_MEASURING_INTERVAL_EPSILON) {
                transferRate = pKinesisVideoStream->diagnostics.accumulatedByteCount / deltaInSeconds;

                // Update the current frame rate
                pKinesisVideoStream->diagnostics.currentTransferRate = (UINT64) EMA_ACCUMULATOR_GET_NEXT(pKinesisVideoStream->diagnostics.currentTransferRate, transferRate);

                // Zero out for next time measuring
                pKinesisVideoStream->diagnostics.accumulatedByteCount = 0;

                // Store the last frame timestamp
                pKinesisVideoStream->diagnostics.lastTransferRateTimestamp = currentTime;
            }
        } else {
            // Store the last frame timestamp
            pKinesisVideoStream->diagnostics.lastTransferRateTimestamp = currentTime;
        }
    }

    // Special handling for stopped stream when the retention period is zero or no more data available
    if (pKinesisVideoStream->streamStopped) {
        // Trigger stream closed function when we don't need to wait for the persisted ack
        // Or if we do need to wait for the ack and the state of the upload handler is terminated
        if (retStatus == STATUS_END_OF_STREAM &&
            (!WAIT_FOR_PERSISTED_ACK(pKinesisVideoStream) ||
            (pUploadHandleInfo != NULL && pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_TERMINATED))) {

            // Get the duration and the size
            CHK_STATUS(getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize));

            // Get the number of non terminated handle.
            CHK_STATUS(stackQueueGetCount(pKinesisVideoStream->pUploadInfoQueue, &uploadHandleCount));

            // If there is no more data to send and current handle is the last one, wrap up by calling streamClosedFn.
            if (viewByteSize == 0 && uploadHandleCount == 1) {
                CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamClosedFn(
                        pKinesisVideoClient->clientCallbacks.customData,
                        TO_STREAM_HANDLE(pKinesisVideoStream),
                        uploadHandle));
            }
        }
    }

    // when pUploadHandleInfo is in UPLOAD_HANDLE_STATE_TERMINATED or UPLOAD_HANDLE_STATE_AWAITING_ACK, it means current handle
    // has sent all the data. So it's safe to unblock the next one if any.
    if (NULL != pUploadHandleInfo &&
        (pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_TERMINATED || pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_AWAITING_ACK)) {
        // Special handling for the case:
        // When the stream has been stopped so no more putFrame calls that drive the data availability
        // When the current upload handle is in EoS
        // When in offline mode, and the putFrame thread has been blocked so no more putFrame calls that drive
        // the data availability
        // When we still have ready upload handles which are awaiting

        // We need to trigger their data ready callback to enable them

        // Remove the upload handle if it is in UPLOAD_HANDLE_STATE_TERMINATED
        if (pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_TERMINATED) {
            deleteStreamUploadInfo(pKinesisVideoStream, pUploadHandleInfo);
            pUploadHandleInfo = NULL;
        }

        // Get the next upload handle to notify
        pNextUploadHandleInfo = getCurrentStreamUploadInfo(pKinesisVideoStream);

        // Notify the awaiting handle to enable it
        if (pNextUploadHandleInfo != NULL) {
            // Get the duration and the size
            CHK_STATUS(getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize));

            CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                    pKinesisVideoClient->clientCallbacks.customData,
                    TO_STREAM_HANDLE(pKinesisVideoStream),
                    pKinesisVideoStream->streamInfo.name,
                    pNextUploadHandleInfo->handle,
                    duration,
                    viewByteSize));
        }
    }

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    // Return the staleness check status if it's not a success
    if (STATUS_FAILED(stalenessCheckStatus)) {
        retStatus = stalenessCheckStatus;
    }

    LEAVES();
    return retStatus;
}

/**
 * Returns stream metrics.
 */
STATUS getStreamMetrics(PKinesisVideoStream pKinesisVideoStream, PStreamMetrics pStreamMetrics)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pStreamMetrics != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    CHK(pStreamMetrics->version <= STREAM_METRICS_CURRENT_VERSION, STATUS_INVALID_STREAM_METRICS_VERSION);

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    CHK_STATUS(contentViewGetWindowAllocationSize(pKinesisVideoStream->pView, &pStreamMetrics->currentViewSize, &pStreamMetrics->overallViewSize));
    CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &pStreamMetrics->currentViewDuration, &pStreamMetrics->overallViewDuration));

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

    // Store the frame rate and the transfer rate
    pStreamMetrics->currentFrameRate = pKinesisVideoStream->diagnostics.currentFrameRate;
    pStreamMetrics->currentTransferRate = pKinesisVideoStream->diagnostics.currentTransferRate;

CleanUp:

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Await for the frame availability in OFFLINE mode.
 *
 * IMPORTANT: The assumption is that both the stream IS locked but
 * the client is NOT locked.
 *
 * The parameters are assumed to have been sanitized.
 * The call will block only in the offline mode waiting for the
 * available space in the content store and available spot
 * in the content view in order to proceed.
 *
 * The caller thread will be blocked.
 * The operation will return interrupted error in case the stream is terminated.
 */
STATUS waitForAvailability(PKinesisVideoStream pKinesisVideoStream, UINT32 allocationSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    BOOL streamLocked = TRUE, availability = FALSE;

    // Quick check if we need to do any awaiting
    CHK(IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType), STATUS_SUCCESS);

    while (TRUE) {
        // Check if we have enough space to proceed - the stream should be locked
        CHK_STATUS(checkForAvailability(pKinesisVideoStream, allocationSize, &availability));

        // Early return if available
        CHK(!availability, STATUS_SUCCESS);

        // Unlock the stream so other threads can proceed
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                           pKinesisVideoStream->base.lock);
        streamLocked = FALSE;
        // Long path which will await for the availability notification or cancellation
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.waitConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                                pKinesisVideoStream->bufferAvailabilityCondition,
                                                                                pKinesisVideoStream->bufferAvailabilityLock,
                                                                                MAX_BLOCKING_PUT_WAIT));
        // Lock the stream again in order to proceed
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                         pKinesisVideoStream->base.lock);
        streamLocked = TRUE;

        // Check for the stream termination
        CHK(!pKinesisVideoStream->streamStopped, STATUS_BLOCKING_PUT_INTERRUPTED_STREAM_TERMINATED);
    }

CleanUp:

    // Lock the stream again in order to proceed
    if (!streamLocked) {
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                         pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Checks for the availability of space in the content store and if there is enough duration
 * is available in the content view.
 *
 * IMPORTANT: The stream SHOULD be locked and the client is NOT locked
 *
 * All the parameters are assumed to have been sanitized.
 */
STATUS checkForAvailability(PKinesisVideoStream pKinesisVideoStream, UINT32 allocationSize, PBOOL pAvailability)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    BOOL clientLocked = FALSE, availability = FALSE;
    UINT64 heapSize, availableHeapSize;
    PViewItem pTailViewItem;

    // Set to unavailable
    *pAvailability = FALSE;

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the heap size
    CHK_STATUS(heapGetSize(pKinesisVideoClient->pHeap, &heapSize));

    // Unlock the client
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Check for undeflow
    CHK(pKinesisVideoClient->deviceInfo.storageInfo.storageSize > heapSize + MAX_ALLOCATION_OVERHEAD_SIZE +
                                                                  pKinesisVideoStream->maxFrameSizeSeen * FRAME_ALLOC_FRAGMENTATION_FACTOR, STATUS_SUCCESS);

    // Check if we have enough space available. Adding maxFrameSizeSeen to handle fragmentation as well as when curl thread needs to alloc space for
    // sending data.
    availableHeapSize = pKinesisVideoClient->deviceInfo.storageInfo.storageSize - heapSize -
                        MAX_ALLOCATION_OVERHEAD_SIZE - pKinesisVideoStream->maxFrameSizeSeen * FRAME_ALLOC_FRAGMENTATION_FACTOR;

    // Early return if storage space is unavailable.
    CHK(availableHeapSize >= allocationSize, STATUS_SUCCESS);

    // Check to see if we have availability in the content view. This will set the availability
    CHK_STATUS(contentViewCheckAvailability(pKinesisVideoStream->pView, NULL, &availability));

    // Early return if no view availability
    CHK(availability, STATUS_SUCCESS);

    // Also check whether we have a partially consumed frame
    retStatus = contentViewGetTail(pKinesisVideoStream->pView, &pTailViewItem);
    CHK(retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS || retStatus == STATUS_SUCCESS, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

CleanUp:

    // Unlock the client if locked.
    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    // Set the return value
    *pAvailability = availability;

    LEAVES();
    return retStatus;
}

/**
 * Stream format changed. Currently, codec private data only. Will return OK if nothing to be done.
 */
STATUS streamFormatChanged(PKinesisVideoStream pKinesisVideoStream, UINT32 codecPrivateDataSize, PBYTE codecPrivateData, UINT64 trackId)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Ensure we are not in a streaming state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                            STREAM_STATE_READY |
                                            STREAM_STATE_NEW |
                                            STREAM_STATE_DESCRIBE |
                                            STREAM_STATE_CREATE |
                                            STREAM_STATE_GET_ENDPOINT |
                                            STREAM_STATE_GET_TOKEN |
                                            STREAM_STATE_STOPPED));

    CHK_STATUS(mkvgenSetCodecPrivateData(pKinesisVideoStream->pMkvGenerator, trackId, codecPrivateDataSize, codecPrivateData));

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Puts a metadata into the stream
 */
STATUS putFragmentMetadata(PKinesisVideoStream pKinesisVideoStream, PCHAR name, PCHAR value, BOOL persistent) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;
    UINT32 packagedSize = 0, metadataQueueSize;
    PSerializedMetadata pExistingSerializedMetadata;
    StackQueueIterator iterator;
    UINT64 data;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Check if the stream has been stopped
    CHK(!pKinesisVideoStream->streamStopped, STATUS_STREAM_HAS_BEEN_STOPPED);

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                     pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Check if we are in the right state only if we are not in a rotation state
    if (pKinesisVideoStream->streamState != STREAM_STATE_READY) {
        CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                           STREAM_STATE_READY |
                                           STREAM_STATE_PUT_STREAM |
                                           STREAM_STATE_TAG_STREAM |
                                           STREAM_STATE_STREAMING |
                                           STREAM_STATE_GET_ENDPOINT |
                                           STREAM_STATE_GET_TOKEN |
                                           STREAM_STATE_STOPPED));
    }

    // Validate if the customer is not attempting to add an internal metadata
    CHK(0 != STRNCMP(AWS_INTERNAL_METADATA_PREFIX, name, (SIZEOF(AWS_INTERNAL_METADATA_PREFIX) - 1) / SIZEOF(CHAR)),
        STATUS_INVALID_METADATA_NAME);

    // Check whether we are OK to package the metadata but do not package.
    CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                 NULL,
                                 name,
                                 value,
                                 &packagedSize));

    // Check if the metadata exists in case of persistent metadata
    if (persistent) {
        // Iterate linearly and see if we have a match with the name
        CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pMetadataQueue, &iterator));
        while (IS_VALID_ITERATOR(iterator)) {
            CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

            pExistingSerializedMetadata = (PSerializedMetadata) data;
            CHK(pExistingSerializedMetadata != NULL, STATUS_INTERNAL_ERROR);

            // Check to see if we have a persistent metadata and have a match and if we do then just remove
            if (pExistingSerializedMetadata->persistent && (0 == STRCMP(pExistingSerializedMetadata->name, name))) {
                // Remove and delete the object
                stackQueueRemoveItem(pKinesisVideoStream->pMetadataQueue, data);

                // Delete the object
                MEMFREE(pExistingSerializedMetadata);

                // If we need to just remove the metadata then we will also early exit
                // NOTE: value is not NULL as it's been checked in the generator.
                if (value[0] == '\0') {
                    CHK(FALSE, retStatus);
                }

                break;
            }

            CHK_STATUS(stackQueueIteratorNext(&iterator));
        }
    }

    // Ensure we don't have more than MAX size of the metadata queue
    CHK_STATUS(stackQueueGetCount(pKinesisVideoStream->pMetadataQueue, &metadataQueueSize));
    CHK(metadataQueueSize < MAX_FRAGMENT_METADATA_COUNT, STATUS_MAX_FRAGMENT_METADATA_COUNT);

    CHK_STATUS(appendValidatedMetadata(pKinesisVideoStream, name, value, persistent, packagedSize));

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

STATUS streamStartFixupOnReconnect(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 curIndex;
    UINT64 streamStartTs;
    PViewItem pViewItem = NULL;
    BOOL streamLocked = FALSE, clientLocked = FALSE;
    UINT32 headerSize, packagedSize, overallSize, dataOffset;
    UINT64 allocSize;
    PBYTE pAlloc = NULL, pFrame = NULL;
    PKinesisVideoClient pKinesisVideoClient;
    ALLOCATION_HANDLE allocationHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    ALLOCATION_HANDLE oldAllocationHandle;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Fix-up the current item as it might be a stream start
    CHK_STATUS(resetCurrentViewItemStreamStart(pKinesisVideoStream));

    // Reset the current view item on connection reset
    MEMSET(&pKinesisVideoStream->curViewItem, 0x00, SIZEOF(CurrentViewItem));
    pKinesisVideoStream->curViewItem.viewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;

    // Reset the EOS tracker
    pKinesisVideoStream->eosTracker.send = FALSE;
    pKinesisVideoStream->eosTracker.offset = 0;

    // Get the current item
    CHK_STATUS(contentViewGetCurrentIndex(pKinesisVideoStream->pView, &curIndex));
    CHK_STATUS(contentViewGetItemAt(pKinesisVideoStream->pView, curIndex, &pViewItem));

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the required size for the header
    CHK_STATUS(mkvgenGenerateHeader(pKinesisVideoStream->pMkvGenerator,
                                    NULL,
                                    &headerSize,
                                    &streamStartTs));

    // Store the index of the new stream
    pKinesisVideoStream->newSessionIndex = pViewItem->index;

    // Store the timestamp of the new stream
    pKinesisVideoStream->newSessionTimestamp = streamStartTs;

    // Early termination if the item is already has a stream start indicator.
    CHK(!CHECK_ITEM_STREAM_START(pViewItem->flags), retStatus);

    // Get the existing frame allocation
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, pViewItem->handle, (PVOID*) &pFrame, &allocSize));
    CHK(allocSize < MAX_UINT32, STATUS_INVALID_ALLOCATION_SIZE);
    packagedSize = (UINT32) allocSize;
    CHK(pFrame != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Allocate storage for the frame
    dataOffset = GET_ITEM_DATA_OFFSET(pViewItem->flags);
    overallSize = packagedSize + headerSize;
    CHK_STATUS(heapAlloc(pKinesisVideoClient->pHeap, overallSize, &allocationHandle));

    // Ensure we have space and if not then bail
    CHK(IS_VALID_ALLOCATION_HANDLE(allocationHandle), STATUS_STORE_OUT_OF_MEMORY);

    // Map the storage
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, allocationHandle, (PVOID*) &pAlloc, &allocSize));
    overallSize = (UINT32) allocSize;

    // Actually package the bits in the storage
    CHK_STATUS(mkvgenGenerateHeader(pKinesisVideoStream->pMkvGenerator,
                                    pAlloc,
                                    &headerSize,
                                    &streamStartTs));

    // Copy the rest of the packaged frame
    MEMCPY(pAlloc + headerSize, pFrame, packagedSize);

    // Set the old allocation handle to be freed
    oldAllocationHandle = pViewItem->handle;
    pViewItem->handle = allocationHandle;
    SET_ITEM_STREAM_START(pViewItem->flags);
    SET_ITEM_DATA_OFFSET(pViewItem->flags, headerSize + dataOffset);
    pViewItem->length = overallSize;

    // Set the handle that will need to be freed on exit - now we should free the old one
    allocationHandle = oldAllocationHandle;

    // Unlock the client
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                       pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    // Unmap the old mapping
    if (pFrame != NULL) {
        heapUnmap(pKinesisVideoClient->pHeap, (PVOID) pFrame);
    }

    // Unmap the new mapping
    if (pAlloc != NULL) {
        heapUnmap(pKinesisVideoClient->pHeap, (PVOID) pAlloc);
    }

    // Clear up the previous allocation handle
    if (IS_VALID_ALLOCATION_HANDLE(allocationHandle)) {
        // Lock the client if it's not locked
        if (!clientLocked) {
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
            clientLocked = TRUE;
        }

        heapFree(pKinesisVideoClient->pHeap, allocationHandle);
    }

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

STATUS resetCurrentViewItemStreamStart(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PViewItem pViewItem = NULL;
    BOOL streamLocked = FALSE, clientLocked = FALSE;
    UINT32 clusterHeaderSize, packagedSize, overallSize, dataOffset;
    UINT64 allocSize;
    PBYTE pAlloc = NULL, pFrame = NULL;
    PKinesisVideoClient pKinesisVideoClient;
    ALLOCATION_HANDLE allocationHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    ALLOCATION_HANDLE oldAllocationHandle;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Quick check if we need to do anything by checking the current view items allocation handle
    // and whether it has a stream start indicator. Early exit if it is.
    CHK(IS_VALID_ALLOCATION_HANDLE(pKinesisVideoStream->curViewItem.viewItem.handle)
        && CHECK_ITEM_STREAM_START(pKinesisVideoStream->curViewItem.viewItem.flags), retStatus);

    // Get the view item corresponding to the current item
    CHK_STATUS(contentViewGetItemAt(pKinesisVideoStream->pView, pKinesisVideoStream->curViewItem.viewItem.index, &pViewItem));

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the required size for the cluster header
    // NOTE: Stream start is only on a cluster boundary
    CHK_STATUS(mkvgenGetMkvOverheadSize(pKinesisVideoStream->pMkvGenerator,
                                        MKV_STATE_START_CLUSTER,
                                        &clusterHeaderSize));

    // Get the existing frame allocation
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, pViewItem->handle, (PVOID*) &pFrame, &allocSize));
    CHK(allocSize < MAX_UINT32, STATUS_INVALID_ALLOCATION_SIZE);
    packagedSize = (UINT32) allocSize;
    CHK(pFrame != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Allocate storage for the frame
    dataOffset = GET_ITEM_DATA_OFFSET(pViewItem->flags);
    overallSize = packagedSize - dataOffset + clusterHeaderSize;
    CHK_STATUS(heapAlloc(pKinesisVideoClient->pHeap, overallSize, &allocationHandle));

    // Ensure we have space and if not then bail
    CHK(IS_VALID_ALLOCATION_HANDLE(allocationHandle), STATUS_STORE_OUT_OF_MEMORY);

    // Map the storage
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, allocationHandle, (PVOID*) &pAlloc, &allocSize));
    overallSize = (UINT32) allocSize;

    // Copy the rest of the packaged frame
    MEMCPY(pAlloc, pFrame + dataOffset - clusterHeaderSize, overallSize);

    // Set the old allocation handle to be freed
    oldAllocationHandle = pViewItem->handle;
    pViewItem->handle = allocationHandle;
    CLEAR_ITEM_STREAM_START(pViewItem->flags);
    SET_ITEM_DATA_OFFSET(pViewItem->flags, clusterHeaderSize);

    // Check if we need to reset the current view offset.
    // This should only happen if we have consumed the item and the offset
    // is equal to the length. We need to reset the offset so the logic
    // in the getStreamData catches the next view item.
    if (pViewItem->length == pKinesisVideoStream->curViewItem.offset) {
        pKinesisVideoStream->curViewItem.offset = overallSize;
    }

    // Set the new length
    pViewItem->length = overallSize;

    // Set the handle that will need to be freed on exit - now we should free the old one
    allocationHandle = oldAllocationHandle;

    // Unlock the client
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                       pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Re-set back the current
    pKinesisVideoStream->curViewItem.viewItem = *pViewItem;

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

    // Unmap the old mapping
    if (pFrame != NULL) {
        heapUnmap(pKinesisVideoClient->pHeap, (PVOID) pFrame);
    }

    // Unmap the new mapping
    if (pAlloc != NULL) {
        heapUnmap(pKinesisVideoClient->pHeap, (PVOID) pAlloc);
    }

    // Clear up the previous allocation handle
    if (IS_VALID_ALLOCATION_HANDLE(allocationHandle)) {
        // Lock the client if it's not locked
        if (!clientLocked) {
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
            clientLocked = TRUE;
        }

        heapFree(pKinesisVideoClient->pHeap, allocationHandle);
    }

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Finds the next boundary view item. This is needed so we can skip the non-I frames.
 * This is needed for the case when the current item falls off the view window and gets collected.
 */
STATUS getNextBoundaryViewItem(PKinesisVideoStream pKinesisVideoStream, PViewItem* ppViewItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PViewItem pViewItem = NULL;
    BOOL iterate = TRUE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && ppViewItem != NULL, STATUS_NULL_ARG);

    // Set the result to null first
    *ppViewItem = NULL;

    // Iterate until we find a key frame item.
    while (iterate) {
        CHK_STATUS(contentViewGetNext(pKinesisVideoStream->pView, &pViewItem));
        if ((CHECK_ITEM_FRAGMENT_START(pViewItem->flags)) ||
            CHECK_ITEM_FRAGMENT_END(pViewItem->flags)) {
            iterate = FALSE;
        }
    }

    // Assign the return value
    *ppViewItem = pViewItem;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the upload handle info corresponding to the specified handle and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfo(PKinesisVideoStream pKinesisVideoStream, UPLOAD_HANDLE uploadHandle) {
    STATUS retStatus = STATUS_SUCCESS;
    StackQueueIterator iterator;
    PUploadHandleInfo pUploadHandleInfo = NULL, pCurHandleInfo;
    UINT64 data;

    // Iterate linearly and find the first ready state handle
    CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pUploadInfoQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

        pCurHandleInfo = (PUploadHandleInfo) data;
        CHK(pCurHandleInfo != NULL, STATUS_INTERNAL_ERROR);
        if (pCurHandleInfo->handle == uploadHandle) {
            pUploadHandleInfo = pCurHandleInfo;

            // Found the item - early exit
            CHK(FALSE, retStatus);
        }

        CHK_STATUS(stackQueueIteratorNext(&iterator));
    }

CleanUp:
    return pUploadHandleInfo;
}

/**
 * Gets the first upload handle corresponding to the specified state
 */
PUploadHandleInfo getStreamUploadInfoWithState(PKinesisVideoStream pKinesisVideoStream, UINT32 handleState) {
    STATUS retStatus = STATUS_SUCCESS;
    StackQueueIterator iterator;
    PUploadHandleInfo pUploadHandleInfo = NULL, pCurHandleInfo;
    UINT64 data;

    // Iterate linearly and find the first ready state handle
    CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pUploadInfoQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

        pCurHandleInfo = (PUploadHandleInfo) data;
        CHK(pCurHandleInfo != NULL, STATUS_INTERNAL_ERROR);
        if ((handleState & pCurHandleInfo->state) != UPLOAD_HANDLE_STATE_NONE) {
            pUploadHandleInfo = pCurHandleInfo;

            // Found the item - early exit
            CHK(FALSE, retStatus);
        }

        CHK_STATUS(stackQueueIteratorNext(&iterator));
    }

CleanUp:

    return pUploadHandleInfo;
}

/**
 * Peeks the current stream upload info which is not in a terminated state
 */
PUploadHandleInfo getCurrentStreamUploadInfo(PKinesisVideoStream pKinesisVideoStream) {
    return getStreamUploadInfoWithState(pKinesisVideoStream, UPLOAD_HANDLE_STATE_NEW |
            UPLOAD_HANDLE_STATE_READY |
            UPLOAD_HANDLE_STATE_STREAMING |
            UPLOAD_HANDLE_STATE_TERMINATING |
            UPLOAD_HANDLE_STATE_AWAITING_ACK);
}

/**
 * Peeks the earliest stream upload info in an awaiting ACK received state.
 */
PUploadHandleInfo getAckReceivedStreamUploadInfo(PKinesisVideoStream pKinesisVideoStream) {
    return getStreamUploadInfoWithState(pKinesisVideoStream, UPLOAD_HANDLE_STATE_ACK_RECEIVED);
}

/**
 * Deletes and frees the specified stream upload info object
 */
VOID deleteStreamUploadInfo(PKinesisVideoStream pKinesisVideoStream, PUploadHandleInfo pUploadHandleInfo) {
    if (NULL != pUploadHandleInfo) {
        stackQueueRemoveItem(pKinesisVideoStream->pUploadInfoQueue, (UINT64) pUploadHandleInfo);
        MEMFREE(pUploadHandleInfo);
    }
}

/**
 * Calculates the max number of items for the view.
 *
 * This function will calculate either for frames or fragments
 *
 */
UINT32 calculateViewItemCount(PStreamInfo pStreamInfo)
{
    UINT32 viewItemCount = 0;
    switch (pStreamInfo->streamCaps.streamingType)
    {
        case STREAMING_TYPE_REALTIME:
        case STREAMING_TYPE_OFFLINE:
            // Calculate the number of frames for the duration of the buffer.
            viewItemCount = pStreamInfo->streamCaps.frameRate * ((UINT32)(pStreamInfo->streamCaps.bufferDuration / HUNDREDS_OF_NANOS_IN_A_SECOND));
            break;

        case STREAMING_TYPE_NEAR_REALTIME:
            // Calculate the number of the fragments in the buffer.
            viewItemCount = (UINT32)(pStreamInfo->streamCaps.bufferDuration / pStreamInfo->streamCaps.fragmentDuration);
            break;
    }

    return viewItemCount;
}

/**
 * Calculates the buffer duration
 *
 * This function will calculate the buffer duration based on a few criterias passed in
 *
 */
UINT64 calculateViewBufferDuration(PStreamInfo pStreamInfo)
{
    // Get the max of the buffer duration and the replay duration
    UINT64 bufferDuration = MAX(pStreamInfo->streamCaps.bufferDuration, pStreamInfo->streamCaps.replayDuration);

    // If it's still 0 then use the default
    if (pStreamInfo->streamCaps.bufferDuration <= MIN_CONTENT_VIEW_BUFFER_DURATION) {
        bufferDuration = MIN_VIEW_BUFFER_DURATION;
    }

    return bufferDuration;
}

/**
 * Frees the MetadataTracker object if allocated. Resets the values.
 */
VOID freeMetadataTracker(PMetadataTracker pMetadataTracker)
{
    if (pMetadataTracker != NULL && pMetadataTracker->data != NULL) {
        // Free the CPD and set it to NULL
        MEMFREE(pMetadataTracker->data);
        pMetadataTracker->data = NULL;
        pMetadataTracker->send = FALSE;
        pMetadataTracker->offset = 0;
        pMetadataTracker->size = 0;
    }
}

STATUS createPackager(PKinesisVideoStream pKinesisVideoStream, PMkvGenerator* ppGenerator)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 mkvGenFlags = MKV_GEN_FLAG_NONE |
            (pKinesisVideoStream->streamInfo.streamCaps.keyFrameFragmentation ? MKV_GEN_KEY_FRAME_PROCESSING : MKV_GEN_FLAG_NONE) |
            (pKinesisVideoStream->streamInfo.streamCaps.frameTimecodes ? MKV_GEN_IN_STREAM_TIME : MKV_GEN_FLAG_NONE) |
            (pKinesisVideoStream->streamInfo.streamCaps.absoluteFragmentTimes ? MKV_GEN_ABSOLUTE_CLUSTER_TIME : MKV_GEN_FLAG_NONE);

    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Apply the NAL adaptation flags.
    // NOTE: The flag values are the same as defined in the mkvgen
    mkvGenFlags |= pKinesisVideoStream->streamInfo.streamCaps.nalAdaptationFlags;

    // Create the packager
    CHK_STATUS(createMkvGenerator(pKinesisVideoStream->streamInfo.streamCaps.contentType,
                                  mkvGenFlags,
                                  pKinesisVideoStream->streamInfo.streamCaps.timecodeScale,
                                  pKinesisVideoStream->streamInfo.streamCaps.fragmentDuration,
                                  pKinesisVideoStream->streamInfo.streamCaps.segmentUuid,
                                  pKinesisVideoStream->streamInfo.streamCaps.trackInfoList,
                                  pKinesisVideoStream->streamInfo.streamCaps.trackInfoCount,
                                  pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                                  pKinesisVideoClient->clientCallbacks.customData,
                                  ppGenerator));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS streamFragmentBufferingAck(PKinesisVideoStream pKinesisVideoStream, UINT64 timestamp)
{
    ENTERS();
    PViewItem pCurItem;
    STATUS retStatus = STATUS_SUCCESS;

    // The state and the params are validated.
    // We will store the buffering ACK for the given view item
    // for the staleness detection and latency calculations.

    // Get the fragment start frame.
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, TRUE, &pCurItem));

    // Set the buffering ACK
    SET_ITEM_BUFFERING_ACK(pCurItem->flags);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS streamFragmentReceivedAck(PKinesisVideoStream pKinesisVideoStream, UINT64 timestamp)
{
    ENTERS();
    PViewItem pCurItem;
    STATUS retStatus = STATUS_SUCCESS;

    // The state and the params are validated.
    // We will store the buffering ACK for the given view item
    // for the staleness detection and latency calculations.

    // Get the fragment start frame.
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, TRUE, &pCurItem));

    // Set the received ACK
    SET_ITEM_RECEIVED_ACK(pCurItem->flags);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS streamFragmentPersistedAck(PKinesisVideoStream pKinesisVideoStream, UINT64 timestamp, PUploadHandleInfo pUploadHandleInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, setViewStatus = STATUS_SUCCESS;
    PViewItem pCurItem;
    UINT64 curItemIndex, duration, viewByteSize, boundaryItemIndex, data;
    BOOL setCurrentBack = FALSE, trimTail = TRUE, getNextBoundaryItem = TRUE;
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    StackQueueIterator iterator;
    PUploadHandleInfo pCurHandleInfo;

    // The state and the params are validated.
    // We need to find the next fragment to the persistent one and
    // trim the window to that fragment - i.e. move the tail position to the fragment.
    // As we move the tail, the callbacks will be fired to process the items falling out of the window.

    // Update last persistedAck timestamp
    pUploadHandleInfo->lastPersistedAckTs = timestamp;

    // Get the fragment start frame.
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, TRUE, &pCurItem));
    SET_ITEM_PERSISTED_ACK(pCurItem->flags);

    // Iterate linearly and find the first ready state handle
    CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pUploadInfoQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

        pCurHandleInfo = (PUploadHandleInfo) data;
        CHK(pCurHandleInfo != NULL, STATUS_INTERNAL_ERROR);
        if (pCurHandleInfo->handle == pUploadHandleInfo->handle) {
            break;
        }
        if (!IS_UPLOAD_HANDLE_READY_TO_TRIM(pCurHandleInfo)) {
            // got a earlier handle that hasn't finished yet. Therefore cannot trim tail.
            trimTail = FALSE;
            break;
        }

        CHK_STATUS(stackQueueIteratorNext(&iterator));
    }

    // Check if in view and when a persisted ack for upload handle n arrives, we dont trim off upload handle
    // n-1 until upload handle n-1 has received its last persisted ack. If handle n-1 is in
    // UPLOAD_HANDLE_STATE_ACK_RECEIVED or UPLOAD_HANDLE_STATE_TERMINATED state, then it has received the last
    // ack and is safe to trim off.
    CHK(trimTail, retStatus);

    // Remember the current index
    CHK_STATUS(contentViewGetCurrentIndex(pKinesisVideoStream->pView, &curItemIndex));

    // Set the current to the first frame of the ACKed fragments next
    CHK_STATUS(contentViewSetCurrentIndex(pKinesisVideoStream->pView, pCurItem->index + 1));
    setCurrentBack = TRUE;

    // Find the next boundary item which will indicate the start of the next fragment.
    // NOTE: This might fail if we are still assembling a fragment and the ACK is for the previous fragment.
    retStatus = getNextBoundaryViewItem(pKinesisVideoStream, &pCurItem);

    // Skip over already persisted fragments.
    while (STATUS_SUCCEEDED(retStatus) && getNextBoundaryItem) {
        if (CHECK_ITEM_FRAGMENT_START(pCurItem->flags) && CHECK_ITEM_PERSISTED_ACK(pCurItem->flags)) {
            retStatus = getNextBoundaryViewItem(pKinesisVideoStream, &pCurItem);
        } else {
            getNextBoundaryItem = FALSE;
        }
    }

    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS, retStatus);

    // Check if we need to process the awaiting upload info
    if (WAIT_FOR_PERSISTED_ACK(pKinesisVideoStream) &&
        pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_AWAITING_ACK &&
        timestamp == pUploadHandleInfo->lastFragmentTs) {

        // Reset the state to ACK received
        pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_ACK_RECEIVED;

        // Get the available duration and size to send
        CHK_STATUS(getAvailableViewSize(pKinesisVideoStream, &duration, &viewByteSize));

        // Notify the awaiting handle to enable it
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                pKinesisVideoStream->streamInfo.name,
                pUploadHandleInfo->handle,
                duration,
                viewByteSize));
    }

    // Reset the status and early exit in case we have no more items which means the tail is current
    if (retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS) {
        retStatus = STATUS_SUCCESS;
        CHK(FALSE, retStatus);
    }

    boundaryItemIndex = pCurItem->index;

    // If the boundary item is END_OF_FRAGMENT, then it should be trimmed too.
    if (CHECK_ITEM_FRAGMENT_END(pCurItem->flags)) {
        boundaryItemIndex++;
    }

    // Trim the tail
    CHK_STATUS(contentViewTrimTail(pKinesisVideoStream->pView, boundaryItemIndex));

    // Notify in case of an OFFLINE stream since tail has been trimmed
    if (IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType)) {
        pKinesisVideoClient->clientCallbacks.signalConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                       pKinesisVideoStream->bufferAvailabilityCondition);
    }
CleanUp:

    // Set the current back if we had modified it
    if (setCurrentBack
        && STATUS_FAILED((setViewStatus = contentViewSetCurrentIndex(pKinesisVideoStream->pView, curItemIndex)))) {
        DLOGW("Failed to set the current back to index %" PRIu64 " with status 0x%08x", curItemIndex, setViewStatus);
    }

    LEAVES();
    return retStatus;
}

STATUS streamFragmentErrorAck(PKinesisVideoStream pKinesisVideoStream, UINT64 timestamp, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    STATUS errStatus;
    PViewItem pCurItem;
    PUploadHandleInfo pUploadHandleInfo;
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    UPLOAD_HANDLE uploadHandle = INVALID_UPLOAD_HANDLE_VALUE;

    // The state and the params are validated.
    // Set the latest to the timestamp of the failed fragment for re-transmission
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, TRUE, &pCurItem));
    CHK_STATUS(contentViewSetCurrentIndex(pKinesisVideoStream->pView, pCurItem->index));

    // Trigger the stream termination
    // IMPORTANT!!! Currently, all Error ACKs will terminate the connection from the Backend.
    // We will proactively terminate the connection as the higher-level clients like CURL
    // might not terminate the connection as they are still streaming.
    pUploadHandleInfo = getCurrentStreamUploadInfo(pKinesisVideoStream);
    if (NULL != pUploadHandleInfo) {
        // Store the upload handle
        uploadHandle = pUploadHandleInfo->handle;
        CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, pUploadHandleInfo->handle, callResult));
    }

    // As we have an error ACK this also means that the inlet host has terminated the connection.
    // We will indicate the stream termination to the client
    if (pKinesisVideoClient->clientCallbacks.streamErrorReportFn != NULL) {
        // Call the stream error notification callback
        errStatus = serviceCallResultCheck(callResult);
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamErrorReportFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                uploadHandle,
                pCurItem->timestamp,
                errStatus));
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS checkForConnectionStaleness(PKinesisVideoStream pKinesisVideoStream, PViewItem pCurView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PViewItem pViewItem = pCurView;
    UINT64 curIndex = 0;
    UINT64 lastAckDuration = 0;

    // Check if we need to do anything
    CHK(pKinesisVideoStream->streamInfo.streamCaps.connectionStalenessDuration != CONNECTION_STALENESS_DETECTION_SENTINEL &&
        pKinesisVideoStream->streamInfo.streamCaps.fragmentAcks &&
        pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.streamConnectionStaleFn != NULL,
        retStatus);

    // pViewItem should point to the last view item considered. We need to see how far back is the last
    // buffering ACK and if its more than a defined threshold then notify the app.
    curIndex = pViewItem->index;
    while (TRUE) {
        CHK_STATUS(contentViewGetItemAt(pKinesisVideoStream->pView, curIndex, &pViewItem));

        // See if we are within the range - early exit if we are OK
        CHK(!CHECK_ITEM_BUFFERING_ACK(pViewItem->flags), retStatus);

        // See if we are over the threshold
        lastAckDuration = pCurView->timestamp - pViewItem->timestamp;
        if (lastAckDuration > pKinesisVideoStream->streamInfo.streamCaps.connectionStalenessDuration) {
            // Ok, we are above the threshold - call the callback
            CHK_STATUS(pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.streamConnectionStaleFn(
                    pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.customData,
                    TO_STREAM_HANDLE(pKinesisVideoStream),
                    lastAckDuration));
            break;
        }

        curIndex--;
    }

CleanUp:

    // If we go past the tail then we ignore the value of the error
    if (retStatus == STATUS_CONTENT_VIEW_INVALID_INDEX) {
        // Fix-up the error code.
        retStatus = STATUS_SUCCESS;
    }

    LEAVES();
    return retStatus;
}

STATUS checkStreamingTokenExpiration(PKinesisVideoStream pKinesisVideoStream) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;

    // Check if we need to do anything and early exit
    // If we are already in a grace period then bail out
    CHK(!pKinesisVideoStream->gracePeriod, retStatus);

    // Get the current time so we can check whether we are in a grace period and need to rotate the token
    currentTime = pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(
            pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.customData);

    CHK(currentTime >= pKinesisVideoStream->streamingAuthInfo.expiration ||
        pKinesisVideoStream->streamingAuthInfo.expiration - currentTime <= STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD,
        retStatus);

    // We are in a grace period - need to initiate the streaming token rotation.
    // We will have to transact to the get streaming endpoint state.

    // Set the streaming mode to stopped to trigger the transitions.
    // Set the result that will move the state machinery to the get endpoint state
    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, INVALID_UPLOAD_HANDLE_VALUE, SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD));

    // Set the grace period which will be reset when the new token is in place.
    pKinesisVideoStream->gracePeriod = TRUE;

    if (IS_OFFLINE_STREAMING_MODE(pKinesisVideoStream->streamInfo.streamCaps.streamingType)) {
        // Set an indicator to reset the generator on the next key frame only when we are in the stopped state
        pKinesisVideoStream->resetGeneratorOnKeyFrame = TRUE;
    } else {
        // For REALTIME mode, reset generator after delay.
        pKinesisVideoStream->resetGeneratorTime = currentTime + STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Converts the stream to a stream handle
 */
STREAM_HANDLE toStreamHandle(PKinesisVideoStream pKinesisVideoStream) {
    if (pKinesisVideoStream == NULL) {
        return INVALID_STREAM_HANDLE_VALUE;
    } else {
        return (STREAM_HANDLE) &pKinesisVideoStream->pKinesisVideoClient->streams[pKinesisVideoStream->streamId];
    }
}

/**
 * Converts handle to a stream
 */
PKinesisVideoStream fromStreamHandle(STREAM_HANDLE streamHandle) {
    if (streamHandle == INVALID_STREAM_HANDLE_VALUE) {
        return NULL;
    } else {
        return *((PKinesisVideoStream*) streamHandle);
    }
}

/**
 * Packages the stream metadata.
 */
STATUS packageStreamMetadata(PKinesisVideoStream pKinesisVideoStream, MKV_STREAM_STATE generatorState, BOOL notSentOnly, PBYTE pBuffer, PUINT32 pSize) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 packagedSize = 0, metadataCount = 0, metadataSize = 0, overallSize = 0, i;
    UINT64 item;
    PSerializedMetadata pSerializedMetadata = NULL;
    BOOL firstTimeCheck = TRUE;

    // NOTE: Assuming the locking is already done.
    // We will calculate the size and return the size only if buffer is NULL
    CHK(pKinesisVideoStream != NULL && pSize != NULL, STATUS_NULL_ARG);

    // Quick check for the state.
    CHK(generatorState == MKV_STATE_START_STREAM || generatorState == MKV_STATE_START_CLUSTER, STATUS_INVALID_OPERATION);

    // Get the count of metadata
    CHK_STATUS(stackQueueGetCount(pKinesisVideoStream->pMetadataQueue, &metadataCount));

    // Iterate to get the required size if we only need the size
    if (pBuffer == NULL) {
        for (i = 0; i < metadataCount; i++) {
            // Get the item at the index which will be the raw pointer to the serialized metadata
            CHK_STATUS(stackQueueGetAt(pKinesisVideoStream->pMetadataQueue, i, &item));

            pSerializedMetadata = (PSerializedMetadata) item;
            CHK(pSerializedMetadata != NULL, STATUS_INTERNAL_ERROR);

            if (!(pSerializedMetadata->applied && notSentOnly)) {
                // Special processing for the first element in case of the stream start
                // as we had packaged the metadata in the previous session and not included
                // the MKV EBML header which will be required for the new session.
                if (firstTimeCheck && generatorState == MKV_STATE_START_STREAM) {
                    firstTimeCheck = FALSE;
                    // Need to re-calculate the packaged metadata size again
                    CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                                 NULL,
                                                 pSerializedMetadata->name,
                                                 pSerializedMetadata->value,
                                                 &metadataSize));
                    packagedSize += metadataSize;
                } else {
                    // Simply add the stored size from the packaged metadata.
                    packagedSize += pSerializedMetadata->packagedSize;
                }
            }
        }

        // Early return
        CHK(FALSE, retStatus);
    }

    // Start with the full buffer
    overallSize = *pSize;

    // Package the metadata
    while (metadataCount-- != 0) {
        // Dequeue the current item
        CHK_STATUS(stackQueueDequeue(pKinesisVideoStream->pMetadataQueue, &item));

        // Package the metadata and delete the allocation
        pSerializedMetadata = (PSerializedMetadata) item;
        if (!(pSerializedMetadata->applied && notSentOnly)) {
            metadataSize = overallSize;
            CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                         pBuffer + packagedSize,
                                         pSerializedMetadata->name,
                                         pSerializedMetadata->value,
                                         &metadataSize));

            // Account for the sizes
            packagedSize += metadataSize;
            overallSize -= metadataSize;

            // Mark as the metadata has been "applied"
            pSerializedMetadata->applied = TRUE;
        }

        // If we have a persistent metadata then enqueue it back
        if (pSerializedMetadata->persistent) {
            CHK_STATUS(stackQueueEnqueue(pKinesisVideoStream->pMetadataQueue, item));
        } else {
            // Delete the allocation otherwise
            MEMFREE(pSerializedMetadata);
        }
    }

CleanUp:

    if (STATUS_SUCCEEDED(retStatus)) {
        // Set the size and the state before return
        *pSize = packagedSize;
    }

    LEAVES();
    return retStatus;
}

/**
 * Generates and stores the EoS.
 *
 * We are pre-generating the EoS as a packaged EoFr metadata with known name and store it.
 * The EoS will be appended to the last frame to indicate the EoS to the backed.
 * The EoFr metadata will need to be packaged as a non-stream start, which means the MKV EBML header should
 * not be included. This and the fact that the metadata packaging mutates the state of the generator
 * requires us to generate a dummy metadata (to get the generator in the non start state), generate
 * the metadata and then reset the generator back.
 *
 */
STATUS generateEosMetadata(PKinesisVideoStream pKinesisVideoStream) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 size;
    BYTE tempBuff[MAX_PACKAGED_METADATA_LEN];

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // NOTE: We will generate it twice - the first time it will get the generator into TAG state
    size = MAX_PACKAGED_METADATA_LEN;
    CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                 tempBuff,
                                 (PCHAR) EOFR_METADATA_NAME,
                                 (PCHAR) "",
                                 &size));

    // Now we will store it
    size = MAX_PACKAGED_METADATA_LEN;
    CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                 tempBuff,
                                 (PCHAR) EOFR_METADATA_NAME,
                                 (PCHAR) "",
                                 &size));

    // Allocate enough storage and copy the data from the temp buffer
    CHK(NULL != (pKinesisVideoStream->eosTracker.data = (PBYTE) MEMALLOC(size)), STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pKinesisVideoStream->eosTracker.data, tempBuff, size);

    // Set the overall size
    pKinesisVideoStream->eosTracker.size = size;

CleanUp:

    // Reset the generator in any case
    if (pKinesisVideoStream != NULL) {
        // Whether we mutated the state or not, we are resetting the generator
        mkvgenResetGenerator(pKinesisVideoStream->pMkvGenerator);

        // Set the initial state
        pKinesisVideoStream->eosTracker.offset = 0;
        pKinesisVideoStream->eosTracker.send = FALSE;
    }

    LEAVES();
    return retStatus;
}

STATUS checkForNotSentMetadata(PKinesisVideoStream pKinesisVideoStream, PBOOL pNotSent) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    StackQueueIterator iterator;
    UINT64 data;
    PSerializedMetadata pSerializedMetadata = NULL;

    CHK(pKinesisVideoStream != NULL && pNotSent != NULL, STATUS_NULL_ARG);
    *pNotSent = FALSE;

    // We can't send any pending metadata if we have an EoFr being the last frame
    CHK(!pKinesisVideoStream->eofrFrame, retStatus);

    // Iterate over the metadata and see if we have any "not applied" metadata
    CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pMetadataQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

        pSerializedMetadata = (PSerializedMetadata) data;
        CHK(pSerializedMetadata != NULL, STATUS_INTERNAL_ERROR);

        // Check if the metadata has been "applied" which means it has been already packaged.
        // This could happen in case when the metadata has been added after the fragment start
        // and/or a persistent metadata value has been modified after the fragment start.
        if (!pSerializedMetadata->applied) {
            *pNotSent = TRUE;

            // Early return
            CHK(FALSE, retStatus);
        }

        CHK_STATUS(stackQueueIteratorNext(&iterator));
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS packageNotSentMetadata(PKinesisVideoStream pKinesisVideoStream) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    StackQueueIterator iterator;
    UINT64 data;
    UINT32 overallSize = 0, packagedSize = 0, allocSize = 0, metadataCount = 0;
    PBYTE pBuffer = NULL;
    PSerializedMetadata pSerializedMetadata = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // Get the item count first
    CHK_STATUS(stackQueueGetIterator(pKinesisVideoStream->pMetadataQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        CHK_STATUS(stackQueueIteratorGetItem(iterator, &data));

        pSerializedMetadata = (PSerializedMetadata) data;
        CHK(pSerializedMetadata != NULL, STATUS_INTERNAL_ERROR);

        // Check to see if the metadata has not been applied and account for the size.
        if (!pSerializedMetadata->applied) {
            allocSize += pSerializedMetadata->packagedSize;
        }

        CHK_STATUS(stackQueueIteratorNext(&iterator));
    }

    // Allocate the buffer for storing all the metadata
    CHK(NULL != (pBuffer = (PBYTE) MEMALLOC(allocSize)), STATUS_NOT_ENOUGH_MEMORY);

    // Iterate and package the metadata
    CHK_STATUS(stackQueueGetCount(pKinesisVideoStream->pMetadataQueue, &metadataCount));
    while (metadataCount-- != 0) {
        // Dequeue the current item
        stackQueueDequeue(pKinesisVideoStream->pMetadataQueue, &data);

        pSerializedMetadata = (PSerializedMetadata) data;
        CHK(pSerializedMetadata != NULL, STATUS_INTERNAL_ERROR);

        if (!pSerializedMetadata->applied) {
            packagedSize = allocSize;
            CHK_STATUS(mkvgenGenerateTag(pKinesisVideoStream->pMkvGenerator,
                                         pBuffer + overallSize,
                                         pSerializedMetadata->name,
                                         pSerializedMetadata->value,
                                         &packagedSize));

            allocSize -= packagedSize;
            overallSize += packagedSize;
        }

        // The item is the allocation so free it
        MEMFREE(pSerializedMetadata);
    }

    pKinesisVideoStream->metadataTracker.send = TRUE;
    pKinesisVideoStream->metadataTracker.size = overallSize;
    pKinesisVideoStream->metadataTracker.offset = 0;
    if (pKinesisVideoStream->metadataTracker.data != NULL) {
        MEMFREE(pKinesisVideoStream->metadataTracker.data);
    }

    pKinesisVideoStream->metadataTracker.data = pBuffer;

CleanUp:

    if (STATUS_FAILED(retStatus) && pBuffer != NULL) {
        MEMFREE(pBuffer);
    }

    LEAVES();
    return retStatus;
}

/**
 * Appends the already validated metadata to the metadata queue
 *
 * NOTE: The stream is assumed locked and the parameters have already been sanitized
 */
STATUS appendValidatedMetadata(PKinesisVideoStream pKinesisVideoStream, PCHAR name, PCHAR value, BOOL persistent, UINT32 packagedSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 metadataNameSize, metadataValueSize;
    PSerializedMetadata pSerializedMetadata = NULL;

    // Sizes are validated in the gen metadata call
    metadataNameSize = (UINT32) STRLEN(name);
    metadataValueSize = (UINT32) STRLEN(value);

    // Allocate and store the data in sized allocation.
    // NOTE: We add NULL terminator for both name and value
    pSerializedMetadata = (PSerializedMetadata) MEMALLOC((metadataNameSize + 1 + metadataValueSize + 1) * SIZEOF(CHAR) + SIZEOF(SerializedMetadata));
    CHK(pSerializedMetadata != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the overall packaged size
    pSerializedMetadata->packagedSize = packagedSize;

    // Set the metadata as not "applied". The overwritten value for the persistent metadata
    // will mean the metadata is not "applied" yet.
    pSerializedMetadata->applied = FALSE;

    // Set the name to point to the end of the structure
    pSerializedMetadata->name = (PCHAR) (pSerializedMetadata + 1);

    // Copy the name of the metadata
    STRCPY(pSerializedMetadata->name, name);

    // Add the null terminator
    pSerializedMetadata->name[metadataNameSize] = '\0';

    // Set the value to point after the end of start and the null terminator
    pSerializedMetadata->value = pSerializedMetadata->name + metadataNameSize + 1;

    // Copy the value of the metadata
    STRCPY(pSerializedMetadata->value, value);

    // Null terminate
    pSerializedMetadata->value[metadataValueSize] = '\0';

    // Set the persistent mode
    pSerializedMetadata->persistent = persistent;

    // Store the in the queue
    CHK_STATUS(stackQueueEnqueue(pKinesisVideoStream->pMetadataQueue, (UINT64) pSerializedMetadata));

CleanUp:

    // Free the allocation on error.
    if (STATUS_FAILED(retStatus) && pSerializedMetadata != NULL) {
        MEMFREE(pSerializedMetadata);
    }

    LEAVES();
    return retStatus;
}

STATUS getAvailableViewSize(PKinesisVideoStream pKinesisVideoStream, PUINT64 pDuration, PUINT64 pViewSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 duration = 0, viewByteSize = 0;

    // NOTE: Parameters are assumed to have been validated

    // Get the duration from current point to the head
    CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &duration, NULL));

    // Get the size of the allocation from current point to the head
    CHK_STATUS(contentViewGetWindowAllocationSize(pKinesisVideoStream->pView, &viewByteSize, NULL));

    // Account for the partially sent frame
    viewByteSize += pKinesisVideoStream->curViewItem.viewItem.length - pKinesisVideoStream->curViewItem.offset;

    // Account for the EOS and the metadata that hasn't been packaged
    if (pKinesisVideoStream->metadataTracker.send) {
        viewByteSize += pKinesisVideoStream->metadataTracker.size;
    }

    if (pKinesisVideoStream->eosTracker.send) {
        viewByteSize += pKinesisVideoStream->eosTracker.size;
    }

CleanUp:

    *pViewSize = viewByteSize;
    *pDuration = duration;

    LEAVES();
    return retStatus;
}
