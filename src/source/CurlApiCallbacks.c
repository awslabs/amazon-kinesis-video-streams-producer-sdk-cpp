/**
 * Kinesis Video Producer CURL based API Callbacks
 */
#define LOG_CLASS "CurlApiCallbacks"
#include "Include_i.h"

/**
 * Creates CURL based API callbacks provider
 */
STATUS createCurlApiCallbacks(PCallbacksProvider pCallbacksProvider, PCHAR region, BOOL cachingEndpoint,
                              UINT64 endpointCachingPeriod, PCHAR controlPlaneUrl, PCHAR certPath,
                              PCHAR userAgentNamePostfix, PCHAR customUserAgent, PCurlApiCallbacks* ppCurlApiCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, status;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;

    CHK(pCallbacksProvider != NULL && ppCurlApiCallbacks != NULL, STATUS_NULL_ARG);
    CHK(certPath == NULL || STRNLEN(certPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_INVALID_CERT_PATH_LENGTH);
    CHK(customUserAgent == NULL || STRNLEN(customUserAgent, MAX_CUSTOM_USER_AGENT_LEN + 1) <= MAX_CUSTOM_USER_AGENT_LEN,
        STATUS_MAX_CUSTOM_USER_AGENT_LEN_EXCEEDED);
    CHK(userAgentNamePostfix == NULL ||
        STRNLEN(userAgentNamePostfix, MAX_CUSTOM_USER_AGENT_NAME_POSTFIX_LEN + 1) <= MAX_CUSTOM_USER_AGENT_NAME_POSTFIX_LEN,
        STATUS_MAX_USER_AGENT_NAME_POSTFIX_LEN_EXCEEDED);

    CHK(endpointCachingPeriod <= MAX_ENDPOINT_CACHE_UPDATE_PERIOD, STATUS_INVALID_ENDPOINT_CACHING_PERIOD);

    // Fix-up the endpoint TTL value
    if (endpointCachingPeriod == ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE) {
        endpointCachingPeriod = DEFAULT_ENDPOINT_CACHE_UPDATE_PERIOD;
    }

    // Allocate the entire structure
    pCurlApiCallbacks = (PCurlApiCallbacks) MEMCALLOC(1, SIZEOF(CurlApiCallbacks));
    CHK(pCurlApiCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pCurlApiCallbacks->apiCallbacks.version = API_CALLBACKS_CURRENT_VERSION;
    pCurlApiCallbacks->apiCallbacks.customData = (UINT64) pCurlApiCallbacks;
    pCurlApiCallbacks->streamingRequestCount = 0;

    // Endpoint period will be the same for all streams
    pCurlApiCallbacks->cacheUpdatePeriod = endpointCachingPeriod;

    // Set invalid guard locks
    pCurlApiCallbacks->activeRequestsLock = INVALID_MUTEX_VALUE;
    pCurlApiCallbacks->activeUploadsLock = INVALID_MUTEX_VALUE;
    pCurlApiCallbacks->cachedEndpointsLock = INVALID_MUTEX_VALUE;

    // Store the back pointer as we will be using the other callbacks
    pCurlApiCallbacks->pCallbacksProvider = pCallbacksProvider;

    // Set the callbacks
    if (cachingEndpoint) {
        pCurlApiCallbacks->apiCallbacks.createStreamFn = createStreamCachingCurl;
        pCurlApiCallbacks->apiCallbacks.describeStreamFn = describeStreamCachingCurl;
        pCurlApiCallbacks->apiCallbacks.getStreamingEndpointFn = getStreamingEndpointCachingCurl;
        pCurlApiCallbacks->apiCallbacks.tagResourceFn = tagResourceCachingCurl;
    } else {
        pCurlApiCallbacks->apiCallbacks.createStreamFn = createStreamCurl;
        pCurlApiCallbacks->apiCallbacks.describeStreamFn = describeStreamCurl;
        pCurlApiCallbacks->apiCallbacks.getStreamingEndpointFn = getStreamingEndpointCurl;
        pCurlApiCallbacks->apiCallbacks.tagResourceFn = tagResourceCurl;
    }

    pCurlApiCallbacks->apiCallbacks.createDeviceFn = createDeviceCurl;
    pCurlApiCallbacks->apiCallbacks.putStreamFn = putStreamCurl;
    pCurlApiCallbacks->apiCallbacks.freeApiCallbacksFn = freeApiCallbacksCurl;

    if (region == NULL || region[0] == '\0') {
        STRCPY(pCurlApiCallbacks->region, DEFAULT_AWS_REGION);
    } else {
        STRNCPY(pCurlApiCallbacks->region, region, MAX_REGION_NAME_LEN);
        pCurlApiCallbacks->region[MAX_REGION_NAME_LEN] = '\0';
    }

    status = getUserAgentString(userAgentNamePostfix, customUserAgent, MAX_USER_AGENT_LEN, pCurlApiCallbacks->userAgent);
    pCurlApiCallbacks->userAgent[MAX_USER_AGENT_LEN] = '\0';
    if (STATUS_FAILED(status)) {
        DLOGW("Failed to generate user agent string with error 0x%08x.", status);
    }

    // Set the control plane URL
    if (controlPlaneUrl == NULL || controlPlaneUrl[0] == '\0') {
        // Create a fully qualified URI
        SNPRINTF(pCurlApiCallbacks->controlPlaneUrl,
                 MAX_URI_CHAR_LEN,
                 "%s%s.%s%s",
                 CONTROL_PLANE_URI_PREFIX,
                 KINESIS_VIDEO_SERVICE_NAME,
                 pCurlApiCallbacks->region,
                 CONTROL_PLANE_URI_POSTFIX);
    } else {
        STRNCPY(pCurlApiCallbacks->controlPlaneUrl, controlPlaneUrl, MAX_URI_CHAR_LEN);
    }

    // NULL terminate in case we overrun
    pCurlApiCallbacks->controlPlaneUrl[MAX_URI_CHAR_LEN] = '\0';

    if (certPath != NULL) {
        STRNCPY(pCurlApiCallbacks->certPath, certPath, MAX_PATH_LEN);
    } else {
        // empty cert path
        pCurlApiCallbacks->certPath[0] = '\0';
    }

    // Create the tracking ongoing requests
    CHK_STATUS(hashTableCreateWithParams(STREAM_MAPPING_HASH_TABLE_BUCKET_COUNT, STREAM_MAPPING_HASH_TABLE_BUCKET_LENGTH, &pCurlApiCallbacks->pActiveRequests));
    CHK_STATUS(doubleListCreate(&pCurlApiCallbacks->pActiveUploads));

    // Create the hash table for tracking endpoints
    CHK_STATUS(hashTableCreateWithParams(STREAM_MAPPING_HASH_TABLE_BUCKET_COUNT, STREAM_MAPPING_HASH_TABLE_BUCKET_LENGTH, &pCurlApiCallbacks->pCachedEndpoints));

    // Create the guard locks
    pCurlApiCallbacks->activeUploadsLock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, TRUE);
    CHK(pCurlApiCallbacks->activeUploadsLock != INVALID_MUTEX_VALUE , STATUS_INVALID_OPERATION);
    pCurlApiCallbacks->activeRequestsLock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, TRUE);
    CHK(pCurlApiCallbacks->activeRequestsLock != INVALID_MUTEX_VALUE , STATUS_INVALID_OPERATION);
    pCurlApiCallbacks->cachedEndpointsLock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, TRUE);
    CHK(pCurlApiCallbacks->cachedEndpointsLock != INVALID_MUTEX_VALUE , STATUS_INVALID_OPERATION);

#if !defined __WINDOWS_BUILD__
    signal(SIGPIPE, SIG_IGN);
#endif

    // Testability hooks initialization
    pCurlApiCallbacks->hookCustomData = 0;
    pCurlApiCallbacks->curlEasyPerformHookFn = NULL;
    pCurlApiCallbacks->curlWriteCallbackHookFn = NULL;
    pCurlApiCallbacks->curlReadCallbackHookFn = NULL;
    // end testability hooks initialization

    // CURL global initialization
    CHK(0 == curl_global_init(CURL_GLOBAL_ALL), STATUS_CURL_LIBRARY_INIT_FAILED);

    // Not in shutdown
    pCurlApiCallbacks->shutdown = FALSE;

    // Prepare the Stream callbacks
    pCurlApiCallbacks->streamCallbacks.version = STREAM_CALLBACKS_CURRENT_VERSION;
    pCurlApiCallbacks->streamCallbacks.customData = (UINT64) pCurlApiCallbacks;
    pCurlApiCallbacks->streamCallbacks.streamDataAvailableFn = dataAvailableCurl;
    pCurlApiCallbacks->streamCallbacks.streamClosedFn = streamClosedCurl;
    pCurlApiCallbacks->streamCallbacks.streamErrorReportFn = NULL;
    pCurlApiCallbacks->streamCallbacks.fragmentAckReceivedFn = fragmentAckReceivedCurl;
    pCurlApiCallbacks->streamCallbacks.streamShutdownFn = shutdownStreamCurl;

    // Prepare the Producer callbacks
    pCurlApiCallbacks->producerCallbacks.version = PRODUCER_CALLBACKS_CURRENT_VERSION;
    pCurlApiCallbacks->producerCallbacks.customData = (UINT64) pCurlApiCallbacks;
    pCurlApiCallbacks->producerCallbacks.clientShutdownFn = clientShutdownCurl;

    // Append to the API callback chain
    CHK_STATUS(addApiCallbacks((PClientCallbacks) pCallbacksProvider, (PApiCallbacks) pCurlApiCallbacks));

    // Append the stream callbacks to the Stream callback chain
    CHK_STATUS(addStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pCurlApiCallbacks->streamCallbacks));

    // Append the producer callbacks to the Producer callback chain
    CHK_STATUS(addProducerCallbacks((PClientCallbacks) pCallbacksProvider, &pCurlApiCallbacks->producerCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeCurlApiCallbacks(&pCurlApiCallbacks);
        pCurlApiCallbacks = NULL;
    }

    // Set the return value if it's not NULL
    if (ppCurlApiCallbacks != NULL) {
        *ppCurlApiCallbacks = pCurlApiCallbacks;
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the CURL API callbacks object
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 * NOTE: The call is idempotent
 */
STATUS freeCurlApiCallbacks(PCurlApiCallbacks* ppCurlApiCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider;

    CHK(ppCurlApiCallbacks != NULL, STATUS_NULL_ARG);

    pCurlApiCallbacks = *ppCurlApiCallbacks;

    // Call is idempotent
    CHK(pCurlApiCallbacks != NULL, retStatus);

    // At this stage we should have the callback provider set
    CHK(pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Shutdown the CURL API callbacks with minimal wait
    curlApiCallbacksShutdown(pCurlApiCallbacks, CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT);

    // Release the auxiliary structures
    hashTableFree(pCurlApiCallbacks->pActiveRequests);
    doubleListFree(pCurlApiCallbacks->pActiveUploads);
    hashTableFree(pCurlApiCallbacks->pCachedEndpoints);

    // Free the locks
    if (pCurlApiCallbacks->activeRequestsLock != INVALID_MUTEX_VALUE) {
        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    }

    if (pCurlApiCallbacks->activeUploadsLock != INVALID_MUTEX_VALUE) {
        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    }

    if (pCurlApiCallbacks->cachedEndpointsLock != INVALID_MUTEX_VALUE) {
        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->cachedEndpointsLock);
    }

    // Global release of CURL object
    curl_global_cleanup();

    // Release the object
    MEMFREE(pCurlApiCallbacks);

    // Set the pointer to NULL
    *ppCurlApiCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeApiCallbacksCurl(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks;

    CHK(customData != NULL, STATUS_NULL_ARG);
    pCurlApiCallbacks = (PCurlApiCallbacks) *customData;
    CHK_STATUS(freeCurlApiCallbacks(&pCurlApiCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

/*
 * curlApiCallbacksShutdown terminates and free all active requests, active upload handles, and cached endpoints across
 * all streams. After curlApiCallbacksShutdown, all threads originated from curApiCallbacks are expected to be terminated.
 * curlApiCallbacksShutdown is idempotent.
 */
STATUS curlApiCallbacksShutdown(PCurlApiCallbacks pCurlApiCallbacks, UINT64 timeout)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL requestsLocked = FALSE, endpointsLocked = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Set the shutdown state first
    pCurlApiCallbacks->shutdown = TRUE;
    pCurlApiCallbacks->shutdownTimeout = timeout;

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestsLocked = TRUE;

    // Iterate every item in the map and terminate the stream
    CHK_STATUS(hashTableIterateEntries(pCurlApiCallbacks->pActiveRequests, (UINT64) pCurlApiCallbacks,
                                       curlApiCallbacksActiveRequestsTableShutdownCallback));
    CHK_STATUS(hashTableClear(pCurlApiCallbacks->pActiveRequests));

    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestsLocked = FALSE;

    // Remove and terminate all ongoing uploads
    CHK_STATUS(curlApiCallbacksShutdownActiveUploads(pCurlApiCallbacks, INVALID_STREAM_HANDLE_VALUE,
                                                     INVALID_UPLOAD_HANDLE_VALUE, timeout, FALSE));


    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->cachedEndpointsLock);
    endpointsLocked = TRUE;
    // Iterate every item in the cached endpoints and free
    CHK_STATUS(hashTableIterateEntries(pCurlApiCallbacks->pCachedEndpoints, (UINT64) pCurlApiCallbacks,
                                       curlApiCallbacksCachedEndpointsTableShutdownCallback));
    CHK_STATUS(hashTableClear(pCurlApiCallbacks->pCachedEndpoints));

    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->cachedEndpointsLock);
    endpointsLocked = FALSE;

CleanUp:

    // Unlock only if previously locked
    if (requestsLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    }

    if (endpointsLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->cachedEndpointsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksActiveRequestsTableShutdownCallback(UINT64 customData, PHashEntry pHashEntry)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pHashEntry != NULL, STATUS_INVALID_ARG);

    // The hash entry key is the stream handle
    CHK_STATUS(curlApiCallbacksShutdownActiveRequests(pCurlApiCallbacks,
                                                      (STREAM_HANDLE) pHashEntry->key,
                                                      pCurlApiCallbacks->shutdownTimeout,
                                                      FALSE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksCachedEndpointsTableShutdownCallback(UINT64 customData, PHashEntry pHashEntry)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pHashEntry != NULL, STATUS_INVALID_ARG);

    // The hash entry key is the stream handle
    CHK_STATUS(curlApiCallbacksShutdownCachedEndpoints(pCurlApiCallbacks,
                                                       (STREAM_HANDLE) pHashEntry->key,
                                                       FALSE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksFreeRequest(PCurlRequest pCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pCurlRequest != NULL, STATUS_INVALID_ARG);

    // Set the TID as invalid
    pCurlRequest->threadId = INVALID_TID_VALUE;

    // The hash entry key is the stream handle
    CHK_STATUS(curlApiCallbacksShutdownActiveRequests(pCurlRequest->pCurlApiCallbacks,
                                                      pCurlRequest->streamHandle,
                                                      CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                                      TRUE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksShutdownActiveRequests(PCurlApiCallbacks pCurlApiCallbacks,
                                              STREAM_HANDLE streamHandle,
                                              UINT64 timeout,
                                              BOOL removeFromTable)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL requestsLocked = FALSE;
    UINT64 value;
    PCurlRequest pCurlRequest;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for exclusive operation
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestsLocked = TRUE;

    // Get the entry if any
    retStatus = hashTableGet(pCurlApiCallbacks->pActiveRequests, (UINT64) streamHandle, &value);

    CHK(retStatus == STATUS_HASH_KEY_NOT_PRESENT || retStatus == STATUS_SUCCESS, retStatus);

    if (retStatus == STATUS_HASH_KEY_NOT_PRESENT) {
        // Reset the status if not found
        retStatus = STATUS_SUCCESS;
    } else {
        // We have the request
        pCurlRequest = (PCurlRequest) value;

        // Check if the request is already being terminated. If it is then early exit
        CHK(!pCurlRequest->requestInfo.terminating, retStatus);

        // Set the terminating status on the request
        pCurlRequest->requestInfo.terminating = TRUE;

        // Proceed to termination if not completed
        if (!pCurlRequest->pCurlResponse->terminated) {

            // Terminate the curl request in flight
            terminateCurlSession(pCurlRequest->pCurlResponse, timeout);

            // Kill the thread if needed
            if (IS_VALID_TID_VALUE(pCurlRequest->threadId)) {
                THREAD_CANCEL(pCurlRequest->threadId);
            }
        }

        if (removeFromTable) {
            // Remove from the table only when we are not shutting down the entire curl API callbacks
            // as the entries will be removed by the shutdown process itself
            CHK_STATUS(hashTableRemove(pCurlApiCallbacks->pActiveRequests, (UINT64) streamHandle));
        }

        // Free the request object
        CHK_STATUS(freeCurlRequest(&pCurlRequest));
    }

    // No longer need to hold the lock to the requests
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestsLocked = FALSE;

CleanUp:

    // Unlock only if previously locked
    if (requestsLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksShutdownCachedEndpoints(PCurlApiCallbacks pCurlApiCallbacks,
                                               STREAM_HANDLE streamHandle,
                                               BOOL removeFromTable)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL endpointLocked = FALSE;
    UINT64 value = 0;
    PEndpointTracker pEndpointTracker;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for exclusive operation
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                    pCurlApiCallbacks->cachedEndpointsLock);
    endpointLocked = TRUE;

    // Get the entry if any
    retStatus = hashTableGet(pCurlApiCallbacks->pCachedEndpoints, (UINT64) streamHandle, &value);

    CHK(retStatus == STATUS_HASH_KEY_NOT_PRESENT || retStatus == STATUS_SUCCESS, retStatus);

    if (retStatus == STATUS_HASH_KEY_NOT_PRESENT) {
        // Reset the status if not found
        retStatus = STATUS_SUCCESS;
    } else {
        // We have the endpoint tracker
        pEndpointTracker = (PEndpointTracker) value;

        if (removeFromTable) {
            // Remove from the table only when we are not shutting down the entire curl API callbacks
            // as the entries will be removed by the shutdown process itself
            CHK_STATUS(hashTableRemove(pCurlApiCallbacks->pCachedEndpoints, (UINT64) streamHandle));
        }

        // Free the request object
        MEMFREE(pEndpointTracker);
    }

    // No longer need to hold the lock to the requests
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                      pCurlApiCallbacks->cachedEndpointsLock);
    endpointLocked = FALSE;

CleanUp:

    // Unlock only if previously locked
    if (endpointLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->cachedEndpointsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS curlApiCallbacksShutdownActiveUploads(PCurlApiCallbacks pCurlApiCallbacks, STREAM_HANDLE streamHandle,
                                             UPLOAD_HANDLE uploadHandle, UINT64 timeout, BOOL terminateCurlOnly)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL uploadsLocked = FALSE;
    PDoubleListNode pCurNode, pNode;
    PCurlRequest pCurlRequest;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    uploadsLocked = TRUE;

    // Iterate through the list and find the requests matching the stream
    CHK_STATUS(doubleListGetHeadNode(pCurlApiCallbacks->pActiveUploads, &pNode));
    while (pNode != NULL) {
        pCurlRequest = (PCurlRequest) pNode->data;

        // Get the next node before processing
        pCurNode = pNode;
        CHK_STATUS(doubleListGetNextNode(pNode, &pNode));

        // Process the curl request ONLY if the stream handle matches or the specified handle is invalid,
        // it's not already in terminating state
        // and if the uploadHandle is invalid or is equal to the current
        if (!pCurlRequest->requestInfo.terminating &&
            (!IS_VALID_STREAM_HANDLE(streamHandle) || pCurlRequest->streamHandle == streamHandle) &&
            (!IS_VALID_UPLOAD_HANDLE(uploadHandle) || uploadHandle == pCurlRequest->uploadHandle)) {

            // Terminate the curl request in flight
            terminateCurlSession(pCurlRequest->pCurlResponse, timeout);

            if (!terminateCurlOnly) {
                // Set the terminating status on the request
                pCurlRequest->requestInfo.terminating = TRUE;
                // Kill the thread if needed
                if (IS_VALID_TID_VALUE(pCurlRequest->threadId)) {
                    THREAD_CANCEL(pCurlRequest->threadId);
                }

                // Free the request
                CHK_STATUS(freeCurlRequest(&pCurlRequest));

                // Remove and delete from the list
                CHK_STATUS(doubleListDeleteNode(pCurlApiCallbacks->pActiveUploads, pCurNode));
            }
        }
    }

CleanUp:

    // Unlock only if previously locked
    if (uploadsLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS notifyCallResult(PCallbacksProvider pCallbacksProvider, STATUS status, STREAM_HANDLE streamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    // Check if we need to do anything
    CHK(status != STATUS_SUCCESS && status != STATUS_NULL_ARG, retStatus);

    // Check if the callback is present
    CHK(pCallbacksProvider->clientCallbacks.streamErrorReportFn != NULL, retStatus);

    // Notify the aggregate callback functionality
    CHK_STATUS(pCallbacksProvider->clientCallbacks.streamErrorReportFn(pCallbacksProvider->clientCallbacks.customData,
                                                                       streamHandle,
                                                                       INVALID_UPLOAD_HANDLE_VALUE,
                                                                       0,
                                                                       status));

CleanUp:

    LEAVES();
    return retStatus;
}

STREAM_STATUS getStreamStatusFromString(PCHAR status, UINT32 length)
{
    // Assume the stream Deleting status first
    STREAM_STATUS streamStatus = STREAM_STATUS_DELETING;

    if (0 == STRNCMP((PCHAR) "ACTIVE", status, length)) {
        streamStatus = STREAM_STATUS_ACTIVE;
    } else if (0 == STRNCMP((PCHAR) "CREATING", status, length)) {
        streamStatus = STREAM_STATUS_CREATING;
    } else if (0 == STRNCMP((PCHAR) "UPDATING", status, length)) {
        streamStatus = STREAM_STATUS_UPDATING;
    } else if (0 == STRNCMP((PCHAR) "DELETING", status, length)) {
        streamStatus = STREAM_STATUS_DELETING;
    }

    return streamStatus;
}

STATUS curlCallApi(PCurlRequest pCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlResponse pCurlResponse;

    CHK(pCurlRequest != NULL, STATUS_INVALID_ARG);
    CHK(pCurlRequest->pCurlApiCallbacks != NULL && pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_OPERATION);
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Complete the call synchronously
    CHK_STATUS(curlCompleteSync(pCurlResponse));

CleanUp:

    LEAVES();
    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Producer Callbacks function implementations
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS clientShutdownCurl(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UNUSED_PARAM(clientHandle);

    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    // Bail out if already shutdown
    CHK(!pCurlApiCallbacks->shutdown, retStatus);

    // Set the shutdown state first
    pCurlApiCallbacks->shutdown = TRUE;
    pCurlApiCallbacks->shutdownTimeout = CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT;

CleanUp:

    LEAVES();
    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Callbacks function implementations
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS shutdownStreamCurl(UINT64 customData, STREAM_HANDLE streamHandle, BOOL resetStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    // terminate curl for all active uploads. Then block in the condition variable waiting for curl to return control back
    // to us. Even if stopStreamSync is called first before freeStream, this could potentially terminate curl prematurely
    // and cause curl to throw timeout error, but everything is already sent and persisted in KVS if stopStreamSync returned
    // successfully.
    CHK_STATUS(curlApiCallbacksShutdownActiveUploads(pCurlApiCallbacks,
                                                     streamHandle,
                                                     INVALID_UPLOAD_HANDLE_VALUE,
                                                     CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                                     TRUE));

    // Shutdown active requests
    CHK_STATUS(curlApiCallbacksShutdownActiveRequests(pCurlApiCallbacks,
                                                      streamHandle,
                                                      CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                                      TRUE));
    // Shutdown active uploads
    CHK_STATUS(curlApiCallbacksShutdownActiveUploads(pCurlApiCallbacks,
                                                     streamHandle,
                                                     INVALID_UPLOAD_HANDLE_VALUE,
                                                     CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                                     FALSE));

    // Clear out the cached endpoints
    CHK_STATUS(curlApiCallbacksShutdownCachedEndpoints(pCurlApiCallbacks,
                                                       streamHandle,
                                                       TRUE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS dataAvailableCurl(UINT64 customData, STREAM_HANDLE streamHandle, PCHAR streamName, UPLOAD_HANDLE uploadHandle,
                         UINT64 durationAvailable, UINT64 bytesAvailable)
{
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(streamName);

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = NULL;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL locked = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    locked = TRUE;

    CHK_STATUS(findRequestWithUploadHandle(uploadHandle, pCurlApiCallbacks, &pCurlRequest));
    CHK(pCurlRequest != NULL, retStatus);

    // Early return if it's been shutdown
    CHK(!pCurlRequest->pCurlResponse->endOfStream && !pCurlRequest->requestInfo.terminating, retStatus);

    CHK_STATUS(notifyDataAvailable(pCurlRequest->pCurlResponse, durationAvailable, bytesAvailable));

CleanUp:

    if (locked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS streamClosedCurl(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle)
{
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(uploadHandle);

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;

    // Shutdown active requests.
    // NOTE: There shouldn't really be any outstanding requests during streaming. The only case is
    // if we end up with upload handles in NEW states while terminating.
    CHK_STATUS(curlApiCallbacksShutdownActiveRequests(pCurlApiCallbacks,
                                                      streamHandle,
                                                      CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                                      TRUE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fragmentAckReceivedCurl(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle,
                               PFragmentAck pFragmentAck)
{
    UNUSED_PARAM(streamHandle);

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = NULL;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL locked = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    locked = TRUE;

    CHK_STATUS(findRequestWithUploadHandle(uploadHandle, pCurlApiCallbacks, &pCurlRequest));
    CHK(pCurlRequest != NULL, retStatus);

    // Early return if it's been shutdown
    CHK(!pCurlRequest->pCurlResponse->endOfStream, retStatus);

    // Early return if not persisted ACK
    CHK(pFragmentAck->ackType == FRAGMENT_ACK_TYPE_PERSISTED, retStatus);

    // Un-pause the curl reader thread
    CHK_STATUS(notifyDataAvailable(pCurlRequest->pCurlResponse, 0, 0));

CleanUp:

    if (locked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    }

    LEAVES();
    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// API Callbacks function implementations
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS createDeviceCurl(UINT64 customData, PCHAR deviceName, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL requestLocked = FALSE;

    UNUSED_PARAM(deviceName);

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestLocked = TRUE;

    // INFO: Currently, no create device API is defined in the backend.
    // Notify PIC with the result only if we haven't been force terminated
    if (!pCurlApiCallbacks->shutdown) {
        CHK_STATUS(createDeviceResultEvent(pServiceCallContext->customData, SERVICE_CALL_RESULT_OK, DUMMY_DEVICE_ARN));
    }

CleanUp:

    // Unlock only if previously locked
    if (requestLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS createStreamCurl(UINT64 customData, PCHAR deviceName, PCHAR streamName, PCHAR contentType,
                        PCHAR kmsKeyId, UINT64 retentionPeriod, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CHAR kmsKey[MAX_JSON_KMS_KEY_ID_STRING_LEN];
    CHAR paramsJson[MAX_JSON_PARAMETER_STRING_LEN];
    CHAR url[MAX_PATH_LEN + 1];
    UINT64 retentionInHours, currentTime;
    PAwsCredentials pCredentials = NULL;
    TID threadId = INVALID_TID_VALUE;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlRequest pCurlRequest = NULL;
    BOOL startLocked = FALSE, requestLocked = FALSE, streamShutdown = FALSE, incrementedOngoingOperation = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Prepare the JSON string for the KMS param.
    // NOTE: the kms key id is optional
    if (kmsKeyId != NULL && kmsKeyId[0] != '\0') {
        SNPRINTF(kmsKey, ARRAY_SIZE(kmsKey), KMS_KEY_PARAM_JSON_TEMPLATE, kmsKeyId);
    } else {
        // Empty string
        kmsKey[0] = '\0';
    }

    // Expressed in hours
    retentionInHours = (UINT64) (retentionPeriod / HUNDREDS_OF_NANOS_IN_AN_HOUR);
    SNPRINTF(paramsJson, ARRAY_SIZE(paramsJson), CREATE_STREAM_PARAM_JSON_TEMPLATE,
             deviceName, streamName, contentType, kmsKey, retentionInHours);

    // Validate the credentials
    CHK_STATUS(deserializeAwsCredentials(pServiceCallContext->pAuthInfo->data));
    pCredentials = (PAwsCredentials) pServiceCallContext->pAuthInfo->data;
    CHK(pCredentials->version <= AWS_CREDENTIALS_CURRENT_VERSION, STATUS_INVALID_AWS_CREDENTIALS_VERSION);
    CHK(pCredentials->size == pServiceCallContext->pAuthInfo->size, STATUS_INTERNAL_ERROR);

    // Create the API url
    STRCPY(url, pCurlApiCallbacks->controlPlaneUrl);
    STRCAT(url, CREATE_API_POSTFIX);

    // Create a request object
    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(createCurlRequest(HTTP_REQUEST_VERB_POST, url, paramsJson, (STREAM_HANDLE) pServiceCallContext->customData,
                                 pCurlApiCallbacks->region, currentTime,
                                 CURL_API_DEFAULT_CONNECTION_TIMEOUT, pServiceCallContext->timeout,
                                 pServiceCallContext->callAfter, pCurlApiCallbacks->certPath, pCredentials,
                                 pCurlApiCallbacks, &pCurlRequest));

    // Set the necessary headers
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "user-agent", 0, pCurlApiCallbacks->userAgent, 0));

    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestLocked = TRUE;

    // Lock the startup mutex so the created thread will wait until we are done with bookkeeping
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    startLocked = TRUE;

    // Start the request/response thread
    CHK_STATUS(THREAD_CREATE(&threadId, createStreamCurlHandler, (PVOID) pCurlRequest));
    CHK_STATUS(THREAD_DETACH(threadId));

    // Set the thread ID in the request, add to the hash table
    pCurlRequest->threadId = threadId;

    CHK_STATUS(hashTablePut(pCurlApiCallbacks->pActiveRequests, pServiceCallContext->customData, (UINT64) pCurlRequest));

CleanUp:

    if (STATUS_FAILED(retStatus) || streamShutdown) {
        if (IS_VALID_TID_VALUE(threadId)) {
            THREAD_CANCEL(threadId);
        }

        freeCurlRequest(&pCurlRequest);
    } else if (startLocked) {
        // Release the lock to let the awaiting handler thread to continue
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    }

    if (requestLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS createStreamCachingCurl(UINT64 customData, PCHAR deviceName, PCHAR streamName, PCHAR contentType,
                               PCHAR kmsKeyId, UINT64 retentionPeriod, PServiceCallContext pServiceCallContext)
{
    ENTERS();

    UNUSED_PARAM(deviceName);
    UNUSED_PARAM(contentType);
    UNUSED_PARAM(kmsKeyId);
    UNUSED_PARAM(retentionPeriod);

    STATUS retStatus = STATUS_SUCCESS;
    STREAM_HANDLE streamHandle;
    BOOL streamShutdown = FALSE;

    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    DLOGV("No-op CreateStream API call");

    streamHandle = (STREAM_HANDLE) pServiceCallContext->customData;

    retStatus = createStreamResultEvent(streamHandle, SERVICE_CALL_RESULT_OK, streamName);

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

CleanUp:

    LEAVES();
    return retStatus;
}

PVOID createStreamCurlHandler(PVOID arg)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) arg;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlResponse pCurlResponse = NULL;
    UINT64 curTime;
    CHAR streamArn[MAX_ARN_LEN + 1];
    PCHAR pResponseStr;
    jsmn_parser parser;
    jsmntok_t tokens[MAX_JSON_TOKEN_COUNT];
    UINT32 i, strLen, resultLen;
    INT32 tokenCount;
    BOOL stopLoop, streamShutdown = FALSE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    SERVICE_CALL_RESULT callResult = SERVICE_CALL_RESULT_NOT_SET;

    CHECK(pCurlRequest != NULL &&
          pCurlRequest->pCurlApiCallbacks != NULL &&
          pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL &&
          pCurlRequest->pCurlResponse != NULL);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;
    streamArn[0] = '\0';

    // Acquire and release the startup lock to ensure the startup sequence is clear
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);

    // Sign the request
    CHK_STATUS(signAwsRequestInfo(&pCurlRequest->requestInfo));

    // Wait for the specified amount of time before calling the API
    if (pCurlRequest->requestInfo.currentTime < pCurlRequest->requestInfo.callAfter) {
        THREAD_SLEEP(pCurlRequest->requestInfo.callAfter - pCurlRequest->requestInfo.currentTime);
    }

    // Execute the request
    CHK_STATUS(curlCallApi(pCurlRequest));

    // Set the response object
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Get the response
    CHK(pCurlResponse->callInfo.callResult != SERVICE_CALL_RESULT_NOT_SET, STATUS_INVALID_OPERATION);
    pResponseStr = pCurlResponse->callInfo.responseData;
    resultLen = pCurlResponse->callInfo.responseDataLen;

    DLOGD("CreateStream API response: %.*s", resultLen, pResponseStr);

    // Parse the response
    jsmn_init(&parser);
    tokenCount = jsmn_parse(&parser, pResponseStr, resultLen, tokens, SIZEOF(tokens) / SIZEOF(jsmntok_t));
    CHK(tokenCount > 1, STATUS_INVALID_API_CALL_RETURN_JSON);
    CHK(tokens[0].type == JSMN_OBJECT, STATUS_INVALID_API_CALL_RETURN_JSON);
    for (i = 1, stopLoop = FALSE; i < tokenCount && !stopLoop; i++) {
        if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "StreamARN")) {
            strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(strLen <= MAX_ARN_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            STRNCPY(streamArn, pResponseStr + tokens[i + 1].start, strLen);
            streamArn[strLen] = '\0';
            i++;

            // No need to iterate further
            stopLoop = TRUE;
        }
    }

CleanUp:

    // Preserve the values as we need to free the request before the event notification
    if (pCurlRequest->pCurlResponse != NULL) {
        callResult = pCurlRequest->pCurlResponse->callInfo.callResult;
    }

    // Set the thread id just before returning
    pCurlRequest->threadId = INVALID_TID_VALUE;

    streamHandle = pCurlRequest->streamHandle;

    // Free the request object
    curlApiCallbacksFreeRequest(pCurlRequest);

    retStatus = createStreamResultEvent(streamHandle, callResult, streamArn);

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

    LEAVES();

    // Returning STATUS as PVOID casting first to ptr type to avoid compiler warnings on 64bit platforms.
    return (PVOID) (ULONG_PTR) retStatus;
}

STATUS describeStreamCurl(UINT64 customData, PCHAR streamName, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CHAR paramsJson[MAX_JSON_PARAMETER_STRING_LEN];
    CHAR url[MAX_PATH_LEN + 1];
    PAwsCredentials pCredentials = NULL;
    TID threadId = INVALID_TID_VALUE;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCurlRequest pCurlRequest = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL startLocked = FALSE, requestLocked = FALSE, streamShutdown = FALSE, incrementedOngoingOperation = FALSE;
    UINT64 currentTime;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Prepare the json params for the call
    SNPRINTF(paramsJson, ARRAY_SIZE(paramsJson), DESCRIBE_STREAM_PARAM_JSON_TEMPLATE, streamName);

    // Validate the credentials
    CHK_STATUS(deserializeAwsCredentials(pServiceCallContext->pAuthInfo->data));
    pCredentials = (PAwsCredentials) pServiceCallContext->pAuthInfo->data;
    CHK(pCredentials->version <= AWS_CREDENTIALS_CURRENT_VERSION, STATUS_INVALID_AWS_CREDENTIALS_VERSION);
    CHK(pCredentials->size == pServiceCallContext->pAuthInfo->size, STATUS_INTERNAL_ERROR);

    // Create the API url
    STRCPY(url, pCurlApiCallbacks->controlPlaneUrl);
    STRCAT(url, DESCRIBE_API_POSTFIX);

    // Create a request object
    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(createCurlRequest(HTTP_REQUEST_VERB_POST, url, paramsJson, (STREAM_HANDLE) pServiceCallContext->customData,
                                 pCurlApiCallbacks->region, currentTime,
                                 CURL_API_DEFAULT_CONNECTION_TIMEOUT, pServiceCallContext->timeout,
                                 pServiceCallContext->callAfter, pCurlApiCallbacks->certPath, pCredentials,
                                 pCurlApiCallbacks, &pCurlRequest));

    // Set the necessary headers
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "user-agent", 0, pCurlApiCallbacks->userAgent, 0));

    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestLocked = TRUE;

    // Lock the startup mutex so the created thread will wait until we are done with bookkeeping
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    startLocked = TRUE;

    // Start the request/response thread
    CHK_STATUS(THREAD_CREATE(&threadId, describeStreamCurlHandler, (PVOID) pCurlRequest));
    CHK_STATUS(THREAD_DETACH(threadId));

    // Set the thread ID in the request, add to the hash table
    pCurlRequest->threadId = threadId;
    CHK_STATUS(hashTablePut(pCurlApiCallbacks->pActiveRequests, pServiceCallContext->customData, (UINT64) pCurlRequest));

CleanUp:

    if (STATUS_FAILED(retStatus) || streamShutdown) {
        if (IS_VALID_TID_VALUE(threadId)) {
            THREAD_CANCEL(threadId);
        }

        freeCurlRequest(&pCurlRequest);
    } else if (startLocked) {
        // Release the lock to let the awaiting handler thread to continue
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlRequest->startLock);
    }

    if (requestLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->activeRequestsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS describeStreamCachingCurl(UINT64 customData, PCHAR streamName, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    STREAM_HANDLE streamHandle;
    StreamDescription streamDescription;
    PStreamInfo pStreamInfo;
    BOOL streamShutdown = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    streamHandle = (STREAM_HANDLE) pServiceCallContext->customData;

    // Get the stream info from the stream handle
    CHK_STATUS(kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));

    // Null out the fields before processing
    MEMSET(&streamDescription, 0x00, SIZEOF(StreamDescription));
    streamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
    STRCPY(streamDescription.deviceName, "DEVICE_NAME");
    STRCPY(streamDescription.contentType, pStreamInfo->streamCaps.contentType);
    STRCPY(streamDescription.updateVersion, "VERSION");
    STRCPY(streamDescription.kmsKeyId, pStreamInfo->kmsKeyId);
    STRNCPY(streamDescription.streamName, streamName, MAX_STREAM_NAME_LEN);
    STRNCPY(streamDescription.streamArn, streamName, MAX_ARN_LEN);
    streamDescription.retention = pStreamInfo->retention;
    streamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    streamDescription.creationTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);

    DLOGV("No-op DescribeStream API call");
    retStatus = describeStreamResultEvent(streamHandle, SERVICE_CALL_RESULT_OK, &streamDescription);

    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

CleanUp:

    LEAVES();
    return retStatus;
}

PVOID describeStreamCurlHandler(PVOID arg)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) arg;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlResponse pCurlResponse = NULL;
    PCHAR pResponseStr;
    jsmn_parser parser;
    jsmntok_t tokens[MAX_JSON_TOKEN_COUNT];
    UINT32 i, strLen, resultLen;
    INT32 tokenCount;
    UINT64 retention;
    BOOL jsonInStreamDescription = FALSE, streamShutdown = FALSE;
    StreamDescription streamDescription;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    SERVICE_CALL_RESULT callResult = SERVICE_CALL_RESULT_NOT_SET;

    CHECK(pCurlRequest != NULL &&
          pCurlRequest->pCurlApiCallbacks != NULL &&
          pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL &&
          pCurlRequest->pCurlResponse != NULL);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Acquire and release the startup lock to ensure the startup sequence is clear
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);

    // Sign the request
    CHK_STATUS(signAwsRequestInfo(&pCurlRequest->requestInfo));

    // Wait for the specified amount of time before calling the API
    if (pCurlRequest->requestInfo.currentTime < pCurlRequest->requestInfo.callAfter) {
        THREAD_SLEEP(pCurlRequest->requestInfo.callAfter - pCurlRequest->requestInfo.currentTime);
    }

    // Execute the request
    CHK_STATUS(curlCallApi(pCurlRequest));

    // Set the response object
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Get the response
    CHK(pCurlResponse->callInfo.callResult != SERVICE_CALL_RESULT_NOT_SET, STATUS_INVALID_OPERATION);
    pResponseStr = pCurlResponse->callInfo.responseData;
    resultLen = pCurlResponse->callInfo.responseDataLen;

    DLOGD("DescribeStream API response: %.*s", resultLen, pResponseStr);

    // skip json parsing if call result not ok
    CHK(pCurlResponse->callInfo.callResult == SERVICE_CALL_RESULT_OK && resultLen != 0 && pResponseStr != NULL, retStatus);

    // Parse the response
    jsmn_init(&parser);
    tokenCount = jsmn_parse(&parser, pResponseStr, resultLen, tokens, SIZEOF(tokens) / SIZEOF(jsmntok_t));
    CHK(tokenCount > 1, STATUS_INVALID_API_CALL_RETURN_JSON);
    CHK(tokens[0].type == JSMN_OBJECT, STATUS_INVALID_API_CALL_RETURN_JSON);

    // Null out the fields before processing
    MEMSET(&streamDescription, 0x00, SIZEOF(StreamDescription));

    // Loop through the tokens and extract the stream description
    for (i = 1; i < tokenCount; i++) {
        if (!jsonInStreamDescription) {
            if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "StreamInfo")) {
                streamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
                jsonInStreamDescription = TRUE;
                i++;
            }
        } else {
            if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "DeviceName")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_DEVICE_NAME_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.deviceName, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.deviceName[MAX_DEVICE_NAME_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "MediaType")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_CONTENT_TYPE_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.contentType, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.contentType[MAX_CONTENT_TYPE_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "KmsKeyId")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_ARN_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.kmsKeyId, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.kmsKeyId[MAX_ARN_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "StreamARN")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_ARN_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.streamArn, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.streamArn[MAX_ARN_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "StreamName")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_STREAM_NAME_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.streamName, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.streamName[MAX_STREAM_NAME_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "Version")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_UPDATE_VERSION_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                STRNCPY(streamDescription.updateVersion, pResponseStr + tokens[i + 1].start, strLen);
                streamDescription.updateVersion[MAX_UPDATE_VERSION_LEN] = '\0';
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "Status")) {
                strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
                CHK(strLen <= MAX_DESCRIBE_STREAM_STATUS_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
                streamDescription.streamStatus = getStreamStatusFromString(pResponseStr + tokens[i + 1].start, strLen);
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "DataRetentionInHours")) {
                CHK_STATUS(STRTOUI64(pResponseStr + tokens[i + 1].start, pResponseStr + tokens[i + 1].end, 10, &retention));

                // NOTE: Retention value is in hours
                streamDescription.retention = retention * HUNDREDS_OF_NANOS_IN_AN_HOUR;
                i++;
            } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "CreationTime")) {
                // TODO: In the future parse out the creation time but currently we don't need it
                i++;
            }
        }
    }

CleanUp:

    // Preserve the values as we need to free the request before the event notification
    if (pCurlRequest->pCurlResponse != NULL) {
        callResult = pCurlRequest->pCurlResponse->callInfo.callResult;
    }

    // Set the thread id just before returning
    pCurlRequest->threadId = INVALID_TID_VALUE;

    streamHandle = pCurlRequest->streamHandle;

    // Free the request object
    curlApiCallbacksFreeRequest(pCurlRequest);

    if (callResult == SERVICE_CALL_RESULT_OK && !jsonInStreamDescription) {
        // Notify PIC with the invalid result
        describeStreamResultEvent(streamHandle, SERVICE_CALL_INVALID_ARG, &streamDescription);

        // Overwrite the result with more precise info
        retStatus = STATUS_INVALID_DESCRIBE_STREAM_RETURN_JSON;
    } else {
        // Notify PIC with the result
        retStatus = describeStreamResultEvent(streamHandle, callResult, &streamDescription);
    }

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

    LEAVES();

    // Returning STATUS as PVOID casting first to ptr type to avoid compiler warnings on 64bit platforms.
    return (PVOID) (ULONG_PTR) retStatus;
}

STATUS getStreamingEndpointCurl(UINT64 customData, PCHAR streamName, PCHAR apiName,
                                PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CHAR paramsJson[MAX_JSON_PARAMETER_STRING_LEN];
    CHAR url[MAX_PATH_LEN + 1];
    PAwsCredentials pCredentials = NULL;
    TID threadId = INVALID_TID_VALUE;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCurlRequest pCurlRequest = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL startLocked = FALSE, requestLocked = FALSE, streamShutdown = FALSE, incrementedOngoingOperation = FALSE;
    UINT64 currentTime;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Prepare the json params for the call
    SNPRINTF(paramsJson, ARRAY_SIZE(paramsJson), GET_DATA_ENDPOINT_PARAM_JSON_TEMPLATE, streamName, apiName);

    // Validate the credentials
    CHK_STATUS(deserializeAwsCredentials(pServiceCallContext->pAuthInfo->data));
    pCredentials = (PAwsCredentials) pServiceCallContext->pAuthInfo->data;
    CHK(pCredentials->version <= AWS_CREDENTIALS_CURRENT_VERSION, STATUS_INVALID_AWS_CREDENTIALS_VERSION);
    CHK(pCredentials->size == pServiceCallContext->pAuthInfo->size, STATUS_INTERNAL_ERROR);

    // Create the API url
    STRCPY(url, pCurlApiCallbacks->controlPlaneUrl);
    STRCAT(url, GET_DATA_ENDPOINT_API_POSTFIX);

    // Create a request object
    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(createCurlRequest(HTTP_REQUEST_VERB_POST, url, paramsJson, (STREAM_HANDLE) pServiceCallContext->customData,
                                 pCurlApiCallbacks->region, currentTime,
                                 CURL_API_DEFAULT_CONNECTION_TIMEOUT, pServiceCallContext->timeout,
                                 pServiceCallContext->callAfter, pCurlApiCallbacks->certPath, pCredentials,
                                 pCurlApiCallbacks, &pCurlRequest));

    // Set the necessary headers
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "user-agent", 0, pCurlApiCallbacks->userAgent, 0));

    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestLocked = TRUE;

    // Lock the startup mutex so the created thread will wait until we are done with bookkeeping
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    startLocked = TRUE;

    // Start the request/response thread
    CHK_STATUS(THREAD_CREATE(&threadId, getStreamingEndpointCurlHandler, (PVOID) pCurlRequest));
    CHK_STATUS(THREAD_DETACH(threadId));

    // Set the thread ID in the request, add to the hash table
    pCurlRequest->threadId = threadId;
    CHK_STATUS(hashTablePut(pCurlApiCallbacks->pActiveRequests, pServiceCallContext->customData, (UINT64) pCurlRequest));

CleanUp:

    if (STATUS_FAILED(retStatus) || streamShutdown) {
        if (IS_VALID_TID_VALUE(threadId)) {
            THREAD_CANCEL(threadId);
        }

        freeCurlRequest(&pCurlRequest);
    } else if (startLocked) {
        // Release the lock to let the awaiting handler thread to continue
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    }

    if (requestLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->activeRequestsLock);
    }

    LEAVES();
    return retStatus;
}

STATUS getStreamingEndpointCachingCurl(UINT64 customData, PCHAR streamName,
                                       PCHAR apiName, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    UINT64 curTime, value;
    STREAM_HANDLE streamHandle;
    PEndpointTracker pEndpointTracker = NULL;
    BOOL refreshEndpoint = TRUE, endpointLocked = FALSE, streamShutdown = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    streamHandle = (STREAM_HANDLE) pServiceCallContext->customData;
    curTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);

    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                    pCurlApiCallbacks->cachedEndpointsLock);
    endpointLocked = TRUE;

    // Attempt to retrieve the cached value
    retStatus = hashTableGet(pCurlApiCallbacks->pCachedEndpoints, (UINT64) streamHandle, &value);

    CHK(retStatus == STATUS_HASH_KEY_NOT_PRESENT || retStatus == STATUS_SUCCESS, retStatus);

    if (retStatus == STATUS_HASH_KEY_NOT_PRESENT) {
        // Reset the status if not found
        retStatus = STATUS_SUCCESS;
    } else {
        pEndpointTracker = (PEndpointTracker) value;

        if (pEndpointTracker != NULL &&
            pEndpointTracker->streamingEndpoint[0] != '\0' &&
            pEndpointTracker->endpointLastUpdateTime + pCurlApiCallbacks->cacheUpdatePeriod > curTime) {
            refreshEndpoint = FALSE;
        }
    }

    // Check if we have cached value and if the TTL is still valid
    if (refreshEndpoint) {
        // No longer need to hold the endpoint lock
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->cachedEndpointsLock);
        endpointLocked = FALSE;

        // Delegate the call to the main routine
        CHK_STATUS(getStreamingEndpointCurl(customData, streamName, apiName, pServiceCallContext));
    } else {
        DLOGV("Caching GetStreamingEndpoint API call");
        retStatus = getStreamingEndpointResultEvent(streamHandle, SERVICE_CALL_RESULT_OK,
                pEndpointTracker->streamingEndpoint);

        notifyCallResult(pCallbacksProvider, retStatus, streamHandle);
    }

CleanUp:

    if (endpointLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->cachedEndpointsLock);
    }

    LEAVES();
    return retStatus;
}

PVOID getStreamingEndpointCurlHandler(PVOID arg)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) arg;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlResponse pCurlResponse = NULL;
    PEndpointTracker pEndpointTracker = NULL;
    UINT64 value;
    PCHAR pResponseStr;
    jsmn_parser parser;
    jsmntok_t tokens[MAX_JSON_TOKEN_COUNT];
    UINT32 i, strLen, resultLen;
    INT32 tokenCount;
    BOOL stopLoop, streamShutdown = FALSE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    SERVICE_CALL_RESULT callResult = SERVICE_CALL_RESULT_NOT_SET;
    CHAR streamingEndpoint[MAX_URI_CHAR_LEN + 1];


    CHECK(pCurlRequest != NULL &&
          pCurlRequest->pCurlApiCallbacks != NULL &&
          pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL &&
          pCurlRequest->pCurlResponse != NULL);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Acquire and release the startup lock to ensure the startup sequence is clear
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);

    // Sign the request
    CHK_STATUS(signAwsRequestInfo(&pCurlRequest->requestInfo));

    // Wait for the specified amount of time before calling the API
    if (pCurlRequest->requestInfo.currentTime < pCurlRequest->requestInfo.callAfter) {
        THREAD_SLEEP(pCurlRequest->requestInfo.callAfter - pCurlRequest->requestInfo.currentTime);
    }

    // Execute the request
    CHK_STATUS(curlCallApi(pCurlRequest));

    // Set the response object
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Get the response
    CHK(pCurlResponse->callInfo.callResult != SERVICE_CALL_RESULT_NOT_SET, STATUS_INVALID_OPERATION);
    pResponseStr = pCurlResponse->callInfo.responseData;
    resultLen = pCurlResponse->callInfo.responseDataLen;

    DLOGD("GetStreamingEndpoint API response: %.*s", resultLen, pResponseStr);

    // Parse the response
    jsmn_init(&parser);
    tokenCount = jsmn_parse(&parser, pResponseStr, resultLen, tokens, SIZEOF(tokens) / SIZEOF(jsmntok_t));
    CHK(tokenCount > 1, STATUS_INVALID_API_CALL_RETURN_JSON);
    CHK(tokens[0].type == JSMN_OBJECT, STATUS_INVALID_API_CALL_RETURN_JSON);

    // Loop through the tokens and extract the endpoint
    for (i = 1, stopLoop = FALSE; i < tokenCount && !stopLoop; i++) {
        if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "DataEndpoint")) {
            strLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(strLen <= MAX_URI_CHAR_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            STRNCPY(streamingEndpoint, pResponseStr + tokens[i + 1].start, strLen);
            streamingEndpoint[strLen] = '\0';
            i++;

            // No need to iterate further
            stopLoop = TRUE;
        }
    }

    // Make sure we found the token
    CHK(stopLoop, STATUS_GET_STREAMING_ENDPOINT_CALL_FAILED);

CleanUp:

    // We need to store the endpoint in the cache
    if (STATUS_SUCCEEDED(retStatus)) {
        pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                        pCurlApiCallbacks->cachedEndpointsLock);

        // Attempt to retrieve the cached value
        retStatus = hashTableGet(pCurlApiCallbacks->pCachedEndpoints, (UINT64) pCurlRequest->streamHandle, &value);
        pEndpointTracker = (PEndpointTracker) value;

        if (STATUS_FAILED(retStatus) || pEndpointTracker == NULL) {
            // Create new tracker and insert in the table
            pEndpointTracker = (PEndpointTracker) MEMALLOC(SIZEOF(EndpointTracker));

            if (pEndpointTracker != NULL) {
                // Insert into the table
                retStatus = hashTablePut(pCurlApiCallbacks->pCachedEndpoints, (UINT64) pCurlRequest->streamHandle,
                                         (UINT64) pEndpointTracker);
            }
        }

        if (pEndpointTracker != NULL) {
            STRNCPY(pEndpointTracker->streamingEndpoint, streamingEndpoint, MAX_URI_CHAR_LEN);
            pEndpointTracker->streamingEndpoint[MAX_URI_CHAR_LEN] = '\0';
            pEndpointTracker->endpointLastUpdateTime = pCurlRequest->requestInfo.currentTime;
        }

        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->cachedEndpointsLock);
    }

    // Preserve the values as we need to free the request before the event notification
    if (pCurlRequest->pCurlResponse != NULL) {
        callResult = pCurlRequest->pCurlResponse->callInfo.callResult;
    }

    // Set the thread id just before returning
    pCurlRequest->threadId = INVALID_TID_VALUE;

    streamHandle = pCurlRequest->streamHandle;

    // Free the request object
    curlApiCallbacksFreeRequest(pCurlRequest);
    retStatus = getStreamingEndpointResultEvent(streamHandle, callResult, streamingEndpoint);

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

    LEAVES();

    // Returning STATUS as PVOID casting first to ptr type to avoid compiler warnings on 64bit platforms.
    return (PVOID) (ULONG_PTR) retStatus;
}

STATUS tagResourceCurl(UINT64 customData, PCHAR streamArn, UINT32 tagCount, PTag tags, PServiceCallContext pServiceCallContext)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR paramsJson = NULL;
    PCHAR tagsJson = NULL;
    CHAR url[MAX_PATH_LEN + 1];
    PAwsCredentials pCredentials;
    TID threadId = INVALID_TID_VALUE;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCurlRequest pCurlRequest = NULL;
    UINT32 i;
    INT32 charsCopied;
    PCHAR pCurPtr;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL startLocked = FALSE, requestLocked = FALSE, streamShutdown = FALSE, incrementedOngoingOperation = FALSE;
    UINT64 currentTime;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    CHK(tagCount > 0 && tags != NULL, STATUS_INTERNAL_ERROR);

    // Allocate enough space for the string manipulation. We don't want to reserve stack space for this
    CHK(NULL != (paramsJson = (PCHAR) MEMALLOC(MAX_TAGS_JSON_PARAMETER_STRING_LEN)), STATUS_NOT_ENOUGH_MEMORY);
    CHK(NULL != (tagsJson = (PCHAR) MEMALLOC(MAX_TAGS_JSON_PARAMETER_STRING_LEN)), STATUS_NOT_ENOUGH_MEMORY);

    // Prepare the tags elements
    for (i = 0, pCurPtr = tagsJson; i < tagCount; i++) {
        charsCopied = SNPRINTF(pCurPtr, MAX_TAGS_JSON_PARAMETER_STRING_LEN - (pCurPtr - tagsJson), TAG_PARAM_JSON_TEMPLATE, tags[i].name, tags[i].value);
        CHK(charsCopied > 0, STATUS_INTERNAL_ERROR);
        pCurPtr += charsCopied;
    }

    // Remove the tailing comma
    *(pCurPtr - 1) = '\0';

    // Prepare the json params for the call
    SNPRINTF(paramsJson, MAX_TAGS_JSON_PARAMETER_STRING_LEN, TAG_RESOURCE_PARAM_JSON_TEMPLATE, streamArn, tagsJson);

    // Validate the credentials
    CHK_STATUS(deserializeAwsCredentials(pServiceCallContext->pAuthInfo->data));
    pCredentials = (PAwsCredentials) pServiceCallContext->pAuthInfo->data;
    CHK(pCredentials->version <= AWS_CREDENTIALS_CURRENT_VERSION, STATUS_INVALID_AWS_CREDENTIALS_VERSION);
    CHK(pCredentials->size == pServiceCallContext->pAuthInfo->size, STATUS_INTERNAL_ERROR);

    // Create the API url
    STRCPY(url, pCurlApiCallbacks->controlPlaneUrl);
    STRCAT(url, TAG_RESOURCE_API_POSTFIX);

    // Create a request object
    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(createCurlRequest(HTTP_REQUEST_VERB_POST, url, paramsJson, (STREAM_HANDLE) pServiceCallContext->customData,
                                 pCurlApiCallbacks->region, currentTime,
                                 CURL_API_DEFAULT_CONNECTION_TIMEOUT, pServiceCallContext->timeout,
                                 pServiceCallContext->callAfter, pCurlApiCallbacks->certPath, pCredentials,
                                 pCurlApiCallbacks, &pCurlRequest));

    // Set the necessary headers
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "user-agent", 0, pCurlApiCallbacks->userAgent, 0));

    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeRequestsLock);
    requestLocked = TRUE;

    // Lock the startup mutex so the created thread will wait until we are done with bookkeeping
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    startLocked = TRUE;

    // Start the request/response thread
    CHK_STATUS(THREAD_CREATE(&threadId, tagResourceCurlHandler, (PVOID) pCurlRequest));
    CHK_STATUS(THREAD_DETACH(threadId));

    // Set the thread ID in the request, add to the hash table
    pCurlRequest->threadId = threadId;
    CHK_STATUS(hashTablePut(pCurlApiCallbacks->pActiveRequests, pServiceCallContext->customData, (UINT64) pCurlRequest));

CleanUp:

    if (STATUS_FAILED(retStatus) || streamShutdown) {
        if (IS_VALID_TID_VALUE(threadId)) {
            THREAD_CANCEL(threadId);
        }

        freeCurlRequest(&pCurlRequest);
    } else if (startLocked) {
        // Release the lock to let the awaiting handler thread to continue
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    }

    if (requestLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->activeRequestsLock);
    }

    if (paramsJson != NULL) {
        MEMFREE(paramsJson);
    }

    if (tagsJson != NULL) {
        MEMFREE(tagsJson);
    }

    LEAVES();
    return retStatus;
}

STATUS tagResourceCachingCurl(UINT64 customData, PCHAR streamArn, UINT32 tagCount, PTag tags,
                              PServiceCallContext pServiceCallContext)
{
    ENTERS();
    UNUSED_PARAM(streamArn);
    UNUSED_PARAM(tagCount);
    UNUSED_PARAM(tags);

    STATUS retStatus = STATUS_SUCCESS;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCallbacksProvider pCallbacksProvider = NULL;
    STREAM_HANDLE streamHandle;
    BOOL streamShutdown = FALSE;

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    streamHandle = (STREAM_HANDLE) pServiceCallContext->customData;
    retStatus = tagResourceResultEvent(streamHandle, SERVICE_CALL_RESULT_OK);

    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

CleanUp:

    LEAVES();
    return retStatus;
}

PVOID tagResourceCurlHandler(PVOID arg)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, status = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) arg;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlResponse pCurlResponse = NULL;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    SERVICE_CALL_RESULT callResult = SERVICE_CALL_RESULT_NOT_SET;
    BOOL streamShutdown = FALSE;

    CHECK(pCurlRequest != NULL &&
          pCurlRequest->pCurlApiCallbacks != NULL &&
          pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL &&
          pCurlRequest->pCurlResponse != NULL);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Acquire and release the startup lock to ensure the startup sequence is clear
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);

    // Sign the request
    CHK_STATUS(signAwsRequestInfo(&pCurlRequest->requestInfo));

    // Wait for the specified amount of time before calling the API
    if (pCurlRequest->requestInfo.currentTime < pCurlRequest->requestInfo.callAfter) {
        THREAD_SLEEP(pCurlRequest->requestInfo.callAfter - pCurlRequest->requestInfo.currentTime);
    }

    // Execute the request
    CHK_STATUS(curlCallApi(pCurlRequest));

    // Set the response object
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Get the response
    CHK(pCurlResponse->callInfo.callResult != SERVICE_CALL_RESULT_NOT_SET, STATUS_INVALID_OPERATION);

CleanUp:

    // Preserve the values as we need to free the request before the event notification
    if (pCurlRequest->pCurlResponse != NULL) {
        callResult = pCurlRequest->pCurlResponse->callInfo.callResult;
    }

    streamHandle = pCurlRequest->streamHandle;

    // Set the thread id just before returning
    pCurlRequest->threadId = INVALID_TID_VALUE;

    // Free the request object
    curlApiCallbacksFreeRequest(pCurlRequest);
    status = tagResourceResultEvent(streamHandle, callResult);

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, status, streamHandle);

    LEAVES();

    // Returning STATUS as PVOID casting first to ptr type to avoid compiler warnings on 64bit platforms.
    return (PVOID) (ULONG_PTR) retStatus;
}

STATUS putStreamCurl(UINT64 customData, PCHAR streamName, PCHAR containerType, UINT64 startTimestamp,
                     BOOL absoluteFragmentTimestamp, BOOL acksEnabled, PCHAR streamingEndpoint,
                     PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(containerType);

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, status = STATUS_SUCCESS;
    CHAR url[MAX_PATH_LEN + 1];
    CHAR startTimestampStr[MAX_TIMESTAMP_STR_LEN + 1] = {0};
    PAwsCredentials pCredentials = NULL;
    TID threadId = INVALID_TID_VALUE;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) customData;
    PCurlRequest pCurlRequest = NULL;
    UINT64 startTimestampMillis, currentTime;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL startLocked = FALSE, uploadsLocked = FALSE, streamShutdown = FALSE, incrementedOngoingOperation = FALSE;

    CHECK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL && pServiceCallContext != NULL);
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Validate the credentials
    CHK_STATUS(deserializeAwsCredentials(pServiceCallContext->pAuthInfo->data));
    pCredentials = (PAwsCredentials) pServiceCallContext->pAuthInfo->data;
    CHK(pCredentials->version <= AWS_CREDENTIALS_CURRENT_VERSION, STATUS_INVALID_AWS_CREDENTIALS_VERSION);
    CHK(pCredentials->size == pServiceCallContext->pAuthInfo->size, STATUS_INTERNAL_ERROR);

    // Create the API url
    STRCPY(url, streamingEndpoint);
    STRCAT(url, PUT_MEDIA_API_POSTFIX);

    // Lock for the guards for exclusive access
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlApiCallbacks->activeUploadsLock);
    uploadsLocked = TRUE;

    // Create a request object
    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(createCurlRequest(HTTP_REQUEST_VERB_POST, url, NULL, (STREAM_HANDLE) pServiceCallContext->customData,
                                 pCurlApiCallbacks->region, currentTime,
                                 CURL_API_DEFAULT_CONNECTION_TIMEOUT, pServiceCallContext->timeout,
                                 pServiceCallContext->callAfter, pCurlApiCallbacks->certPath, pCredentials,
                                 pCurlApiCallbacks, &pCurlRequest));

    // Set the necessary headers
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "user-agent", 0, pCurlApiCallbacks->userAgent, 0));
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "x-amzn-stream-name", 0, streamName, 0));
    startTimestampMillis = startTimestamp / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    SNPRINTF(startTimestampStr, MAX_TIMESTAMP_STR_LEN + 1, (PCHAR) "%" PRIu64 ".%" PRIu64, startTimestampMillis / 1000, startTimestampMillis % 1000);
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "x-amzn-producer-start-timestamp", 0, startTimestampStr, 0));
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "x-amzn-fragment-acknowledgment-required", 0, (PCHAR) (acksEnabled ? "1" : "0"), 0));
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "x-amzn-fragment-timecode-type", 0, (PCHAR) (absoluteFragmentTimestamp ? "ABSOLUTE" : "RELATIVE"), 0));
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "transfer-encoding", 0, (PCHAR) "chunked", 0));
    CHK_STATUS(setRequestHeader(&pCurlRequest->requestInfo, (PCHAR) "connection", 0, (PCHAR) "keep-alive", 0));

    // Lock the startup mutex so the created thread will wait until we are done with bookkeeping
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    startLocked = TRUE;

    // Start the request/response thread
    CHK_STATUS(THREAD_CREATE(&threadId, putStreamCurlHandler, (PVOID) pCurlRequest));
    CHK_STATUS(THREAD_DETACH(threadId));

    // Set the thread ID in the request, add to the hash table
    pCurlRequest->threadId = threadId;

    CHK_STATUS(doubleListInsertItemTail(pCurlApiCallbacks->pActiveUploads, (UINT64) pCurlRequest));

CleanUp:

    if (STATUS_FAILED(retStatus) || streamShutdown) {
        if (IS_VALID_TID_VALUE(threadId)) {
            THREAD_CANCEL(threadId);
        }

        freeCurlRequest(&pCurlRequest);
    }

    // unlock in case if locked.
    if (uploadsLocked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                          pCurlApiCallbacks->activeUploadsLock);
    }

    if (STATUS_SUCCEEDED(retStatus) && !streamShutdown) {
        status = putStreamResultEvent(pCurlRequest->streamHandle,
                                      STATUS_FAILED(retStatus) ? SERVICE_CALL_UNKNOWN : SERVICE_CALL_RESULT_OK,
                                      pCurlRequest->uploadHandle);

        if (startLocked) {
            // Release the lock to let the awaiting handler thread to continue
            pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData,
                                                              pCurlRequest->startLock);
        }
    }

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, status, (STREAM_HANDLE) pServiceCallContext->customData);

    return retStatus;
}

PVOID putStreamCurlHandler(PVOID arg)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) arg;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlResponse pCurlResponse = NULL;
    BOOL endOfStream = FALSE, streamShutdown = FALSE;
    SERVICE_CALL_RESULT callResult = SERVICE_CALL_RESULT_NOT_SET;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    UPLOAD_HANDLE uploadHandle = INVALID_UPLOAD_HANDLE_VALUE;

    CHECK(pCurlRequest != NULL &&
          pCurlRequest->pCurlApiCallbacks != NULL &&
          pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL &&
          pCurlRequest->pCurlResponse != NULL);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Acquire and release the startup lock to ensure the startup sequence is clear
    pCallbacksProvider->clientCallbacks.lockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);

    // Sign the request
    CHK_STATUS(signAwsRequestInfo(&pCurlRequest->requestInfo));

    // Wait for the specified amount of time before calling the API
    if (pCurlRequest->requestInfo.currentTime < pCurlRequest->requestInfo.callAfter) {
        THREAD_SLEEP(pCurlRequest->requestInfo.callAfter - pCurlRequest->requestInfo.currentTime);
    }

    // Execute the request
    CHK_STATUS(curlCallApi(pCurlRequest));

    // Set the response object
    pCurlResponse = pCurlRequest->pCurlResponse;

    // Get the response
    CHK(pCurlResponse->callInfo.callResult != SERVICE_CALL_RESULT_NOT_SET, STATUS_INVALID_OPERATION);

    DLOGD("Network thread for Kinesis Video stream: %s with upload handle: %" PRIu64 " exited. http status: %u",
          pCurlRequest->pStreamInfo->name, pCurlRequest->uploadHandle, pCurlResponse->callInfo.httpStatus);

CleanUp:

    // Set the thread id just before returning
    // Ensure we set the response in case we ended up with an error and
    // the local variable is not yet set.
    if (pCurlRequest->pCurlResponse != NULL) {
        endOfStream = pCurlRequest->pCurlResponse->endOfStream;
        callResult = pCurlRequest->pCurlResponse->callInfo.callResult;
    }

    streamHandle = pCurlRequest->streamHandle;
    uploadHandle = pCurlRequest->uploadHandle;
    pCurlRequest->threadId = INVALID_TID_VALUE;

    // Free the request object
    curlApiCallbacksShutdownActiveUploads(pCurlApiCallbacks,
                                          streamHandle,
                                          uploadHandle,
                                          CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT,
                                          FALSE);

    if (!endOfStream) {
        DLOGW("Stream with streamHandle %" PRIu64 " has exited without triggering end-of-stream. Service call result: %u",
                streamHandle, callResult);
        kinesisVideoStreamTerminated(streamHandle, uploadHandle, callResult);
    }

    // Bubble the notification to potential listeners
    notifyCallResult(pCallbacksProvider, retStatus, streamHandle);

    LEAVES();
    // Returning STATUS as PVOID casting first to ptr type to avoid compiler warnings on 64bit platforms.
    return (PVOID) (ULONG_PTR) retStatus;
}

// Accquire activeUploads lock before calling this function!!!
STATUS findRequestWithUploadHandle(UPLOAD_HANDLE uploadHandle, PCurlApiCallbacks pCurlApiCallbacks,
                                   PCurlRequest *ppCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL requestFound = FALSE;
    PDoubleListNode pNode = NULL;
    PCurlRequest pCurlRequest = NULL;

    CHK(ppCurlRequest != NULL, STATUS_NULL_ARG);
    CHK(IS_VALID_UPLOAD_HANDLE(uploadHandle), retStatus);

    CHK(pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    // Iterate through the list and find the requests matching the uploadHandle
    CHK_STATUS(doubleListGetHeadNode(pCurlApiCallbacks->pActiveUploads, &pNode));
    while (pNode != NULL && !requestFound) {
        pCurlRequest = (PCurlRequest) pNode->data;

        // Get the next node before processing
        CHK_STATUS(doubleListGetNextNode(pNode, &pNode));

        if (pCurlRequest->uploadHandle == uploadHandle) {
            requestFound = TRUE;
        }
    }

CleanUp:

    if (!requestFound) {
        pCurlRequest = NULL;
    }

    *ppCurlRequest = pCurlRequest;

    LEAVES();
    return retStatus;
}
