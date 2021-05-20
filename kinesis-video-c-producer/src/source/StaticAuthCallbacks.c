/**
 * Kinesis Video Producer Static Auth Callbacks
 */
#define LOG_CLASS "StaticAuthCallbacks"
#include "Include_i.h"

/**
 * Creates Static Auth callbacks
 */
STATUS createStaticAuthCallbacks(PClientCallbacks pCallbacksProvider, PCHAR accessKeyId, PCHAR secretKey, PCHAR sessionToken, UINT64 expiration,
                                 PAuthCallbacks* ppStaticAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStaticAuthCallbacks pStaticAuthCallbacks = NULL;
    PAwsCredentialProvider pCredentialProvider = NULL;

    CHK(pCallbacksProvider != NULL && ppStaticAuthCallbacks != NULL, STATUS_NULL_ARG);

    // Allocate the entire structure
    pStaticAuthCallbacks = (PStaticAuthCallbacks) MEMCALLOC(1, SIZEOF(StaticAuthCallbacks));
    CHK(pStaticAuthCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pStaticAuthCallbacks->authCallbacks.version = AUTH_CALLBACKS_CURRENT_VERSION;
    pStaticAuthCallbacks->authCallbacks.customData = (UINT64) pStaticAuthCallbacks;

    // Store the back pointer as we will be using the other callbacks
    pStaticAuthCallbacks->pCallbacksProvider = (PCallbacksProvider) pCallbacksProvider;

    // Set the callbacks
    pStaticAuthCallbacks->authCallbacks.getStreamingTokenFn = getStreamingTokenStaticFunc;
    pStaticAuthCallbacks->authCallbacks.getSecurityTokenFn = getSecurityTokenStaticFunc;
    pStaticAuthCallbacks->authCallbacks.freeAuthCallbacksFn = freeStaticAuthCallbacksFunc;
    pStaticAuthCallbacks->authCallbacks.getDeviceCertificateFn = NULL;
    pStaticAuthCallbacks->authCallbacks.deviceCertToTokenFn = NULL;
    pStaticAuthCallbacks->authCallbacks.getDeviceFingerprintFn = NULL;

    // Initialize the provider
    CHK_STATUS(createStaticCredentialProvider(accessKeyId, 0, secretKey, 0, sessionToken, 0, expiration, &pCredentialProvider));
    pStaticAuthCallbacks->pCredentialProvider = (PAwsCredentialProvider) pCredentialProvider;

    // Append to the auth chain
    CHK_STATUS(addAuthCallbacks(pCallbacksProvider, (PAuthCallbacks) pStaticAuthCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeStaticAuthCallbacks((PAuthCallbacks*) &pStaticAuthCallbacks);
        pStaticAuthCallbacks = NULL;
    }

    // Set the return value if it's not NULL
    if (ppStaticAuthCallbacks != NULL) {
        *ppStaticAuthCallbacks = (PAuthCallbacks) pStaticAuthCallbacks;
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the Static Auth callbacks object
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 */
STATUS freeStaticAuthCallbacks(PAuthCallbacks* ppStaticAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStaticAuthCallbacks pStaticAuthCallbacks = NULL;

    CHK(ppStaticAuthCallbacks != NULL, STATUS_NULL_ARG);

    pStaticAuthCallbacks = (PStaticAuthCallbacks) *ppStaticAuthCallbacks;

    // Call is idempotent
    CHK(pStaticAuthCallbacks != NULL, retStatus);

    // Release the underlying credential provider
    freeStaticCredentialProvider((PAwsCredentialProvider*) &pStaticAuthCallbacks->pCredentialProvider);

    // Release the object
    MEMFREE(pStaticAuthCallbacks);

    // Set the pointer to NULL
    *ppStaticAuthCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeStaticAuthCallbacksFunc(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStaticAuthCallbacks pAuthCallbacks;

    CHK(customData != NULL, STATUS_NULL_ARG);
    pAuthCallbacks = (PStaticAuthCallbacks) *customData;
    CHK_STATUS(freeStaticAuthCallbacks((PAuthCallbacks*) &pAuthCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getStreamingTokenStaticFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode, PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);
    ENTERS();

    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials;
    PAwsCredentialProvider pCredentialProvider;

    PStaticAuthCallbacks pStaticAuthCallbacks = (PStaticAuthCallbacks) customData;

    CHK(pStaticAuthCallbacks != NULL, STATUS_NULL_ARG);

    pCredentialProvider = (PAwsCredentialProvider) pStaticAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    retStatus = getStreamingTokenResultEvent(pServiceCallContext->customData, SERVICE_CALL_RESULT_OK, (PBYTE) pAwsCredentials, pAwsCredentials->size,
                                             pAwsCredentials->expiration);

    PCallbacksProvider pCallbacksProvider = pStaticAuthCallbacks->pCallbacksProvider;

    notifyCallResult(pCallbacksProvider, retStatus, customData);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getSecurityTokenStaticFunc(UINT64 customData, PBYTE* ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStaticAuthCallbacks pStaticAuthCallbacks = (PStaticAuthCallbacks) customData;
    PAwsCredentials pAwsCredentials;
    PAwsCredentialProvider pCredentialProvider;

    CHK(pStaticAuthCallbacks != NULL && ppBuffer != NULL && pSize != NULL && pExpiration != NULL, STATUS_NULL_ARG);

    pCredentialProvider = (PAwsCredentialProvider) pStaticAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    *pExpiration = pAwsCredentials->expiration;
    *pSize = pAwsCredentials->size;
    *ppBuffer = (PBYTE) pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}
