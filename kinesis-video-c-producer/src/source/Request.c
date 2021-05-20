/**
 * Kinesis Video Producer Request functionality
 */
#define LOG_CLASS "Request"
#include "Include_i.h"

/**
 * Create request object
 */
STATUS createCurlRequest(HTTP_REQUEST_VERB curlVerb, PCHAR url, PCHAR body, STREAM_HANDLE streamHandle, PCHAR region, UINT64 currentTime,
                         UINT64 connectionTimeout, UINT64 completionTimeout, UINT64 callAfter, PCHAR certPath, PAwsCredentials pAwsCredentials,
                         PCurlApiCallbacks pCurlApiCallbacks, PCurlRequest* ppCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = NULL;
    UINT32 size = SIZEOF(CurlRequest), bodySize = 0;
    PCallbacksProvider pCallbacksProvider;
    PStreamInfo pStreamInfo;

    CHK(ppCurlRequest != NULL && url != NULL && pCurlApiCallbacks != NULL && pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Add body to the size excluding NULL terminator
    if (body != NULL) {
        bodySize = (UINT32)(STRLEN(body) * SIZEOF(CHAR));
        size += bodySize;
    }

    // Allocate the entire structure
    pCurlRequest = (PCurlRequest) MEMCALLOC(1, size);
    CHK(pCurlRequest != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pCurlRequest->requestInfo.pAwsCredentials = pAwsCredentials;
    pCurlRequest->requestInfo.verb = curlVerb;
    pCurlRequest->streamHandle = streamHandle;
    pCurlRequest->requestInfo.completionTimeout = completionTimeout;
    pCurlRequest->requestInfo.connectionTimeout = connectionTimeout;
    pCurlRequest->requestInfo.callAfter = callAfter;
    pCurlRequest->pCurlApiCallbacks = pCurlApiCallbacks;
    ATOMIC_STORE_BOOL(&pCurlRequest->requestInfo.terminating, FALSE);
    ATOMIC_STORE_BOOL(&pCurlRequest->blockedInCurl, FALSE);
    pCurlRequest->threadId = INVALID_TID_VALUE;
    pCurlRequest->requestInfo.bodySize = bodySize;
    pCurlRequest->requestInfo.currentTime = currentTime;

    STRNCPY(pCurlRequest->requestInfo.region, region, MAX_REGION_NAME_LEN);
    STRNCPY(pCurlRequest->requestInfo.url, url, MAX_URI_CHAR_LEN);
    if (certPath != NULL) {
        STRNCPY(pCurlRequest->requestInfo.certPath, certPath, MAX_PATH_LEN);
    }

    // If the body is specified then it will be a request/response call
    // Otherwise we are streaming
    if (body != NULL) {
        pCurlRequest->requestInfo.body = (PCHAR)(pCurlRequest + 1);
        pCurlRequest->streaming = FALSE;
        MEMCPY(pCurlRequest->requestInfo.body, body, bodySize);
    } else {
        pCurlRequest->streaming = TRUE;
        pCurlRequest->requestInfo.body = NULL;
        pCurlRequest->uploadHandle = (UPLOAD_HANDLE) pCurlApiCallbacks->streamingRequestCount++;
    }

    // Create a list of headers
    CHK_STATUS(singleListCreate(&pCurlRequest->requestInfo.pRequestHeaders));

    // Create the mutex
    pCurlRequest->startLock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, FALSE);
    CHK(pCurlRequest->startLock != INVALID_MUTEX_VALUE, STATUS_INVALID_OPERATION);

    // Set the stream name
    CHK_STATUS(kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));
    STRNCPY(pCurlRequest->streamName, pStreamInfo->name, MAX_STREAM_NAME_LEN);

    // Create the response object
    CHK_STATUS(createCurlResponse(pCurlRequest, &pCurlRequest->pCurlResponse));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeCurlRequest(&pCurlRequest);
        pCurlRequest = NULL;
    }

    // Set the return value if it's not NULL
    if (ppCurlRequest != NULL) {
        *ppCurlRequest = pCurlRequest;
    }

    LEAVES();
    return retStatus;
}

STATUS freeCurlRequest(PCurlRequest* ppCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = NULL;
    PCallbacksProvider pCallbacksProvider;

    CHK(ppCurlRequest != NULL, STATUS_NULL_ARG);

    pCurlRequest = (PCurlRequest) *ppCurlRequest;

    // Call is idempotent
    CHK(pCurlRequest != NULL, retStatus);

    // Release the underlying response
    if (pCurlRequest->pCurlResponse != NULL) {
        freeCurlResponse(&pCurlRequest->pCurlResponse);
    }

    // Free the start lock
    if (pCurlRequest->startLock != INVALID_MUTEX_VALUE && pCurlRequest->pCurlApiCallbacks != NULL &&
        pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL) {
        pCallbacksProvider = pCurlRequest->pCurlApiCallbacks->pCallbacksProvider;

        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    }

    // Remove and free the headers
    removeRequestHeaders(&pCurlRequest->requestInfo);

    stackQueueFree(pCurlRequest->requestInfo.pRequestHeaders);

    // Release the object
    MEMFREE(pCurlRequest);

    // Set the pointer to NULL
    *ppCurlRequest = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}
