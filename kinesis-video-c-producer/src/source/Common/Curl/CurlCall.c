/**
 * Helper functionality for CURL
 */
#define LOG_CLASS "CurlCall"
#include "../Include_i.h"

STATUS blockingCurlCall(PRequestInfo pRequestInfo, PCallInfo pCallInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CURL* curl = NULL;
    CURLcode res;
    PCHAR url;
    UINT32 httpStatusCode;
    struct curl_slist* pHeaderList = NULL;
    CHAR errorBuffer[CURL_ERROR_SIZE];
    errorBuffer[0] = '\0';
    UINT32 length;
    STAT_STRUCT entryStat;
    BOOL secureConnection;

    CHK(pRequestInfo != NULL && pCallInfo != NULL, STATUS_NULL_ARG);

    // CURL global initialization
    CHK(0 == curl_global_init(CURL_GLOBAL_ALL), STATUS_CURL_LIBRARY_INIT_FAILED);
    curl = curl_easy_init();
    CHK(curl != NULL, STATUS_CURL_INIT_FAILED);

    CHK_STATUS(createCurlHeaderList(pRequestInfo, &pHeaderList));

    // set verification for SSL connections
    CHK_STATUS(requestRequiresSecureConnection(pRequestInfo->url, &secureConnection));
    if (secureConnection) {
        // Use the default cert store at /etc/ssl in most common platforms
        if (pRequestInfo->certPath[0] != '\0') {
            CHK(0 == FSTAT(pRequestInfo->certPath, &entryStat), STATUS_DIRECTORY_ENTRY_STAT_ERROR);

            if (S_ISDIR(entryStat.st_mode)) {
                // Assume it's the path as we have a directory
                curl_easy_setopt(curl, CURLOPT_CAPATH, pRequestInfo->certPath);
            } else {
                // We should check for the extension being PEM
                length = (UINT32) STRNLEN(pRequestInfo->certPath, MAX_PATH_LEN);
                CHK(length > ARRAY_SIZE(CA_CERT_PEM_FILE_EXTENSION), STATUS_INVALID_ARG_LEN);
                CHK(0 == STRCMPI(CA_CERT_PEM_FILE_EXTENSION, &pRequestInfo->certPath[length - ARRAY_SIZE(CA_CERT_PEM_FILE_EXTENSION) + 1]),
                    STATUS_INVALID_CA_CERT_PATH);

                curl_easy_setopt(curl, CURLOPT_CAINFO, pRequestInfo->certPath);
            }
        }

        // Enforce the public cert verification - even though this is the default
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pHeaderList);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, pRequestInfo->url);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, getSslCertNameFromType(pRequestInfo->certType));
    curl_easy_setopt(curl, CURLOPT_SSLCERT, pRequestInfo->sslCertPath);
    curl_easy_setopt(curl, CURLOPT_SSLKEY, pRequestInfo->sslPrivateKeyPath);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCurlResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pCallInfo);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, pRequestInfo->connectionTimeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    if (pRequestInfo->completionTimeout != SERVICE_CALL_INFINITE_TIMEOUT) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, pRequestInfo->completionTimeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    // Setting up limits for curl timeout
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, pRequestInfo->lowSpeedTimeLimit / HUNDREDS_OF_NANOS_IN_A_SECOND);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, pRequestInfo->lowSpeedLimit);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        DLOGE("Curl perform failed for url %s with result %s : %s ", url, curl_easy_strerror(res), errorBuffer);
        CHK(FALSE, STATUS_IOT_FAILED);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
    CHK_ERR(httpStatusCode == HTTP_STATUS_CODE_OK, STATUS_IOT_FAILED, "Curl call response failed with http status %lu", httpStatusCode);

CleanUp:

    if (pHeaderList != NULL) {
        curl_slist_free_all(pHeaderList);
    }

    if (curl != NULL) {
        curl_easy_cleanup(curl);
    }

    LEAVES();
    return retStatus;
}

SIZE_T writeCurlResponseCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    PCallInfo pCallInfo = (PCallInfo) customData;

    // Does not include the NULL terminator
    SIZE_T dataSize = size * numItems;

    if (pCallInfo == NULL) {
        return CURL_READFUNC_ABORT;
    }

    // Alloc and copy if needed
    PCHAR pNewBuffer = pCallInfo->responseData == NULL
        ? (PCHAR) MEMALLOC(pCallInfo->responseDataLen + dataSize + SIZEOF(CHAR))
        : (PCHAR) REALLOC(pCallInfo->responseData, pCallInfo->responseDataLen + dataSize + SIZEOF(CHAR));
    if (pNewBuffer != NULL) {
        // Append the new data
        MEMCPY((PBYTE) pNewBuffer + pCallInfo->responseDataLen, pBuffer, dataSize);

        pCallInfo->responseData = pNewBuffer;
        pCallInfo->responseDataLen += (UINT32) dataSize;
        pCallInfo->responseData[pCallInfo->responseDataLen] = '\0';
    } else {
        return CURL_READFUNC_ABORT;
    }

    return dataSize;
}

STATUS createCurlHeaderList(PRequestInfo pRequestInfo, struct curl_slist** ppHeaderList)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    struct curl_slist* pHeaderList = NULL;
    PSingleListNode pCurNode;
    UINT64 item;
    PCHAR pCurPtr;
    CHAR headerBuffer[MAX_REQUEST_HEADER_STRING_LEN];
    PRequestHeader pRequestHeader;

    CHK(pRequestInfo != NULL && ppHeaderList != NULL, STATUS_NULL_ARG);

    // Add headers using a temporary buffer accounting for the delimiter
    CHK_STATUS(singleListGetHeadNode(pRequestInfo->pRequestHeaders, &pCurNode));

    // Iterate through the headers
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pRequestHeader = (PRequestHeader) item;
        pCurPtr = headerBuffer;
        MEMCPY(pCurPtr, pRequestHeader->pName, pRequestHeader->nameLen * SIZEOF(CHAR));
        pCurPtr += pRequestHeader->nameLen;
        MEMCPY(pCurPtr, REQUEST_HEADER_DELIMITER, REQUEST_HEADER_DELIMITER_SIZE);
        pCurPtr += REQUEST_HEADER_DELIMITER_SIZE;
        MEMCPY(pCurPtr, pRequestHeader->pValue, pRequestHeader->valueLen * SIZEOF(CHAR));
        pCurPtr += pRequestHeader->valueLen;
        *pCurPtr = '\0';

        pHeaderList = curl_slist_append(pHeaderList, headerBuffer);

        // Iterate
        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

    *ppHeaderList = pHeaderList;

CleanUp:

    LEAVES();
    return retStatus;
}
