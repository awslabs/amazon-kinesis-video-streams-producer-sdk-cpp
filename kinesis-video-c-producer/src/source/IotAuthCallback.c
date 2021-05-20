/**
 * Kinesis Video Producer IotCredential Auth Callback
 */
#define LOG_CLASS "IotAuthCallbacks"
#include "Include_i.h"

/*
 * Create IoT credentials callback
 */
STATUS createIotAuthCallbacks(PClientCallbacks pCallbacksProvider, PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath,
                              PCHAR caCertPath, PCHAR roleAlias, PCHAR streamName, PAuthCallbacks* ppIotAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    PIotAuthCallbacks pIotAuthCallbacks = NULL;

    CHK(pCallbacksProvider != NULL && ppIotAuthCallbacks != NULL, STATUS_NULL_ARG);

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

    CHK_STATUS(createCurlIotCredentialProviderWithTime(iotGetCredentialEndpoint, certPath, privateKeyPath, caCertPath, roleAlias, streamName,
                                                       pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.getCurrentTimeFn,
                                                       pIotAuthCallbacks->pCallbacksProvider->clientCallbacks.customData,
                                                       (PAwsCredentialProvider*) &pIotAuthCallbacks->pCredentialProvider));

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

    PIotAuthCallbacks pIotAuthCallbacks = NULL;

    CHK(ppIotAuthCallbacks != NULL, STATUS_NULL_ARG);

    pIotAuthCallbacks = (PIotAuthCallbacks) *ppIotAuthCallbacks;

    // Call is idempotent
    CHK(pIotAuthCallbacks != NULL, retStatus);

    // Release the underlying AWS credentials provider object
    freeIotCredentialProvider((PAwsCredentialProvider*) &pIotAuthCallbacks->pCredentialProvider);

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

STATUS getStreamingTokenIotFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode, PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);

    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials = NULL;
    PAwsCredentialProvider pCredentialProvider;
    PCallbacksProvider pCallbacksProvider = NULL;
    PIotAuthCallbacks pIotAuthCallbacks = (PIotAuthCallbacks) customData;

    CHK(pIotAuthCallbacks != NULL && pServiceCallContext != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pIotAuthCallbacks->pCallbacksProvider;
    pCredentialProvider = (PAwsCredentialProvider) pIotAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    CHK_STATUS(getStreamingTokenResultEvent(pServiceCallContext->customData, SERVICE_CALL_RESULT_OK, (PBYTE) pAwsCredentials, pAwsCredentials->size,
                                            pAwsCredentials->expiration));

CleanUp:

    if (STATUS_FAILED(retStatus) && pServiceCallContext != NULL) {
        // Notify PIC on failure
        getStreamingTokenResultEvent(pServiceCallContext->customData, SERVICE_CALL_UNKNOWN, NULL, 0, 0);

        // Notify clients
        notifyCallResult(pCallbacksProvider, retStatus, pServiceCallContext->customData);
    }

    LEAVES();
    return retStatus;
}

STATUS getSecurityTokenIotFunc(UINT64 customData, PBYTE* ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials;
    PAwsCredentialProvider pCredentialProvider;

    PIotAuthCallbacks pIotAuthCallbacks = (PIotAuthCallbacks) customData;
    CHK(pIotAuthCallbacks != NULL && ppBuffer != NULL && pSize != NULL && pExpiration != NULL, STATUS_NULL_ARG);

    pCredentialProvider = (PAwsCredentialProvider) pIotAuthCallbacks->pCredentialProvider;
    CHK_STATUS(pCredentialProvider->getCredentialsFn(pCredentialProvider, &pAwsCredentials));

    *pExpiration = pAwsCredentials->expiration;
    *pSize = pAwsCredentials->size;
    *ppBuffer = (PBYTE) pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}
