/**
 * Kinesis Video Producer Response functionality
 */
#define LOG_CLASS "Response"
#include "Include_i.h"

/**
 * Create Response object
 */
STATUS createCurlResponse(PCurlRequest pCurlRequest, PCurlResponse* ppCurlResponse)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlResponse pCurlResponse = NULL;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PCallbacksProvider pCallbacksProvider;
    PCHAR debugDumpDataFileDir = NULL;
    UINT32 debugDumpDataFileDirLen = 0;

    CHK(ppCurlResponse != NULL && pCurlRequest != NULL && pCurlRequest->pCurlApiCallbacks != NULL &&
            pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL,
        STATUS_NULL_ARG);
    CHK(IS_VALID_STREAM_HANDLE(pCurlRequest->streamHandle), STATUS_INVALID_ARG);
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    pCallbacksProvider = pCurlApiCallbacks->pCallbacksProvider;

    // Allocate the entire structure
    pCurlResponse = (PCurlResponse) MEMCALLOC(1, SIZEOF(CurlResponse));
    CHK(pCurlResponse != NULL, STATUS_NOT_ENOUGH_MEMORY);
    ATOMIC_STORE_BOOL(&pCurlResponse->terminated, FALSE);

    // init putMedia related members
    pCurlResponse->endOfStream = FALSE;
    pCurlResponse->paused = TRUE;
    pCurlResponse->debugDumpFile = FALSE;
    pCurlResponse->debugDumpFilePath[0] = '\0';

    if ((debugDumpDataFileDir = getenv(KVS_DEBUG_DUMP_DATA_FILE_DIR_ENV_VAR)) != NULL && pCurlRequest->streaming) {
        // debugDumpDataFileDirLen is MAX_PATH_LEN + 1 at max plus null terminator.
        debugDumpDataFileDirLen = (UINT32) SNPRINTF(pCurlResponse->debugDumpFilePath, MAX_PATH_LEN + 1, (PCHAR) "%s/%s_%" PRIu64 ".mkv",
                                                    debugDumpDataFileDir, pCurlRequest->streamName, (UINT64) pCurlRequest->uploadHandle);
        CHK(debugDumpDataFileDirLen <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
        pCurlResponse->debugDumpFile = TRUE;
        pCurlResponse->debugDumpFilePath[MAX_PATH_LEN] = '\0';
        // create file to override any existing file with the same name
        CHK_STATUS(createFile(pCurlResponse->debugDumpFilePath, 0));
    }
    // end init putMedia related members

    // Create the mutex
    pCurlResponse->lock = pCallbacksProvider->clientCallbacks.createMutexFn(pCallbacksProvider->clientCallbacks.customData, TRUE);
    CHK(pCurlResponse->lock != INVALID_MUTEX_VALUE, STATUS_INVALID_OPERATION);

    // Set the parent object
    pCurlResponse->pCurlRequest = pCurlRequest;

    CHK_STATUS(initializeCurlSession(&pCurlRequest->requestInfo, &pCurlResponse->callInfo, &pCurlResponse->pCurl, pCurlRequest, writeHeaderCallback,
                                     postReadCallback, postWriteCallback, postResponseWriteCallback));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeCurlResponse(&pCurlResponse);
        pCurlResponse = NULL;
    }

    // Set the return value if it's not NULL
    if (ppCurlResponse != NULL) {
        *ppCurlResponse = pCurlResponse;
    }

    LEAVES();
    return retStatus;
}

STATUS freeCurlResponse(PCurlResponse* ppCurlResponse)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCurlResponse pCurlResponse = NULL;
    PCallbacksProvider pCallbacksProvider = NULL;
    BOOL locked = FALSE;

    CHK(ppCurlResponse != NULL, STATUS_NULL_ARG);

    pCurlResponse = (PCurlResponse) *ppCurlResponse;

    // Call is idempotent
    CHK(pCurlResponse != NULL, retStatus);

    if (pCurlResponse->pCurlRequest != NULL && pCurlResponse->pCurlRequest->pCurlApiCallbacks != NULL &&
        pCurlResponse->pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL) {
        pCallbacksProvider = pCurlResponse->pCurlRequest->pCurlApiCallbacks->pCallbacksProvider;
    }

    if (pCallbacksProvider != NULL && IS_VALID_MUTEX_VALUE(pCurlResponse->lock)) {
        locked = pCallbacksProvider->clientCallbacks.tryLockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlResponse->lock);
        // if it fail to acquire the lock that means another thread is tearing down the response already.
        // thus current thread can return early.
        // TODO: eliminate the possibility of two threads calling freeCurlResponse.
        CHK(locked, retStatus);
    }

    // Close curl handles
    closeCurlHandles(pCurlResponse);

    // Set the pointer to NULL. So thread that comes after unlock return early because *ppCurlResponse is NULL
    *ppCurlResponse = NULL;

    if (locked) {
        pCallbacksProvider->clientCallbacks.unlockMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlResponse->lock);
        pCallbacksProvider->clientCallbacks.freeMutexFn(pCallbacksProvider->clientCallbacks.customData, pCurlResponse->lock);
        locked = FALSE;
    }

    // Release the object
    MEMFREE(pCurlResponse);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS initializeCurlSession(PRequestInfo pRequestInfo, PCallInfo pCallInfo, CURL** ppCurl, PVOID data, CurlCallbackFunc writeHeaderFn,
                             CurlCallbackFunc readFn, CurlCallbackFunc writeFn, CurlCallbackFunc responseWriteFn)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL secureConnection;
    CURL* pCurl = NULL;
    UINT32 length;
    STAT_STRUCT entryStat;

    CHK(pRequestInfo != NULL && pCallInfo != NULL && ppCurl != NULL, STATUS_NULL_ARG);

    pCallInfo->pRequestInfo = pRequestInfo;
    pCallInfo->httpStatus = HTTP_STATUS_CODE_NOT_SET;
    pCallInfo->callResult = SERVICE_CALL_RESULT_NOT_SET;

    // Initialize curl and set options
    pCurl = curl_easy_init();
    CHK(pCurl != NULL, STATUS_CURL_INIT_FAILED);

    // set up the friendly error message buffer
    pCallInfo->errorBuffer[0] = '\0';
    curl_easy_setopt(pCurl, CURLOPT_ERRORBUFFER, pCallInfo->errorBuffer);

    curl_easy_setopt(pCurl, CURLOPT_URL, pRequestInfo->url);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);

    // Setting up limits for curl timeout
    curl_easy_setopt(pCurl, CURLOPT_LOW_SPEED_TIME, DEFAULT_LOW_SPEED_TIME_LIMIT / HUNDREDS_OF_NANOS_IN_A_SECOND);
    curl_easy_setopt(pCurl, CURLOPT_LOW_SPEED_LIMIT, DEFAULT_LOW_SPEED_LIMIT);

    // set verification for SSL connections
    CHK_STATUS(requestRequiresSecureConnection(pRequestInfo->url, &secureConnection));
    if (secureConnection) {
        // Use the default cert store at /etc/ssl in most common platforms
        if (pRequestInfo->certPath[0] != '\0') {
            CHK(0 == FSTAT(pRequestInfo->certPath, &entryStat), STATUS_DIRECTORY_ENTRY_STAT_ERROR);

            if (S_ISDIR(entryStat.st_mode)) {
                // Assume it's the path as we have a directory
                curl_easy_setopt(pCurl, CURLOPT_CAPATH, pRequestInfo->certPath);
            } else {
                // We should check for the extension being PEM
                length = (UINT32) STRNLEN(pRequestInfo->certPath, MAX_PATH_LEN);
                CHK(length > ARRAY_SIZE(CA_CERT_FILE_SUFFIX), STATUS_INVALID_ARG_LEN);
                CHK(0 == STRCMPI(CA_CERT_FILE_SUFFIX, &pRequestInfo->certPath[length - ARRAY_SIZE(CA_CERT_FILE_SUFFIX) + 1]),
                    STATUS_INVALID_CA_CERT_PATH);

                curl_easy_setopt(pCurl, CURLOPT_CAINFO, pRequestInfo->certPath);
            }
        }

        // Enforce the public cert verification - even though this is the default
        curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(pCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    }

    // set request completion timeout in milliseconds
    if (pRequestInfo->completionTimeout != SERVICE_CALL_INFINITE_TIMEOUT) {
        curl_easy_setopt(pCurl, CURLOPT_TIMEOUT_MS, pRequestInfo->completionTimeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT_MS, pRequestInfo->connectionTimeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    curl_easy_setopt(pCurl, CURLOPT_TCP_NODELAY, 1);

    // set header callback
    curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, writeHeaderFn);
    curl_easy_setopt(pCurl, CURLOPT_HEADERDATA, data);

    switch (pRequestInfo->verb) {
        case HTTP_REQUEST_VERB_GET:
            curl_easy_setopt(pCurl, CURLOPT_HTTPGET, 1L);
            break;

        case HTTP_REQUEST_VERB_PUT:
            curl_easy_setopt(pCurl, CURLOPT_PUT, 1L);
            break;

        case HTTP_REQUEST_VERB_POST:
            curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
            if (pRequestInfo->body == NULL) {
                // Set the read callback from the request
                curl_easy_setopt(pCurl, CURLOPT_READFUNCTION, readFn);
                curl_easy_setopt(pCurl, CURLOPT_READDATA, data);

                // Set the write callback from the request
                curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, writeFn);
                curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, data);
            } else {
                // Set the read data and it's size
                curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, pRequestInfo->bodySize);
                curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, pRequestInfo->body);

                // Set response callback
                curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, responseWriteFn);
                curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, data);
            }

            break;
    }

    // Create the response headers list
    CHK_STATUS(stackQueueCreate(&pCallInfo->pResponseHeaders));

    *ppCurl = pCurl;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS closeCurlHandles(PCurlResponse pCurlResponse)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pCurlResponse != NULL, STATUS_NULL_ARG);

    if (pCurlResponse->pRequestHeaders != NULL) {
        curl_slist_free_all(pCurlResponse->pRequestHeaders);
        pCurlResponse->pRequestHeaders = NULL;
    }

    if (pCurlResponse->pCurl) {
        curl_easy_cleanup(pCurlResponse->pCurl);
        pCurlResponse->pCurl = NULL;
    }

    CHK_STATUS(releaseCallInfo(&pCurlResponse->callInfo));

CleanUp:

    return retStatus;
}

VOID terminateCurlSession(PCurlResponse pCurlResponse, UINT64 timeout)
{
    if (pCurlResponse != NULL) {
        DLOGV("Force stopping the curl connection");

        // Currently, it seems that the only "good" way to stop CURL is to set
        // the timeout to a small value which will timeout the connection.
        // There is also a potential race condition on stream stopping as termination
        // could be fast and the bits in the CURL buffer might not have been sent
        // by the time the terminate() call is issued. We can't control this timing
        // of the CURL internal buffers so we need to introduce a timeout here before
        // the main curl termination path.
        if (!ATOMIC_LOAD_BOOL(&pCurlResponse->terminated)) {
            // give curl sometime to terminate gracefully before actually timing it out.
            THREAD_SLEEP(timeout);
            // unpause curl in case curl is paused
            curl_easy_pause(pCurlResponse->pCurl, CURLPAUSE_SEND_CONT);
            curl_easy_setopt(pCurlResponse->pCurl, CURLOPT_TIMEOUT_MS, TIMEOUT_AFTER_STREAM_STOPPED);
            // after timing out curl, give some time for it to take effect.
            THREAD_SLEEP(timeout);
            ATOMIC_STORE_BOOL(&pCurlResponse->terminated, TRUE);
        }
    }
}

VOID dumpResponseCurlEasyInfo(PCurlResponse pCurlResponse)
{
    DOUBLE time;
    CURLcode ret;

    if (pCurlResponse != NULL) {
        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_TOTAL_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("TOTAL_TIME: %f", time);
        } else {
            DLOGW("Unable to dump TOTAL_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_NAMELOOKUP_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("NAMELOOKUP_TIME: %f", time);
        } else {
            DLOGW("Unable to dump NAMELOOKUP_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_CONNECT_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("CONNECT_TIME: %f", time);
        } else {
            DLOGW("Unable to dump CONNECT_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_APPCONNECT_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("APPCONNECT_TIME: %f", time);
        } else {
            DLOGW("Unable to dump APPCONNECT_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_PRETRANSFER_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("PRETRANSFER_TIME: %f", time);
        } else {
            DLOGW("Unable to dump PRETRANSFER_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_STARTTRANSFER_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("STARTTRANSFER_TIME: %f", time);
        } else {
            DLOGW("Unable to dump STARTTRANSFER_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_REDIRECT_TIME, &time);
        if (ret == CURLE_OK) {
            DLOGI("REDIRECT_TIME: %f", time);
        } else {
            DLOGW("Unable to dump REDIRECT_TIME");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_SPEED_UPLOAD, &time);
        if (ret == CURLE_OK) {
            DLOGI("CURLINFO_SPEED_UPLOAD: %f", time);
        } else {
            DLOGW("Unable to dump CURLINFO_SPEED_UPLOAD");
        }

        ret = curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_SIZE_UPLOAD, &time);
        if (ret == CURLE_OK) {
            DLOGI("CURLINFO_SIZE_UPLOAD: %f", time);
        } else {
            DLOGW("Unable to dump CURLINFO_SIZE_UPLOAD");
        }
    }
}

SERVICE_CALL_RESULT getServiceCallResultFromCurlStatus(CURLcode curlStatus)
{
    switch (curlStatus) {
        case CURLE_OK:
            return SERVICE_CALL_RESULT_OK;
        case CURLE_UNSUPPORTED_PROTOCOL:
            return SERVICE_CALL_INVALID_ARG;
        case CURLE_OPERATION_TIMEDOUT:
            return SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CACERT:
            return SERVICE_CALL_NOT_AUTHORIZED;
        default:
            return SERVICE_CALL_UNKNOWN;
    }
}

// Forward declaration
STATUS createCurlHeaderList(PRequestInfo pRequestInfo, struct curl_slist** ppHeaderList);

STATUS curlCompleteSync(PCurlResponse pCurlResponse)
{
    STATUS retStatus = STATUS_SUCCESS, status = STATUS_SUCCESS;
    PCHAR url;
    struct curl_slist* header;
    PCurlApiCallbacks pCurlApiCallbacks;
    CURLcode result;
    PCurlRequest pCurlRequest;
    CHAR headers[MAX_REQUEST_HEADER_COUNT * (MAX_REQUEST_HEADER_STRING_LEN + MAX_REQUEST_HEADER_OUTPUT_DELIMITER)];

    CHK(pCurlResponse != NULL && pCurlResponse->pCurlRequest != NULL && pCurlResponse->pCurlRequest->pCurlApiCallbacks != NULL, STATUS_NULL_ARG);
    pCurlRequest = pCurlResponse->pCurlRequest;
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;

    CHK_STATUS(createCurlHeaderList(&pCurlRequest->requestInfo, &pCurlResponse->pRequestHeaders));

    curl_easy_setopt(pCurlResponse->pCurl, CURLOPT_HTTPHEADER, pCurlResponse->pRequestHeaders);

    // Check if we need to call the hook function first
    if (pCurlApiCallbacks->curlEasyPerformHookFn != NULL) {
        // NOTE: On failed status this will skip the real function call
        CHK_STATUS(pCurlApiCallbacks->curlEasyPerformHookFn(pCurlResponse));
    }

    // first check the request is not being terminated
    CHK(!ATOMIC_LOAD_BOOL(&pCurlRequest->requestInfo.terminating), retStatus);
    ATOMIC_STORE_BOOL(&pCurlRequest->blockedInCurl, TRUE);

    // NOTE: Blocking call!
    result = curl_easy_perform(pCurlResponse->pCurl);

    ATOMIC_STORE_BOOL(&pCurlRequest->blockedInCurl, FALSE);
    CHK(!ATOMIC_LOAD_BOOL(&pCurlRequest->requestInfo.terminating), retStatus);

    if (result != CURLE_OK && ATOMIC_LOAD_BOOL(&pCurlResponse->terminated)) {
        // The transmission has been force terminated.
        pCurlResponse->callInfo.httpStatus = HTTP_STATUS_CODE_REQUEST_TIMEOUT;
        pCurlResponse->callInfo.callResult = SERVICE_CALL_REQUEST_TIMEOUT;
    } else if (result != CURLE_OK) {
        curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_EFFECTIVE_URL, &url);
        DLOGW("curl perform failed for url %s with result %s: %s", url, curl_easy_strerror(result), pCurlResponse->callInfo.errorBuffer);

        pCurlResponse->callInfo.callResult = getServiceCallResultFromCurlStatus(result);
    } else {
        // get the response code and note the request completion time
        curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_RESPONSE_CODE, &pCurlResponse->callInfo.httpStatus);
        pCurlResponse->callInfo.callResult = getServiceCallResultFromHttpStatus(pCurlResponse->callInfo.httpStatus);
    }

    // warn and log request/response info if there was an error return code
    if (HTTP_STATUS_CODE_OK != pCurlResponse->callInfo.httpStatus) {
        curl_easy_getinfo(pCurlResponse->pCurl, CURLINFO_EFFECTIVE_URL, &url);
        headers[0] = '\0';
        for (header = pCurlResponse->pRequestHeaders; header; header = header->next) {
            STRCAT(headers, (PCHAR) "\n    ");
            STRCAT(headers, header->data);
        }

        DLOGW("HTTP Error %lu : Response: %s\nRequest URL: %s\nRequest Headers:%s", pCurlResponse->callInfo.httpStatus,
              pCurlResponse->callInfo.responseData, url, headers);
    }

CleanUp:

    CHK_LOG_ERR(retStatus);

    return STATUS_FAILED(retStatus) ? retStatus : status;
}

STATUS notifyDataAvailable(PCurlResponse pCurlResponse, UINT64 durationAvailable, UINT64 sizeAvailable)
{
    STATUS retStatus = STATUS_SUCCESS;
    CURLcode result;

    CHK(pCurlResponse != NULL, STATUS_NULL_ARG);

    // pCurlResponse should be a putMedia session
    if (!ATOMIC_LOAD_BOOL(&pCurlResponse->terminated)) {
        DLOGV("Note data received: duration(100ns): %" PRIu64 " bytes %" PRIu64 " for stream handle %" PRIu64, durationAvailable, sizeAvailable,
              pCurlResponse->pCurlRequest->uploadHandle);

        if (pCurlResponse->paused && pCurlResponse->pCurl != NULL) {
            pCurlResponse->paused = FALSE;
            // frequent pause unpause causes curl segfault in offline scenario
            THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
            // un-pause curl
            result = curl_easy_pause(pCurlResponse->pCurl, CURLPAUSE_SEND_CONT);
            if (result != CURLE_OK) {
                DLOGW("Failed to un-pause curl with error: %u. Curl object %p", result, pCurlResponse->pCurl);
            }
        }
    }

CleanUp:

    return retStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Curl callbacks
//////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T writeHeaderCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    PCurlResponse pCurlResponse;
    SIZE_T dataSize, nameLen, valueLen;
    PCHAR pValueStart, pValueEnd;
    PRequestHeader pRequestHeader;

    PCurlRequest pCurlRequest = (PCurlRequest) customData;
    if (pCurlRequest == NULL) {
        return CURL_READFUNC_ABORT;
    }

    pCurlResponse = pCurlRequest->pCurlResponse;
    dataSize = size * numItems;

    if (pCurlResponse == NULL) {
        return CURL_READFUNC_ABORT;
    }

    PCHAR pDelimiter = STRNCHR(pBuffer, (UINT32) dataSize, ':');
    if (pDelimiter != NULL) {
        // parse the header name
        nameLen = pDelimiter - pBuffer;

        // The value should be after the delimiter
        pDelimiter++;

        // Assuming the entire size and we will trim it later
        valueLen = dataSize - nameLen - 1;

        TRIMSTRALL(pDelimiter, (UINT32) valueLen, &pValueStart, &pValueEnd);
        valueLen = (pValueEnd - pValueStart) / SIZEOF(CHAR);

        // Only process if we have less than max
        if (nameLen < MAX_REQUEST_HEADER_NAME_LEN && valueLen < MAX_REQUEST_HEADER_VALUE_LEN) {
            createRequestHeader(pBuffer, (UINT32) nameLen, pValueStart, (UINT32) valueLen, &pRequestHeader);

            if (pRequestHeader != NULL) {
                stackQueueEnqueue(pCurlResponse->callInfo.pResponseHeaders, (UINT64) pRequestHeader);

                if (STRNCMP(KVS_REQUEST_ID_HEADER_NAME, pRequestHeader->pName, nameLen) == 0) {
                    pCurlResponse->callInfo.pRequestId = pRequestHeader;
                    DLOGI("RequestId: %.*s", pCurlResponse->callInfo.pRequestId->valueLen, pCurlResponse->callInfo.pRequestId->pValue);
                }
            }
        }
    }

    return dataSize;
}

SIZE_T postResponseWriteCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    DLOGV("postResponseWriteCallback (curl callback) invoked");
    PCHAR pNewBuffer;
    PCurlRequest pCurlRequest = (PCurlRequest) customData;
    PCurlResponse pCurlResponse;

    // Does not include the NULL terminator
    SIZE_T dataSize = size * numItems;

    if (pCurlRequest == NULL || pCurlRequest->pCurlResponse == NULL || ATOMIC_LOAD_BOOL(&pCurlRequest->requestInfo.terminating)) {
        return CURL_READFUNC_ABORT;
    }

    pCurlResponse = pCurlRequest->pCurlResponse;

    // Alloc and copy if needed
    pNewBuffer = (PCHAR) MEMALLOC(pCurlResponse->callInfo.responseDataLen + dataSize + SIZEOF(CHAR));
    if (pNewBuffer != NULL) {
        // Copy forward the old
        MEMCPY(pNewBuffer, pCurlResponse->callInfo.responseData, pCurlResponse->callInfo.responseDataLen);

        // Append the new data
        MEMCPY((PBYTE) pNewBuffer + pCurlResponse->callInfo.responseDataLen, pBuffer, dataSize);

        // Free the old if exists
        if (pCurlResponse->callInfo.responseData != NULL) {
            MEMFREE(pCurlResponse->callInfo.responseData);
        }

        pCurlResponse->callInfo.responseData = pNewBuffer;
        pCurlResponse->callInfo.responseDataLen += (UINT32) dataSize;
        pCurlResponse->callInfo.responseData[pCurlResponse->callInfo.responseDataLen] = '\0';
    } else {
        return CURL_READFUNC_ABORT;
    }

    return dataSize;
}

SIZE_T postWriteCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    DLOGV("postBodyStreamingWriteFunc (curl callback) invoked");
    PCurlResponse pCurlResponse;
    PCurlApiCallbacks pCurlApiCallbacks;
    SIZE_T dataSize = size * numItems, bufferSize;
    STATUS retStatus = STATUS_SUCCESS;
    PCurlRequest pCurlRequest = (PCurlRequest) customData;

    if (pCurlRequest == NULL || pCurlRequest->pCurlResponse == NULL || pCurlRequest->pCurlApiCallbacks == NULL ||
        ATOMIC_LOAD_BOOL(&pCurlRequest->requestInfo.terminating)) {
        return CURL_READFUNC_ABORT;
    }

    pCurlResponse = pCurlRequest->pCurlResponse;
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;

    DLOGD("Curl post body write function for stream with handle: %s and upload handle: %" PRIu64 " returned: %.*s", pCurlRequest->streamName,
          pCurlResponse->pCurlRequest->uploadHandle, dataSize, pBuffer);

    bufferSize = dataSize;
    if (pCurlApiCallbacks->curlWriteCallbackHookFn != NULL) {
        CHK_STATUS(pCurlApiCallbacks->curlWriteCallbackHookFn(pCurlResponse, pBuffer, (UINT32) dataSize, &pBuffer, (PUINT32) &bufferSize));
    }

    CHK_STATUS(kinesisVideoStreamParseFragmentAck(pCurlResponse->pCurlRequest->streamHandle, pCurlResponse->pCurlRequest->uploadHandle, pBuffer,
                                                  (UINT32) bufferSize));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        DLOGW("Failed to submit ACK: %.*s with status code: 0x%08x", bufferSize, pBuffer, retStatus);

    } else {
        DLOGV("Processed ACK OK.");
    }

    return dataSize;
}

SIZE_T postReadCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    DLOGV("postBodyStreamingReadFunc (curl callback) invoked");
    PCurlResponse pCurlResponse = NULL;
    PCurlApiCallbacks pCurlApiCallbacks;
    SIZE_T bufferSize = size * numItems, bytesWritten = 0;
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 retrievedSize = 0;
    UPLOAD_HANDLE uploadHandle;
    PCurlRequest pCurlRequest = (PCurlRequest) customData;

    if (pCurlRequest == NULL || pCurlRequest->pCurlResponse == NULL || pCurlRequest->pCurlApiCallbacks == NULL) {
        bytesWritten = CURL_READFUNC_ABORT;
        CHK(FALSE, retStatus);
    }

    pCurlResponse = pCurlRequest->pCurlResponse;
    pCurlApiCallbacks = pCurlRequest->pCurlApiCallbacks;
    uploadHandle = pCurlResponse->pCurlRequest->uploadHandle;

    if (pCurlResponse->paused) {
        bytesWritten = CURL_READFUNC_PAUSE;
        CHK(FALSE, retStatus);
    }

    if (pCurlResponse->endOfStream || ATOMIC_LOAD_BOOL(&pCurlRequest->requestInfo.terminating)) {
        DLOGI("Closing connection for upload stream handle: %" PRIu64, uploadHandle);
        CHK(FALSE, retStatus);
    }

    retStatus =
        getKinesisVideoStreamData(pCurlResponse->pCurlRequest->streamHandle, uploadHandle, (PBYTE) pBuffer, (UINT32) bufferSize, &retrievedSize);

    if (pCurlApiCallbacks->curlReadCallbackHookFn != NULL) {
        retStatus =
            pCurlApiCallbacks->curlReadCallbackHookFn(pCurlResponse, uploadHandle, (PBYTE) pBuffer, (UINT32) bufferSize, &retrievedSize, retStatus);
    }

    bytesWritten = (SIZE_T) retrievedSize;

    DLOGV("Get Stream data returned: buffer size: %u written bytes: %u for upload handle: %" PRIu64 " current stream handle: %" PRIu64, bufferSize,
          bytesWritten, uploadHandle, pCurlResponse->pCurlRequest->streamHandle);

    // The return should be OK, no more data or an end of stream
    switch (retStatus) {
        case STATUS_SUCCESS:
        case STATUS_NO_MORE_DATA_AVAILABLE:
            // Media pipeline thread might be blocked due to heap or temporal limit.
            // Pause curl read and wait for persisted ack.
            if (bytesWritten == 0) {
                DLOGD("Pausing CURL read for upload handle: %" PRIu64, uploadHandle);
                bytesWritten = CURL_READFUNC_PAUSE;
            }
            break;

        case STATUS_END_OF_STREAM:
            DLOGI("Reported end-of-stream for stream %s. Upload handle: %" PRIu64, pCurlRequest->streamName, uploadHandle);

            pCurlResponse->endOfStream = TRUE;

            // Output the remaining bytes

            break;

        case STATUS_AWAITING_PERSISTED_ACK:
            // If bytes_written == 0, set it to pause to exit the loop
            if (bytesWritten == 0) {
                DLOGD("Pausing CURL read for upload handle: %" PRIu64 " waiting for last ack.", uploadHandle);
                bytesWritten = CURL_READFUNC_PAUSE;
            }
            break;

        case STATUS_UPLOAD_HANDLE_ABORTED:
            DLOGW("Reported abort-connection for Upload handle: %" PRIu64, uploadHandle);
            bytesWritten = CURL_READFUNC_ABORT;

            // Graceful shutdown as PIC is aware of terminated stream
            pCurlResponse->endOfStream = TRUE;
            break;

        default:
            DLOGE("Failed to get data from the stream with an error: 0x%08x", retStatus);

            // set bytes_written to terminate and close the connection
            bytesWritten = CURL_READFUNC_ABORT;
    }

CleanUp:

    if (bytesWritten != CURL_READFUNC_ABORT && bytesWritten != CURL_READFUNC_PAUSE) {
        DLOGD("Wrote %u bytes to Kinesis Video. Upload stream handle: %" PRIu64, bytesWritten, uploadHandle);

        if (bytesWritten != 0 && pCurlResponse->debugDumpFile) {
            retStatus = writeFile(pCurlResponse->debugDumpFilePath, TRUE, TRUE, (PBYTE) pBuffer, bytesWritten);
            if (STATUS_FAILED(retStatus)) {
                DLOGW("Failed to write to debug dump file with error: 0x%08x", retStatus);
            }
        }
    } else if (bytesWritten == CURL_READFUNC_PAUSE) {
        pCurlResponse->paused = TRUE;
    }

    // Since curl is about to terminate gracefully, set flag to prevent shutdown thread from timing it out.
    if ((bytesWritten == CURL_READFUNC_ABORT || bytesWritten == 0) && pCurlResponse != NULL) {
        ATOMIC_STORE_BOOL(&pCurlResponse->terminated, TRUE);
    }

    return bytesWritten;
}
