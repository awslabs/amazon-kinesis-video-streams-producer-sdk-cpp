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
 * Creates a client
 */
STATUS createKinesisVideoClient(PDeviceInfo pDeviceInfo, PClientCallbacks pClientCallbacks,
                                PCLIENT_HANDLE pClientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PStateMachine pStateMachine = NULL;
    BOOL tearDownOnError = TRUE;
    UINT32 allocationSize, heapFlags, i;
    PCHAR pCurPnt;
    CLIENT_HANDLE clientHandle;

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

    // Allocate the main struct with an array of stream pointers following the structure and the array of tags following it
    // NOTE: The calloc will Zero the fields
    allocationSize = SIZEOF(KinesisVideoClient) + pDeviceInfo->streamCount * SIZEOF(PKinesisVideoStream) + pDeviceInfo->tagCount * TAG_FULL_LENGTH;
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

    // Set the client to not-ready
    pKinesisVideoClient->clientReady = FALSE;

    // Copy the structures in their entirety
    MEMCPY(&pKinesisVideoClient->clientCallbacks, pClientCallbacks, SIZEOF(ClientCallbacks));
    MEMCPY(&pKinesisVideoClient->deviceInfo, pDeviceInfo, SIZEOF(DeviceInfo));

    // Fix-up the name of the device if not specified
    if (pDeviceInfo->name[0] == '\0') {
        createRandomName(pDeviceInfo->name,
                         DEFAULT_DEVICE_NAME_LEN,
                         pKinesisVideoClient->clientCallbacks.getRandomNumberFn,
                         pKinesisVideoClient->clientCallbacks.customData);
    }

    // Create the storage
    heapFlags = pKinesisVideoClient->deviceInfo.storageInfo.storageType == DEVICE_STORAGE_TYPE_IN_MEM ?
                MEMORY_BASED_HEAP_FLAGS : FILE_BASED_HEAP_FLAGS;
    CHK_STATUS(heapInitialize(pKinesisVideoClient->deviceInfo.storageInfo.storageSize,
                              pKinesisVideoClient->deviceInfo.storageInfo.spillRatio,
                              heapFlags,
                              &pKinesisVideoClient->pHeap));

    // Set the streams pointer right after the struct
    pKinesisVideoClient->streams = (PKinesisVideoStream*)(pKinesisVideoClient + 1);

    // Set the tags pointer to point after the array of streams
    pKinesisVideoClient->deviceInfo.tags = (PTag)((PKinesisVideoStream*)(pKinesisVideoClient + 1) + pDeviceInfo->streamCount);

    // Copy the tags if any. First, set the pointer to the end of the tags to iterate
    pCurPnt = (PCHAR) (pKinesisVideoClient->deviceInfo.tags + pKinesisVideoClient->deviceInfo.tagCount);
    for (i = 0; i < pKinesisVideoClient->deviceInfo.tagCount; i++) {
        pKinesisVideoClient->deviceInfo.tags[i].version = pDeviceInfo->tags[i].version;
        // Fix-up the pointers first
        pKinesisVideoClient->deviceInfo.tags[i].name = pCurPnt;
        pCurPnt += MAX_TAG_NAME_LEN + 1;

        pKinesisVideoClient->deviceInfo.tags[i].value = pCurPnt;
        pCurPnt += MAX_TAG_VALUE_LEN + 1;

        // Copy the strings
        STRNCPY(pKinesisVideoClient->deviceInfo.tags[i].name, pDeviceInfo->tags[i].name, MAX_TAG_NAME_LEN);
        pKinesisVideoClient->deviceInfo.tags[i].name[MAX_TAG_NAME_LEN] = '\0';
        STRNCPY(pKinesisVideoClient->deviceInfo.tags[i].value, pDeviceInfo->tags[i].value, MAX_TAG_VALUE_LEN);
        pKinesisVideoClient->deviceInfo.tags[i].value[MAX_TAG_VALUE_LEN] = '\0';
    }

    // Create the lock
    pKinesisVideoClient->base.lock = pKinesisVideoClient->clientCallbacks.createMutexFn(
            pKinesisVideoClient->clientCallbacks.customData,
            TRUE);

    // Create the state machine and step it
    CHK_STATUS(createStateMachine(CLIENT_STATE_MACHINE_STATES,
                                  CLIENT_STATE_MACHINE_STATE_COUNT,
                                  TO_CUSTOM_DATA(pKinesisVideoClient),
                                  pKinesisVideoClient->clientCallbacks.getCurrentTimeFn,
                                  pKinesisVideoClient->clientCallbacks.customData,
                                  &pStateMachine));

    pKinesisVideoClient->base.pStateMachine = pStateMachine;

    // Set the call result to unknown to start
    pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_NOT_SET;

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

    if (STATUS_FAILED(retStatus) && tearDownOnError) {
        clientHandle = TO_CLIENT_HANDLE(pKinesisVideoClient);
        freeKinesisVideoClient(&clientHandle);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the client object
 */
STATUS freeKinesisVideoClient(PCLIENT_HANDLE pClientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS,
            freeStreamStatus = STATUS_SUCCESS,
            freeStateMachineStatus = STATUS_SUCCESS,
            freeHeapStatus = STATUS_SUCCESS;
    UINT32 i = 0;
    PKinesisVideoClient pKinesisVideoClient = NULL;

    DLOGI("Freeing Kinesis Video Client");

    // Check if valid handle is passed
    CHK(pClientHandle != NULL, STATUS_NULL_ARG);

    // Get the client handle
    pKinesisVideoClient = FROM_CLIENT_HANDLE(*pClientHandle);

    // Call is idempotent
    CHK(pKinesisVideoClient != NULL, retStatus);

    // Lock the client
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    // Release the underlying objects
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        // Call is idempotent so NULL is OK
        retStatus = freeStream(pKinesisVideoClient->streams[i]);
        freeStreamStatus = STATUS_FAILED(retStatus) ? retStatus : freeStreamStatus;
    }

    // Release the heap
    heapDebugCheckAllocator(pKinesisVideoClient->pHeap, TRUE);
    freeHeapStatus = heapRelease(pKinesisVideoClient->pHeap);

    // Release the state machine
    freeStateMachineStatus = freeStateMachine(pKinesisVideoClient->base.pStateMachine);

    // Unlock the client
    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    pKinesisVideoClient->clientCallbacks.freeMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);

    // Release the object
    MEMFREE(pKinesisVideoClient);

    // Set the client handle pointer to invalid
    *pClientHandle = INVALID_CLIENT_HANDLE_VALUE;

CleanUp:

    LEAVES();
    return retStatus | freeStreamStatus | freeHeapStatus | freeStateMachineStatus;
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

    DLOGI("Get the memory metrics size.");

    CHK(pKinesisVideoClient != NULL && pKinesisVideoMetrics != NULL, STATUS_NULL_ARG);
    CHK(pKinesisVideoMetrics->version <= CLIENT_METRICS_CURRENT_VERSION, STATUS_INVALID_CLIENT_METRICS_VERSION);

    CHK_STATUS(heapGetSize(pKinesisVideoClient->pHeap, &heapSize));

    pKinesisVideoMetrics->contentStoreSize = pKinesisVideoClient->deviceInfo.storageInfo.storageSize;
    pKinesisVideoMetrics->contentStoreAllocatedSize = heapSize;
    // Calculate the available size from the overall heap limit
    pKinesisVideoMetrics->contentStoreAvailableSize = pKinesisVideoClient->deviceInfo.storageInfo.storageSize - heapSize;

    pKinesisVideoMetrics->totalContentViewsSize = 0;
    pKinesisVideoMetrics->totalTransferRate = 0;
    pKinesisVideoMetrics->totalFrameRate = 0;
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            CHK_STATUS(contentViewGetAllocationSize(pKinesisVideoClient->streams[i]->pView, &viewAllocationSize));
            pKinesisVideoMetrics->totalContentViewsSize += viewAllocationSize;
            pKinesisVideoMetrics->totalFrameRate += (UINT64)pKinesisVideoClient->streams[i]->diagnostics.currentFrameRate;
            pKinesisVideoMetrics->totalTransferRate += pKinesisVideoClient->streams[i]->diagnostics.currentTransferRate;
        }
    }

CleanUp:

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

    DLOGI("Get stream metrics for Stream %016" PRIx64 ".", streamHandle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pStreamMetrics != NULL, STATUS_NULL_ARG);

    CHK_STATUS(getStreamMetrics(pKinesisVideoStream, pStreamMetrics));

CleanUp:

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
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);

    DLOGI("Stopping Kinesis Video Streams.");

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Iterate over the streams and stop them
    for (i = 0; i < pKinesisVideoClient->deviceInfo.streamCount; i++) {
        if (NULL != pKinesisVideoClient->streams[i]) {
            // We will bail out if the stream stopping fails
            CHK_STATUS(stopKinesisVideoStream(TO_STREAM_HANDLE(pKinesisVideoClient->streams[i])));
        }
    }

CleanUp:

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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Stopping Kinesis Video Stream %016" PRIx64 ".", streamHandle);

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(stopStream(pKinesisVideoStream));

CleanUp:

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
    PKinesisVideoStream pKinesisVideoStream = NULL;
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);

    DLOGI("Creating Kinesis Video Stream.");

    CHK(pKinesisVideoClient != NULL && pStreamHandle != NULL, STATUS_NULL_ARG);

    // Check if we are in the right state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoClient->base.pStateMachine, CLIENT_STATE_READY));

    // Create the actual stream
    CHK_STATUS(createStream(pKinesisVideoClient, pStreamInfo, &pKinesisVideoStream));

    // Convert to handle
    *pStreamHandle = TO_STREAM_HANDLE(pKinesisVideoStream);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Stops and frees the stream.
 */
STATUS freeKinesisVideoStream(PSTREAM_HANDLE pStreamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;

    DLOGI("Freeing Kinesis Video stream.");

    CHK(pStreamHandle != NULL, STATUS_NULL_ARG);
    CHK_STATUS(freeStream(FROM_STREAM_HANDLE(*pStreamHandle)));

    // Set the stream to invalid
    *pStreamHandle = INVALID_STREAM_HANDLE_VALUE;

CleanUp:

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

    DLOGS("Putting frame into an Kinesis Video stream.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Process and store the result
    CHK_STATUS(putFrame(pKinesisVideoStream, pFrame));

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Puts a metadata (key/value pair) into the Kinesis Video stream.
 */
STATUS putKinesisVideoFragmentMetadata(STREAM_HANDLE streamHandle, PCHAR name, PCHAR value, BOOL persistent)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Put metadata into an Kinesis Video stream.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(putFragmentMetadata(pKinesisVideoStream, name, value, persistent));

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Stream format changed.
 *
 * NOTE: Currently, the format change should happen while it's not streaming.
 */
STATUS kinesisVideoStreamFormatChanged(STREAM_HANDLE streamHandle, UINT32 codecPrivateDataSize,
                                                  PBYTE codecPrivateData, UINT64 trackId)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Stream format changed.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Process and store the result
    CHK_STATUS(streamFormatChanged(pKinesisVideoStream, codecPrivateDataSize, codecPrivateData, trackId));

CleanUp:

    LEAVES();
    return retStatus;

}

/**
 * Fills in the buffer for a given stream
 * @return Status of the operation
 */
STATUS getKinesisVideoStreamData(STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, PBYTE pBuffer,
                                 UINT32 bufferSize, PUINT32 pFilledSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Getting data from an Kinesis Video stream.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Process and store the result
    CHK_STATUS(getStreamData(pKinesisVideoStream, uploadHandle, pBuffer, bufferSize, pFilledSize));

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream get streamInfo from STREAM_HANDLE
 */
STATUS kinesisVideoStreamGetStreamInfo(STREAM_HANDLE stream_handle, PPStreamInfo ppStreamInfo) {
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(stream_handle);

    CHK(pKinesisVideoStream != NULL && ppStreamInfo != NULL, STATUS_NULL_ARG);
    *ppStreamInfo = &pKinesisVideoStream->streamInfo;

CleanUp:
    LEAVE();
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
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    DLOGI("Create device result event.");

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(createDeviceResult(pKinesisVideoClient, callResult, deviceArn));

CleanUp:
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
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    DLOGI("Device certificate to token exchange result event.");

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(deviceCertToTokenResult(pKinesisVideoClient, callResult, pToken, tokenSize, expiration));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Describe stream result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(describeStreamResult(pKinesisVideoStream, callResult, pStreamDescription));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Create stream result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(createStreamResult(pKinesisVideoStream, callResult, streamArn));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Get streaming token result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(getStreamingTokenResult(pKinesisVideoStream, callResult, pToken, tokenSize, expiration));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Get streaming endpoint result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(getStreamingEndpointResult(pKinesisVideoStream, callResult, pEndpoint));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Put stream with endless payload API call result event
 */
STATUS putStreamResultEvent(UINT64 customData, SERVICE_CALL_RESULT callResult, UPLOAD_HANDLE streamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

    DLOGI("Put stream result event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(putStreamResult(pKinesisVideoStream, callResult, streamHandle));

CleanUp:
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
    UINT32 objectIdentifier;
    PKinesisVideoStream pKinesisVideoStream = NULL;
    PKinesisVideoClient pKinesisVideoClient = NULL;

    DLOGI("Tag resource result event.");

    CHK(IS_VALID_HANDLE((HANDLE)customData), STATUS_NULL_ARG);

    objectIdentifier = KINESIS_VIDEO_OBJECT_IDENTIFIER_FROM_CUSTOM_DATA(customData);

    // Check if it's a client
    if (objectIdentifier == KINESIS_VIDEO_OBJECT_IDENTIFIER_CLIENT) {
        pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

        // Call tagging result for client
        CHK_STATUS(tagClientResult(pKinesisVideoClient, callResult));
    } else {
        pKinesisVideoStream = FROM_STREAM_HANDLE(customData);

        // Validate it's a stream object
        CHK(pKinesisVideoStream->base.identifier == KINESIS_VIDEO_OBJECT_IDENTIFIER_STREAM, STATUS_INVALID_CUSTOM_DATA);

        // Call tagging result for stream
        CHK_STATUS(tagStreamResult(pKinesisVideoStream, callResult));
    }

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Kinesis Video stream terminated notification
 */
STATUS kinesisVideoStreamTerminated(STREAM_HANDLE streamHandle, UPLOAD_HANDLE streamUploadHandle, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGI("Stream terminated event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    CHK_STATUS(streamTerminatedEvent(pKinesisVideoStream, streamUploadHandle, callResult));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Stream fragment ACK event.");

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pFragmentAck != NULL, STATUS_NULL_ARG);

    CHK_STATUS(streamFragmentAckEvent(pKinesisVideoStream, uploadHandle, pFragmentAck));

CleanUp:
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
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(streamHandle);

    DLOGS("Parsing stream fragment ACK.");

    CHK(pKinesisVideoStream != NULL && ackSegment != NULL, STATUS_NULL_ARG);

    CHK_STATUS(parseFragmentAck(pKinesisVideoStream, uploadHandle, ackSegment, ackSegmentSize));

CleanUp:
    LEAVES();
    return retStatus;
}

//////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////

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
    PUploadHandleInfo pUploadHandleInfo;
    UPLOAD_HANDLE uploadHandle = INVALID_UPLOAD_HANDLE_VALUE;

    // Validate the input just in case
    CHK(pContentView != NULL && pViewItem != NULL && pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Notify the client about the dropped frame - only if we are removing the current view item
    // or the item that's been partially sent is being removed which was the one before the current.
    if (currentRemoved ||
            (pViewItem->handle == pKinesisVideoStream->curViewItem.viewItem.handle &&
                    pKinesisVideoStream->curViewItem.offset != pKinesisVideoStream->curViewItem.viewItem.length)) {
        DLOGW("Reporting a dropped frame/fragment.");

        // Invalidate the streams current view item
        MEMSET(&pKinesisVideoStream->curViewItem, 0x00, SIZEOF(CurrentViewItem));
        pKinesisVideoStream->curViewItem.viewItem.handle = INVALID_ALLOCATION_HANDLE_VALUE;

        switch (pKinesisVideoStream->streamInfo.streamCaps.streamingType) {
            case STREAMING_TYPE_REALTIME:
            case STREAMING_TYPE_OFFLINE:
                // The callback is optional so check if specified first
                if (pKinesisVideoClient->clientCallbacks.droppedFrameReportFn != NULL) {
                    CHK_STATUS(pKinesisVideoClient->clientCallbacks.droppedFrameReportFn(
                                       pKinesisVideoClient->clientCallbacks.customData,
                                       TO_STREAM_HANDLE(pKinesisVideoStream),
                                       pViewItem->timestamp));
                }

                break;

            case STREAMING_TYPE_NEAR_REALTIME:
                // The callback is optional so check if specified first
                if (pKinesisVideoClient->clientCallbacks.droppedFragmentReportFn != NULL) {
                    CHK_STATUS(pKinesisVideoClient->clientCallbacks.droppedFragmentReportFn(
                                       pKinesisVideoClient->clientCallbacks.customData,
                                       TO_STREAM_HANDLE(pKinesisVideoStream),
                                       pViewItem->timestamp));
                }

                break;

            default:
                // do nothing
                break;
        }
    }

CleanUp:

    if (pKinesisVideoClient != NULL)
    {
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
        curChar = randFn(customData) % (10 + 26);
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
