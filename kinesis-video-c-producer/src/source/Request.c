/**
 * Kinesis Video Producer Request functionality
 */
#define LOG_CLASS "Request"
#include "Include_i.h"

/**
 * Create request object
 */
STATUS createCurlRequest(CURL_VERB curlVerb, PCHAR url, PCHAR body, STREAM_HANDLE streamHandle,
                         UINT64 connectionTimeout, UINT64 completionTimeout,
                         UINT64 callAfter, PCHAR certPath, PAwsCredentials pAwsCredentials,
                         PCurlApiCallbacks pCurlApiCallbacks, PCurlRequest* ppCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = NULL;
    UINT32 size = SIZEOF(CurlRequest), bodySize = 0;
    PCallbacksProvider pCallbacksProvider;
    PStreamInfo pStreamInfo;

    CHK(ppCurlRequest != NULL &&
        url != NULL &&
        pCurlApiCallbacks != NULL &&
        pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Add body to the size excluding NULL terminator
    if (body != NULL) {
        bodySize = (UINT32) (STRLEN(body) * SIZEOF(CHAR));
        size += bodySize;
    }

    // Allocate the entire structure
    pCurlRequest = (PCurlRequest) MEMCALLOC(1, size);
    CHK(pCurlRequest != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pCurlRequest->pAwsCredentials = pAwsCredentials;
    pCurlRequest->verb = curlVerb;
    pCurlRequest->streamHandle = streamHandle;
    pCurlRequest->completionTimeout = completionTimeout;
    pCurlRequest->connectionTimeout = connectionTimeout;
    pCurlRequest->callAfter = callAfter;
    pCurlRequest->pCurlApiCallbacks = pCurlApiCallbacks;
    pCurlRequest->terminating = FALSE;
    pCurlRequest->threadId = INVALID_TID_VALUE;
    pCurlRequest->bodySize = bodySize;
    STRNCPY(pCurlRequest->url, url, MAX_URI_CHAR_LEN);
    if (certPath != NULL) {
        STRNCPY(pCurlRequest->certPath, certPath, MAX_PATH_LEN);
    }

    // If the body is specified then it will be a request/response call
    // Otherwise we are streaming
    if (body != NULL) {
        pCurlRequest->body = (PCHAR) (pCurlRequest + 1);
        pCurlRequest->streaming = FALSE;
        MEMCPY(pCurlRequest->body, body, bodySize);
    } else {
        pCurlRequest->streaming = TRUE;
        pCurlRequest->body = NULL;
        pCurlRequest->uploadHandle = (UPLOAD_HANDLE) pCurlApiCallbacks->streamingRequestCount++;
    }

    // Create a list of headers
    CHK_STATUS(singleListCreate(&pCurlRequest->pRequestHeaders));

    // Create the mutex
    pCurlRequest->startLock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, FALSE);
    CHK(pCurlRequest->startLock != INVALID_MUTEX_VALUE, STATUS_INVALID_OPERATION);

    // Set the stream name
    CHK_STATUS(kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));
    pCurlRequest->pStreamInfo = pStreamInfo;

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
    UINT32 itemCount;
    PSingleListNode pNode;
    PCurlRequestHeader pCurlRequestHeader;

    CHK(ppCurlRequest != NULL, STATUS_NULL_ARG);

    pCurlRequest = (PCurlRequest) *ppCurlRequest;

    // Call is idempotent
    CHK(pCurlRequest != NULL, retStatus);

    // Release the underlying response
    if (pCurlRequest->pCurlResponse != NULL) {
        freeCurlResponse(&pCurlRequest->pCurlResponse);
    }

    // Free the start lock
    if (pCurlRequest->startLock != INVALID_MUTEX_VALUE &&
        pCurlRequest->pCurlApiCallbacks != NULL &&
        pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL) {
        pCallbacksProvider = pCurlRequest->pCurlApiCallbacks->pCallbacksProvider;

        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlRequest->startLock);
    }

    // Free the request headers
    singleListGetNodeCount(pCurlRequest->pRequestHeaders, &itemCount);
    while (itemCount-- != 0) {
        // Remove and delete the data
        singleListGetHeadNode(pCurlRequest->pRequestHeaders, &pNode);

        pCurlRequestHeader = (PCurlRequestHeader) pNode->data;
        if (pCurlRequestHeader != NULL) {
            MEMFREE(pCurlRequestHeader);
        }

        singleListDeleteHead(pCurlRequest->pRequestHeaders);
    }

    stackQueueFree(pCurlRequest->pRequestHeaders);

    // Release the object
    MEMFREE(pCurlRequest);

    // Set the pointer to NULL
    *ppCurlRequest = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS requestRequiresSecureConnection(PCurlRequest pCurlRequest, PBOOL pSecure)
{
    STATUS retStatus = STATUS_SUCCESS;
    CHK(pCurlRequest != NULL && pSecure != NULL, STATUS_NULL_ARG);

    *pSecure = (0 == STRNCMPI(pCurlRequest->url, CURL_HTTPS_SCHEME, SIZEOF(CURL_HTTPS_SCHEME) - 1));

CleanUp:

    return retStatus;
}

STATUS createRequestHeader(PCHAR headerName, UINT32 headerNameLen, PCHAR headerValue, UINT32 headerValueLen, PCurlRequestHeader* ppHeader)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 nameLen, valueLen, size;
    PCurlRequestHeader pCurlRequestHeader = NULL;

    CHK(ppHeader != NULL && headerName != NULL && headerValue != NULL, STATUS_NULL_ARG);

    // Calculate the length if needed
    if (headerNameLen == 0) {
        nameLen = (UINT32) STRLEN(headerName);
    } else {
        nameLen = headerNameLen;
    }

    if (headerValueLen == 0) {
        valueLen = (UINT32) STRLEN(headerValue);
    } else {
        valueLen = headerValueLen;
    }

    CHK(nameLen > 0 && valueLen > 0, STATUS_INVALID_ARG);
    CHK(nameLen < MAX_REQUEST_HEADER_NAME_LEN, STATUS_MAX_REQUEST_HEADER_NAME_LEN);
    CHK(valueLen < MAX_REQUEST_HEADER_VALUE_LEN, STATUS_MAX_REQUEST_HEADER_VALUE_LEN);

    size = SIZEOF(CurlRequestHeader) + (nameLen + 1 + valueLen + 1) * SIZEOF(CHAR);

    // Create the request header
    pCurlRequestHeader = (PCurlRequestHeader) MEMALLOC(size);
    CHK(pCurlRequestHeader != NULL, STATUS_NOT_ENOUGH_MEMORY);
    pCurlRequestHeader->nameLen = nameLen;
    pCurlRequestHeader->valueLen = valueLen;

    // Pointing after the structure
    pCurlRequestHeader->pName = (PCHAR) (pCurlRequestHeader + 1);
    pCurlRequestHeader->pValue = pCurlRequestHeader->pName + nameLen + 1;

    MEMCPY(pCurlRequestHeader->pName, headerName, nameLen * SIZEOF(CHAR));
    pCurlRequestHeader->pName[nameLen] = '\0';
    MEMCPY(pCurlRequestHeader->pValue, headerValue, valueLen * SIZEOF(CHAR));
    pCurlRequestHeader->pValue[valueLen] = '\0';

CleanUp:

    if (STATUS_FAILED(retStatus) && pCurlRequestHeader != NULL) {
        MEMFREE(pCurlRequestHeader);
        pCurlRequestHeader = NULL;
    }

    if (ppHeader != NULL) {
        *ppHeader = pCurlRequestHeader;
    }

    return retStatus;
}

STATUS setCurlRequestHeader(PCurlRequest pCurlRequest, PCHAR headerName, UINT32 headerNameLen, PCHAR headerValue, UINT32 headerValueLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 count;
    PSingleListNode pCurNode, pPrevNode = NULL;
    PCurlRequestHeader pCurlRequestHeader = NULL, pCurrentHeader;
    UINT64 item;

    CHK(pCurlRequest != NULL && headerName != NULL && headerValue != NULL, STATUS_NULL_ARG);
    CHK_STATUS(singleListGetNodeCount(pCurlRequest->pRequestHeaders, &count));
    CHK(count < MAX_REQUEST_HEADER_COUNT, STATUS_MAX_REQUEST_HEADER_COUNT);

    CHK_STATUS(createRequestHeader(headerName, headerNameLen, headerValue, headerValueLen, &pCurlRequestHeader));

    // Iterate through the list and insert in an alpha order
    CHK_STATUS(singleListGetHeadNode(pCurlRequest->pRequestHeaders, &pCurNode));
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pCurrentHeader = (PCurlRequestHeader) item;

        if (STRCMPI(pCurrentHeader->pName, pCurlRequestHeader->pName) > 0) {
            if (pPrevNode == NULL) {
                // Insert at the head
                CHK_STATUS(singleListInsertItemHead(pCurlRequest->pRequestHeaders, (UINT64) pCurlRequestHeader));
            } else {
                CHK_STATUS(singleListInsertItemAfter(pCurlRequest->pRequestHeaders, pPrevNode,
                                                     (UINT64) pCurlRequestHeader));
            }

            // Early return
            CHK(FALSE, retStatus);
        }

        pPrevNode = pCurNode;

        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

    // If not inserted then add to the tail
    CHK_STATUS(singleListInsertItemTail(pCurlRequest->pRequestHeaders, (UINT64) pCurlRequestHeader));

CleanUp:

    if (STATUS_FAILED(retStatus) && pCurlRequestHeader != NULL) {
        MEMFREE(pCurlRequestHeader);
    }

    return retStatus;
}

PCHAR getRequestVerbString(CURL_VERB verb)
{
    switch (verb) {
        case CURL_VERB_PUT:
            return CURL_VERB_PUT_STRING;
        case CURL_VERB_GET:
            return CURL_VERB_GET_STRING;
        case CURL_VERB_POST:
            return CURL_VERB_POST_STRING;
    }

    return NULL;
}

STATUS getCanonicalUri(PCurlRequest pCurlRequest, PCHAR* ppStart, PCHAR* ppEnd)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pStart = NULL, pEnd = NULL;
    UINT32 urlLen;

    CHK(pCurlRequest != NULL && ppStart != NULL && ppEnd != NULL, STATUS_NULL_ARG);

    // We know for sure url is NULL terminated
    urlLen = (UINT32) STRLEN(pCurlRequest->url);

    // Start from the schema delimiter
    pStart = STRSTR(pCurlRequest->url, SCHEMA_DELIMITER_STRING);
    CHK(pStart != NULL, STATUS_INVALID_ARG);

    // Advance the pStart past the delimiter
    pStart += STRLEN(SCHEMA_DELIMITER_STRING);

    // Ensure we are not past the string
    CHK(pCurlRequest->url + urlLen > pStart, STATUS_INVALID_ARG);

    // Check if we have the host delimiter
    pStart = STRNCHR(pStart, urlLen - (UINT32) (pStart - pCurlRequest->url), '/');
    if (pStart == NULL) {
        pStart = DEFAULT_CANONICAL_URI_STRING;
        pEnd = pStart + 1;

        // Early exit
        CHK (FALSE, retStatus);
    }

    pEnd = STRNCHR(pStart, urlLen - (UINT32) (pStart - pCurlRequest->url), '?');

    if (pEnd == NULL) {
        pEnd = pCurlRequest->url + urlLen;
    }

CleanUp:

    if (ppStart != NULL) {
        *ppStart = pStart;
    }

    if (ppEnd != NULL) {
        *ppEnd = pEnd;
    }

    return retStatus;
}

STATUS getRequestHost(PCurlRequest pCurlRequest, PCHAR* ppStart, PCHAR* ppEnd)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pStart = NULL, pEnd = NULL, pCurPtr;
    UINT32 urlLen;
    BOOL iterate = TRUE;

    CHK(pCurlRequest != NULL && ppStart != NULL && ppEnd != NULL, STATUS_NULL_ARG);

    // We know for sure url is NULL terminated
    urlLen = (UINT32) STRLEN(pCurlRequest->url);

    // Start from the schema delimiter
    pStart = STRSTR(pCurlRequest->url, SCHEMA_DELIMITER_STRING);
    CHK(pStart != NULL, STATUS_INVALID_ARG);

    // Advance the pStart past the delimiter
    pStart += STRLEN(SCHEMA_DELIMITER_STRING);

    // Ensure we are not past the string
    CHK(pCurlRequest->url + urlLen > pStart, STATUS_INVALID_ARG);

    // Set the end first
    pEnd = pCurlRequest->url + urlLen;

    // Find the delimiter which would indicate end of the host - either one of "/:?"
    pCurPtr = pStart;
    while (iterate && pCurPtr <= pEnd) {
        switch (*pCurPtr) {
            case '/':
            case ':':
            case '?':
                iterate = FALSE;

                // Set the new end value
                pEnd = pCurPtr;
            default:
                pCurPtr++;
        }
    }

CleanUp:

    if (ppStart != NULL) {
        *ppStart = pStart;
    }

    if (ppEnd != NULL) {
        *ppEnd = pEnd;
    }

    return retStatus;
}
