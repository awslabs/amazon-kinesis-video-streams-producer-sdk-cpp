#define LOG_CLASS "RequestInfo"
#include "Include_i.h"

STATUS createRequestInfo(PCHAR url, PCHAR body, PCHAR region, PCHAR certPath, PCHAR sslCertPath, PCHAR sslPrivateKeyPath,
                         SSL_CERTIFICATE_TYPE certType, PCHAR userAgent, UINT64 connectionTimeout, UINT64 completionTimeout, UINT64 lowSpeedLimit,
                         UINT64 lowSpeedTimeLimit, PAwsCredentials pAwsCredentials, PRequestInfo* ppRequestInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRequestInfo pRequestInfo = NULL;
    UINT32 size = SIZEOF(RequestInfo), bodySize = 0;

    CHK(region != NULL && url != NULL && ppRequestInfo != NULL, STATUS_NULL_ARG);

    // Add body to the size excluding NULL terminator
    if (body != NULL) {
        bodySize = (UINT32)(STRLEN(body) * SIZEOF(CHAR));
        size += bodySize;
    }

    // Allocate the entire structure
    pRequestInfo = (PRequestInfo) MEMCALLOC(1, size);
    CHK(pRequestInfo != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pRequestInfo->pAwsCredentials = pAwsCredentials;
    pRequestInfo->verb = HTTP_REQUEST_VERB_POST;
    pRequestInfo->completionTimeout = completionTimeout;
    pRequestInfo->connectionTimeout = connectionTimeout;
    ATOMIC_STORE_BOOL(&pRequestInfo->terminating, FALSE);
    pRequestInfo->bodySize = bodySize;
    pRequestInfo->currentTime = GETTIME();
    pRequestInfo->callAfter = pRequestInfo->currentTime;
    STRNCPY(pRequestInfo->region, region, MAX_REGION_NAME_LEN);
    STRNCPY(pRequestInfo->url, url, MAX_URI_CHAR_LEN);
    if (certPath != NULL) {
        STRNCPY(pRequestInfo->certPath, certPath, MAX_PATH_LEN);
    }

    if (sslCertPath != NULL) {
        STRNCPY(pRequestInfo->sslCertPath, sslCertPath, MAX_PATH_LEN);
    }

    if (sslPrivateKeyPath != NULL) {
        STRNCPY(pRequestInfo->sslPrivateKeyPath, sslPrivateKeyPath, MAX_PATH_LEN);
    }

    pRequestInfo->certType = certType;
    pRequestInfo->lowSpeedLimit = lowSpeedLimit;
    pRequestInfo->lowSpeedTimeLimit = lowSpeedTimeLimit;

    // If the body is specified then it will be a request/response call
    // Otherwise we are streaming
    if (body != NULL) {
        pRequestInfo->body = (PCHAR)(pRequestInfo + 1);
        MEMCPY(pRequestInfo->body, body, bodySize);
    }

    // Create a list of headers
    CHK_STATUS(singleListCreate(&pRequestInfo->pRequestHeaders));

    // Set user agent header
    CHK_STATUS(setRequestHeader(pRequestInfo, (PCHAR) "user-agent", 0, userAgent, 0));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeRequestInfo(&pRequestInfo);
        pRequestInfo = NULL;
    }

    // Set the return value if it's not NULL
    if (ppRequestInfo != NULL) {
        *ppRequestInfo = pRequestInfo;
    }

    LEAVES();
    return retStatus;
}

STATUS freeRequestInfo(PRequestInfo* ppRequestInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRequestInfo pRequestInfo = NULL;

    CHK(ppRequestInfo != NULL, STATUS_NULL_ARG);

    pRequestInfo = (PRequestInfo) *ppRequestInfo;

    // Call is idempotent
    CHK(pRequestInfo != NULL, retStatus);

    // Remove and free the headers
    removeRequestHeaders(pRequestInfo);

    // Free the header list itself
    singleListFree(pRequestInfo->pRequestHeaders);

    // Release the object
    MEMFREE(pRequestInfo);

    // Set the pointer to NULL
    *ppRequestInfo = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS requestRequiresSecureConnection(PCHAR pUrl, PBOOL pSecure)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pUrl != NULL && pSecure != NULL, STATUS_NULL_ARG);
    *pSecure =
        (0 == STRNCMPI(pUrl, HTTPS_SCHEME_NAME, SIZEOF(HTTPS_SCHEME_NAME) - 1) || 0 == STRNCMPI(pUrl, WSS_SCHEME_NAME, SIZEOF(WSS_SCHEME_NAME) - 1));

CleanUp:

    return retStatus;
}

STATUS createRequestHeader(PCHAR headerName, UINT32 headerNameLen, PCHAR headerValue, UINT32 headerValueLen, PRequestHeader* ppHeader)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 nameLen, valueLen, size;
    PRequestHeader pRequestHeader = NULL;

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

    size = SIZEOF(RequestHeader) + (nameLen + 1 + valueLen + 1) * SIZEOF(CHAR);

    // Create the request header
    pRequestHeader = (PRequestHeader) MEMALLOC(size);
    CHK(pRequestHeader != NULL, STATUS_NOT_ENOUGH_MEMORY);
    pRequestHeader->nameLen = nameLen;
    pRequestHeader->valueLen = valueLen;

    // Pointing after the structure
    pRequestHeader->pName = (PCHAR)(pRequestHeader + 1);
    pRequestHeader->pValue = pRequestHeader->pName + nameLen + 1;

    MEMCPY(pRequestHeader->pName, headerName, nameLen * SIZEOF(CHAR));
    pRequestHeader->pName[nameLen] = '\0';
    MEMCPY(pRequestHeader->pValue, headerValue, valueLen * SIZEOF(CHAR));
    pRequestHeader->pValue[valueLen] = '\0';

CleanUp:

    if (STATUS_FAILED(retStatus) && pRequestHeader != NULL) {
        MEMFREE(pRequestHeader);
        pRequestHeader = NULL;
    }

    if (ppHeader != NULL) {
        *ppHeader = pRequestHeader;
    }

    return retStatus;
}

STATUS setRequestHeader(PRequestInfo pRequestInfo, PCHAR headerName, UINT32 headerNameLen, PCHAR headerValue, UINT32 headerValueLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 count;
    PSingleListNode pCurNode, pPrevNode = NULL;
    PRequestHeader pRequestHeader = NULL, pCurrentHeader;
    UINT64 item;

    CHK(pRequestInfo != NULL && headerName != NULL && headerValue != NULL, STATUS_NULL_ARG);
    CHK_STATUS(singleListGetNodeCount(pRequestInfo->pRequestHeaders, &count));
    CHK(count < MAX_REQUEST_HEADER_COUNT, STATUS_MAX_REQUEST_HEADER_COUNT);

    CHK_STATUS(createRequestHeader(headerName, headerNameLen, headerValue, headerValueLen, &pRequestHeader));

    // Iterate through the list and insert in an alpha order
    CHK_STATUS(singleListGetHeadNode(pRequestInfo->pRequestHeaders, &pCurNode));
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pCurrentHeader = (PRequestHeader) item;

        if (STRCMPI(pCurrentHeader->pName, pRequestHeader->pName) > 0) {
            if (pPrevNode == NULL) {
                // Insert at the head
                CHK_STATUS(singleListInsertItemHead(pRequestInfo->pRequestHeaders, (UINT64) pRequestHeader));
            } else {
                CHK_STATUS(singleListInsertItemAfter(pRequestInfo->pRequestHeaders, pPrevNode, (UINT64) pRequestHeader));
            }

            // Early return
            CHK(FALSE, retStatus);
        }

        pPrevNode = pCurNode;

        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

    // If not inserted then add to the tail
    CHK_STATUS(singleListInsertItemTail(pRequestInfo->pRequestHeaders, (UINT64) pRequestHeader));

CleanUp:

    if (STATUS_FAILED(retStatus) && pRequestHeader != NULL) {
        MEMFREE(pRequestHeader);
    }

    return retStatus;
}

STATUS removeRequestHeader(PRequestInfo pRequestInfo, PCHAR headerName)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSingleListNode pCurNode;
    PRequestHeader pCurrentHeader = NULL;
    UINT64 item;

    CHK(pRequestInfo != NULL && headerName != NULL, STATUS_NULL_ARG);

    // Iterate through the list and insert in an alpha order
    CHK_STATUS(singleListGetHeadNode(pRequestInfo->pRequestHeaders, &pCurNode));
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pCurrentHeader = (PRequestHeader) item;

        if (STRCMPI(pCurrentHeader->pName, headerName) == 0) {
            CHK_STATUS(singleListDeleteNode(pRequestInfo->pRequestHeaders, pCurNode));

            // Early return
            CHK(FALSE, retStatus);
        }

        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

CleanUp:

    SAFE_MEMFREE(pCurrentHeader);

    return retStatus;
}

STATUS removeRequestHeaders(PRequestInfo pRequestInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSingleListNode pNode;
    UINT32 itemCount;
    PRequestHeader pRequestHeader;

    CHK(pRequestInfo != NULL, STATUS_NULL_ARG);

    singleListGetNodeCount(pRequestInfo->pRequestHeaders, &itemCount);
    while (itemCount-- != 0) {
        // Remove and delete the data
        singleListGetHeadNode(pRequestInfo->pRequestHeaders, &pNode);
        pRequestHeader = (PRequestHeader) pNode->data;
        SAFE_MEMFREE(pRequestHeader);

        // Iterate
        singleListDeleteHead(pRequestInfo->pRequestHeaders);
    }

CleanUp:

    return retStatus;
}

SERVICE_CALL_RESULT getServiceCallResultFromHttpStatus(UINT32 httpStatus)
{
    switch (httpStatus) {
        case SERVICE_CALL_RESULT_OK:
        case SERVICE_CALL_INVALID_ARG:
        case SERVICE_CALL_RESOURCE_NOT_FOUND:
        case SERVICE_CALL_FORBIDDEN:
        case SERVICE_CALL_RESOURCE_DELETED:
        case SERVICE_CALL_NOT_AUTHORIZED:
        case SERVICE_CALL_NOT_IMPLEMENTED:
        case SERVICE_CALL_INTERNAL_ERROR:
        case SERVICE_CALL_REQUEST_TIMEOUT:
        case SERVICE_CALL_GATEWAY_TIMEOUT:
        case SERVICE_CALL_NETWORK_READ_TIMEOUT:
        case SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT:
            return (SERVICE_CALL_RESULT) httpStatus;
        default:
            return SERVICE_CALL_UNKNOWN;
    }
}

STATUS releaseCallInfo(PCallInfo pCallInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 item;
    UINT32 itemCount;
    PRequestHeader pRequestHeader;

    CHK(pCallInfo != NULL, STATUS_NULL_ARG);

    // Free the response buffer
    if (pCallInfo->responseData != NULL) {
        MEMFREE(pCallInfo->responseData);
        pCallInfo->responseData = NULL;
        pCallInfo->responseDataLen = 0;
    }

    // Free the response headers
    if (pCallInfo->pResponseHeaders != NULL) {
        stackQueueGetCount(pCallInfo->pResponseHeaders, &itemCount);
        while (itemCount-- != 0) {
            // Dequeue the current item
            stackQueueDequeue(pCallInfo->pResponseHeaders, &item);

            pRequestHeader = (PRequestHeader) item;
            if (pRequestHeader != NULL) {
                MEMFREE(pRequestHeader);
            }
        }

        stackQueueFree(pCallInfo->pResponseHeaders);
        pCallInfo->pResponseHeaders = NULL;
    }

CleanUp:

    return retStatus;
}