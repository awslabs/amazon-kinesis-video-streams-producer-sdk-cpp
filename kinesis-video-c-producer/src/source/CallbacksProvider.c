/**
 * Kinesis Video Producer Callbacks Provider
 */
#define LOG_CLASS "CallbacksProvider"
#include "Include_i.h"

//////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////

/**
 * Create the default callbacks provider
 */
STATUS createDefaultCallbacksProvider(UINT32 callbackChainCount, PCHAR accessKeyId, PCHAR secretKey, PCHAR sessionToken, UINT64 expiration,
                                      PCHAR region, PCHAR controlPlaneUrl, PCHAR certPath, PCHAR userAgentPostfix, PCHAR customUserAgent,
                                      API_CALL_CACHE_TYPE cacheType, UINT64 endpointCachingPeriod, BOOL continuousRetry,
                                      PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PAuthCallbacks pAuthCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;

    CHK_STATUS(createAbstractDefaultCallbacksProvider(callbackChainCount, cacheType, endpointCachingPeriod, region, controlPlaneUrl, certPath,
                                                      userAgentPostfix, customUserAgent, ppClientCallbacks));

    pCallbacksProvider = (PCallbacksProvider) *ppClientCallbacks;

    CHK_STATUS(createStaticAuthCallbacks((PClientCallbacks) pCallbacksProvider, accessKeyId, secretKey, sessionToken, expiration, &pAuthCallbacks));

    if (continuousRetry) {
        CHK_STATUS(createContinuousRetryStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pStreamCallbacks));
    }

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pCallbacksProvider != NULL) {
            freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);
        }

        if (pAuthCallbacks != NULL) {
            freeStaticAuthCallbacks(&pAuthCallbacks);
        }

        if (pStreamCallbacks != NULL) {
            freeContinuousRetryStreamCallbacks(&pStreamCallbacks);
        }

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

/**
 * Create the default callbacks provider with AWS credentials and defaults
 */
STATUS createDefaultCallbacksProviderWithAwsCredentials(PCHAR accessKeyId, PCHAR secretKey, PCHAR sessionToken, UINT64 expiration, PCHAR region,
                                                        PCHAR caCertPath, PCHAR userAgentPostfix, PCHAR customUserAgent,
                                                        PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PAuthCallbacks pAuthCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;

    CHK_STATUS(createAbstractDefaultCallbacksProvider(DEFAULT_CALLBACK_CHAIN_COUNT, API_CALL_CACHE_TYPE_ALL, ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE,
                                                      region, EMPTY_STRING, caCertPath, userAgentPostfix, customUserAgent, ppClientCallbacks));

    pCallbacksProvider = (PCallbacksProvider) *ppClientCallbacks;

    CHK_STATUS(createStaticAuthCallbacks((PClientCallbacks) pCallbacksProvider, accessKeyId, secretKey, sessionToken, expiration, &pAuthCallbacks));

    CHK_STATUS(createContinuousRetryStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pStreamCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pCallbacksProvider != NULL) {
            freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);
        }

        if (pAuthCallbacks != NULL) {
            freeStaticAuthCallbacks(&pAuthCallbacks);
        }

        if (pStreamCallbacks != NULL) {
            freeContinuousRetryStreamCallbacks(&pStreamCallbacks);
        }

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS createDefaultCallbacksProviderWithIotCertificate(PCHAR endpoint, PCHAR iotCertPath, PCHAR privateKeyPath, PCHAR caCertPath, PCHAR roleAlias,
                                                        PCHAR streamName, PCHAR region, PCHAR userAgentPostfix, PCHAR customUserAgent,
                                                        PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PAuthCallbacks pAuthCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;

    CHK_STATUS(createAbstractDefaultCallbacksProvider(DEFAULT_CALLBACK_CHAIN_COUNT, API_CALL_CACHE_TYPE_ALL, ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE,
                                                      region, EMPTY_STRING, caCertPath, userAgentPostfix, customUserAgent, ppClientCallbacks));

    pCallbacksProvider = (PCallbacksProvider) *ppClientCallbacks;

    CHK_STATUS(createIotAuthCallbacks((PClientCallbacks) pCallbacksProvider, endpoint, iotCertPath, privateKeyPath, caCertPath, roleAlias, streamName,
                                      &pAuthCallbacks));

    CHK_STATUS(createContinuousRetryStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pStreamCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pCallbacksProvider != NULL) {
            freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);
        }

        if (pAuthCallbacks != NULL) {
            freeIotAuthCallbacks(&pAuthCallbacks);
        }

        if (pStreamCallbacks != NULL) {
            freeContinuousRetryStreamCallbacks(&pStreamCallbacks);
        }

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS createDefaultCallbacksProviderWithFileAuth(PCHAR credentialsFilePath, PCHAR region, PCHAR caCertPath, PCHAR userAgentPostfix,
                                                  PCHAR customUserAgent, PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PAuthCallbacks pAuthCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;

    CHK_STATUS(createAbstractDefaultCallbacksProvider(DEFAULT_CALLBACK_CHAIN_COUNT, API_CALL_CACHE_TYPE_ALL, ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE,
                                                      region, EMPTY_STRING, caCertPath, userAgentPostfix, customUserAgent, ppClientCallbacks));

    pCallbacksProvider = (PCallbacksProvider) *ppClientCallbacks;

    CHK_STATUS(createFileAuthCallbacks((PClientCallbacks) pCallbacksProvider, credentialsFilePath, &pAuthCallbacks));

    CHK_STATUS(createContinuousRetryStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pStreamCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pCallbacksProvider != NULL) {
            freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);
        }

        if (pAuthCallbacks != NULL) {
            freeIotAuthCallbacks(&pAuthCallbacks);
        }

        if (pStreamCallbacks != NULL) {
            freeContinuousRetryStreamCallbacks(&pStreamCallbacks);
        }

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS createDefaultCallbacksProviderWithAuthCallbacks(PAuthCallbacks pAuthCallbacks, PCHAR region, PCHAR caCertPath, PCHAR userAgentPostfix,
                                                       PCHAR customUserAgent, PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;

    CHK_STATUS(createAbstractDefaultCallbacksProvider(DEFAULT_CALLBACK_CHAIN_COUNT, API_CALL_CACHE_TYPE_ALL, ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE,
                                                      region, EMPTY_STRING, caCertPath, userAgentPostfix, customUserAgent, ppClientCallbacks));
    pCallbacksProvider = (PCallbacksProvider) *ppClientCallbacks;
    CHK_STATUS(addAuthCallbacks(*ppClientCallbacks, pAuthCallbacks));
    CHK_STATUS(createContinuousRetryStreamCallbacks((PClientCallbacks) pCallbacksProvider, &pStreamCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pCallbacksProvider != NULL) {
            freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);
        }

        if (pStreamCallbacks != NULL) {
            freeContinuousRetryStreamCallbacks(&pStreamCallbacks);
        }

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS createAbstractDefaultCallbacksProvider(UINT32 callbackChainCount, API_CALL_CACHE_TYPE cacheType, UINT64 endpointCachingPeriod, PCHAR region,
                                              PCHAR controlPlaneUrl, PCHAR certPath, PCHAR userAgentName, PCHAR customUserAgent,
                                              PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = NULL;
    PCurlApiCallbacks pCurlApiCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;
    UINT32 size;

    CHK(ppClientCallbacks != NULL, STATUS_NULL_ARG);
    CHK(callbackChainCount < MAX_CALLBACK_CHAIN_COUNT, STATUS_INVALID_ARG);

    // Calculate the size
    size = SIZEOF(CallbacksProvider) +
        callbackChainCount * (SIZEOF(ProducerCallbacks) + SIZEOF(StreamCallbacks) + SIZEOF(AuthCallbacks) + SIZEOF(ApiCallbacks));

    // Allocate the entire structure
    pCallbacksProvider = (PCallbacksProvider) MEMCALLOC(1, size);
    CHK(pCallbacksProvider != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the correct values.
    // NOTE: Fields are zero-ed by MEMCALLOC
    pCallbacksProvider->clientCallbacks.version = CALLBACKS_CURRENT_VERSION;
    pCallbacksProvider->callbackChainCount = callbackChainCount;

    // Set the custom data field to self
    pCallbacksProvider->clientCallbacks.customData = (UINT64) pCallbacksProvider;

    // Set callback chain pointers
    pCallbacksProvider->pProducerCallbacks = (PProducerCallbacks)(pCallbacksProvider + 1);
    pCallbacksProvider->pStreamCallbacks = (PStreamCallbacks)(pCallbacksProvider->pProducerCallbacks + callbackChainCount);
    pCallbacksProvider->pAuthCallbacks = (PAuthCallbacks)(pCallbacksProvider->pStreamCallbacks + callbackChainCount);
    pCallbacksProvider->pApiCallbacks = (PApiCallbacks)(pCallbacksProvider->pAuthCallbacks + callbackChainCount);

    // Set the default Platform callbacks
    CHK_STATUS(setDefaultPlatformCallbacks(pCallbacksProvider));

    // Create the default Curl API callbacks
    CHK_STATUS(createCurlApiCallbacks(pCallbacksProvider, region, cacheType, endpointCachingPeriod, controlPlaneUrl, certPath, userAgentName,
                                      customUserAgent, &pCurlApiCallbacks));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeCurlApiCallbacks(&pCurlApiCallbacks);
        freeStreamCallbacks(&pStreamCallbacks);
        freeCallbacksProvider((PClientCallbacks*) &pCallbacksProvider);

        pCallbacksProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppClientCallbacks != NULL) {
        *ppClientCallbacks = (PClientCallbacks) pCallbacksProvider;
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the callbacks provider
 *
 * NOTE: The caller should have passed a pointer which was previously created by the corresponding function
 * NOTE: The call is idempotent
 */
STATUS freeCallbacksProvider(PClientCallbacks* ppClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbackProvider = NULL;
    UINT32 i;

    CHK(ppClientCallbacks != NULL, STATUS_NULL_ARG);

    pCallbackProvider = (PCallbacksProvider) *ppClientCallbacks;

    // Call is idempotent
    CHK(pCallbackProvider != NULL, retStatus);

    // Iterate and free any callbacks
    if (pCallbackProvider->platformCallbacks.freePlatformCallbacksFn != NULL) {
        pCallbackProvider->platformCallbacks.freePlatformCallbacksFn(&pCallbackProvider->platformCallbacks.customData);
    }

    for (i = 0; i < pCallbackProvider->producerCallbacksCount; i++) {
        if (pCallbackProvider->pProducerCallbacks[i].freeProducerCallbacksFn != NULL) {
            pCallbackProvider->pProducerCallbacks[i].freeProducerCallbacksFn(&pCallbackProvider->pProducerCallbacks[i].customData);
        }
    }

    for (i = 0; i < pCallbackProvider->streamCallbacksCount; i++) {
        if (pCallbackProvider->pStreamCallbacks[i].freeStreamCallbacksFn != NULL) {
            pCallbackProvider->pStreamCallbacks[i].freeStreamCallbacksFn(&pCallbackProvider->pStreamCallbacks[i].customData);
        }
    }

    for (i = 0; i < pCallbackProvider->authCallbacksCount; i++) {
        if (pCallbackProvider->pAuthCallbacks[i].freeAuthCallbacksFn != NULL) {
            pCallbackProvider->pAuthCallbacks[i].freeAuthCallbacksFn(&pCallbackProvider->pAuthCallbacks[i].customData);
        }
    }

    for (i = 0; i < pCallbackProvider->apiCallbacksCount; i++) {
        if (pCallbackProvider->pApiCallbacks[i].freeApiCallbacksFn != NULL) {
            pCallbackProvider->pApiCallbacks[i].freeApiCallbacksFn(&pCallbackProvider->pApiCallbacks[i].customData);
        }
    }

    // Release the object
    MEMFREE(pCallbackProvider);

    // Set the pointer to NULL
    *ppClientCallbacks = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS setDefaultPlatformCallbacks(PCallbacksProvider pClientCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL, STATUS_NULL_ARG);

    // Set the default implementations
    pCallbackProvider->clientCallbacks.getCurrentTimeFn = kinesisVideoStreamDefaultGetCurrentTime;
    pCallbackProvider->clientCallbacks.getRandomNumberFn = kinesisVideoStreamDefaultGetRandomNumber;
    pCallbackProvider->clientCallbacks.createMutexFn = kinesisVideoStreamDefaultCreateMutex;
    pCallbackProvider->clientCallbacks.lockMutexFn = kinesisVideoStreamDefaultLockMutex;
    pCallbackProvider->clientCallbacks.unlockMutexFn = kinesisVideoStreamDefaultUnlockMutex;
    pCallbackProvider->clientCallbacks.tryLockMutexFn = kinesisVideoStreamDefaultTryLockMutex;
    pCallbackProvider->clientCallbacks.freeMutexFn = kinesisVideoStreamDefaultFreeMutex;
    pCallbackProvider->clientCallbacks.createConditionVariableFn = kinesisVideoStreamDefaultCreateConditionVariable;
    pCallbackProvider->clientCallbacks.signalConditionVariableFn = kinesisVideoStreamDefaultSignalConditionVariable;
    pCallbackProvider->clientCallbacks.broadcastConditionVariableFn = kinesisVideoStreamDefaultBroadcastConditionVariable;
    pCallbackProvider->clientCallbacks.waitConditionVariableFn = kinesisVideoStreamDefaultWaitConditionVariable;
    pCallbackProvider->clientCallbacks.freeConditionVariableFn = kinesisVideoStreamDefaultFreeConditionVariable;
    pCallbackProvider->clientCallbacks.logPrintFn = defaultLogPrint;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS setPlatformCallbacks(PClientCallbacks pClientCallbacks, PPlatformCallbacks pPlatformCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL && pPlatformCallbacks != NULL, STATUS_NULL_ARG);

    // Validate the version first
    CHK(pPlatformCallbacks->version <= PLATFORM_CALLBACKS_CURRENT_VERSION, STATUS_INVALID_PLATFORM_CALLBACKS_VERSION);

    // Struct-copy the values
    pCallbackProvider->platformCallbacks = *pPlatformCallbacks;

    // Set the aggregates
    if (pCallbackProvider->platformCallbacks.getCurrentTimeFn != NULL) {
        pCallbackProvider->clientCallbacks.getCurrentTimeFn = getCurrentTimeAggregate;
    }

    if (pCallbackProvider->platformCallbacks.getRandomNumberFn != NULL) {
        pCallbackProvider->clientCallbacks.getRandomNumberFn = getRandomNumberAggregate;
    }

    if (pCallbackProvider->platformCallbacks.createMutexFn != NULL) {
        pCallbackProvider->clientCallbacks.createMutexFn = createMutexAggregate;
    }

    if (pCallbackProvider->platformCallbacks.lockMutexFn != NULL) {
        pCallbackProvider->clientCallbacks.lockMutexFn = lockMutexAggregate;
    }

    if (pCallbackProvider->platformCallbacks.unlockMutexFn != NULL) {
        pCallbackProvider->clientCallbacks.unlockMutexFn = unlockMutexAggregate;
    }

    if (pCallbackProvider->platformCallbacks.tryLockMutexFn != NULL) {
        pCallbackProvider->clientCallbacks.tryLockMutexFn = tryLockMutexAggregate;
    }

    if (pCallbackProvider->platformCallbacks.freeMutexFn != NULL) {
        pCallbackProvider->clientCallbacks.freeMutexFn = freeMutexAggregate;
    }

    if (pCallbackProvider->platformCallbacks.createConditionVariableFn != NULL) {
        pCallbackProvider->clientCallbacks.createConditionVariableFn = createConditionVariableAggregate;
    }

    if (pCallbackProvider->platformCallbacks.signalConditionVariableFn != NULL) {
        pCallbackProvider->clientCallbacks.signalConditionVariableFn = signalConditionVariableAggregate;
    }

    if (pCallbackProvider->platformCallbacks.broadcastConditionVariableFn != NULL) {
        pCallbackProvider->clientCallbacks.broadcastConditionVariableFn = broadcastConditionVariableAggregate;
    }

    if (pCallbackProvider->platformCallbacks.waitConditionVariableFn != NULL) {
        pCallbackProvider->clientCallbacks.waitConditionVariableFn = waitConditionVariableAggregate;
    }

    if (pCallbackProvider->platformCallbacks.freeConditionVariableFn != NULL) {
        pCallbackProvider->clientCallbacks.freeConditionVariableFn = freeConditionVariableAggregate;
    }

    // Special handling for the logging function
    if (pCallbackProvider->platformCallbacks.logPrintFn != NULL) {
        pCallbackProvider->clientCallbacks.logPrintFn = pCallbackProvider->platformCallbacks.logPrintFn;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS addProducerCallbacks(PClientCallbacks pClientCallbacks, PProducerCallbacks pProducerCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL && pProducerCallbacks != NULL, STATUS_NULL_ARG);

    // Validate the version first
    CHK(pProducerCallbacks->version <= PRODUCER_CALLBACKS_CURRENT_VERSION, STATUS_INVALID_PRODUCER_CALLBACKS_VERSION);

    // Check if we have place to put it
    CHK(pCallbackProvider->producerCallbacksCount < pCallbackProvider->callbackChainCount, STATUS_MAX_CALLBACK_CHAIN);

    // Guard against adding same callbacks multiple times (duplicate) - This prevents freeing memory twice
    for (i = 0; i < pCallbackProvider->producerCallbacksCount; i++) {
        CHK(pProducerCallbacks->freeProducerCallbacksFn == NULL ||
                pCallbackProvider->pProducerCallbacks[i].customData != pProducerCallbacks->customData ||
                pCallbackProvider->pProducerCallbacks[i].freeProducerCallbacksFn != pProducerCallbacks->freeProducerCallbacksFn,
            STATUS_DUPLICATE_PRODUCER_CALLBACK_FREE_FUNC);
    }

    // Struct-copy the values and increment the current counter
    pCallbackProvider->pProducerCallbacks[pCallbackProvider->producerCallbacksCount++] = *pProducerCallbacks;

    // Set the aggregates
    if (pProducerCallbacks->storageOverflowPressureFn != NULL) {
        pCallbackProvider->clientCallbacks.storageOverflowPressureFn = storageOverflowPressureAggregate;
    }

    if (pProducerCallbacks->clientReadyFn != NULL) {
        pCallbackProvider->clientCallbacks.clientReadyFn = clientReadyAggregate;
    }

    if (pProducerCallbacks->clientShutdownFn != NULL) {
        pCallbackProvider->clientCallbacks.clientShutdownFn = clientShutdownAggregate;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS addStreamCallbacks(PClientCallbacks pClientCallbacks, PStreamCallbacks pStreamCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL && pStreamCallbacks != NULL, STATUS_NULL_ARG);

    // Validate the version first
    CHK(pStreamCallbacks->version <= STREAM_CALLBACKS_CURRENT_VERSION, STATUS_INVALID_STREAM_CALLBACKS_VERSION);

    // Check if we have place to put it
    CHK(pCallbackProvider->streamCallbacksCount < pCallbackProvider->callbackChainCount, STATUS_MAX_CALLBACK_CHAIN);

    // Guard against adding same callbacks multiple times (duplicate) - This prevents freeing memory twice
    for (i = 0; i < pCallbackProvider->streamCallbacksCount; i++) {
        CHK(pStreamCallbacks->freeStreamCallbacksFn == NULL || pCallbackProvider->pStreamCallbacks[i].customData != pStreamCallbacks->customData ||
                pCallbackProvider->pStreamCallbacks[i].freeStreamCallbacksFn != pStreamCallbacks->freeStreamCallbacksFn,
            STATUS_DUPLICATE_STREAM_CALLBACK_FREE_FUNC);
    }

    // Struct-copy the values and increment the current counter
    pCallbackProvider->pStreamCallbacks[pCallbackProvider->streamCallbacksCount++] = *pStreamCallbacks;

    // Set the aggregates
    if (pStreamCallbacks->streamUnderflowReportFn != NULL) {
        pCallbackProvider->clientCallbacks.streamUnderflowReportFn = streamUnderflowReportAggregate;
    }

    if (pStreamCallbacks->bufferDurationOverflowPressureFn != NULL) {
        pCallbackProvider->clientCallbacks.bufferDurationOverflowPressureFn = bufferDurationOverflowPressureAggregate;
    }

    if (pStreamCallbacks->streamLatencyPressureFn != NULL) {
        pCallbackProvider->clientCallbacks.streamLatencyPressureFn = streamLatencyPressureAggregate;
    }

    if (pStreamCallbacks->streamConnectionStaleFn != NULL) {
        pCallbackProvider->clientCallbacks.streamConnectionStaleFn = streamConnectionStaleAggregate;
    }

    if (pStreamCallbacks->droppedFrameReportFn != NULL) {
        pCallbackProvider->clientCallbacks.droppedFrameReportFn = droppedFrameReportAggregate;
    }

    if (pStreamCallbacks->droppedFragmentReportFn != NULL) {
        pCallbackProvider->clientCallbacks.droppedFragmentReportFn = droppedFragmentReportAggregate;
    }

    if (pStreamCallbacks->streamErrorReportFn != NULL) {
        pCallbackProvider->clientCallbacks.streamErrorReportFn = streamErrorReportAggregate;
    }

    if (pStreamCallbacks->fragmentAckReceivedFn != NULL) {
        pCallbackProvider->clientCallbacks.fragmentAckReceivedFn = fragmentAckReceivedAggregate;
    }

    if (pStreamCallbacks->streamDataAvailableFn != NULL) {
        pCallbackProvider->clientCallbacks.streamDataAvailableFn = streamDataAvailableAggregate;
    }

    if (pStreamCallbacks->streamReadyFn != NULL) {
        pCallbackProvider->clientCallbacks.streamReadyFn = streamReadyAggregate;
    }

    if (pStreamCallbacks->streamClosedFn != NULL) {
        pCallbackProvider->clientCallbacks.streamClosedFn = streamClosedAggregate;
    }

    if (pStreamCallbacks->streamShutdownFn != NULL) {
        pCallbackProvider->clientCallbacks.streamShutdownFn = streamShutdownAggregate;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS addAuthCallbacks(PClientCallbacks pClientCallbacks, PAuthCallbacks pAuthCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL && pAuthCallbacks != NULL, STATUS_NULL_ARG);

    // Validate the version first
    CHK(pAuthCallbacks->version <= AUTH_CALLBACKS_CURRENT_VERSION, STATUS_INVALID_AUTH_CALLBACKS_VERSION);

    // Check if we have place to put it
    CHK(pCallbackProvider->authCallbacksCount < pCallbackProvider->callbackChainCount, STATUS_MAX_CALLBACK_CHAIN);

    // Guard against adding same callbacks multiple times (duplicate) - This prevents freeing memory twice
    for (i = 0; i < pCallbackProvider->authCallbacksCount; i++) {
        CHK(pAuthCallbacks->freeAuthCallbacksFn == NULL || pCallbackProvider->pAuthCallbacks[i].customData != pAuthCallbacks->customData ||
                pCallbackProvider->pAuthCallbacks[i].freeAuthCallbacksFn != pAuthCallbacks->freeAuthCallbacksFn,
            STATUS_DUPLICATE_AUTH_CALLBACK_FREE_FUNC);
    }

    // Struct-copy the values and increment the current counter
    pCallbackProvider->pAuthCallbacks[pCallbackProvider->authCallbacksCount++] = *pAuthCallbacks;

    // Set the aggregates
    if (pAuthCallbacks->getSecurityTokenFn != NULL) {
        pCallbackProvider->clientCallbacks.getSecurityTokenFn = getSecurityTokenAggregate;
    }

    if (pAuthCallbacks->getDeviceCertificateFn != NULL) {
        pCallbackProvider->clientCallbacks.getDeviceCertificateFn = getDeviceCertificateAggregate;
    }

    if (pAuthCallbacks->deviceCertToTokenFn != NULL) {
        pCallbackProvider->clientCallbacks.deviceCertToTokenFn = deviceCertToTokenAggregate;
    }

    if (pAuthCallbacks->getDeviceFingerprintFn != NULL) {
        pCallbackProvider->clientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintAggregate;
    }

    if (pAuthCallbacks->getStreamingTokenFn != NULL) {
        pCallbackProvider->clientCallbacks.getStreamingTokenFn = getStreamingTokenAggregate;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS addApiCallbacks(PClientCallbacks pClientCallbacks, PApiCallbacks pApiCallbacks)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;

    CHK(pCallbackProvider != NULL && pApiCallbacks != NULL, STATUS_NULL_ARG);

    // Validate the version first
    CHK(pApiCallbacks->version <= API_CALLBACKS_CURRENT_VERSION, STATUS_INVALID_API_CALLBACKS_VERSION);

    // Check if we have place to put it
    CHK(pCallbackProvider->apiCallbacksCount < pCallbackProvider->callbackChainCount, STATUS_MAX_CALLBACK_CHAIN);

    // Guard against adding same callbacks multiple times (duplicate) - This prevents freeing memory twice
    for (i = 0; i < pCallbackProvider->apiCallbacksCount; i++) {
        CHK(pApiCallbacks->freeApiCallbacksFn == NULL || pCallbackProvider->pApiCallbacks[i].customData != pApiCallbacks->customData ||
                pCallbackProvider->pApiCallbacks[i].freeApiCallbacksFn != pApiCallbacks->freeApiCallbacksFn,
            STATUS_DUPLICATE_API_CALLBACK_FREE_FUNC);
    }

    // Struct-copy the values and increment the current counter
    pCallbackProvider->pApiCallbacks[pCallbackProvider->apiCallbacksCount++] = *pApiCallbacks;

    // Set the aggregates
    if (pApiCallbacks->createStreamFn != NULL) {
        pCallbackProvider->clientCallbacks.createStreamFn = createStreamAggregate;
    }

    if (pApiCallbacks->describeStreamFn != NULL) {
        pCallbackProvider->clientCallbacks.describeStreamFn = describeStreamAggregate;
    }

    if (pApiCallbacks->getStreamingEndpointFn != NULL) {
        pCallbackProvider->clientCallbacks.getStreamingEndpointFn = getStreamingEndpointAggregate;
    }

    if (pApiCallbacks->putStreamFn != NULL) {
        pCallbackProvider->clientCallbacks.putStreamFn = putStreamAggregate;
    }

    if (pApiCallbacks->tagResourceFn != NULL) {
        pCallbackProvider->clientCallbacks.tagResourceFn = tagResourceAggregate;
    }

    if (pApiCallbacks->createDeviceFn != NULL) {
        pCallbackProvider->clientCallbacks.createDeviceFn = createDeviceAggregate;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Auth callback aggregates
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS getDeviceCertificateAggregate(UINT64 customData, PBYTE* buffer, PUINT32 size, PUINT64 expiration)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->authCallbacksCount; i++) {
        if (pCallbacksProvider->pAuthCallbacks[i].getDeviceCertificateFn != NULL) {
            retStatus = pCallbacksProvider->pAuthCallbacks[i].getDeviceCertificateFn(pCallbacksProvider->pAuthCallbacks[i].customData, buffer, size,
                                                                                     expiration);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS getSecurityTokenAggregate(UINT64 customData, PBYTE* buffer, PUINT32 size, PUINT64 expiration)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->authCallbacksCount; i++) {
        if (pCallbacksProvider->pAuthCallbacks[i].getSecurityTokenFn != NULL) {
            retStatus =
                pCallbacksProvider->pAuthCallbacks[i].getSecurityTokenFn(pCallbacksProvider->pAuthCallbacks[i].customData, buffer, size, expiration);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS getDeviceFingerprintAggregate(UINT64 customData, PCHAR* fingerprint)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->authCallbacksCount; i++) {
        if (pCallbacksProvider->pAuthCallbacks[i].getDeviceFingerprintFn != NULL) {
            retStatus = pCallbacksProvider->pAuthCallbacks[i].getDeviceFingerprintFn(pCallbacksProvider->pAuthCallbacks[i].customData, fingerprint);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS deviceCertToTokenAggregate(UINT64 customData, PCHAR deviceName, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->authCallbacksCount; i++) {
        if (pCallbacksProvider->pAuthCallbacks[i].deviceCertToTokenFn != NULL) {
            retStatus = pCallbacksProvider->pAuthCallbacks[i].deviceCertToTokenFn(pCallbacksProvider->pAuthCallbacks[i].customData, deviceName,
                                                                                  pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS getStreamingTokenAggregate(UINT64 customData, PCHAR streamName, STREAM_ACCESS_MODE accessMode, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->authCallbacksCount; i++) {
        if (pCallbacksProvider->pAuthCallbacks[i].getStreamingTokenFn != NULL) {
            retStatus = pCallbacksProvider->pAuthCallbacks[i].getStreamingTokenFn(pCallbacksProvider->pAuthCallbacks[i].customData, streamName,
                                                                                  accessMode, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Producer callback aggregates
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS storageOverflowPressureAggregate(UINT64 customData, UINT64 remainingBytes)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->producerCallbacksCount; i++) {
        if (pCallbacksProvider->pProducerCallbacks[i].storageOverflowPressureFn != NULL) {
            retStatus = pCallbacksProvider->pProducerCallbacks[i].storageOverflowPressureFn(pCallbacksProvider->pProducerCallbacks[i].customData,
                                                                                            remainingBytes);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS clientReadyAggregate(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->producerCallbacksCount; i++) {
        if (pCallbacksProvider->pProducerCallbacks[i].clientReadyFn != NULL) {
            retStatus = pCallbacksProvider->pProducerCallbacks[i].clientReadyFn(pCallbacksProvider->pProducerCallbacks[i].customData, clientHandle);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS clientShutdownAggregate(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->producerCallbacksCount; i++) {
        if (pCallbacksProvider->pProducerCallbacks[i].clientShutdownFn != NULL) {
            retStatus =
                pCallbacksProvider->pProducerCallbacks[i].clientShutdownFn(pCallbacksProvider->pProducerCallbacks[i].customData, clientHandle);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// API callback aggregates
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS createStreamAggregate(UINT64 customData, PCHAR deviceName, PCHAR streamName, PCHAR contentType, PCHAR kmsKeyId, UINT64 retentionPeriod,
                             PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].createStreamFn != NULL) {
            retStatus = pCallbacksProvider->pApiCallbacks[i].createStreamFn(pCallbacksProvider->pApiCallbacks[i].customData, deviceName, streamName,
                                                                            contentType, kmsKeyId, retentionPeriod, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS describeStreamAggregate(UINT64 customData, PCHAR streamName, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].describeStreamFn != NULL) {
            retStatus = pCallbacksProvider->pApiCallbacks[i].describeStreamFn(pCallbacksProvider->pApiCallbacks[i].customData, streamName,
                                                                              pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS getStreamingEndpointAggregate(UINT64 customData, PCHAR streamName, PCHAR apiName, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].getStreamingEndpointFn != NULL) {
            retStatus = pCallbacksProvider->pApiCallbacks[i].getStreamingEndpointFn(pCallbacksProvider->pApiCallbacks[i].customData, streamName,
                                                                                    apiName, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS putStreamAggregate(UINT64 customData, PCHAR streamName, PCHAR containerType, UINT64 streamStart, BOOL isAbsolute, BOOL fragmentAcks,
                          PCHAR streamingEndpoint, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].putStreamFn != NULL) {
            retStatus =
                pCallbacksProvider->pApiCallbacks[i].putStreamFn(pCallbacksProvider->pApiCallbacks[i].customData, streamName, containerType,
                                                                 streamStart, isAbsolute, fragmentAcks, streamingEndpoint, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS tagResourceAggregate(UINT64 customData, PCHAR resourceArn, UINT32 tagCount, PTag tags, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].tagResourceFn != NULL) {
            retStatus = pCallbacksProvider->pApiCallbacks[i].tagResourceFn(pCallbacksProvider->pApiCallbacks[i].customData, resourceArn, tagCount,
                                                                           tags, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS createDeviceAggregate(UINT64 customData, PCHAR deviceName, PServiceCallContext pServiceCallContext)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->apiCallbacksCount; i++) {
        if (pCallbacksProvider->pApiCallbacks[i].createDeviceFn != NULL) {
            retStatus =
                pCallbacksProvider->pApiCallbacks[i].createDeviceFn(pCallbacksProvider->pApiCallbacks[i].customData, deviceName, pServiceCallContext);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream callback aggregates
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STATUS streamUnderflowReportAggregate(UINT64 customData, STREAM_HANDLE streamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamUnderflowReportFn != NULL) {
            retStatus =
                pCallbacksProvider->pStreamCallbacks[i].streamUnderflowReportFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS bufferDurationOverflowPressureAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 remainingDuration)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].bufferDurationOverflowPressureFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].bufferDurationOverflowPressureFn(pCallbacksProvider->pStreamCallbacks[i].customData,
                                                                                                 streamHandle, remainingDuration);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamLatencyPressureAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 bufferDuration)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamLatencyPressureFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamLatencyPressureFn(pCallbacksProvider->pStreamCallbacks[i].customData,
                                                                                        streamHandle, bufferDuration);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamConnectionStaleAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 stalenessDuration)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamConnectionStaleFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamConnectionStaleFn(pCallbacksProvider->pStreamCallbacks[i].customData,
                                                                                        streamHandle, stalenessDuration);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS droppedFrameReportAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 frameTimestamp)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].droppedFrameReportFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].droppedFrameReportFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle,
                                                                                     frameTimestamp);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS droppedFragmentReportAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 fragmentTimestamp)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].droppedFragmentReportFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].droppedFragmentReportFn(pCallbacksProvider->pStreamCallbacks[i].customData,
                                                                                        streamHandle, fragmentTimestamp);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamErrorReportAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, UINT64 errorTimestamp,
                                  STATUS errorStatus)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamErrorReportFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamErrorReportFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle,
                                                                                    uploadHandle, errorTimestamp, errorStatus);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS fragmentAckReceivedAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle, PFragmentAck pFragmentAck)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].fragmentAckReceivedFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].fragmentAckReceivedFn(pCallbacksProvider->pStreamCallbacks[i].customData,
                                                                                      streamHandle, uploadHandle, pFragmentAck);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamDataAvailableAggregate(UINT64 customData, STREAM_HANDLE streamHandle, PCHAR streamName, UPLOAD_HANDLE uploadHandle,
                                    UINT64 availableDuration, UINT64 availableSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamDataAvailableFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamDataAvailableFn(
                pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle, streamName, uploadHandle, availableDuration, availableSize);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamReadyAggregate(UINT64 customData, STREAM_HANDLE streamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamReadyFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamReadyFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamShutdownAggregate(UINT64 customData, STREAM_HANDLE streamHandle, BOOL resetStream)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamShutdownFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamShutdownFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle,
                                                                                 resetStream);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

STATUS streamClosedAggregate(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE uploadHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;
    UINT32 i;

    CHK(pCallbacksProvider != NULL, STATUS_INVALID_ARG);

    for (i = 0; i < pCallbacksProvider->streamCallbacksCount; i++) {
        if (pCallbacksProvider->pStreamCallbacks[i].streamClosedFn != NULL) {
            retStatus = pCallbacksProvider->pStreamCallbacks[i].streamClosedFn(pCallbacksProvider->pStreamCallbacks[i].customData, streamHandle,
                                                                               uploadHandle);

            // Break on stop processing
            CHK(retStatus != STATUS_STOP_CALLBACK_CHAIN, STATUS_SUCCESS);

            CHK_STATUS(retStatus);
        }
    }

CleanUp:

    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform callback aggregates
///////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 getCurrentTimeAggregate(UINT64 customData)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.getCurrentTimeFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.getCurrentTimeFn(pCallbacksProvider->platformCallbacks.customData);
}

UINT32 getRandomNumberAggregate(UINT64 customData)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.getRandomNumberFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.getRandomNumberFn(pCallbacksProvider->platformCallbacks.customData);
}

MUTEX createMutexAggregate(UINT64 customData, BOOL reentrant)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.createMutexFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.createMutexFn(pCallbacksProvider->platformCallbacks.customData, reentrant);
}

VOID lockMutexAggregate(UINT64 customData, MUTEX mutex)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.lockMutexFn != NULL, "Called an aggregate for a NULL function");
    pCallbacksProvider->platformCallbacks.lockMutexFn(pCallbacksProvider->platformCallbacks.customData, mutex);
}

VOID unlockMutexAggregate(UINT64 customData, MUTEX mutex)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.unlockMutexFn != NULL, "Called an aggregate for a NULL function");
    pCallbacksProvider->platformCallbacks.unlockMutexFn(pCallbacksProvider->platformCallbacks.customData, mutex);
}

BOOL tryLockMutexAggregate(UINT64 customData, MUTEX mutex)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.tryLockMutexFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.tryLockMutexFn(pCallbacksProvider->platformCallbacks.customData, mutex);
}

VOID freeMutexAggregate(UINT64 customData, MUTEX mutex)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.freeMutexFn != NULL, "Called an aggregate for a NULL function");
    pCallbacksProvider->platformCallbacks.freeMutexFn(pCallbacksProvider->platformCallbacks.customData, mutex);
}

CVAR createConditionVariableAggregate(UINT64 customData)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.createConditionVariableFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.createConditionVariableFn(pCallbacksProvider->platformCallbacks.customData);
}

STATUS signalConditionVariableAggregate(UINT64 customData, CVAR cvar)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.signalConditionVariableFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.signalConditionVariableFn(pCallbacksProvider->platformCallbacks.customData, cvar);
}

STATUS broadcastConditionVariableAggregate(UINT64 customData, CVAR cvar)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.broadcastConditionVariableFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.broadcastConditionVariableFn(pCallbacksProvider->platformCallbacks.customData, cvar);
}

STATUS waitConditionVariableAggregate(UINT64 customData, CVAR cvar, MUTEX mutex, UINT64 timeout)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.waitConditionVariableFn != NULL, "Called an aggregate for a NULL function");
    return pCallbacksProvider->platformCallbacks.waitConditionVariableFn(pCallbacksProvider->platformCallbacks.customData, cvar, mutex, timeout);
}

VOID freeConditionVariableAggregate(UINT64 customData, CVAR cvar)
{
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) customData;

    CHECK_EXT(pCallbacksProvider != NULL, "NULL callback provider.");
    CHECK_EXT(pCallbacksProvider->platformCallbacks.freeConditionVariableFn != NULL, "Called an aggregate for a NULL function");
    pCallbacksProvider->platformCallbacks.freeConditionVariableFn(pCallbacksProvider->platformCallbacks.customData, cvar);
}
