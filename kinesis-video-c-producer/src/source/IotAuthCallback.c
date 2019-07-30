/**
 * Kinesis Video Producer IotCredential Auth Callback
 */
#define LOG_CLASS "IotAuthCallbacks"
#include "Include_i.h"

/*
 * Create IoT credentials callback
 */
STATUS createIotAuthCallbacks(PClientCallbacks pCallbacksProvider,
                             PCHAR iotGetCredentialEndpoint,
                             PCHAR certPath,
                             PCHAR privateKeyPath,
                             PCHAR caCertPath,
                             PCHAR roleAlias,
                             PCHAR streamName,
                             PAuthCallbacks *ppIotAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    PIotAuthCallbacks pIotAuthCallbacks = NULL;

    CHK(pCallbacksProvider != NULL && ppIotAuthCallbacks != NULL && iotGetCredentialEndpoint != NULL &&
            certPath != NULL && privateKeyPath != NULL && caCertPath != NULL &&
            roleAlias != NULL && streamName != NULL, STATUS_NULL_ARG);

    // Allocate the entire structure
    pIotAuthCallbacks = (PIotAuthCallbacks) MEMCALLOC(1, SIZEOF(IotAuthCallbacks));
    CHK(pIotAuthCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pIotAuthCallbacks->authCallbacks.version = AUTH_CALLBACKS_CURRENT_VERSION;
    pIotAuthCallbacks->authCallbacks.customData = (UINT64) pIotAuthCallbacks;

    // Store the back pointer as we will be using the other callbacks
    pIotAuthCallbacks->pCallbacksProvider = (PCallbacksProvider) pCallbacksProvider;

    // Set the callbacks
    pIotAuthCallbacks->authCallbacks.getStreamingTokenFn = getStreamingTokenIotFunc;
    pIotAuthCallbacks->authCallbacks.getSecurityTokenFn = getSecurityTokenIotFunc;
    pIotAuthCallbacks->authCallbacks.freeAuthCallbacksFn = freeIotAuthCallbacksFunc;
    pIotAuthCallbacks->authCallbacks.getDeviceCertificateFn = NULL;
    pIotAuthCallbacks->authCallbacks.deviceCertToTokenFn = NULL;
    pIotAuthCallbacks->authCallbacks.getDeviceFingerprintFn = NULL;

    CHK(STRNLEN(iotGetCredentialEndpoint, MAX_URI_CHAR_LEN + 1) <= MAX_URI_CHAR_LEN, MAX_URI_CHAR_LEN);
    STRNCPY(pIotAuthCallbacks->iotGetCredentialEndpoint, iotGetCredentialEndpoint, MAX_URI_CHAR_LEN);

    CHK(STRNLEN(roleAlias, MAX_ROLE_ALIAS_LEN + 1) <= MAX_ROLE_ALIAS_LEN, STATUS_MAX_ROLE_ALIAS_LEN_EXCEEDED);
    STRNCPY(pIotAuthCallbacks->roleAlias, roleAlias, MAX_ROLE_ALIAS_LEN);

    CHK(STRNLEN(privateKeyPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    STRNCPY(pIotAuthCallbacks->privateKeyPath, privateKeyPath, MAX_PATH_LEN);

    CHK(STRNLEN(caCertPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    STRNCPY(pIotAuthCallbacks->caCertPath, caCertPath, MAX_PATH_LEN);

    CHK(STRNLEN(certPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    STRNCPY(pIotAuthCallbacks->certPath, certPath, MAX_PATH_LEN);

    CHK(STRNLEN(streamName, MAX_STREAM_NAME_LEN + 1) <= MAX_STREAM_NAME_LEN, STATUS_INVALID_STREAM_NAME_LENGTH);
    STRNCPY(pIotAuthCallbacks->streamName, streamName, MAX_STREAM_NAME_LEN);

    CHK_STATUS(iotCurlHandler(pIotAuthCallbacks));

    CHK_STATUS(addAuthCallbacks(pCallbacksProvider, (PAuthCallbacks) pIotAuthCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeIotAuthCallbacks((PAuthCallbacks*) &pIotAuthCallbacks);
        pIotAuthCallbacks = NULL;
    }
    // Set the return value if it's not NULL
    if (ppIotAuthCallbacks != NULL) {
        *ppIotAuthCallbacks = (PAuthCallbacks) pIotAuthCallbacks;
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the IotCredential Auth callback object
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 */
STATUS freeIotAuthCallbacks(PAuthCallbacks* ppIotAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    //TODO abort ongoing request if free is called.

    PIotAuthCallbacks pIotAuthCallbacks = NULL;

    CHK(ppIotAuthCallbacks != NULL, STATUS_NULL_ARG);

    pIotAuthCallbacks = (PIotAuthCallbacks) *ppIotAuthCallbacks;

    // Call is idempotent
    CHK(pIotAuthCallbacks != NULL, retStatus);

    curl_easy_cleanup(pIotAuthCallbacks->curl);

    // Release the underlying AWS credentials object
    if (pIotAuthCallbacks->pAwsCredentials != NULL) {
        freeAwsCredentials(&pIotAuthCallbacks->pAwsCredentials);
    }

    if (pIotAuthCallbacks->responseData != NULL) {
        MEMFREE(pIotAuthCallbacks->responseData);
    }

    // Release the object
    MEMFREE(pIotAuthCallbacks);

    // Set the pointer to NULL
    *ppIotAuthCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeIotAuthCallbacksFunc(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PIotAuthCallbacks pAuthCallbacks;

    CHK(customData != NULL, STATUS_NULL_ARG);
    pAuthCallbacks = (PIotAuthCallbacks) *customData;
    CHK_STATUS(freeIotAuthCallbacks((PAuthCallbacks*) &pAuthCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getStreamingTokenIotFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode,
                                PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);
    UINT64 currentTime;

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS, status = STATUS_SUCCESS;
    PIotAuthCallbacks pIotAuthCallbacks = (PIotAuthCallbacks) customData;

    CHK(pIotAuthCallbacks != NULL, STATUS_NULL_ARG);

    // Refresh the credentials by calling IoT endpoint if needed
    currentTime = pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    if (currentTime + IOT_CREDENTIAL_FETCH_GRACE_PERIOD > pIotAuthCallbacks->pAwsCredentials->expiration) {
        CHK_STATUS(iotCurlHandler(pIotAuthCallbacks));
    }

CleanUp:

    status = getStreamingTokenResultEvent(pServiceCallContext->customData,
                                          STATUS_SUCCEEDED(retStatus) ? SERVICE_CALL_RESULT_OK : SERVICE_CALL_UNKNOWN,
                                          (PBYTE) pIotAuthCallbacks->pAwsCredentials,
                                          pIotAuthCallbacks->pAwsCredentials->size,
                                          pIotAuthCallbacks->pAwsCredentials->expiration);

    if (!STATUS_SUCCEEDED(status) &&
        pIotAuthCallbacks->pCallbacksProvider->pStreamCallbacks->streamErrorReportFn != NULL) {
        pIotAuthCallbacks->pCallbacksProvider->pStreamCallbacks->streamErrorReportFn(
                pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData,
                INVALID_STREAM_HANDLE_VALUE,
                INVALID_UPLOAD_HANDLE_VALUE,
                0,
                status);
    }

    LEAVES();
    return retStatus;
}

STATUS getSecurityTokenIotFunc(UINT64 customData, PBYTE *ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;

    PIotAuthCallbacks pIotAuthCallbacks = (PIotAuthCallbacks) customData;
    CHK(pIotAuthCallbacks != NULL && ppBuffer != NULL && pSize != NULL && pExpiration != NULL,
        STATUS_NULL_ARG);

    // Refresh the credentials by calling IoT endpoint if needed
    currentTime = pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    if (currentTime + IOT_CREDENTIAL_FETCH_GRACE_PERIOD > pIotAuthCallbacks->pAwsCredentials->expiration) {
        CHK_STATUS(iotCurlHandler(pIotAuthCallbacks));
    }

    *pExpiration = pIotAuthCallbacks->pAwsCredentials->expiration;
    *pSize = pIotAuthCallbacks->pAwsCredentials->size;
    *ppBuffer = (PBYTE) pIotAuthCallbacks->pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS parseIotResponse(PIotAuthCallbacks pIotAuthCallbacks)
{

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    UINT32 i, resultLen, accessKeyIdLen = 0, secretKeyLen = 0, sessionTokenLen = 0, expirationTimestampLen = 0;
    INT32 tokenCount;
    jsmn_parser parser;
    jsmntok_t tokens[MAX_JSON_TOKEN_COUNT];
    PCHAR accessKeyId = NULL, secretKey = NULL, sessionToken = NULL, expirationTimestamp = NULL, pResponseStr = NULL;
    UINT64 expiration, currentTime;
    CHAR expirationTimestampStr[MAX_EXPIRATION_LEN + 1];

    CHK(pIotAuthCallbacks != NULL, STATUS_NULL_ARG);

    resultLen = pIotAuthCallbacks->responseDataLen;
    pResponseStr = pIotAuthCallbacks->responseData;
    CHK(resultLen > 0, STATUS_IOT_FAILED);

    jsmn_init(&parser);
    tokenCount = jsmn_parse(&parser, pResponseStr, resultLen, tokens, SIZEOF(tokens) / SIZEOF(jsmntok_t));

    CHK(tokenCount > 1, STATUS_INVALID_API_CALL_RETURN_JSON);
    CHK(tokens[0].type == JSMN_OBJECT, STATUS_INVALID_API_CALL_RETURN_JSON);

    for (i = 1; i < tokenCount; i++) {
        if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "accessKeyId")) {
            accessKeyIdLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(accessKeyIdLen <= MAX_ACCESS_KEY_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            accessKeyId = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "secretAccessKey")) {
            secretKeyLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(secretKeyLen <= MAX_SECRET_KEY_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            secretKey = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "sessionToken")) {
            sessionTokenLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(sessionTokenLen <= MAX_SESSION_TOKEN_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            sessionToken = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, "expiration")) {
            expirationTimestampLen = (UINT32) (tokens[i + 1].end - tokens[i + 1].start);
            CHK(expirationTimestampLen <= MAX_EXPIRATION_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            expirationTimestamp = pResponseStr + tokens[i + 1].start;
            MEMCPY(expirationTimestampStr, expirationTimestamp, expirationTimestampLen);
            expirationTimestampStr[expirationTimestampLen] = '\0';
            i++;
        }
    }

    CHK(accessKeyId != NULL && secretKey != NULL && sessionToken != NULL, STATUS_IOT_FAILED);

    currentTime = pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(convertTimestampToEpoch(expirationTimestampStr, currentTime / HUNDREDS_OF_NANOS_IN_A_SECOND, &expiration));
    DLOGD("Iot credential expiration time %" PRIu64, expiration / HUNDREDS_OF_NANOS_IN_A_SECOND);

    if (pIotAuthCallbacks->pAwsCredentials != NULL) {
        freeAwsCredentials(&pIotAuthCallbacks->pAwsCredentials);
        pIotAuthCallbacks->pAwsCredentials = NULL;
    }

    // Fix-up the expiration to be no more than max enforced token rotation to avoid extra token rotations
    // as we are caching the returned value which is likely to be an hour but we are enforcing max
    // rotation to be more frequent.
    expiration = MIN(expiration, currentTime + MAX_ENFORCED_TOKEN_EXPIRATION_DURATION);

    CHK_STATUS(createAwsCredentials(accessKeyId,
                                    accessKeyIdLen,
                                    secretKey,
                                    secretKeyLen,
                                    sessionToken,
                                    sessionTokenLen,
                                    expiration,
                                    &pIotAuthCallbacks->pAwsCredentials));

CleanUp:

    LEAVES();
    return retStatus;
}

SIZE_T writeIotResponseCallback(PCHAR pBuffer, SIZE_T size, SIZE_T numItems, PVOID customData)
{
    PIotAuthCallbacks pIotAuthCallbacks = (PIotAuthCallbacks) customData;

    // Does not include the NULL terminator
    SIZE_T dataSize = size * numItems;

    if (pIotAuthCallbacks == NULL) {
        return CURL_READFUNC_ABORT;
    }

    // Alloc and copy if needed
    PCHAR pNewBuffer = pIotAuthCallbacks->responseData == NULL ?
                       (PCHAR) MEMALLOC(pIotAuthCallbacks->responseDataLen + dataSize + SIZEOF(CHAR)) :
                       (PCHAR) REALLOC(pIotAuthCallbacks->responseData,
                                       pIotAuthCallbacks->responseDataLen + dataSize + SIZEOF(CHAR));
    if (pNewBuffer != NULL) {
        // Append the new data
        MEMCPY((PBYTE)pNewBuffer + pIotAuthCallbacks->responseDataLen, pBuffer, dataSize);

        pIotAuthCallbacks->responseData = pNewBuffer;
        pIotAuthCallbacks->responseDataLen += dataSize;
        pIotAuthCallbacks->responseData[pIotAuthCallbacks->responseDataLen] = '\0';
    } else {
        return CURL_READFUNC_ABORT;
    }

    return dataSize;
}

STATUS iotCurlHandler(PIotAuthCallbacks pIotAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CURL* curl = NULL;
    CURLcode res;
    PCHAR url;
    LONG httpStatusCode;
    CHAR errorBuffer[CURL_ERROR_SIZE];
    errorBuffer[0] = '\0';
    UINT64 thingNameStrLen = 0, currentTime;
    UINT32 formatLen = 0;
    CHAR thingName[SIZEOF(IOT_THING_NAME_HEADER) + MAX_STREAM_NAME_LEN + SIZEOF(CHAR)];
    CHAR serviceUrl[MAX_URI_CHAR_LEN + 1];
    struct curl_slist* headerList = NULL;

    currentTime = pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    // check if existing pAwsCredential is still valid.
    CHK(pIotAuthCallbacks->pAwsCredentials == NULL ||
        (currentTime + MIN_STREAMING_TOKEN_EXPIRATION_DURATION) >= pIotAuthCallbacks->pAwsCredentials->expiration, retStatus);
    CHK(pIotAuthCallbacks->curl == NULL, STATUS_INTERNAL_ERROR);
    curl = pIotAuthCallbacks->curl;

    formatLen = SNPRINTF(serviceUrl, MAX_URI_CHAR_LEN, "%s%s%s%c%s%s", SERVICE_CALL_PREFIX,
                         pIotAuthCallbacks->iotGetCredentialEndpoint,
                         ROLE_ALIASES_PATH,
                         '/',
                         pIotAuthCallbacks->roleAlias,
                         CREDENTIAL_SERVICE);
    CHK(formatLen > 0 && formatLen < MAX_URI_CHAR_LEN, STATUS_IOT_FAILED);

    thingNameStrLen = STRLEN(pIotAuthCallbacks->streamName) + STRLEN(IOT_THING_NAME_HEADER) + SIZEOF(CHAR);

    // old responseData will be freed at next curl write data call
    pIotAuthCallbacks->responseDataLen = 0;

    formatLen =
            SNPRINTF(thingName, thingNameStrLen, "%s%s", IOT_THING_NAME_HEADER, pIotAuthCallbacks->streamName);
    CHK(formatLen > 0 && formatLen < thingNameStrLen, STATUS_IOT_FAILED);

    // CURL global initialization
    CHK(0 == curl_global_init(CURL_GLOBAL_ALL), STATUS_CURL_LIBRARY_INIT_FAILED);
    curl = curl_easy_init();
    CHK(curl != NULL, STATUS_CURL_INIT_FAILED);

    headerList = curl_slist_append(headerList, thingName);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, serviceUrl);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, IOT_CERT_TYPE);
    curl_easy_setopt(curl, CURLOPT_SSLCERT, pIotAuthCallbacks->certPath);
    curl_easy_setopt(curl, CURLOPT_SSLKEY, pIotAuthCallbacks->privateKeyPath);
    curl_easy_setopt(curl, CURLOPT_CAINFO, pIotAuthCallbacks->caCertPath);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeIotResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pIotAuthCallbacks);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, REQUEST_COMPLETION_TIMEOUT_MS);

    // Setting up limits for curl timeout
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, CURLOPT_LOW_SPEED_TIME_VALUE);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, CURLOPT_LOW_SPEED_LIMIT_VALUE);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        DLOGE("IoT curl perform failed for url %s with result %s : %s ", url, curl_easy_strerror(res), errorBuffer);
        CHK(FALSE, STATUS_IOT_FAILED);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
    CHK_ERR(httpStatusCode == HTTP_STATUS_CODE_OK, STATUS_IOT_FAILED, "Iot get credential response http status %lu was not ok", httpStatusCode);
    CHK_STATUS(parseIotResponse(pIotAuthCallbacks));

CleanUp:

    if (headerList != NULL) {
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
        pIotAuthCallbacks->curl = NULL;
    }

    return retStatus;
}
