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
    PMkvGenerator pMkvGenerator = NULL;
    PStateMachine pStateMachine = NULL;
    UINT32 allocationSize, maxViewItems, i;
    PCHAR pCurPnt = NULL;
    BOOL locked = FALSE;
    BOOL tearDownOnError = TRUE;
    CHAR tempStreamName[MAX_STREAM_NAME_LEN];

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
    } else {
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

    // Allocate the main struct
    // NOTE: The calloc will Zero the fields
    // We are allocating more than the structure size to accommodate for
    // the variable size buffers which will come at the end and for the tags
    allocationSize = SIZEOF(KinesisVideoStream) + pStreamInfo->streamCaps.codecPrivateDataSize + pStreamInfo->tagCount * TAG_FULL_LENGTH;
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

    // Set the client stream handles
    pKinesisVideoStream->streamHandle = INVALID_UPLOAD_HANDLE_VALUE;
    pKinesisVideoStream->newStreamHandle = INVALID_UPLOAD_HANDLE_VALUE;

    // Set the stream start timestamps
    pKinesisVideoStream->streamTimestamp.newStreamTimestamp = 0;
    pKinesisVideoStream->streamTimestamp.streamTimestampBuffering = 0;
    pKinesisVideoStream->streamTimestamp.streamTimestampReceived = 0;
    pKinesisVideoStream->streamTimestamp.streamTimestampPersisted = 0;
    pKinesisVideoStream->streamTimestamp.streamTimestampError = 0;

    // Not in a grace period
    pKinesisVideoStream->gracePeriod = FALSE;

    // Shouldn't reset the generator on next key frame
    pKinesisVideoStream->resetGeneratorOnKeyFrame = FALSE;

    // Set the reset item index to 0
    pKinesisVideoStream->resetViewItemIndex = 0;

    // No connections have been dropped as this is a new stream
    pKinesisVideoStream->connectionDropped = FALSE;

    // Set the initial diagnostics information from the defaults
    pKinesisVideoStream->diagnostics.currentFrameRate = pStreamInfo->streamCaps.frameRate;
    pKinesisVideoStream->diagnostics.currentTransferRate = pStreamInfo->streamCaps.avgBandwidthBps;
    pKinesisVideoStream->diagnostics.accumulatedByteCount = 0;
    pKinesisVideoStream->diagnostics.lastFrameRateTimestamp = pKinesisVideoStream->diagnostics.lastTransferRateTimestamp = 0;

    // Set the current view item
    pKinesisVideoStream->curViewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;
    pKinesisVideoStream->curViewItem.length = pKinesisVideoStream->curViewItem.offset = 0;

    // Copy the structures in their entirety
    MEMCPY(&pKinesisVideoStream->streamInfo, pStreamInfo, SIZEOF(StreamInfo));
    if (pKinesisVideoStream->streamInfo.streamCaps.codecPrivateDataSize != 0 &&
        pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData != NULL) {
        // Set the pointer to the end of the structure
        pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData = (PBYTE) (pKinesisVideoStream + 1);
        MEMCPY(pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData,
            pStreamInfo->streamCaps.codecPrivateData,
            pStreamInfo->streamCaps.codecPrivateDataSize);
    } else {
        pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData = NULL;
    }

    // Fix-up the stream name if not specified
    if (pKinesisVideoStream->streamInfo.name[0] == '\0') {
        STRCPY(pKinesisVideoStream->streamInfo.name, tempStreamName);
    }

    // Create the stream lock
    pKinesisVideoStream->base.lock = pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, TRUE);

    // Fix-up the buffer duration first
    pKinesisVideoStream->streamInfo.streamCaps.bufferDuration = calculateViewBufferDuration(&pKinesisVideoStream->streamInfo);

    // Set the tags pointer to point after the codec private data
    pKinesisVideoStream->streamInfo.tags = (PTag)((PBYTE) (pKinesisVideoStream + 1) + pKinesisVideoStream->streamInfo.streamCaps.codecPrivateDataSize);

    // Copy the tags if any. First, set the pointer to the end of the tags to iterate
    pCurPnt = (PCHAR) (pKinesisVideoStream->streamInfo.tags + pKinesisVideoStream->streamInfo.tagCount);
    for (i = 0; i < pKinesisVideoStream->streamInfo.tagCount; i++) {
        pKinesisVideoStream->streamInfo.tags[i].version = pStreamInfo->tags[i].version;
        // Fix-up the pointers first
        pKinesisVideoStream->streamInfo.tags[i].name = pCurPnt;
        pCurPnt += MAX_TAG_NAME_LEN;

        pKinesisVideoStream->streamInfo.tags[i].value = pCurPnt;
        pCurPnt += MAX_TAG_VALUE_LEN;

        // Copy the strings
        STRNCPY(pKinesisVideoStream->streamInfo.tags[i].name, pStreamInfo->tags[i].name, MAX_TAG_NAME_LEN);
        STRNCPY(pKinesisVideoStream->streamInfo.tags[i].value, pStreamInfo->tags[i].value, MAX_TAG_VALUE_LEN);
    }

    // Calculate the max items in the view
    maxViewItems = calculateViewItemCount(&pKinesisVideoStream->streamInfo);

    // Create the view
    CHK_STATUS(createContentView(maxViewItems,
                                 pKinesisVideoStream->streamInfo.streamCaps.bufferDuration,
                                 viewItemRemoved,
                                 (UINT64) pKinesisVideoStream,
                                 &pView));
    pKinesisVideoStream->pView = pView;

    // Create an MKV generator
    CHK_STATUS(createPackager(&pKinesisVideoStream->streamInfo,
                              pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                              pKinesisVideoClient->clientCallbacks.customData,
                              &pMkvGenerator));

    // Set the generator object
    pKinesisVideoStream->pMkvGenerator = pMkvGenerator;

    // Create the state machine
    CHK_STATUS(createStateMachine(STREAM_STATE_MACHINE_STATES,
                                  STREAM_STATE_MACHINE_STATE_COUNT,
                                  TO_CUSTOM_DATA(pKinesisVideoStream),
                                  pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                                  pKinesisVideoClient->clientCallbacks.customData,
                                  &pStateMachine));
    pKinesisVideoStream->base.pStateMachine = pStateMachine;

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

    // Free the codec private data if any
    freeCodecPrivateData(pKinesisVideoStream);

    // Lock the client to update the streams
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    // Remove from the parent object by setting the ref to NULL
    pKinesisVideoClient->streams[pKinesisVideoStream->streamId] = NULL;
    pKinesisVideoClient->streamCount--;

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

/**
 * Stops the stream
 */
STATUS stopStream(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient;
    UINT64 duration, viewByteSize;
    BOOL streamLocked = FALSE, clientLocked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // It's a no-op if stop has been called
    CHK(!pKinesisVideoStream->streamStopped, retStatus);

    // Set the stream end flag
    pKinesisVideoStream->streamStopped = TRUE;

    // Send an event to terminate the state machine
    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, SERVICE_CALL_RESULT_NOT_SET));

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the duration from current point to the head
    CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &duration, NULL));

    // Get the size of the allocation from current point to the head
    CHK_STATUS(contentViewGetWindowAllocationSize(pKinesisVideoStream->pView, &viewByteSize, NULL));

    // Pulse the data available callback so the callee will know of a state change
    if (pKinesisVideoClient->clientCallbacks.streamDataAvailableFn != NULL) {
        // Call the notification callback
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                pKinesisVideoStream->streamInfo.name,
                duration,
                viewByteSize + pKinesisVideoStream->curViewItem.length - pKinesisVideoStream->curViewItem.offset));
    }

    // We need to proactively call the EOS notification as the client
    // can be waiting for the notification on the data availability but we
    // no longer put frames into the stream.
    if (duration == 0 || viewByteSize == 0) {
        // Call the notification callback
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamClosedFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream)));
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
STATUS putFrame(PKinesisVideoStream pKinesisVideoStream, PFrame pFrame)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    ALLOCATION_HANDLE allocHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT64 remainingSize = 0, thresholdPercent = 0, duration = 0, viewByteSize = 0;
    PBYTE pAlloc = NULL;
    UINT32 allocSize = 0, packagedSize = 0;
    UINT32 itemFlags;
    BOOL streamLocked = FALSE, clientLocked = FALSE, freeOnError = TRUE, storeStreamTimestamp = FALSE;
    EncodedFrameInfo encodedFrameInfo;
    PViewItem pViewItem;
    UINT64 currentTime;
    DOUBLE frameRate, deltaInSeconds;

    CHK(pKinesisVideoStream != NULL && pFrame != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Check if the stream has been stopped
    CHK(!pKinesisVideoStream->streamStopped, STATUS_STREAM_HAS_BEEN_STOPPED);

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Check if we are in the right state only if we are not in a rotation state
    if (pKinesisVideoStream->streamState != STREAM_STATE_READY
        && pKinesisVideoStream->streamState != STREAM_STATE_ERROR) {
        CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                           STREAM_STATE_READY |
                                           STREAM_STATE_PUT_STREAM |
                                           STREAM_STATE_PUT_STREAM |
                                           STREAM_STATE_STREAMING |
                                           STREAM_STATE_GET_ENDPOINT |
                                           STREAM_STATE_GET_TOKEN |
                                           STREAM_STATE_STOPPED));
    }

    // Need to check whether the streaming token has expired.
    // If we have the streaming token and it's in the grace period
    // then we need to go back to the get streaming end point and
    // get the streaming end point and the new streaming token.
    CHK_STATUS(checkStreamingTokenExpiration(pKinesisVideoStream));

    // NOTE: If the connection has been reset we need to start from a new header
    if (pKinesisVideoStream->streamState == STREAM_STATE_NEW
        || pKinesisVideoStream->streamState == STREAM_STATE_STOPPED
        || pKinesisVideoStream->streamState == STREAM_STATE_ERROR) {

        // Set an indicator to reset the generator on the next key frame only when we are in the stopped state
        if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED && pKinesisVideoStream->gracePeriod) {
            pKinesisVideoStream->resetGeneratorOnKeyFrame = TRUE;
        }

        // Store the stream timestamp when in new state because the stream can have relative timestamps and
        // start from a non-zero timecode.
        if (pKinesisVideoStream->streamState == STREAM_STATE_NEW) {
            storeStreamTimestamp = TRUE;
        }

        // Step the state machine once to get out of the Ready state or to execute the token rotation
        CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));
    }

    // if we need to reset the generator on the next key frame (during the rotation only)
    if (pKinesisVideoStream->resetGeneratorOnKeyFrame && CHECK_FRAME_FLAG_KEY_FRAME(pFrame->flags)) {
        // Store the last stream timestamp before resetting the generator
        CHK_STATUS(mkvgenResetGenerator(pKinesisVideoStream->pMkvGenerator));
        pKinesisVideoStream->resetGeneratorOnKeyFrame = FALSE;
        storeStreamTimestamp = TRUE;
    }

    // Package and store the frame.

    // Currently, we are storing every frame in a separate allocation
    // We will need to optimize for storing perhaps an entire cluster but
    // Not right now as we need to deal with mapping/unmapping and allocating
    // the cluster storage up-front which might be more hassle than needed.

    // Get the size of the packaged frame
    CHK_STATUS(mkvgenPackageFrame(pKinesisVideoStream->pMkvGenerator,
                                  pFrame,
                                  NULL,
                                  &packagedSize,
                                  &encodedFrameInfo));

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Allocate storage for the frame
    CHK_STATUS(heapAlloc(pKinesisVideoClient->pHeap, packagedSize, &allocHandle));

    // Ensure we have space and if not then bail
    CHK(IS_VALID_ALLOCATION_HANDLE(allocHandle), STATUS_STORE_OUT_OF_MEMORY);

    // Map the storage
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, allocHandle, (PVOID*) &pAlloc, &allocSize));

    // Validate we had allocated enough storage just in case
    CHK(packagedSize <= allocSize, STATUS_ALLOCATION_SIZE_SMALLER_THAN_REQUESTED);

    // Actually package the bits in the storage
    CHK_STATUS(mkvgenPackageFrame(pKinesisVideoStream->pMkvGenerator,
                                  pFrame,
                                  pAlloc,
                                  &packagedSize,
                                  &encodedFrameInfo));

    // Store the stream start timestamp for ACK timecode adjustment for relative cluster timecode streams
    if (storeStreamTimestamp) {
        pKinesisVideoStream->streamTimestamp.newStreamTimestamp = encodedFrameInfo.streamStartTs;
    }

    // Unmap the storage for the frame
    CHK_STATUS(heapUnmap(pKinesisVideoClient->pHeap, ((PVOID) pAlloc)));

    // Check for storage pressures
    remainingSize = pKinesisVideoClient->pHeap->heapLimit - pKinesisVideoClient->pHeap->heapSize;
    thresholdPercent = (UINT32) (((DOUBLE) remainingSize / pKinesisVideoClient->pHeap->heapLimit) * 100);

    if (thresholdPercent <= STORAGE_PRESSURE_NOTIFICATION_THRESHOLD &&
            pKinesisVideoClient->clientCallbacks.storageOverflowPressureFn != NULL) {
        // Notify the client app about buffer pressure
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.storageOverflowPressureFn(
                pKinesisVideoClient->clientCallbacks.customData,
                remainingSize));
    }

    // Generate the view flags
    itemFlags = ITEM_FLAG_NONE;
    switch (encodedFrameInfo.streamState) {
        case MKV_STATE_START_STREAM:
            SET_ITEM_STREAM_START(itemFlags);
            // fall-through
        case MKV_STATE_START_CLUSTER:
            SET_ITEM_FRAGMENT_START(itemFlags);
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
    CHK_STATUS(contentViewAddItem(pKinesisVideoStream->pView,
                                  encodedFrameInfo.clusterTs + encodedFrameInfo.frameDts,
                                  pFrame->duration,
                                  allocHandle,
                                  0,
                                  packagedSize,
                                  itemFlags));

    // If it's a new header then preserve the item so we can reset the stream
    if (encodedFrameInfo.streamState == MKV_STATE_START_STREAM && storeStreamTimestamp) {
        CHK_STATUS(contentViewGetHead(pKinesisVideoStream->pView, &pViewItem));
        pKinesisVideoStream->resetViewItemIndex = pViewItem->index;
    }

    // From now on we don't need to free the allocation as it's in the view already and will be collected
    freeOnError = FALSE;

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
    if (pKinesisVideoClient->clientCallbacks.streamDataAvailableFn != NULL) {
        // Get the duration from current point to the head
        CHK_STATUS(contentViewGetWindowDuration(pKinesisVideoStream->pView, &duration, NULL));

        // Get the size of the allocation from current point to the head
        CHK_STATUS(contentViewGetWindowAllocationSize(pKinesisVideoStream->pView, &viewByteSize, NULL));

        // Call the notification callback
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                pKinesisVideoStream->streamInfo.name,
                duration,
                viewByteSize));
    }

    // Recalculate framerate if enabled
    if (pKinesisVideoStream->streamInfo.streamCaps.recalculateMetrics) {
        // Calculate the current frame rate only after the first iteration
        currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
        if (!storeStreamTimestamp) {
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
 * @return STATUS of the operation
 */
STATUS getStreamData(PKinesisVideoStream pKinesisVideoStream, PUINT64 pClientStreamHandle, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pFillSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, stalenessCheckStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PViewItem pViewItem = NULL;
    PBYTE pAlloc = NULL;
    UINT32 size = 0, remainingSize = bufferSize;
    PBYTE pCurPnt = pBuffer;
    BOOL streamLocked = FALSE, clientLocked = FALSE, rollbackToLastAck, restarted = FALSE;
    UINT64 currentTime;
    DOUBLE transferRate, deltaInSeconds;

    CHK(pKinesisVideoStream != NULL &&
                pKinesisVideoStream->pKinesisVideoClient != NULL &&
                pBuffer != NULL &&
                pClientStreamHandle != NULL &&
                pFillSize != NULL,
        STATUS_NULL_ARG);
    CHK(bufferSize != 0, STATUS_INVALID_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // If the stream is not in stopped state and if the connection has been reset and we are not in grace period
    // then we need to rollback the current view pointer
    if (!pKinesisVideoStream->streamStopped
            && pKinesisVideoStream->connectionDropped
            && pKinesisVideoStream->connectionDroppedResult != SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD) {
        // Calculate the rollback duration based on the stream replay duration and the latest ACKs received.
        // If the acks are not enabled or we have potentially dead host situation then we rollback to the replay duration.
        // NOTE: The received ACK should be sequential and not out-of-order.
        rollbackToLastAck = pKinesisVideoStream->streamInfo.streamCaps.fragmentAcks &&
                            CONNECTION_DROPPED_HOST_ALIVE(pKinesisVideoStream->connectionDroppedResult);
        CHK_STATUS(contentViewRollbackCurrent(pKinesisVideoStream->pView,
                                              pKinesisVideoStream->streamInfo.streamCaps.replayDuration,
                                              TRUE,
                                              rollbackToLastAck));

        // Reset the connection dropped indicator
        pKinesisVideoStream->connectionDropped = FALSE;

        // Reset the current view item on connection reset
        MEMSET(&pKinesisVideoStream->curViewItem, 0x00, SIZEOF(ViewItem));
        pKinesisVideoStream->curViewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;

        // Fix-up the current element
        CHK_STATUS(streamStartFixupOnReconnect(pKinesisVideoStream));

        restarted = TRUE;
    }

    // Set the size first
    *pFillSize = 0;

    // If we haven't yet set the stream handle then set it and return it
    if (!IS_VALID_UPLOAD_HANDLE(pKinesisVideoStream->streamHandle)) {
        pKinesisVideoStream->streamHandle = pKinesisVideoStream->newStreamHandle;
    }

    *pClientStreamHandle = pKinesisVideoStream->streamHandle;

    // Continue filling the buffer from the point we left off
    do {
        pViewItem = &pKinesisVideoStream->curViewItem;

        // First and foremost, we need to check whether we have an allocation handle.
        // This could happen in case when we start getting the data out or
        // when the last time we got the data the item fell off the view window.
        if (!IS_VALID_ALLOCATION_HANDLE(pViewItem->handle)) {
            // Need to find the next key frame boundary in case of the current rolling out of the window
            CHK_STATUS(getNextBoundaryViewItem(pKinesisVideoStream, &pViewItem));

            if (pViewItem != NULL) {
                // Use this as a current for the next iteration
                pKinesisVideoStream->curViewItem = *pViewItem;
            } else {
                // Couldn't find any boundary items, default to empty
                pKinesisVideoStream->curViewItem.offset = pKinesisVideoStream->curViewItem.length = 0;
                pKinesisVideoStream->curViewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;

                // We don't have any items currently - early return
                CHK(FALSE, STATUS_NO_MORE_DATA_AVAILABLE);
            }
        } else if (pViewItem->offset == pViewItem->length) {
            // Second, we need to check whether the existing view item has been exhausted
            CHK_STATUS(contentViewGetNext(pKinesisVideoStream->pView, &pViewItem));

            // Store it in the current view
            pKinesisVideoStream->curViewItem = *pViewItem;

            // Check if we are finishing the previous stream and we have a boundary item
            if (pKinesisVideoStream->curViewItem.index == pKinesisVideoStream->resetViewItemIndex) {
                // Set the stream handle as we have exhausted the previous
                pKinesisVideoStream->streamHandle = pKinesisVideoStream->newStreamHandle;

                // The client should terminate and close the stream after finalizing any remaining transfer.
                CHK(FALSE, STATUS_END_OF_STREAM);
            }
        } else {
            // Now, we can stream enough data out if we don't have a zero item
            CHK(pViewItem->offset != pViewItem->length, STATUS_NO_MORE_DATA_AVAILABLE);

            // Lock the client
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
            clientLocked = TRUE;

            // Fill the rest of the buffer of the current view item first
            // Map the storage
            CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, pViewItem->handle, (PVOID *) &pAlloc, &size));

            // Validate we had allocated enough storage just in case
            CHK(pViewItem->length - pViewItem->offset <= size, STATUS_VIEW_ITEM_SIZE_GREATER_THAN_ALLOCATION);

            // Copy as much as we can
            size = MIN(remainingSize, pViewItem->length - pViewItem->offset);
            MEMCPY(pCurPnt, pAlloc + pViewItem->offset, size);

            // Unmap the storage for the frame
            CHK_STATUS(heapUnmap(pKinesisVideoClient->pHeap, ((PVOID) pAlloc)));

            // Unlock the client
            pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                         pKinesisVideoClient->base.lock);
            clientLocked = FALSE;

            // Set the values
            pViewItem->offset += size;
            pCurPnt += size;
            remainingSize -= size;
            *pFillSize += size;
        }
    } while (remainingSize != 0);

CleanUp:

    // Run staleness detection if we have ACKs enabled and if we have retrieved any data
    if (pFillSize != NULL && *pFillSize != 0) {
        stalenessCheckStatus = checkForConnectionStaleness(pKinesisVideoStream, pViewItem);
    }

    if (retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS) {
        // Replace it with a client side error
        retStatus = STATUS_NO_MORE_DATA_AVAILABLE;
    }

    // Calculate the metrics for transfer rate on successful call and when we do have some data.
    if (pKinesisVideoStream->streamInfo.streamCaps.recalculateMetrics &&
            (retStatus == STATUS_SUCCESS
             || retStatus == STATUS_NO_MORE_DATA_AVAILABLE
             || retStatus == STATUS_END_OF_STREAM)
        && pFillSize != NULL && *pFillSize != 0) {
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

    // Special handling for stopped stream and no more data available
    if (pKinesisVideoStream->streamStopped &&
            (retStatus == STATUS_NO_MORE_DATA_AVAILABLE || retStatus == STATUS_END_OF_STREAM)) {
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamClosedFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream)));
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
 * Stream format changed. Currently, codec private data only. Will return OK if nothing to be done.
 */
STATUS streamFormatChanged(PKinesisVideoStream pKinesisVideoStream, UINT32 codecPrivateDataSize, PBYTE codecPrivateData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);
    // These checks are borrowed from MKV. TODO: Consolidate these at some stage.
    CHK(codecPrivateDataSize <= MKV_MAX_CODEC_PRIVATE_LEN, STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH);
    CHK(codecPrivateDataSize == 0 || codecPrivateData != NULL, STATUS_MKV_CODEC_PRIVATE_NULL);

    // Ensure we are not in a streaming state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                            STREAM_STATE_READY |
                                            STREAM_STATE_NEW |
                                            STREAM_STATE_DESCRIBE |
                                            STREAM_STATE_CREATE |
                                            STREAM_STATE_GET_ENDPOINT |
                                            STREAM_STATE_GET_TOKEN |
                                            STREAM_STATE_STOPPED));

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Free the existing allocation if any.
    freeCodecPrivateData(pKinesisVideoStream);

    // Set the size first
    // IMPORTANT: The CPD size could be set to 0 to clear any previous CPD.
    pKinesisVideoStream->streamInfo.streamCaps.codecPrivateDataSize = codecPrivateDataSize;

    // Check if we need to do anything
    if (codecPrivateDataSize != 0) {
        pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData = (PBYTE) MEMALLOC(codecPrivateDataSize);
        CHK(pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData != NULL, STATUS_NOT_ENOUGH_MEMORY);

        // Copy the bits
        MEMCPY(pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData, codecPrivateData, codecPrivateDataSize);
    }

    // Need to free and re-create the packager
    if (pKinesisVideoStream->pMkvGenerator != NULL) {
        freeMkvGenerator(pKinesisVideoStream->pMkvGenerator);
    }

    CHK_STATUS(createPackager(&pKinesisVideoStream->streamInfo,
                              pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                              pKinesisVideoClient->clientCallbacks.customData,
                              &pKinesisVideoStream->pMkvGenerator));

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
    UINT32 curIndex;
    PViewItem pViewItem = NULL;
    BOOL streamLocked = FALSE, clientLocked = FALSE;
    UINT32 headerSize, packagedSize, overallSize;
    PBYTE pAlloc = NULL, pFrame = NULL;
    PKinesisVideoClient pKinesisVideoClient;
    ALLOCATION_HANDLE allocationHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    ALLOCATION_HANDLE oldAllocationHandle;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Get the current item
    CHK_STATUS(contentViewGetCurrentIndex(pKinesisVideoStream->pView, &curIndex));
    CHK_STATUS(contentViewGetItemAt(pKinesisVideoStream->pView, curIndex, &pViewItem));

    // Early termination if the item is already has a stream start indicator.
    CHK(!CHECK_ITEM_STREAM_START(pViewItem->flags), retStatus);

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    // Get the existing frame allocation
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, pViewItem->handle, (PVOID*) &pFrame, &packagedSize));
    CHK(pFrame != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Get the required size for the header
    CHK_STATUS(mkvgenGenerateHeader(pKinesisVideoStream->pMkvGenerator,
                                    NULL,
                                    &headerSize));

    // Allocate storage for the frame
    overallSize = packagedSize + headerSize;
    CHK_STATUS(heapAlloc(pKinesisVideoClient->pHeap, packagedSize + headerSize, &allocationHandle));

    // Ensure we have space and if not then bail
    CHK(IS_VALID_ALLOCATION_HANDLE(allocationHandle), STATUS_STORE_OUT_OF_MEMORY);

    // Map the storage
    CHK_STATUS(heapMap(pKinesisVideoClient->pHeap, allocationHandle, (PVOID*) &pAlloc, &overallSize));

    // Validate we had allocated enough storage just in case
    CHK(packagedSize + headerSize <= overallSize, STATUS_INTERNAL_ERROR);

    // Actually package the bits in the storage
    CHK_STATUS(mkvgenGenerateHeader(pKinesisVideoStream->pMkvGenerator,
                                    pAlloc,
                                    &headerSize));

    // Copy the rest of the packaged frame
    MEMCPY(pAlloc + headerSize, pFrame, packagedSize);

    // Unmap the storage for the frame
    CHK_STATUS(heapUnmap(pKinesisVideoClient->pHeap, ((PVOID) pAlloc)));

    // Set the old allocation handle to be freed
    oldAllocationHandle = pViewItem->handle;
    pViewItem->handle = allocationHandle;
    SET_ITEM_STREAM_START(pViewItem->flags);
    pViewItem->length = overallSize;
    pViewItem->offset = 0;
    allocationHandle = oldAllocationHandle;

    // Unlock the client
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData,
                                                 pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

    // Unlock the stream (even though it will be unlocked in the cleanup
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = FALSE;

CleanUp:

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
        if (CHECK_ITEM_FRAGMENT_START(pViewItem->flags)) {
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
            // Calculate the number of frames for the duration of the buffer.
            viewItemCount = pStreamInfo->streamCaps.frameRate * ((UINT32)(pStreamInfo->streamCaps.bufferDuration / HUNDREDS_OF_NANOS_IN_A_SECOND));
            break;

        case STREAMING_TYPE_NEAR_REALTIME:
        case STREAMING_TYPE_OFFLINE:
            // Calculate the number of the fragments in the buffer.
            viewItemCount = (UINT32)((pStreamInfo->streamCaps.bufferDuration / HUNDREDS_OF_NANOS_IN_A_SECOND) / pStreamInfo->streamCaps.fragmentDuration);
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
 * Frees the CPD allocation if previously allocated.
 *
 * IMPORTANT: The way we know the previously allocated CPD is by checking for NULL and whether it points
 * to the end of the structure or not.
 **/
VOID freeCodecPrivateData(PKinesisVideoStream pKinesisVideoStream)
{
    if (pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData != NULL &&
            pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData != (PBYTE)(pKinesisVideoStream + 1)) {

        // Free the CPD and set it to NULL
        MEMFREE(pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData);
        pKinesisVideoStream->streamInfo.streamCaps.codecPrivateData = NULL;
    }
}

STATUS createPackager(PStreamInfo pStreamInfo, GetCurrentTimeFunc getCurrentTimeFn, UINT64 customData, PMkvGenerator* ppGenerator)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 mkvGenFlags = MKV_GEN_FLAG_NONE |
            (pStreamInfo->streamCaps.keyFrameFragmentation ? MKV_GEN_KEY_FRAME_PROCESSING : MKV_GEN_FLAG_NONE) |
            (pStreamInfo->streamCaps.frameTimecodes ? MKV_GEN_IN_STREAM_TIME : MKV_GEN_FLAG_NONE) |
            (pStreamInfo->streamCaps.absoluteFragmentTimes ? MKV_GEN_ABSOLUTE_CLUSTER_TIME : MKV_GEN_FLAG_NONE);

    // Apply the NAL adaptation flags.
    // NOTE: The flag values are the same as defined in the mkvgen
    mkvGenFlags |= pStreamInfo->streamCaps.nalAdaptationFlags;

    // Create the packager
    CHK_STATUS(createMkvGenerator(pStreamInfo->streamCaps.contentType,
                                  mkvGenFlags,
                                  pStreamInfo->streamCaps.timecodeScale,
                                  pStreamInfo->streamCaps.fragmentDuration,
                                  pStreamInfo->streamCaps.codecId,
                                  pStreamInfo->streamCaps.trackName,
                                  pStreamInfo->streamCaps.codecPrivateData,
                                  pStreamInfo->streamCaps.codecPrivateDataSize,
                                  getCurrentTimeFn,
                                  customData,
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
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, &pCurItem));

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
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, &pCurItem));

    // Set the received ACK
    SET_ITEM_RECEIVED_ACK(pCurItem->flags);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS streamFragmentPersistedAck(PKinesisVideoStream pKinesisVideoStream, UINT64 timestamp)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, setViewStatus = STATUS_SUCCESS;
    PViewItem pCurItem;
    UINT32 curItemIndex;
    BOOL setCurrentBack = FALSE;

    // The state and the params are validated.
    // We need to find the next fragment to the persistent one and
    // trim the window to that fragment - i.e. move the tail position to the fragment.
    // As we move the tail, the callbacks will be fired to process the items falling out of the window.

    // Get the fragment start frame.
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, &pCurItem));

    // Remember the current index
    CHK_STATUS(contentViewGetCurrentIndex(pKinesisVideoStream->pView, &curItemIndex));

    // Set the current to the first frame of the ACKed fragments next
    CHK_STATUS(contentViewSetCurrentIndex(pKinesisVideoStream->pView, pCurItem->index + 1));
    setCurrentBack = TRUE;

    // Find the next boundary item which will indicate the start of the next fragment.
    // NOTE: This might fail if we are still assembling a fragment and the ACK is for the previous fragment.
    retStatus = getNextBoundaryViewItem(pKinesisVideoStream, &pCurItem);
    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS, retStatus);

    // Reset the status and early exit in case we have no more items which means the tail is current
    if (retStatus == STATUS_CONTENT_VIEW_NO_MORE_ITEMS) {
        retStatus = STATUS_SUCCESS;
        CHK(FALSE, retStatus);
    }

    // Trim the tail
    CHK_STATUS(contentViewTrimTail(pKinesisVideoStream->pView, pCurItem->index));

CleanUp:

    // Set the current back if we had modified it
    if (setCurrentBack
        && STATUS_FAILED((setViewStatus = contentViewSetCurrentIndex(pKinesisVideoStream->pView, curItemIndex)))) {
        DLOGE("Failed to set the current back to index %d with status 0x%08x", curItemIndex, setViewStatus);
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
    PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // The state and the params are validated.
    // Set the latest to the timestamp of the failed fragment for re-transmission
    CHK_STATUS(contentViewGetItemWithTimestamp(pKinesisVideoStream->pView, timestamp, &pCurItem));
    CHK_STATUS(contentViewSetCurrentIndex(pKinesisVideoStream->pView, pCurItem->index));

    // As we have an error ACK this also means that the inlet host has terminated the connection.
    // We will indicate the stream termination to the client
    if (pKinesisVideoClient->clientCallbacks.streamErrorReportFn != NULL) {
        // Call the stream error notification callback
        errStatus = serviceCallResultCheck(callResult);
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamErrorReportFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
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
    UINT32 curIndex = 0;
    UINT64 lastAckDuration = 0;

    // Check if we need to do anything
    CHK(pKinesisVideoStream->streamInfo.streamCaps.connectionStalenessDuration != CONNECTION_STALENESS_DETECTION_SENTINEL &&
        !pKinesisVideoStream->streamInfo.streamCaps.fragmentAcks &&
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
    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD));

    // Set the grace period which will be reset when the new token is in place.
    pKinesisVideoStream->gracePeriod = TRUE;

CleanUp:

    LEAVES();
    return retStatus;
}