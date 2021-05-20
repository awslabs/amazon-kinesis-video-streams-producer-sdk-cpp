/**
 * Kinesis Video Producer file Auth Callbacks
 */
#define LOG_CLASS "FileAuthCallbacks"
#include "Include_i.h"

/**
 * Creates File Auth callbacks
 */
STATUS createFileAuthCallbacks(PClientCallbacks pCallbacksProvider, PCHAR pCredentialsFilepath, PAuthCallbacks* ppFileAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PFileAuthCallbacks pFileAuthCallbacks = NULL;
    CHK(pCallbacksProvider != NULL && ppFileAuthCallbacks != NULL, STATUS_NULL_ARG);

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

    CHK_STATUS(createFileCredentialProviderWithTime(pCredentialsFilepath, pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn,
                                                    pFileAuthCallbacks->pCallbacksProvider->clientCallbacks.customData,
                                                    (PAwsCredentialProvider*) &pFileAuthCallbacks->pCredentialProvider));

    // Append to the auth chain
    CHK_STATUS(addAuthCallbacks(pCallbacksProvider, (PAuthCallbacks) pFileAuthCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeFileAuthCallbacks((PAuthCallbacks*) &pFileAuthCallbacks);
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
STATUS freeFileAuthCallbacks(PAuthCallbacks* ppAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PFileAuthCallbacks pFileAuthCallbacks = NULL;

    CHK(ppAuthCallbacks != NULL, STATUS_NULL_ARG);

    pFileAuthCallbacks = (PFileAuthCallbacks) *ppAuthCallbacks;

    // Call is idempotent
    CHK(pFileAuthCallbacks != NULL, retStatus);

    // Release the underlying AWS credentials provider object
    freeFileCredentialProvider((PAwsCredentialProvider*) &pFileAuthCallbacks->pCredentialProvider);

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

STATUS getStreamingTokenFileFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode, PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);
    ENTERS();

    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials;
    PAwsCredentialProvider pCredentialProvider;
    PCallbacksProvider pCallbacksProvider = NULL;
    PFileAuthCallbacks pFileAuthCallbacks = (PFileAuthCallbacks) customData;

    CHK(pFileAuthCallbacks != NULL && pServiceCallContext != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pFileAuthCallbacks->pCallbacksProvider;
    pCredentialProvider = (PAwsCredentialProvider) pFileAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    CHK_STATUS(getStreamingTokenResultEvent(pServiceCallContext->customData, SERVICE_CALL_RESULT_OK, (PBYTE) pAwsCredentials, pAwsCredentials->size,
                                            pAwsCredentials->expiration));

CleanUp:

    if (STATUS_FAILED(retStatus) && pServiceCallContext != NULL) {
        // Notify PIC on error
        getStreamingTokenResultEvent(pServiceCallContext->customData, SERVICE_CALL_UNKNOWN, NULL, 0, 0);

        // Notify clients
        notifyCallResult(pCallbacksProvider, retStatus, pServiceCallContext->customData);
    }

    LEAVES();
    return retStatus;
}

STATUS getSecurityTokenFileFunc(UINT64 customData, PBYTE* ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials;
    PAwsCredentialProvider pCredentialProvider;

    PFileAuthCallbacks pFileAuthCallbacks = (PFileAuthCallbacks) customData;
    CHK(pFileAuthCallbacks != NULL && ppBuffer != NULL && pSize != NULL && pExpiration != NULL, STATUS_NULL_ARG);

    pCredentialProvider = (PAwsCredentialProvider) pFileAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    *pExpiration = pAwsCredentials->expiration;
    *pSize = pAwsCredentials->size;
    *ppBuffer = (PBYTE) pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}
