/**
 * Kinesis Video Producer file Auth Callbacks
 */
#define LOG_CLASS "FileAuthCallbacks"
#include "Include_i.h"

/**
 * Creates File Auth callbacks
 */
STATUS createFileAuthCallbacks(PClientCallbacks pCallbacksProvider,
                               PCHAR pCredentialsFilepath,
                               PAuthCallbacks *ppFileAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PFileAuthCallbacks pFileAuthCallbacks = NULL;
    UINT64 currentTime;
    CHK(pCallbacksProvider != NULL && ppFileAuthCallbacks != NULL, STATUS_NULL_ARG);
    CHK(pCredentialsFilepath != NULL && pCredentialsFilepath[0] != '\0', STATUS_INVALID_ARG);
    CHK((UINT32) STRLEN(pCredentialsFilepath) <= MAX_PATH_LEN, STATUS_INVALID_ARG);
    // Allocate the entire structure
    pFileAuthCallbacks = (PFileAuthCallbacks) MEMCALLOC(1, SIZEOF(FileAuthCallbacks));
    CHK(pFileAuthCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pFileAuthCallbacks->authCallbacks.version = AUTH_CALLBACKS_CURRENT_VERSION;
    pFileAuthCallbacks->authCallbacks.customData = (UINT64) pFileAuthCallbacks;

    // Store the back pointer as we will be using the other callbacks
    pFileAuthCallbacks->pCallbacksProvider = (PCallbacksProvider) pCallbacksProvider;

    // Set the callbacks
    pFileAuthCallbacks->authCallbacks.getStreamingTokenFn = getStreamingTokenFileFunc;
    pFileAuthCallbacks->authCallbacks.getSecurityTokenFn = getSecurityTokenFileFunc;
    pFileAuthCallbacks->authCallbacks.freeAuthCallbacksFn = freeFileAuthCallbacksFunc;
    pFileAuthCallbacks->authCallbacks.getDeviceCertificateFn = NULL;
    pFileAuthCallbacks->authCallbacks.deviceCertToTokenFn = NULL;
    pFileAuthCallbacks->authCallbacks.getDeviceFingerprintFn = NULL;

    // Store the file path in case we need to access it again
    STRCPY((PCHAR) pFileAuthCallbacks->credentialsFilepath, pCredentialsFilepath);

    // Create the credentials object
    currentTime = pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    CHK_STATUS(readFileCredentials(pFileAuthCallbacks, currentTime));

    // Append to the auth chain
    CHK_STATUS(addAuthCallbacks(pCallbacksProvider, (PAuthCallbacks) pFileAuthCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeFileAuthCallbacks((PAuthCallbacks *) &pFileAuthCallbacks);
        pFileAuthCallbacks = NULL;
    }

    // Set the return value if it's not NULL
    if (ppFileAuthCallbacks != NULL) {
        *ppFileAuthCallbacks = (PAuthCallbacks) pFileAuthCallbacks;
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the File Auth callbacks object
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 */
STATUS freeFileAuthCallbacks(PAuthCallbacks *ppAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PFileAuthCallbacks pFileAuthCallbacks = NULL;

    CHK(ppAuthCallbacks != NULL, STATUS_NULL_ARG);

    pFileAuthCallbacks = (PFileAuthCallbacks) *ppAuthCallbacks;

    // Call is idempotent
    CHK(pFileAuthCallbacks != NULL, retStatus);

    // Release the underlying AWS credentials object
    freeAwsCredentials(&pFileAuthCallbacks->pAwsCredentials);

    // Release the object
    MEMFREE(pFileAuthCallbacks);

    // Set the pointer to NULL
    *ppAuthCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeFileAuthCallbacksFunc(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PFileAuthCallbacks pAuthCallbacks;

    CHK(customData != NULL, STATUS_NULL_ARG);
    pAuthCallbacks = (PFileAuthCallbacks) *customData;
    CHK_STATUS(freeFileAuthCallbacks((PAuthCallbacks*) &pAuthCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getStreamingTokenFileFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode,
                                 PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);
    ENTERS();

    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;

    PFileAuthCallbacks pFileAuthCallbacks = (PFileAuthCallbacks) customData;

    CHK(pFileAuthCallbacks != NULL, STATUS_NULL_ARG);

    // Refresh the credentials by reading from the credentials file if needed
    currentTime = pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    if (currentTime + CREDENTIAL_FILE_READ_GRACE_PERIOD > pFileAuthCallbacks->pAwsCredentials->expiration) {
        CHK_STATUS(readFileCredentials(pFileAuthCallbacks,
                                       currentTime));
    }

    retStatus = getStreamingTokenResultEvent(
            pServiceCallContext->customData, SERVICE_CALL_RESULT_OK,
            (PBYTE) pFileAuthCallbacks->pAwsCredentials,
            pFileAuthCallbacks->pAwsCredentials->size,
            pFileAuthCallbacks->pAwsCredentials->expiration);

    PCallbacksProvider pCallbacksProvider = pFileAuthCallbacks->pCallbacksProvider;

    notifyCallResult(pCallbacksProvider, retStatus, customData);

CleanUp:

    LEAVES();
    return retStatus;

}

STATUS getSecurityTokenFileFunc(UINT64 customData, PBYTE *ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;

    PFileAuthCallbacks pFileAuthCallbacks = (PFileAuthCallbacks) customData;
    CHK(pFileAuthCallbacks != NULL &&
        ppBuffer != NULL &&
        pSize != NULL &&
        pExpiration != NULL, STATUS_NULL_ARG);

    // Refresh the credentials by reading from the credentials file if needed
    currentTime = pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn(
            pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.customData);
    if (currentTime + CREDENTIAL_FILE_READ_GRACE_PERIOD > pFileAuthCallbacks->pAwsCredentials->expiration) {
        CHK_STATUS(readFileCredentials(pFileAuthCallbacks,
                                       currentTime));
    }

    *pExpiration = pFileAuthCallbacks->pAwsCredentials->expiration;
    *pSize = pFileAuthCallbacks->pAwsCredentials->size;
    *ppBuffer = (PBYTE) pFileAuthCallbacks->pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Read the credential file and sets the values of the AWS credentials object
 *
 * @param - PFileAuthCallbacks - the PFileAuthCallbacks object
 * @param - UINT64 - current local time in 100ns
 *
 * @return - STATUS code of the execution
 */

STATUS readFileCredentials(PFileAuthCallbacks pFileAuthCallbacks, UINT64 currentTime)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 fileLen;
    FILE *fp = NULL;
    CHAR credentialMarker[12];
    CHAR thirdTokenStr[MAX(MAX_EXPIRATION_LEN, MAX_SECRET_KEY_LEN) + 1], fourthTokenStr[MAX_SECRET_KEY_LEN + 1], accessKeyId[MAX_ACCESS_KEY_LEN + 1];
    CHAR sessionToken[MAX_SESSION_TOKEN_LEN + 1];
    PCHAR expirationStr = NULL, secretKey = NULL;
    UINT32 accessKeyIdLen = 0, secretKeyLen = 0, sessionTokenLen = 0;
    UINT64 expiration;

    CHK(pFileAuthCallbacks != NULL && pFileAuthCallbacks->credentialsFilepath != NULL, STATUS_NULL_ARG);

    fp = FOPEN((PCHAR) pFileAuthCallbacks->credentialsFilepath, "r");

    CHK(fp != NULL, STATUS_OPEN_FILE_FAILED);

    // Get the size of the file
    FSEEK(fp, 0, SEEK_END);

    fileLen = (UINT64) FTELL(fp);

    DLOGV("Reading AWS credentials from file: %s, file length = %" PRIu64 ".", pFileAuthCallbacks->credentialsFilepath, fileLen);

    CHK(fileLen < MAX_CREDENTIAL_FILE_LEN, STATUS_INVALID_ARG);

    FSEEK(fp, 0, SEEK_SET);

    // empty buffers
    thirdTokenStr[0] = '\0';
    fourthTokenStr[0] = '\0';
    accessKeyId[0] = '\0';
    sessionToken[0] = '\0';

    /*
     * Currently the credential file can have two formats, one is "CREDENTIALS accessKey expiration secretKey sessionToken", and
     * the other is just "CREDENTIALS accessKey secretKey". So the second token can be either expiration or secret key.
     */

    FSCANF(fp, "%11s %" STR(MAX_ACCESS_KEY_LEN) "s %" STR(MAX_EXPIRATION_LEN) "s %" STR(MAX_SECRET_KEY_LEN) "s %" STR(MAX_SESSION_TOKEN_LEN) "s",
           credentialMarker,
           accessKeyId,
           thirdTokenStr,
           fourthTokenStr,
           sessionToken);

    // if the fourth token is empty, it means the credential file only has accessKey and secretKey
    if (fourthTokenStr[0] == '\0') {
        secretKey = thirdTokenStr;
    } else {
        expirationStr = thirdTokenStr;
        secretKey = fourthTokenStr;
    }

    CHK(STRCMP(credentialMarker, "CREDENTIALS") == 0 , STATUS_INVALID_ARG);

    // Set the lengths
    accessKeyIdLen = (UINT32) STRNLEN(accessKeyId, MAX_ACCESS_KEY_LEN);
    secretKeyLen = (UINT32) STRNLEN(secretKey, MAX_SECRET_KEY_LEN);
    sessionTokenLen = (UINT32) STRNLEN(sessionToken, MAX_SESSION_TOKEN_LEN);

    if (expirationStr != NULL) {
        convertTimestampToEpoch(expirationStr, currentTime / HUNDREDS_OF_NANOS_IN_A_SECOND, &expiration);
    } else {
        expiration = currentTime + MAX_ENFORCED_TOKEN_EXPIRATION_DURATION;
    }

    // Fix-up the expiration to be no more than max enforced token rotation to avoid extra token rotations
    // as we are caching the returned value which is likely to be an hour but we are enforcing max
    // rotation to be more frequent.
    expiration = MIN(expiration, currentTime + MAX_ENFORCED_TOKEN_EXPIRATION_DURATION);

    CHK(accessKeyIdLen != 0 &&
        secretKeyLen != 0, STATUS_INVALID_AUTH_LEN);

    if (pFileAuthCallbacks->pAwsCredentials != NULL) {
        freeAwsCredentials(&pFileAuthCallbacks->pAwsCredentials);
        pFileAuthCallbacks->pAwsCredentials = NULL;
    }

    CHK_STATUS(createAwsCredentials(accessKeyId,
                                    accessKeyIdLen,
                                    secretKey,
                                    secretKeyLen,
                                    sessionToken,
                                    sessionTokenLen,
                                    expiration,
                                    &pFileAuthCallbacks->pAwsCredentials));

CleanUp:

    if (fp != NULL) {
        FCLOSE(fp);
    }

    return retStatus;
}
