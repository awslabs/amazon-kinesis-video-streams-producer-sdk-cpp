/**
 * Kinesis Video Client
 */
#define LOG_CLASS "KinesisVideoClient"

#include "Include_i.h"

//////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////
/**
 * Static definitions of the states
 */
extern StateMachineState CLIENT_STATE_MACHINE_STATES[];
extern UINT32 CLIENT_STATE_MACHINE_STATE_COUNT;

/**
 * Global singleton client instance.
 * NOTE: This is not multi-client safe
 */
PKinesisVideoClient gKinesisVideoClient = NULL;

/**
 *
 * @param timerId - timerId for timer
 * @param currentTime - the current time when the call back was fired
 * @param customData - pKinesisVideoClient, contains the streams and interval info
 * @return
 */
STATUS checkIntermittentProducerCallback(UINT32 timerId, UINT64 currentTime, UINT64 customData)
{
    ENTERS();
    UNUSED_PARAM(timerId);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = (PKinesisVideoClient) customData;
    UINT32 i;
    PKinesisVideoStream pCurrStream = NULL;
    Frame eofr = EOFR_FRAME_INITIALIZER;

    CHK(pKinesisVideoClient, STATUS_NULL_ARG);

    // These values are set in another thread (so we need a lock)
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    // call pre hook function, useful for testing
    if (pKinesisVideoClient->timerCallbackPreHookFunc != NULL) {
        retStatus = pKinesisVideoClient->timerCallbackPreHookFunc(pKinesisVideoClient->hookCustomData);
    }
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    if (STATUS_SUCCEEDED(retStatus)) {
        // Lock the client streams list lock
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);

        currentTime = GETTIME();
        for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
            if (NULL != pKinesisVideoClient->streams[i]) {
                pCurrStream = pKinesisVideoClient->streams[i];
                // Lock the Stream
                pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pCurrStream->base.lock);
                // Check if last PutFrame is older than max timeout, if so, send EoFR, if not, do nothing
                // Ignoring currentTime it COULD be smaller than pCurrStream->lastPutFrametimestamp
                // Due to this method entering but waiting on stream lock from putFrame call
                if (IS_VALID_TIMESTAMP(pCurrStream->lastPutFrameTimestamp) && currentTime > pCurrStream->lastPutFrameTimestamp &&
                    (currentTime - pCurrStream->lastPutFrameTimestamp) > INTERMITTENT_PRODUCER_MAX_TIMEOUT) {
                    if (!STATUS_SUCCEEDED(retStatus = putKinesisVideoFrame(TO_STREAM_HANDLE(pCurrStream), &eofr))) {
                        DLOGW("Failed to submit auto eofr with 0x%08x, for stream name: %s", retStatus, pCurrStream->streamInfo.name);
                    }
                }

                // Unlock the Stream
                pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pCurrStream->base.lock);
            }
        }

        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    }

CleanUp:
    CHK_LOG_ERR(retStatus);

    LEAVES();
    return retStatus;
}

/**
 * Creates a client
 */
STATUS createKinesisVideoClient(PDeviceInfo pDeviceInfo, PClientCallbacks pClientCallbacks, PCLIENT_HANDLE pClientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PStateMachine pStateMachine = NULL;
    BOOL tearDownOnError = TRUE;
    UINT32 allocationSize, heapFlags, tagsSize;

    // Check the input params
    CHK(pDeviceInfo != NULL && pClientHandle != NULL, STATUS_NULL_ARG);

    // Set the return client handle first
    *pClientHandle = INVALID_CLIENT_HANDLE_VALUE;

    // Validate the input structs
    CHK_STATUS(validateDeviceInfo(pDeviceInfo));

    // Validate the callbacks. Set default callbacks.
    CHK_STATUS(validateClientCallbacks(pDeviceInfo, pClientCallbacks));

    // Report the creation after the validation as we might have the overwritten logger.
    DLOGI("Creating Kinesis Video Client");

    // Get the max tags structure size
    CHK_STATUS(packageTags(pDeviceInfo->tagCount, pDeviceInfo->tags, 0, NULL, &tagsSize));

    // Allocate the main struct with an array of stream pointers following the structure and the array of tags following it
    // NOTE: The calloc will Zero the fields
    allocationSize = SIZEOF(KinesisVideoClient) + pDeviceInfo->streamCount * SIZEOF(PKinesisVideoStream) + tagsSize;
    pKinesisVideoClient = (PKinesisVideoClient) MEMCALLOC(1, allocationSize);
    CHK(pKinesisVideoClient != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the basics
    pKinesisVideoClient->base.identifier = KINESIS_VIDEO_OBJECT_IDENTIFIER_CLIENT;
    pKinesisVideoClient->base.version = KINESIS_VIDEO_CLIENT_CURRENT_VERSION;
    pKinesisVideoClient->certAuthInfo.version = AUTH_INFO_CURRENT_VERSION;
    pKinesisVideoClient->tokenAuthInfo.version = AUTH_INFO_CURRENT_VERSION;

    // Set the auth size and type
    pKinesisVideoClient->certAuthInfo.type = AUTH_INFO_UNDEFINED;
    pKinesisVideoClient->certAuthInfo.size = 0;
    pKinesisVideoClient->tokenAuthInfo.type = AUTH_INFO_UNDEFINED;
    pKinesisVideoClient->tokenAuthInfo.size = 0;

    // Set the initial stream count
    pKinesisVideoClient->streamCount = 0;

    // Set prehook to null
    pKinesisVideoClient->timerCallbackPreHookFunc = NULL;
    pKinesisVideoClient->hookCustomData = 0;

    // Set the client to not-ready
    pKinesisVideoClient->clientReady = FALSE;

    // Set the streams pointer right after the struct
    pKinesisVideoClient->streams = (PKinesisVideoStream*) (pKinesisVideoClient + 1);

    // Copy the structures in their entirety
    MEMCPY(&pKinesisVideoClient->clientCallbacks, pClientCallbacks, SIZEOF(ClientCallbacks));

    // Fix-up the defaults if needed
    // IMPORTANT!!! The calloc allocator will zero the memory which will also act as a
    // sentinel value in case of an earlier version of the structure
    // is used and the remaining fields are not copied
    fixupDeviceInfo(&pKinesisVideoClient->deviceInfo, pDeviceInfo);

    // Fix-up the name of the device if not specified
    if (pKinesisVideoClient->deviceInfo.name[0] == '\0') {
        createRandomName(pKinesisVideoClient->deviceInfo.name, DEFAULT_DEVICE_NAME_LEN, pKinesisVideoClient->clientCallbacks.getRandomNumberFn,
                         pKinesisVideoClient->clientCallbacks.customData);
    }

    // Set logger log level
    SET_LOGGER_LOG_LEVEL(pKinesisVideoClient->deviceInfo.clientInfo.loggerLogLevel);

#ifndef ALIGNED_MEMORY_MODEL
    // In case of in-content-store memory allocation, we need to ensure the heap is aligned
    CHK(pKinesisVideoClient->deviceInfo.storageInfo.storageType != DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC,
        STATUS_NON_ALIGNED_HEAP_WITH_IN_CONTENT_STORE_ALLOCATORS);
#endif

    // Create the storage
    heapFlags = pKinesisVideoClient->deviceInfo.storageInfo.storageType == DEVICE_STORAGE_TYPE_IN_MEM ||
            pKinesisVideoClient->deviceInfo.storageInfo.storageType == DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC
        ? MEMORY_BASED_HEAP_FLAGS
        : FILE_BASED_HEAP_FLAGS;
    CHK_STATUS(heapInitialize(pKinesisVideoClient->deviceInfo.storageInfo.storageSize, pKinesisVideoClient->deviceInfo.storageInfo.spillRatio,
                              heapFlags, pKinesisVideoClient->deviceInfo.storageInfo.rootDirectory, &pKinesisVideoClient->pHeap));

    // Using content store allocator if needed
    // IMPORTANT! This will not be multi-client-safe
    if (pKinesisVideoClient->deviceInfo.storageInfo.storageType == DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC) {
        CHK_STATUS(setContentStoreAllocator(pKinesisVideoClient));
    }

    // Initialize the the ready state condition variable
    pKinesisVideoClient->base.ready = pKinesisVideoClient->clientCallbacks.createConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData);

    CHK(IS_VALID_CVAR_VALUE(pKinesisVideoClient->base.ready), STATUS_NOT_ENOUGH_MEMORY);

    // Set the tags pointer to point after the array of streams
    pKinesisVideoClient->deviceInfo.tags = (PTag)((PKinesisVideoStream*) (pKinesisVideoClient + 1) + pDeviceInfo->streamCount);

    // Package the tags after the structure
    CHK_STATUS(packageTags(pDeviceInfo->tagCount, pDeviceInfo->tags, tagsSize, pKinesisVideoClient->deviceInfo.tags, NULL));
    pKinesisVideoClient->deviceInfo.tagCount = pDeviceInfo->tagCount;

    // Create the client lock
    pKinesisVideoClient->base.lock = pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, TRUE);

    // Create lock for streams list
    pKinesisVideoClient->base.streamListLock =
        pKinesisVideoClient->clientCallbacks.createMutexFn(pKinesisVideoClient->clientCallbacks.customData, TRUE);

    // Create the state machine and step it
    CHK_STATUS(createStateMachine(CLIENT_STATE_MACHINE_STATES, CLIENT_STATE_MACHINE_STATE_COUNT, TO_CUSTOM_DATA(pKinesisVideoClient),
                                  pKinesisVideoClient->clientCallbacks.getCurrentTimeFn, pKinesisVideoClient->clientCallbacks.customData,
                                  &pStateMachine));

    pKinesisVideoClient->base.pStateMachine = pStateMachine;

    if (pKinesisVideoClient->deviceInfo.clientInfo.automaticStreamingFlags == AUTOMATIC_STREAMING_INTERMITTENT_PRODUCER) {
        if (!IS_VALID_TIMER_QUEUE_HANDLE(pKinesisVideoClient->timerQueueHandle)) {
            // Create timer queue
            CHK_STATUS(timerQueueCreate(&pKinesisVideoClient->timerQueueHandle));
            // Store callback in client so we can override in tests
            pKinesisVideoClient->timerCallbackFunc = checkIntermittentProducerCallback;
            CHK_STATUS(timerQueueAddTimer(pKinesisVideoClient->timerQueueHandle, INTERMITTENT_PRODUCER_TIMER_START_DELAY,
                                          pKinesisVideoClient->deviceInfo.clientInfo.reservedCallbackPeriod, pKinesisVideoClient->timerCallbackFunc,
                                          (UINT64) pKinesisVideoClient, &pKinesisVideoClient->timerId));
        }
    }

    // Set the call result to unknown to start
    pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_NOT_SET;

    pKinesisVideoClient->base.shutdown = FALSE;

    // Create sequencing semaphore
    CHK_STATUS(semaphoreCreate(MAX_PIC_REENTRANCY_COUNT, &pKinesisVideoClient->base.shutdownSemaphore));

    // Assign the created object
    *pClientHandle = TO_CLIENT_HANDLE(pKinesisVideoClient);
    tearDownOnError = FALSE;

    // Call to transition the state machine
    // NOTE: This is called after the setting of the newly created
    // object to the OUT parameter as this might evaluate to a service
    // call which might fail. The clients can still choose to proceed
    // with some further processing with a valid object.
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    CHK_LOG_ERR(retStatus);

    if (STATUS_FAILED(retStatus) && tearDownOnError) {
        freeKinesisVideoClientInternal(pKinesisVideoClient, retStatus);
    }

    LEAVES();
    return retStatus;
}

/**
 * Creates a client
 */
STATUS createKinesisVideoClientSync(PDeviceInfo pDeviceInfo, PClientCallbacks pClientCallbacks, PCLIENT_HANDLE pClientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL clientLocked = FALSE;

    CHK_STATUS(createKinesisVideoClient(pDeviceInfo, pClientCallbacks, pClientHandle));

    pKinesisVideoClient = FROM_CLIENT_HANDLE(*pClientHandle);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Need to await for the Ready state before returning
    DLOGV("Awaiting for the Kinesis Video Client to become ready...");

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    do {
        if (pKinesisVideoClient->clientReady) {
            DLOGV("Kinesis Video Client is Ready.");
            break;
        }

        // Blocking path
        // Wait may return without any data available due to a spurious wake up.
        // See: https://en.wikipedia.org/wiki/Spurious_wakeup
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.waitConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                                pKinesisVideoClient->base.ready, pKinesisVideoClient->base.lock,
                                                                                pKinesisVideoClient->deviceInfo.clientInfo.createClientTimeout));

    } while (TRUE);

CleanUp:

    if (retStatus == STATUS_OPERATION_TIMED_OUT) {
        DLOGE("Failed to create Kinesis Video Client - timed out.");
    }

    CHK_LOG_ERR(retStatus);

    // Unlock the client
    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    // Free the client on error
    if (pKinesisVideoClient != NULL && STATUS_FAILED(retStatus)) {
        freeKinesisVideoClientInternal(pKinesisVideoClient, retStatus);
        *pClientHandle = INVALID_CLIENT_HANDLE_VALUE;
    }

    LEAVES();
    return retStatus;
}

/**
 * Free Kinesis Video Client object
 */
STATUS freeKinesisVideoClient(PCLIENT_HANDLE pClientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;

    DLOGI("Freeing Kinesis Video Client");

    // Check if valid handle is passed
    CHK(pClientHandle != NULL, STATUS_NULL_ARG);

    // Get the client handle
    pKinesisVideoClient = FROM_CLIENT_HANDLE(*pClientHandle);

    // Call is idempotent
    CHK(pKinesisVideoClient != NULL, retStatus);

    // Internal functionality to free up the client object
    CHK_STATUS(freeKinesisVideoClientInternal(pKinesisVideoClient, STATUS_SUCCESS));

    // Set the client handle pointer to invalid
    *pClientHandle = INVALID_CLIENT_HANDLE_VALUE;

CleanUp:

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Gets the current memory footprint metrics
 */
STATUS getKinesisVideoMetrics(CLIENT_HANDLE clientHandle, PClientMetrics pKinesisVideoMetrics)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 heapSize;
    UINT32 i, viewAllocationSize;
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    BOOL releaseClientSemaphore = FALSE, locked = FALSE;

    DLOGV("Get the memory metrics size.");

    CHK(pKinesisVideoClient != NULL && pKinesisVideoMetrics != NULL, STATUS_NULL_ARG);
    CHK(pKinesisVideoMetrics->version <= CLIENT_METRICS_CURRENT_VERSION, STATUS_INVALID_CLIENT_METRICS_VERSION);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(heapGetSize(pKinesisVideoClient->pHeap, &heapSize));

    pKinesisVideoMetrics->contentStoreSize = pKinesisVideoClient->deviceInfo.storageInfo.storageSize;
    pKinesisVideoMetrics->contentStoreAllocatedSize = heapSize;
    // Calculate the available size from the overall heap limit
    pKinesisVideoMetrics->contentStoreAvailableSize = pKinesisVideoClient->deviceInfo.storageInfo.storageSize - heapSize;

    pKinesisVideoMetrics->totalContentViewsSize = 0;
    pKinesisVideoMetrics->totalTransferRate = 0;
    pKinesisVideoMetrics->totalFrameRate = 0;

    // Lock the client streams list lock because we're iterating over
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    locked = TRUE;

    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            CHK_STATUS(contentViewGetAllocationSize(pKinesisVideoClient->streams[i]->pView, &viewAllocationSize));
            pKinesisVideoMetrics->totalContentViewsSize += viewAllocationSize;
            pKinesisVideoMetrics->totalFrameRate += (UINT64) pKinesisVideoClient->streams[i]->diagnostics.currentFrameRate;
            pKinesisVideoMetrics->totalTransferRate += pKinesisVideoClient->streams[i]->diagnostics.currentTransferRate;
        }
    }

    // Unlock client streams list lock after iteration.
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    locked = FALSE;

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Gets the stream diagnostics info
 */
STATUS getKinesisVideoStreamMetrics(STREAM_HANDLE streamHandle, PStreamMetrics pStreamMetrics)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    DLOGS("Get stream metrics for Stream %016" PRIx64 ".", streamHandle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pStreamMetrics != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(getStreamMetrics(pKinesisVideoStream, pStreamMetrics));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Stops the streams
 */
STATUS stopKinesisVideoStreams(CLIENT_HANDLE clientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;
    BOOL releaseClientSemaphore = FALSE, streamsListLock = FALSE;
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);

    DLOGI("Stopping Kinesis Video Streams.");

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    // Must lock streams list lock due to iteration
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    streamsListLock = TRUE;

    // Iterate over the streams and stop them
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            // We will bail out if the stream stopping fails
            CHK_STATUS(stopKinesisVideoStream(TO_STREAM_HANDLE(pKinesisVideoClient->streams[i])));
        }
    }

    // unlock streams list lock after iteration
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    streamsListLock = FALSE;

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    if (streamsListLock) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Stops the stream
 */
STATUS stopKinesisVideoStream(STREAM_HANDLE streamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Stopping Kinesis Video Stream %016" PRIx64 ".", streamHandle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(stopStream(pKinesisVideoStream));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Stops the stream synchronously
 */
STATUS stopKinesisVideoStreamSync(STREAM_HANDLE streamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE, streamsListLock = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Synchronously stopping Kinesis Video Stream %016" PRIx64 ".", streamHandle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    // lock streams list lock
    pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.customData,
                                                                          pKinesisVideoStream->pKinesisVideoClient->base.streamListLock);
    streamsListLock = TRUE;

    CHK_STATUS(stopStreamSync(pKinesisVideoStream));

    pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.customData,
                                                                            pKinesisVideoStream->pKinesisVideoClient->base.streamListLock);
    streamsListLock = FALSE;

CleanUp:

    if (streamsListLock) {
        pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoStream->pKinesisVideoClient->clientCallbacks.customData,
                                                                                pKinesisVideoStream->pKinesisVideoClient->base.streamListLock);
    }

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Creates an Kinesis Video stream
 */
STATUS createKinesisVideoStream(CLIENT_HANDLE clientHandle, PStreamInfo pStreamInfo, PSTREAM_HANDLE pStreamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = NULL;
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);

    DLOGI("Creating Kinesis Video Stream.");

    CHK(pKinesisVideoClient != NULL && pStreamHandle != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    // Check if we are in the right state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoClient->base.pStateMachine, CLIENT_STATE_READY));

    // Create the actual stream
    CHK_STATUS(createStream(pKinesisVideoClient, pStreamInfo, &pKinesisVideoStream));

    // Convert to handle
    *pStreamHandle = TO_STREAM_HANDLE(pKinesisVideoStream);

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Creates an Kinesis Video stream synchronously
 */
STATUS createKinesisVideoStreamSync(CLIENT_HANDLE clientHandle, PStreamInfo pStreamInfo, PSTREAM_HANDLE pStreamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    PKinesisVideoStream pKinesisVideoStream = NULL;
    BOOL streamLocked = FALSE, releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;

    CHK_STATUS(createKinesisVideoStream(clientHandle, pStreamInfo, pStreamHandle));

    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    pKinesisVideoStream = FROM_STREAM_HANDLE(*pStreamHandle);

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    // Need to await for the Ready state before returning
    DLOGV("Awaiting for the stream to become ready...");

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    do {
        if (pKinesisVideoStream->streamReady) {
            DLOGV("Kinesis Video Stream is Ready.");
            break;
        }

        if (pKinesisVideoStream->base.shutdown) {
            DLOGW("Kinesis Video Stream is being shut down.");
            break;
        }

        // Blocking path
        // Wait may return without any data available due to a spurious wake up.
        // See: https://en.wikipedia.org/wiki/Spurious_wakeup
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.waitConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                                pKinesisVideoStream->base.ready, pKinesisVideoStream->base.lock,
                                                                                pKinesisVideoClient->deviceInfo.clientInfo.createStreamTimeout));

    } while (TRUE);

CleanUp:

    if (retStatus == STATUS_OPERATION_TIMED_OUT) {
        DLOGE("Failed to create Kinesis Video Stream - timed out.");
    }

    // release stream lock
    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    // Free the stream on error
    if (pKinesisVideoClient != NULL && pKinesisVideoStream != NULL && STATUS_FAILED(retStatus)) {
        freeKinesisVideoStream(pStreamHandle);
        *pStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Stops and frees the stream.
 */
STATUS freeKinesisVideoStream(PSTREAM_HANDLE pStreamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream;
    PKinesisVideoClient pKinesisVideoClient;
    BOOL locked = FALSE;

    DLOGI("Freeing Kinesis Video stream.");

    CHK(pStreamHandle != NULL, STATUS_NULL_ARG);
    pKinesisVideoStream = FROM_STREAM_HANDLE(*pStreamHandle);
    CHK(pKinesisVideoStream != NULL, retStatus);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    CHK(pKinesisVideoClient != NULL, retStatus);
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    // lock stream list lock before freeing stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    locked = TRUE;

    CHK_STATUS(freeStream(pKinesisVideoStream));

    // unlock stream list lock before freeing stream
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    locked = FALSE;

    // Set the stream to invalid
    *pStreamHandle = INVALID_STREAM_HANDLE_VALUE;

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Puts a frame into the Kinesis Video stream.
 */
STATUS putKinesisVideoFrame(STREAM_HANDLE streamHandle, PFrame pFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;

    DLOGS("Putting frame into an Kinesis Video stream.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pFrame != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    DLOGV("debug frame info pts: %" PRIu64 ", dts: %" PRIu64 ", duration: %" PRIu64 ", size: %u, trackId: %" PRIu64 ", isKey %d",
          pFrame->presentationTs, pFrame->decodingTs, pFrame->duration, pFrame->size, pFrame->trackId, CHECK_FRAME_FLAG_KEY_FRAME(pFrame->flags));

    // Process and store the result
    CHK_STATUS(frameOrderCoordinatorPutFrame(pKinesisVideoStream, pFrame));

CleanUp:

    if (STATUS_FAILED(retStatus) && pFrame != NULL) {
        DLOGW("Failed to submit frame to Kinesis Video client. "
              "status: 0x%08x decoding timestamp: %" PRIu64 " presentation timestamp: %" PRIu64,
              retStatus, pFrame->decodingTs, pFrame->presentationTs);
    } else {
        /* In case pFrame is NULL and something failed */
        CHK_LOG_ERR(retStatus);
    }

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    LEAVES();
    return retStatus;
}

/**
 * Puts a metadata (key/value pair) into the Kinesis Video stream.
 */
STATUS putKinesisVideoFragmentMetadata(STREAM_HANDLE streamHandle, PCHAR name, PCHAR value, BOOL persistent)
{
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Put metadata into an Kinesis Video stream.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(putFragmentMetadata(pKinesisVideoStream, name, value, persistent));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Stream format changed.
 *
 * NOTE: Currently, the format change should happen while it's not streaming.
 */
STATUS kinesisVideoStreamFormatChanged(STREAM_HANDLE streamHandle, UINT32 codecPrivateDataSize, PBYTE codecPrivateData, UINT64 trackId)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;

    DLOGI("Stream format changed.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    // Process and store the result
    CHK_STATUS(streamFormatChanged(pKinesisVideoStream, codecPrivateDataSize, codecPrivateData, trackId));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Fills in the buffer for a given stream
 * @return Status of the operation
 */
STATUS getKinesisVideoStreamData(STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pFilledSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Getting data from an Kinesis Video stream.");
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    // Process and store the result
    CHK_STATUS(getStreamData(pKinesisVideoStream, uploadHandle, pBuffer, bufferSize, pFilledSize));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    if (retStatus != STATUS_SUCCESS && retStatus != STATUS_AWAITING_PERSISTED_ACK && retStatus != STATUS_END_OF_STREAM &&
        retStatus != STATUS_UPLOAD_HANDLE_ABORTED && retStatus != STATUS_NO_MORE_DATA_AVAILABLE) {
        CHK_LOG_ERR(retStatus);
    }

    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream get streamInfo from STREAM_HANDLE
 */
STATUS kinesisVideoStreamGetStreamInfo(STREAM_HANDLE streamHandle, PPStreamInfo ppStreamInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    CHK(pKinesisVideoStream != NULL && ppStreamInfo != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    *ppStreamInfo = &pKinesisVideoStream->streamInfo;

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

//////////////////////////////////////////////////////////
// Event functions
//////////////////////////////////////////////////////////
/**
 * Create device API call result event
 */
STATUS createDeviceResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PCHAR deviceArn)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    DLOGI("Create device result event.");
    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(createDeviceResult(pKinesisVideoClient, callResult, deviceArn));

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Device certificate to token exchange result event
 */
STATUS deviceCertToTokenResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PBYTE pToken, UINT32 tokenSize, UINT64 expiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    DLOGI("Device certificate to token exchange result event.");

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(deviceCertToTokenResult(pKinesisVideoClient, callResult, pToken, tokenSize, expiration));

CleanUp:

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Describe stream API call result event
 */
STATUS describeStreamResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PStreamDescription pStreamDescription)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Describe stream result event.");
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(describeStreamResult(pKinesisVideoStream, callResult, pStreamDescription));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Create stream API call result event
 */
STATUS createStreamResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PCHAR streamArn)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Create stream result event.");
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(createStreamResult(pKinesisVideoStream, callResult, streamArn));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Get streaming token API call result event
 */
STATUS getStreamingTokenResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PBYTE pToken, UINT32 tokenSize, UINT64 expiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Get streaming token result event.");
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(getStreamingTokenResult(pKinesisVideoStream, callResult, pToken, tokenSize, expiration));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Get streaming endpoint API call result event
 */
STATUS getStreamingEndpointResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, PCHAR pEndpoint)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Get streaming endpoint result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(getStreamingEndpointResult(pKinesisVideoStream, callResult, pEndpoint));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Put stream with endless payload API call result event
 */
STATUS putStreamResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, UPLOAD_HANDLE uploadHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Put stream result event. New upload handle %" PRIu64, uploadHandle);
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(putStreamResult(pKinesisVideoStream, callResult, uploadHandle));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Tag resource API call result event
 */
STATUS tagResourceResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    UINT32 objectIdentifier;
    PKinesisVideoStream pKinesisVideoStream = NULL;
    PKinesisVideoClient pKinesisVideoClient = NULL;

    DLOGI("Tag resource result event.");

    CHK(IS_VALID_HANDLE((HANDLE) customData), STATUS_NULL_ARG);

    objectIdentifier = KINESIS_VIDEO_OBJECT_IDENTIFIER_FROM_CUSTOM_DATA(customData);

    // Check if it's a client
    if (objectIdentifier == KINESIS_VIDEO_OBJECT_IDENTIFIER_CLIENT) {
        pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

        // Shutdown sequencer
        CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
        releaseClientSemaphore = TRUE;

        // Call tagging result for client
        CHK_STATUS(tagClientResult(pKinesisVideoClient, callResult));
    } else {
        pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

        // Validate it's a stream object
        CHK(pKinesisVideoStream->base.identifier == KINESIS_VIDEO_OBJECT_IDENTIFIER_STREAM, STATUS_INVALID_CUSTOM_DATA);
        pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

        // Shutdown sequencer
        CHK_STATUS(semaphoreAcquire(pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
        releaseClientSemaphore = TRUE;

        CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
        releaseStreamSemaphore = TRUE;

        // Call tagging result for stream
        CHK_STATUS(tagStreamResult(pKinesisVideoStream, callResult));
    }

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream terminated notification. The expectation is no more getKinesisVideoStreamData call from
 * streamUploadHandle after kinesisVideoStreamTerminated is called.
 */
STATUS kinesisVideoStreamTerminated(STREAM_HANDLE streamHandle, UPLOAD_HANDLE streamUploadHandle, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Stream 0x%" PRIx64 " terminated upload handle %" PRIu64 " with service call result %u.", streamHandle, streamUploadHandle, callResult);
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, streamUploadHandle, callResult, FALSE));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream fragment ACK received event
 */
STATUS kinesisVideoStreamFragmentAck(STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, PFragmentAck pFragmentAck)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Stream fragment ACK event.");
    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pFragmentAck != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(streamFragmentAckEvent(pKinesisVideoStream, uploadHandle, pFragmentAck));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream fragment ACK parsing
 */
STATUS kinesisVideoStreamParseFragmentAck(STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, PCHAR ackSegment, UINT32 ackSegmentSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL releaseClientSemaphore = FALSE, releaseStreamSemaphore = FALSE;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Parsing stream fragment ACK.");
    CHK(pKinesisVideoStream != NULL && ackSegment != NULL, STATUS_NULL_ARG);

    // Shutdown sequencer
    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseClientSemaphore = TRUE;

    CHK_STATUS(semaphoreAcquire(pKinesisVideoStream->base.shutdownSemaphore, INFINITE_TIME_VALUE));
    releaseStreamSemaphore = TRUE;

    CHK_STATUS(parseFragmentAck(pKinesisVideoStream, uploadHandle, ackSegment, ackSegmentSize));

CleanUp:

    if (releaseStreamSemaphore) {
        semaphoreRelease(pKinesisVideoStream->base.shutdownSemaphore);
    }

    if (releaseClientSemaphore) {
        semaphoreRelease(pKinesisVideoStream->pKinesisVideoClient->base.shutdownSemaphore);
    }

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

//////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////

/**
 * Frees the client object
 */
STATUS freeKinesisVideoClientInternal(PKinesisVideoClient pKinesisVideoClient, STATUS failStatus)
{
    STATUS retStatus = STATUS_SUCCESS, freeStreamStatus = STATUS_SUCCESS, freeStateMachineStatus = STATUS_SUCCESS, freeHeapStatus = STATUS_SUCCESS;
    UINT32 i = 0;
    BOOL locked = FALSE;

    // Call is idempotent
    CHK(pKinesisVideoClient != NULL && !pKinesisVideoClient->base.shutdown, retStatus);

    pKinesisVideoClient->base.shutdown = TRUE;

    // Lock the streamListLock for iteration
    if (IS_VALID_MUTEX_VALUE(pKinesisVideoClient->base.streamListLock)) {
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
        locked = TRUE;
    }

    // Call stream shutdowns first
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            // Call is idempotent so NULL is OK
            shutdownStream(pKinesisVideoClient->streams[i], FALSE);
        }
    }

    // unlock the streamListLock after iteration
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
        locked = FALSE;
    }

    // Shutdown client
    pKinesisVideoClient->clientCallbacks.clientShutdownFn(pKinesisVideoClient->clientCallbacks.customData, TO_CLIENT_HANDLE(pKinesisVideoClient));

    // After the shutdown
    semaphoreLock(pKinesisVideoClient->base.shutdownSemaphore);
    semaphoreWaitUntilClear(pKinesisVideoClient->base.shutdownSemaphore, CLIENT_SHUTDOWN_SEMAPHORE_TIMEOUT);

    // Need to stop scheduling callbacks
    // This must happen outside the streamslist lock because the callbacks acquire that lock
    // and shutdown won't return until callbacks have completed so we'll deadlock
    if (IS_VALID_TIMER_QUEUE_HANDLE(pKinesisVideoClient->timerQueueHandle)) {
        timerQueueShutdown(pKinesisVideoClient->timerQueueHandle);
    }

    // Lock the streamListLock for iteration
    if (IS_VALID_MUTEX_VALUE(pKinesisVideoClient->base.streamListLock)) {
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
        locked = TRUE;
    }

    // Release the underlying objects
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            // Call is idempotent so NULL is OK
            retStatus = freeStream(pKinesisVideoClient->streams[i]);
            freeStreamStatus = STATUS_FAILED(retStatus) ? retStatus : freeStreamStatus;
        }
    }

    // unlock and free the streamListLock
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
        pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.streamListLock);
        locked = FALSE;
    }

    // Release the state machine
    freeStateMachineStatus = freeStateMachine(pKinesisVideoClient->base.pStateMachine);

    // Release the condition variable if set
    if (IS_VALID_CVAR_VALUE(pKinesisVideoClient->base.ready)) {
        // Broadcast in any case
        pKinesisVideoClient->clientCallbacks.broadcastConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                          pKinesisVideoClient->base.ready);

        // Free the condition variable
        pKinesisVideoClient->clientCallbacks.freeConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                     pKinesisVideoClient->base.ready);
        pKinesisVideoClient->base.ready = INVALID_CVAR_VALUE;
    }

    if (IS_VALID_MUTEX_VALUE(pKinesisVideoClient->base.lock)) {
        pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    if (IS_VALID_SEMAPHORE_HANDLE(pKinesisVideoClient->base.shutdownSemaphore)) {
        semaphoreFree(&pKinesisVideoClient->base.shutdownSemaphore);
    }

    if (IS_VALID_TIMER_QUEUE_HANDLE(pKinesisVideoClient->timerQueueHandle)) {
        timerQueueFree(&pKinesisVideoClient->timerQueueHandle);
    }

    // Reset the stored allocators if replaced
    if (STATUS_SUCCEEDED(failStatus) && pKinesisVideoClient->deviceInfo.storageInfo.storageType == DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC) {
        globalMemAlloc = pKinesisVideoClient->storedMemAlloc;
        globalMemAlignAlloc = pKinesisVideoClient->storedMemAlignAlloc;
        globalMemCalloc = pKinesisVideoClient->storedMemCalloc;
        globalMemFree = pKinesisVideoClient->storedMemFree;

        // Reset the singleton instance
        gKinesisVideoClient = NULL;
    }

    // Release the heap
    if (pKinesisVideoClient->pHeap) {
        heapDebugCheckAllocator(pKinesisVideoClient->pHeap, TRUE);
        freeHeapStatus = heapRelease(pKinesisVideoClient->pHeap);
    }

    DLOGD("Total allocated memory %" PRIu64, pKinesisVideoClient->totalAllocationSize);

    // Release the object
    MEMFREE(pKinesisVideoClient);

CleanUp:

    return retStatus | freeStreamStatus | freeHeapStatus | freeStateMachineStatus;
}

/**
 * Callback function which is invoked when an item gets purged from the view
 */
VOID viewItemRemoved(PContentView pContentView, UINT64 customData, PViewItem pViewItem, BOOL currentRemoved)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;

    // Validate the input just in case
    CHK(pContentView != NULL && pViewItem != NULL && pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL,
        STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Notify the client about the dropped frame - only if we are removing the an item that has not been sent
    if (currentRemoved) {
        DLOGW("Reporting a dropped frame/fragment.");
        switch (pKinesisVideoStream->streamInfo.streamCaps.streamingType) {
            case STREAMING_TYPE_REALTIME:
            case STREAMING_TYPE_OFFLINE:
                pKinesisVideoStream->diagnostics.droppedFrames++;

                // The callback is optional so check if specified first
                if (pKinesisVideoClient->clientCallbacks.droppedFrameReportFn != NULL) {
                    CHK_STATUS(pKinesisVideoClient->clientCallbacks.droppedFrameReportFn(
                        pKinesisVideoClient->clientCallbacks.customData, TO_STREAM_HANDLE(pKinesisVideoStream), pViewItem->timestamp));
                }

                break;

            case STREAMING_TYPE_NEAR_REALTIME:
                pKinesisVideoStream->diagnostics.droppedFragments++;

                // The callback is optional so check if specified first
                if (pKinesisVideoClient->clientCallbacks.droppedFragmentReportFn != NULL) {
                    CHK_STATUS(pKinesisVideoClient->clientCallbacks.droppedFragmentReportFn(
                        pKinesisVideoClient->clientCallbacks.customData, TO_STREAM_HANDLE(pKinesisVideoStream), pViewItem->timestamp));
                }

                break;

            default:
                // do nothing
                break;
        }
    }

CleanUp:

    if (pKinesisVideoClient != NULL) {
        // Lock the client
        pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
        // Remove the item from the storage
        heapFree(pKinesisVideoClient->pHeap, pViewItem->handle);
        pViewItem->handle = INVALID_ALLOCATION_HANDLE_VALUE;

        // Unlock the client
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

        if (streamLocked) {
            // Unlock the stream lock
            pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
        }
    }

    UNUSED_PARAM(retStatus);
    LEAVES();
}

/**
 * Creates random null terminating string. The caller should ensure the
 * buffer has enough space to add the null terminator
 */
VOID createRandomName(PCHAR pName, UINT32 maxChars, GetRandomNumberFunc randFn, UINT64 customData)
{
    CHAR curChar;
    UINT32 i;

    for (i = 0; i < maxChars; i++) {
        // Generate an alpha char
        curChar = (CHAR)(randFn(customData) % (10 + 26));
        if (curChar <= 9) {
            // Assign num
            curChar += '0';
        } else {
            // Assign char
            curChar += 'A' - 10;
        }

        // Store it in the name
        pName[i] = curChar;
    }

    // Null terminate the string - we should still have plenty of buffer space
    pName[maxChars] = '\0';
}

/**
 * Restart/Reset a stream by dropping remaining data and reset stream state machine
 */
STATUS kinesisVideoStreamResetStream(STREAM_HANDLE stream_handle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(stream_handle);

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    CHK_STATUS(resetStream(pKinesisVideoStream));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Reset connection for stream, continue sending existing data in buffer with new connection
 */
STATUS kinesisVideoStreamResetConnection(STREAM_HANDLE stream_handle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(stream_handle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, INVALID_UPLOAD_HANDLE_VALUE, SERVICE_CALL_RESULT_OK, TRUE));

CleanUp:

    LEAVES();
    return retStatus;
}

PVOID contentStoreMemAlloc(SIZE_T size)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    ALLOCATION_HANDLE allocationHandle;
    UINT64 allocationSize = size + SIZEOF(ALLOCATION_HANDLE);
    PVOID pAllocation = NULL;

    DLOGS("Content store malloc %" PRIx64 " bytes", (UINT64) size);
    CHK(gKinesisVideoClient != NULL && gKinesisVideoClient->pHeap != NULL, STATUS_NULL_ARG);

    gKinesisVideoClient->totalAllocationSize += allocationSize;

    // Allocate and returned mapped memory
    CHK_STATUS(heapAlloc(gKinesisVideoClient->pHeap, (UINT64) allocationSize, &allocationHandle));
    CHK_STATUS(heapMap(gKinesisVideoClient->pHeap, allocationHandle, &pAllocation, &allocationSize));

    // Store the allocation handle
    *(PALLOCATION_HANDLE) pAllocation = allocationHandle;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        return NULL;
    }

    LEAVES();

    return (PALLOCATION_HANDLE) pAllocation + 1;
}

PVOID contentStoreMemAlignAlloc(SIZE_T size, SIZE_T alignment)
{
    DLOGS("Content store align malloc %" PRIx64 " bytes", (UINT64) size);
    // Just do malloc
    UNUSED_PARAM(alignment);
    return contentStoreMemAlloc(size);
}

PVOID contentStoreMemCalloc(SIZE_T num, SIZE_T size)
{
    ENTERS();
    PVOID pAllocation;
    SIZE_T overallSize = num * size;
    DLOGS("Content store calloc %" PRIx64 " bytes", (UINT64) overallSize);

    pAllocation = contentStoreMemAlloc(overallSize);

    // Zero out the memory if succeeded the allocation
    if (pAllocation != NULL) {
        MEMSET(pAllocation, 0x00, overallSize);
    }

    LEAVES();

    return pAllocation;
}

VOID contentStoreMemFree(PVOID ptr)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    ALLOCATION_HANDLE allocationHandle;
    PVOID pAllocation;
    UINT64 allocationSize;

    CHK(gKinesisVideoClient != NULL && gKinesisVideoClient->pHeap != NULL && ptr != NULL, STATUS_NULL_ARG);

    pAllocation = (PALLOCATION_HANDLE) ptr - 1;
    allocationHandle = *(PALLOCATION_HANDLE) pAllocation;

    CHK_STATUS(heapGetAllocSize(gKinesisVideoClient->pHeap, allocationHandle, &allocationSize));
    DLOGS("Content store free %" PRIx64 " bytes", allocationSize - SIZEOF(ALLOCATION_HANDLE));
    CHK_STATUS(heapUnmap(gKinesisVideoClient->pHeap, pAllocation));
    CHK_STATUS(heapFree(gKinesisVideoClient->pHeap, allocationHandle));

    gKinesisVideoClient->totalAllocationSize -= allocationSize;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        DLOGW("Failed freeing %p with 0x%08x", ptr, retStatus);
    }

    LEAVES();
}

/**
 * Sets the default allocator from content store
 */
STATUS setContentStoreAllocator(PKinesisVideoClient pKinesisVideoClient)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Make sure we don't re-set the singleton instance.
    CHK(gKinesisVideoClient == NULL, STATUS_INVALID_OPERATION);

    // Need to set the global static to point to the client so the instrumented allocators will find the heap
    gKinesisVideoClient = pKinesisVideoClient;

    // Store the function pointers
    pKinesisVideoClient->totalAllocationSize = 0;
    pKinesisVideoClient->storedMemAlloc = globalMemAlloc;
    pKinesisVideoClient->storedMemAlignAlloc = globalMemAlignAlloc;
    pKinesisVideoClient->storedMemCalloc = globalMemCalloc;
    pKinesisVideoClient->storedMemFree = globalMemFree;

    globalMemAlloc = contentStoreMemAlloc;
    globalMemAlignAlloc = contentStoreMemAlignAlloc;
    globalMemCalloc = contentStoreMemCalloc;
    globalMemFree = contentStoreMemFree;

CleanUp:

    LEAVES();
    return retStatus;
}
