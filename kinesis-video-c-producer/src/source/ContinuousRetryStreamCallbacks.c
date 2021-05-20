/**
 * Kinesis Video Producer Continuous Retry Stream Callbacks
 */
#define LOG_CLASS "ContinuousRetryStreamCallbacks"
#include "Include_i.h"

STATUS createContinuousRetryStreamCallbacks(PClientCallbacks pCallbacksProvider, PStreamCallbacks* ppStreamCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = NULL;

    CHK(pCallbacksProvider != NULL && ppStreamCallbacks != NULL, STATUS_NULL_ARG);

    // Allocate the entire structure
    pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) MEMCALLOC(1, SIZEOF(ContinuousRetryStreamCallbacks));
    CHK(pContinuousRetryStreamCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pContinuousRetryStreamCallbacks->streamCallbacks.version = STREAM_CALLBACKS_CURRENT_VERSION;
    pContinuousRetryStreamCallbacks->streamCallbacks.customData = (UINT64) pContinuousRetryStreamCallbacks;

    // Store the back pointer as we will be using the other callbacks
    pContinuousRetryStreamCallbacks->pCallbacksProvider = (PCallbacksProvider) pCallbacksProvider;

    // Create the mapping table
    CHK_STATUS(hashTableCreateWithParams(STREAM_MAPPING_HASH_TABLE_BUCKET_COUNT, STREAM_MAPPING_HASH_TABLE_BUCKET_LENGTH,
                                         &pContinuousRetryStreamCallbacks->pStreamMapping));

    // Create the guard locks
    pContinuousRetryStreamCallbacks->syncLock = pContinuousRetryStreamCallbacks->pCallbacksProvider->clientCallbacks.createMutexFn(
        pContinuousRetryStreamCallbacks->pCallbacksProvider->clientCallbacks.customData, TRUE);

    // Set callbacks
    pContinuousRetryStreamCallbacks->streamCallbacks.streamConnectionStaleFn = continuousRetryStreamConnectionStaleHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.streamErrorReportFn = continuousRetryStreamErrorReportHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.streamLatencyPressureFn = continuousRetryStreamLatencyPressureHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.streamReadyFn = continuousRetryStreamReadyHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.streamShutdownFn = continuousRetryStreamShutdownHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.streamClosedFn = continuousRetryStreamClosedHandler;
    pContinuousRetryStreamCallbacks->streamCallbacks.freeStreamCallbacksFn = continuousRetryStreamFreeHandler;

    // Append to the stream callbacks chain
    CHK_STATUS(addStreamCallbacks(pCallbacksProvider, (PStreamCallbacks) pContinuousRetryStreamCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeContinuousRetryStreamCallbacks((PStreamCallbacks*) &pContinuousRetryStreamCallbacks);
        pContinuousRetryStreamCallbacks = NULL;
    }

    // Set the return value if it's not NULL
    if (ppStreamCallbacks != NULL) {
        *ppStreamCallbacks = (PStreamCallbacks) pContinuousRetryStreamCallbacks;
    }

    LEAVES();
    return retStatus;
}

STATUS freeContinuousRetryStreamCallbacks(PStreamCallbacks* ppStreamCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider;
    TID threadId;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = NULL;

    CHK(ppStreamCallbacks != NULL, STATUS_NULL_ARG);

    pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) *ppStreamCallbacks;

    // Call is idempotent
    CHK(pContinuousRetryStreamCallbacks != NULL, retStatus);

    pCallbacksProvider = pContinuousRetryStreamCallbacks->pCallbacksProvider;

    // Iterate every item in the mapping table and free
    CHK_STATUS(hashTableIterateEntries(pContinuousRetryStreamCallbacks->pStreamMapping, (UINT64) pContinuousRetryStreamCallbacks,
                                       removeMappingEntryCallback));

    // Free the stream handle mapping table
    hashTableFree(pContinuousRetryStreamCallbacks->pStreamMapping);

    // Free the locks
    if (IS_VALID_MUTEX_VALUE(pContinuousRetryStreamCallbacks->syncLock)) {
        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    }

    // Release the object
    MEMFREE(pContinuousRetryStreamCallbacks);

    // Set the pointer to NULL
    *ppStreamCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS removeMappingEntryCallback(UINT64 customData, PHashEntry pHashEntry)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    CHK(pContinuousRetryStreamCallbacks != NULL && pHashEntry != NULL, STATUS_INVALID_ARG);

    // The hash entry key is the stream handle
    CHK_STATUS(freeStreamMapping(pContinuousRetryStreamCallbacks, (STREAM_HANDLE) pHashEntry->key, FALSE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeStreamMapping(PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks, STREAM_HANDLE streamHandle, BOOL removeFromTable)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL tableLocked = FALSE;
    UINT64 value = 0;
    PCallbackStateMachine pCallbackStateMachine;
    CHK(pContinuousRetryStreamCallbacks != NULL && pContinuousRetryStreamCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pContinuousRetryStreamCallbacks->pCallbacksProvider;

    // Lock for exclusive operation
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    tableLocked = TRUE;

    // Get the entry if any
    retStatus = hashTableGet(pContinuousRetryStreamCallbacks->pStreamMapping, (UINT64) streamHandle, &value);

    CHK(retStatus == STATUS_HASH_KEY_NOT_PRESENT || retStatus == STATUS_SUCCESS, retStatus);
    if (retStatus == STATUS_HASH_KEY_NOT_PRESENT) {
        // Reset the status if not found
        retStatus = STATUS_SUCCESS;
    } else {
        pCallbackStateMachine = (PCallbackStateMachine) value;

        if (removeFromTable) {
            // Remove from the table only when we are not shutting down the entire curl API callbacks
            // as the entries will be removed by the shutdown process itself
            CHK_STATUS(hashTableRemove(pContinuousRetryStreamCallbacks->pStreamMapping, (UINT64) streamHandle));
        }

        // Await for the reset to finish first
        if (IS_VALID_TID_VALUE(pCallbackStateMachine->resetTid)) {
            THREAD_JOIN(pCallbackStateMachine->resetTid, NULL);
        }

        // Free the request object
        MEMFREE(pCallbackStateMachine);
    }

    // No longer need to hold the lock to the requests
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    tableLocked = FALSE;

CleanUp:

    // Unlock only if previously locked
    if (tableLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    }

    LEAVES();
    return retStatus;
}

STATUS getStreamMapping(PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks, STREAM_HANDLE streamHandle,
                        PCallbackStateMachine* ppCallbackStateMachine)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL tableLocked = FALSE, freeOnError = FALSE;
    UINT64 value = 0;
    PCallbackStateMachine pCallbackStateMachine = NULL;

    CHK(pContinuousRetryStreamCallbacks != NULL && pContinuousRetryStreamCallbacks->pCallbacksProvider != NULL && ppCallbackStateMachine != NULL,
        STATUS_INVALID_ARG);
    pCallbacksProvider = pContinuousRetryStreamCallbacks->pCallbacksProvider;

    // Lock for exclusive operation
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    tableLocked = TRUE;

    // Get the entry if any
    retStatus = hashTableGet(pContinuousRetryStreamCallbacks->pStreamMapping, (UINT64) streamHandle, &value);

    CHK(retStatus == STATUS_HASH_KEY_NOT_PRESENT || retStatus == STATUS_SUCCESS, retStatus);

    // Early return if exists
    if (STATUS_SUCCEEDED(retStatus)) {
        pCallbackStateMachine = (PCallbackStateMachine) value;
        CHK(FALSE, retStatus);
    }

    // Reset the status
    retStatus = STATUS_SUCCESS;

    pCallbackStateMachine = (PCallbackStateMachine) MEMCALLOC(1, SIZEOF(CallbackStateMachine));
    CHK(pCallbackStateMachine != NULL, STATUS_NOT_ENOUGH_MEMORY);
    freeOnError = TRUE;

    pCallbackStateMachine->pContinuousRetryStreamCallbacks = pContinuousRetryStreamCallbacks;
    pCallbackStateMachine->streamReady = FALSE;
    pCallbackStateMachine->resetTid = INVALID_TID_VALUE;
    pCallbackStateMachine->streamHandle = INVALID_STREAM_HANDLE_VALUE;

    // Set the initial values
    CHK_STATUS(setConnectionStaleStateMachine(pCallbackStateMachine, STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE, 0, 0, 0));
    CHK_STATUS(setStreamLatencyStateMachine(pCallbackStateMachine, STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE, 0, 0, 0));

    // Insert into the table
    CHK_STATUS(hashTablePut(pContinuousRetryStreamCallbacks->pStreamMapping, streamHandle, (UINT64) pCallbackStateMachine));

    // No longer need to hold the lock to the requests
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    tableLocked = FALSE;

CleanUp:

    // Unlock only if previously locked
    if (tableLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    }

    if (STATUS_SUCCEEDED(retStatus)) {
        *ppCallbackStateMachine = pCallbackStateMachine;
    } else if (freeOnError && pCallbackStateMachine != NULL) {
        MEMFREE(pCallbackStateMachine);
    }

    LEAVES();
    return retStatus;
}

STATUS continuousRetryStreamFreeHandler(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamCallbacks pStreamCallbacks;

    CHK(customData != NULL, STATUS_NULL_ARG);
    pStreamCallbacks = (PStreamCallbacks) *customData;
    CHK_STATUS(freeContinuousRetryStreamCallbacks(&pStreamCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS continuousRetryStreamConnectionStaleHandler(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 lastBufferingAck)
{
    UNUSED_PARAM(lastBufferingAck);
    STATUS retStatus = STATUS_SUCCESS;
    PCallbackStateMachine pCallbackStateMachine = NULL;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    CHK_STATUS(getStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, &pCallbackStateMachine));
    CHK_STATUS(connectionStaleStateMachineHandleConnectionStale(streamHandle, &pCallbackStateMachine->connectionStaleStateMachine));

CleanUp:

    return retStatus;
}

STATUS continuousRetryStreamErrorReportHandler(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, UINT64 erroredTimecode,
                                               STATUS statusCode)
{
    STATUS retStatus = STATUS_SUCCESS;
    BOOL locked = FALSE;
    TID threadId;
    PCallbacksProvider pCallbacksProvider;
    PCallbackStateMachine pCallbackStateMachine;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    // NOTE: STATUS_CONTINUOUS_RETRY_RESET_FAILED error raised by the continuous retry callback error handler reset
    // thread itself and in this case we should ignore it and let the potential application handler to deal with it
    CHK(statusCode != STATUS_CONTINUOUS_RETRY_RESET_FAILED, retStatus);

    CHK(pContinuousRetryStreamCallbacks != NULL && pContinuousRetryStreamCallbacks->pCallbacksProvider != NULL, STATUS_NULL_ARG);
    pCallbacksProvider = pContinuousRetryStreamCallbacks->pCallbacksProvider;

    DLOGW("Reporting stream error. Errored timecode: %" PRIu64 " Status: 0x%08x", erroredTimecode, statusCode);

    // If we have a C producer/Common library layer status codes then we handle it separately
    // from the status codes that are generated by the PIC
    if (!(IS_RETRIABLE_PRODUCER_ERROR(statusCode) || IS_RETRIABLE_COMMON_LIB_ERROR(statusCode))) {
        // return success if the sdk can recover from the error
        CHK(!IS_RECOVERABLE_ERROR(statusCode), retStatus);

        // Here, we check whether the status code is retriable by PIC and let it handle it
        CHK(IS_RETRIABLE_ERROR(statusCode), retStatus);
    }

    CHK_STATUS(getStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, &pCallbackStateMachine));

    // Check if we already have an ongoing reset process
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    locked = TRUE;

    // Early exit if we are in the midst of resetting already
    CHK(!IS_VALID_TID_VALUE(pCallbackStateMachine->resetTid), retStatus);

    // Run the reset in a separate thread
    pCallbackStateMachine->streamHandle = streamHandle;
    pCallbackStateMachine->uploadHandle = uploadHandle;
    pCallbackStateMachine->erroredTimecode = erroredTimecode;
    CHK_STATUS(THREAD_CREATE(&threadId, continuousRetryStreamRestartHandler, (PVOID) pCallbackStateMachine));
    pCallbackStateMachine->resetTid = threadId;

    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    locked = FALSE;

CleanUp:

    if (locked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pContinuousRetryStreamCallbacks->syncLock);
    }

    return retStatus;
}

STATUS continuousRetryStreamLatencyPressureHandler(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 currentBufferDuration)
{
    UNUSED_PARAM(currentBufferDuration);
    STATUS retStatus = STATUS_SUCCESS;
    PCallbackStateMachine pCallbackStateMachine = NULL;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    CHK_STATUS(getStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, &pCallbackStateMachine));
    CHK_STATUS(streamLatencyStateMachineHandleStreamLatency(streamHandle, &pCallbackStateMachine->streamLatencyStateMachine));

CleanUp:

    return retStatus;
}

STATUS continuousRetryStreamReadyHandler(UINT64 customData, STREAM_HANDLE streamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbackStateMachine pCallbackStateMachine = NULL;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    CHK_STATUS(getStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, &pCallbackStateMachine));

    pCallbackStateMachine->streamReady = TRUE;

CleanUp:

    return retStatus;
}

STATUS continuousRetryStreamShutdownHandler(UINT64 customData, STREAM_HANDLE streamHandle, BOOL restart)
{
    UNUSED_PARAM(restart);
    STATUS retStatus = STATUS_SUCCESS;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    if (!restart) {
        CHK_STATUS(freeStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, TRUE));
    }

CleanUp:

    return retStatus;
}

STATUS continuousRetryStreamClosedHandler(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle)
{
    UNUSED_PARAM(uploadHandle);
    STATUS retStatus = STATUS_SUCCESS;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks = (PContinuousRetryStreamCallbacks) customData;

    CHK_STATUS(freeStreamMapping(pContinuousRetryStreamCallbacks, streamHandle, TRUE));

CleanUp:

    return retStatus;
}

PVOID continuousRetryStreamRestartHandler(PVOID args)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo = NULL;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCallbackStateMachine pCallbackStateMachine = (PCallbackStateMachine) args;

    CHK(pCallbackStateMachine != NULL && pCallbackStateMachine->pContinuousRetryStreamCallbacks != NULL &&
            pCallbackStateMachine->pContinuousRetryStreamCallbacks->pCallbacksProvider != NULL,
        STATUS_NULL_ARG);

    pContinuousRetryStreamCallbacks = pCallbackStateMachine->pContinuousRetryStreamCallbacks;
    pCallbacksProvider = pContinuousRetryStreamCallbacks->pCallbacksProvider;
    streamHandle = pCallbackStateMachine->streamHandle;

    // do not reset if in offline mode. Let the application handle the retry
    CHK_STATUS(kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));
    CHK(!IS_OFFLINE_STREAMING_MODE(pStreamInfo->streamCaps.streamingType), retStatus);
    CHK_STATUS(kinesisVideoStreamResetStream(streamHandle));

CleanUp:

    if (pCallbacksProvider != NULL) {
        // In case of an error we will be calling the overall callback to notify the listening application
        if (STATUS_FAILED(retStatus)) {
            // Use the callback provider as the custom data
            // NOTE: This should be a prompt operation and shouldn't block the thread
            // NOTE: The handling application should perform it's own cleanup as continuous retry will not retry
            pCallbacksProvider->clientCallbacks.streamErrorReportFn((UINT64) pCallbacksProvider,
                                                                    pCallbackStateMachine->streamHandle,
                                                                    pCallbackStateMachine->uploadHandle,
                                                                    pCallbackStateMachine->erroredTimecode,
                                                                    STATUS_CONTINUOUS_RETRY_RESET_FAILED);
        }

        // Reset the running thread ID just before returning
        pCallbackStateMachine->resetTid = INVALID_TID_VALUE;
    }

    LEAVES();
    return (PVOID)(ULONG_PTR) retStatus;
}
