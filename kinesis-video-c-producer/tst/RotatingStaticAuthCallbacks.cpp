/**
 * Kinesis Video Producer Static Auth Callbacks
 */
#define LOG_CLASS "RotatingStaticAuthCallbacks"
#include "ProducerTestFixture.h"

/**
 * Creates Static Rotating Auth callbacks
 */
STATUS createRotatingStaticAuthCallbacks(PClientCallbacks pCallbacksProvider, PCHAR accessKeyId, PCHAR secretKey,
                                         PCHAR sessionToken, UINT64 expiration, UINT64 rotationPeriod,
                                         PAuthCallbacks *ppStaticAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRotatingStaticAuthCallbacks pStaticAuthCallbacks = NULL;

    CHK(pCallbacksProvider != NULL && ppStaticAuthCallbacks != NULL, STATUS_NULL_ARG);
    CHK(rotationPeriod <= MAX_ENFORCED_TOKEN_EXPIRATION_DURATION, STATUS_INVALID_ARG);

    // Allocate the entire structure
    pStaticAuthCallbacks = (PRotatingStaticAuthCallbacks) MEMCALLOC(1, SIZEOF(RotatingStaticAuthCallbacks));
    CHK(pStaticAuthCallbacks != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the version, self
    pStaticAuthCallbacks->authCallbacks.version = AUTH_CALLBACKS_CURRENT_VERSION;
    pStaticAuthCallbacks->authCallbacks.customData = (UINT64) pStaticAuthCallbacks;

    // Store the back pointer as we will be using the other callbacks
    pStaticAuthCallbacks->pCallbacksProvider = (PCallbacksProvider) pCallbacksProvider;

    // Set the callbacks
    pStaticAuthCallbacks->authCallbacks.getStreamingTokenFn = getStreamingTokenEnvVarFunc;
    pStaticAuthCallbacks->authCallbacks.getSecurityTokenFn = getSecurityTokenEnvVarFunc;
    pStaticAuthCallbacks->authCallbacks.freeAuthCallbacksFn = freeRotatingStaticAuthCallbacksFunc;
    pStaticAuthCallbacks->authCallbacks.getDeviceCertificateFn = NULL;
    pStaticAuthCallbacks->authCallbacks.deviceCertToTokenFn = NULL;
    pStaticAuthCallbacks->authCallbacks.getDeviceFingerprintFn = NULL;
    pStaticAuthCallbacks->rotationPeriod = rotationPeriod;

    // Initialize the fault injection members to defaults
    pStaticAuthCallbacks->retStatus = STATUS_SUCCESS;
    pStaticAuthCallbacks->invokeCount = 0;
    pStaticAuthCallbacks->failCount = 0;
    pStaticAuthCallbacks->recoverCount = 0;

    // Create the credentials object
    CHK_STATUS(createAwsCredentials(accessKeyId, 0, secretKey, 0, sessionToken, 0, expiration,
                                    &pStaticAuthCallbacks->pAwsCredentials));

    CHK_STATUS(addAuthCallbacks(pCallbacksProvider, (PAuthCallbacks) pStaticAuthCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeRotatingStaticAuthCallbacks((PAuthCallbacks *) &pStaticAuthCallbacks);
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
 * Frees the Rotating Static Auth callbacks object
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 */
STATUS freeRotatingStaticAuthCallbacks(PAuthCallbacks *ppStaticAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRotatingStaticAuthCallbacks pStaticAuthCallbacks = NULL;

    CHK(ppStaticAuthCallbacks != NULL, STATUS_NULL_ARG);

    pStaticAuthCallbacks = (PRotatingStaticAuthCallbacks) *ppStaticAuthCallbacks;

    // Call is idempotent
    CHK(pStaticAuthCallbacks != NULL, retStatus);

    // Release the underlying AWS credentials object
    freeAwsCredentials(&pStaticAuthCallbacks->pAwsCredentials);

    // Release the object
    MEMFREE(pStaticAuthCallbacks);

    // Set the pointer to NULL
    *ppStaticAuthCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS freeRotatingStaticAuthCallbacksFunc(PUINT64 customData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    PRotatingStaticAuthCallbacks pAuthCallbacks;
    CHK(customData != NULL, STATUS_NULL_ARG);

    pAuthCallbacks = (PRotatingStaticAuthCallbacks) *customData;
    CHK_STATUS(freeRotatingStaticAuthCallbacks((PAuthCallbacks*) &pAuthCallbacks));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getStreamingTokenEnvVarFunc(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode,
                                   PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(accessMode);
    ENTERS();
    UINT64 currentTime;
    STATUS retStatus = STATUS_SUCCESS;
    PRotatingStaticAuthCallbacks pStaticAuthCallbacks;
    PCallbacksProvider pCallbacksProvider = NULL;
    UINT64 expirationTime;

    pStaticAuthCallbacks = (PRotatingStaticAuthCallbacks) customData;
    CHK(pStaticAuthCallbacks != NULL && pServiceCallContext != NULL, STATUS_NULL_ARG);
    pCallbacksProvider = pStaticAuthCallbacks->pCallbacksProvider;
    CHK(pCallbacksProvider != NULL, STATUS_NULL_ARG);

    // Fault injection
    if (pStaticAuthCallbacks->invokeCount >= pStaticAuthCallbacks->failCount &&
        pStaticAuthCallbacks->invokeCount < pStaticAuthCallbacks->recoverCount) {
        CHK(FALSE, pStaticAuthCallbacks->retStatus);
    }

    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);

    // expiration is now time plus rotation period
    expirationTime = currentTime + pStaticAuthCallbacks->rotationPeriod;

    retStatus = getStreamingTokenResultEvent(pServiceCallContext->customData,
                                             SERVICE_CALL_RESULT_OK,
                                             (PBYTE) pStaticAuthCallbacks->pAwsCredentials,
                                             pStaticAuthCallbacks->pAwsCredentials->size,
                                             expirationTime);

CleanUp:

    if (pStaticAuthCallbacks != NULL) {
        pStaticAuthCallbacks->invokeCount++;

        if (STATUS_FAILED(retStatus) && pServiceCallContext != NULL) {
            // Notify PIC on failure
            getStreamingTokenResultEvent(pServiceCallContext->customData,
                                         SERVICE_CALL_UNKNOWN,
                                         NULL,
                                         0,
                                         0);

            // Notify clients
            notifyCallResult(pCallbacksProvider, retStatus, pServiceCallContext->customData);
        }
    }

    LEAVES();
    return retStatus;

}

STATUS getSecurityTokenEnvVarFunc(UINT64 customData, PBYTE *ppBuffer, PUINT32 pSize, PUINT64 pExpiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime;
    PCallbacksProvider pCallbacksProvider = NULL;

    PRotatingStaticAuthCallbacks pStaticAuthCallbacks = (PRotatingStaticAuthCallbacks) customData;
    CHK(pStaticAuthCallbacks != NULL && ppBuffer != NULL && pSize != NULL && pExpiration != NULL, STATUS_NULL_ARG);
    pCallbacksProvider = pStaticAuthCallbacks->pCallbacksProvider;

    if (pStaticAuthCallbacks->invokeCount >= pStaticAuthCallbacks->failCount &&
        pStaticAuthCallbacks->invokeCount < pStaticAuthCallbacks->recoverCount) {
        CHK(FALSE, pStaticAuthCallbacks->retStatus);
    }

    currentTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);

    // expiration is now time plus rotation period
    *pExpiration = currentTime + pStaticAuthCallbacks->rotationPeriod;
    *pSize = pStaticAuthCallbacks->pAwsCredentials->size;
    *ppBuffer = (PBYTE) pStaticAuthCallbacks->pAwsCredentials;

CleanUp:

    if (pStaticAuthCallbacks != NULL) {
        pStaticAuthCallbacks->invokeCount++;
    }

    LEAVES();
    return retStatus;
}
