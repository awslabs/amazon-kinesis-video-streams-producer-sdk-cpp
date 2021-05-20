/**
 * Kinesis Video Producer IoT based Credential Provider
 */
#define LOG_CLASS "IotCredentialProvider"
#include "Include_i.h"

STATUS createIotCredentialProviderWithTime(PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath, PCHAR caCertPath, PCHAR roleAlias,
                                           PCHAR thingName, GetCurrentTimeFunc getCurrentTimeFn, UINT64 customData,
                                           BlockingServiceCallFunc serviceCallFn, PAwsCredentialProvider* ppCredentialProvider)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PIotCredentialProvider pIotCredentialProvider = NULL;

    CHK(ppCredentialProvider != NULL && iotGetCredentialEndpoint != NULL && certPath != NULL && privateKeyPath != NULL && roleAlias != NULL &&
            thingName != NULL && serviceCallFn != NULL,
        STATUS_NULL_ARG);

    pIotCredentialProvider = (PIotCredentialProvider) MEMCALLOC(1, SIZEOF(IotCredentialProvider));
    CHK(pIotCredentialProvider != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pIotCredentialProvider->credentialProvider.getCredentialsFn = getIotCredentials;

    // Store the time functionality and specify default if NULL
    pIotCredentialProvider->getCurrentTimeFn = (getCurrentTimeFn == NULL) ? commonDefaultGetCurrentTimeFunc : getCurrentTimeFn;
    pIotCredentialProvider->customData = customData;

    CHK(STRNLEN(iotGetCredentialEndpoint, MAX_URI_CHAR_LEN + 1) <= MAX_URI_CHAR_LEN, MAX_URI_CHAR_LEN);
    STRNCPY(pIotCredentialProvider->iotGetCredentialEndpoint, iotGetCredentialEndpoint, MAX_URI_CHAR_LEN);

    CHK(STRNLEN(roleAlias, MAX_ROLE_ALIAS_LEN + 1) <= MAX_ROLE_ALIAS_LEN, STATUS_MAX_ROLE_ALIAS_LEN_EXCEEDED);
    STRNCPY(pIotCredentialProvider->roleAlias, roleAlias, MAX_ROLE_ALIAS_LEN);

    CHK(STRNLEN(privateKeyPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    STRNCPY(pIotCredentialProvider->privateKeyPath, privateKeyPath, MAX_PATH_LEN);

    if (caCertPath != NULL) {
        CHK(STRNLEN(caCertPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
        STRNCPY(pIotCredentialProvider->caCertPath, caCertPath, MAX_PATH_LEN);
    }

    CHK(STRNLEN(certPath, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    STRNCPY(pIotCredentialProvider->certPath, certPath, MAX_PATH_LEN);

    CHK(STRNLEN(thingName, MAX_IOT_THING_NAME_LEN + 1) <= MAX_IOT_THING_NAME_LEN, STATUS_MAX_IOT_THING_NAME_LENGTH);
    STRNCPY(pIotCredentialProvider->thingName, thingName, MAX_IOT_THING_NAME_LEN);

    pIotCredentialProvider->serviceCallFn = serviceCallFn;

    CHK_STATUS(iotCurlHandler(pIotCredentialProvider));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeIotCredentialProvider((PAwsCredentialProvider*) &pIotCredentialProvider);
        pIotCredentialProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppCredentialProvider != NULL) {
        *ppCredentialProvider = (PAwsCredentialProvider) pIotCredentialProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS freeIotCredentialProvider(PAwsCredentialProvider* ppCredentialProvider)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PIotCredentialProvider pIotCredentialProvider = NULL;

    CHK(ppCredentialProvider != NULL, STATUS_NULL_ARG);

    pIotCredentialProvider = (PIotCredentialProvider) *ppCredentialProvider;

    // Call is idempotent
    CHK(pIotCredentialProvider != NULL, retStatus);

    // Release the underlying AWS credentials object
    freeAwsCredentials(&pIotCredentialProvider->pAwsCredentials);

    // Release the object
    MEMFREE(pIotCredentialProvider);

    // Set the pointer to NULL
    *ppCredentialProvider = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getIotCredentials(PAwsCredentialProvider pCredentialProvider, PAwsCredentials* ppAwsCredentials)
{
    ENTERS();

    STATUS retStatus = STATUS_SUCCESS;

    PIotCredentialProvider pIotCredentialProvider = (PIotCredentialProvider) pCredentialProvider;

    CHK(pIotCredentialProvider != NULL && ppAwsCredentials != NULL, STATUS_NULL_ARG);

    // Fill the credentials
    CHK_STATUS(iotCurlHandler(pIotCredentialProvider));

    *ppAwsCredentials = pIotCredentialProvider->pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS parseIotResponse(PIotCredentialProvider pIotCredentialProvider, PCallInfo pCallInfo)
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

    CHK(pIotCredentialProvider != NULL && pCallInfo != NULL, STATUS_NULL_ARG);

    resultLen = pCallInfo->responseDataLen;
    pResponseStr = pCallInfo->responseData;
    CHK(resultLen > 0, STATUS_IOT_FAILED);

    jsmn_init(&parser);
    tokenCount = jsmn_parse(&parser, pResponseStr, resultLen, tokens, SIZEOF(tokens) / SIZEOF(jsmntok_t));

    CHK(tokenCount > 1, STATUS_INVALID_API_CALL_RETURN_JSON);
    CHK(tokens[0].type == JSMN_OBJECT, STATUS_INVALID_API_CALL_RETURN_JSON);

    for (i = 1; i < (UINT32) tokenCount; i++) {
        if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "accessKeyId")) {
            accessKeyIdLen = (UINT32)(tokens[i + 1].end - tokens[i + 1].start);
            CHK(accessKeyIdLen <= MAX_ACCESS_KEY_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            accessKeyId = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "secretAccessKey")) {
            secretKeyLen = (UINT32)(tokens[i + 1].end - tokens[i + 1].start);
            CHK(secretKeyLen <= MAX_SECRET_KEY_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            secretKey = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, (PCHAR) "sessionToken")) {
            sessionTokenLen = (UINT32)(tokens[i + 1].end - tokens[i + 1].start);
            CHK(sessionTokenLen <= MAX_SESSION_TOKEN_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            sessionToken = pResponseStr + tokens[i + 1].start;
            i++;
        } else if (compareJsonString(pResponseStr, &tokens[i], JSMN_STRING, "expiration")) {
            expirationTimestampLen = (UINT32)(tokens[i + 1].end - tokens[i + 1].start);
            CHK(expirationTimestampLen <= MAX_EXPIRATION_LEN, STATUS_INVALID_API_CALL_RETURN_JSON);
            expirationTimestamp = pResponseStr + tokens[i + 1].start;
            MEMCPY(expirationTimestampStr, expirationTimestamp, expirationTimestampLen);
            expirationTimestampStr[expirationTimestampLen] = '\0';
            i++;
        }
    }

    CHK(accessKeyId != NULL && secretKey != NULL && sessionToken != NULL, STATUS_IOT_FAILED);

    currentTime = pIotCredentialProvider->getCurrentTimeFn(pIotCredentialProvider->customData);
    CHK_STATUS(convertTimestampToEpoch(expirationTimestampStr, currentTime / HUNDREDS_OF_NANOS_IN_A_SECOND, &expiration));
    DLOGD("Iot credential expiration time %" PRIu64, expiration / HUNDREDS_OF_NANOS_IN_A_SECOND);

    if (pIotCredentialProvider->pAwsCredentials != NULL) {
        freeAwsCredentials(&pIotCredentialProvider->pAwsCredentials);
        pIotCredentialProvider->pAwsCredentials = NULL;
    }

    // Fix-up the expiration to be no more than max enforced token rotation to avoid extra token rotations
    // as we are caching the returned value which is likely to be an hour but we are enforcing max
    // rotation to be more frequent.
    expiration = MIN(expiration, currentTime + MAX_ENFORCED_TOKEN_EXPIRATION_DURATION);

    CHK_STATUS(createAwsCredentials(accessKeyId, accessKeyIdLen, secretKey, secretKeyLen, sessionToken, sessionTokenLen, expiration,
                                    &pIotCredentialProvider->pAwsCredentials));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS iotCurlHandler(PIotCredentialProvider pIotCredentialProvider)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;
    UINT32 formatLen = 0;
    CHAR serviceUrl[MAX_URI_CHAR_LEN + 1];
    PRequestInfo pRequestInfo = NULL;
    CallInfo callInfo;

    MEMSET(&callInfo, 0x00, SIZEOF(CallInfo));

    // Refresh the credentials
    currentTime = pIotCredentialProvider->getCurrentTimeFn(pIotCredentialProvider->customData);

    CHK(pIotCredentialProvider->pAwsCredentials == NULL ||
            currentTime + IOT_CREDENTIAL_FETCH_GRACE_PERIOD > pIotCredentialProvider->pAwsCredentials->expiration,
        retStatus);

    formatLen = SNPRINTF(serviceUrl, MAX_URI_CHAR_LEN, "%s%s%s%c%s%s", CONTROL_PLANE_URI_PREFIX, pIotCredentialProvider->iotGetCredentialEndpoint,
                         ROLE_ALIASES_PATH, '/', pIotCredentialProvider->roleAlias, CREDENTIAL_SERVICE);
    CHK(formatLen > 0 && formatLen < MAX_URI_CHAR_LEN, STATUS_IOT_FAILED);

    // Form a new request info based on the params
    CHK_STATUS(createRequestInfo(serviceUrl, NULL, DEFAULT_AWS_REGION, pIotCredentialProvider->caCertPath, pIotCredentialProvider->certPath,
                                 pIotCredentialProvider->privateKeyPath, SSL_CERTIFICATE_TYPE_PEM, DEFAULT_USER_AGENT_NAME,
                                 IOT_REQUEST_CONNECTION_TIMEOUT, IOT_REQUEST_COMPLETION_TIMEOUT, DEFAULT_LOW_SPEED_LIMIT,
                                 DEFAULT_LOW_SPEED_TIME_LIMIT, pIotCredentialProvider->pAwsCredentials, &pRequestInfo));

    callInfo.pRequestInfo = pRequestInfo;

    // Append the IoT header
    CHK_STATUS(setRequestHeader(pRequestInfo, IOT_THING_NAME_HEADER, 0, pIotCredentialProvider->thingName, 0));

    // Perform a blocking call
    CHK_STATUS(pIotCredentialProvider->serviceCallFn(pRequestInfo, &callInfo));

    // Parse the response and get the credentials
    CHK_STATUS(parseIotResponse(pIotCredentialProvider, &callInfo));

CleanUp:

    if (pRequestInfo != NULL) {
        freeRequestInfo(&pRequestInfo);
    }

    releaseCallInfo(&callInfo);

    return retStatus;
}
