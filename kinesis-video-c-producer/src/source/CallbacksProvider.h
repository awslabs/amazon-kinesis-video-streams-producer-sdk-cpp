/*******************************************
CallbacksProvider internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_CALLBACKS_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_CALLBACKS_PROVIDER_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The KVS callbacks provider structure
 */
typedef struct __CallbacksProvider CallbacksProvider;
struct __CallbacksProvider {
    // NOTE: Client callbacks have to be the first member of the struct as this is returned as the callbacks
    ClientCallbacks clientCallbacks;

    // Max number of callbacks to have
    UINT32 callbackChainCount;

    // Platform callbacks
    // NOTE: The platform callbacks are not a chain.
    // NOTE: Logging function is unique as it doesn't take custom data so we directly set it without an aggregate.
    PlatformCallbacks platformCallbacks;

    // Producer callbacks chain pointing to the end of the structure
    PProducerCallbacks pProducerCallbacks;

    // Producer callbacks count
    UINT32 producerCallbacksCount;

    // Stream callbacks chain pointing to the end of the structure
    PStreamCallbacks pStreamCallbacks;

    // Stream callbacks count
    UINT32 streamCallbacksCount;

    // Auth callbacks chain pointing to the end of the structure
    PAuthCallbacks pAuthCallbacks;

    // Auth callbacks count
    UINT32 authCallbacksCount;

    // Api callbacks chain pointing to the end of the structure
    PApiCallbacks pApiCallbacks;

    // Api callbacks count
    UINT32 apiCallbacksCount;
};
typedef struct __CallbacksProvider* PCallbacksProvider;

////////////////////////////////////////////////////
// Internal functionality definitions
////////////////////////////////////////////////////
STATUS setDefaultPlatformCallbacks(PCallbacksProvider);

/**
 * Creates a default callbacks provider.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param - UINT32 - IN - Intended callbacks chain count
 * @param - PCHAR - IN - Access Key Id
 * @param - PCHAR - IN - Secret Key
 * @param - PCHAR - IN/OPT - Session token
 * @param - UINT64 - IN - Credentials expiration time
 * @param - PCHAR - IN - AWS region
 * @param - PCHAR - IN - Service control plane URI
 * @param - PCHAR - IN - Certificate path
 * @param - PCHAR - IN - User agent postfix to be used in the API calls
 * @param - PCHAR - IN - Custom user agent to be used in the API calls
 * @param - API_CALL_CACHE_TYPE - IN - Backend API call caching mode
 * @param - UINT64 - IN - The cache update period in case of caching endpoint only provider
 * @param - BOOL - IN - Whether to create continuous retry callback provider
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
STATUS createDefaultCallbacksProvider(UINT32, PCHAR, PCHAR, PCHAR, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, API_CALL_CACHE_TYPE, UINT64, BOOL,
                                      PClientCallbacks*);

////////////////////////////////////////////////////
// Aggregate callbacks definitions
////////////////////////////////////////////////////
STATUS getDeviceCertificateAggregate(UINT64, PBYTE*, PUINT32, PUINT64);
STATUS getSecurityTokenAggregate(UINT64, PBYTE*, PUINT32, PUINT64);
STATUS getDeviceFingerprintAggregate(UINT64, PCHAR*);
STATUS deviceCertToTokenAggregate(UINT64, PCHAR, PServiceCallContext);
STATUS getStreamingTokenAggregate(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);

STATUS storageOverflowPressureAggregate(UINT64, UINT64);
STATUS clientReadyAggregate(UINT64, CLIENT_HANDLE);
STATUS clientShutdownAggregate(UINT64, CLIENT_HANDLE);

STATUS createStreamAggregate(UINT64, PCHAR, PCHAR, PCHAR, PCHAR, UINT64, PServiceCallContext);
STATUS describeStreamAggregate(UINT64, PCHAR, PServiceCallContext);
STATUS getStreamingEndpointAggregate(UINT64, PCHAR, PCHAR, PServiceCallContext);
STATUS putStreamAggregate(UINT64, PCHAR, PCHAR, UINT64, BOOL, BOOL, PCHAR, PServiceCallContext);
STATUS tagResourceAggregate(UINT64, PCHAR, UINT32, PTag, PServiceCallContext);
STATUS createDeviceAggregate(UINT64, PCHAR, PServiceCallContext);

STATUS streamUnderflowReportAggregate(UINT64, STREAM_HANDLE);
STATUS bufferDurationOverflowPressureAggregate(UINT64, STREAM_HANDLE, UINT64);
STATUS streamLatencyPressureAggregate(UINT64, STREAM_HANDLE, UINT64);
STATUS streamConnectionStaleAggregate(UINT64, STREAM_HANDLE, UINT64);
STATUS droppedFrameReportAggregate(UINT64, STREAM_HANDLE, UINT64);
STATUS droppedFragmentReportAggregate(UINT64, STREAM_HANDLE, UINT64);
STATUS streamErrorReportAggregate(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, STATUS);
STATUS fragmentAckReceivedAggregate(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, PFragmentAck);
STATUS streamDataAvailableAggregate(UINT64, STREAM_HANDLE, PCHAR, UPLOAD_HANDLE, UINT64, UINT64);
STATUS streamReadyAggregate(UINT64, STREAM_HANDLE);
STATUS streamClosedAggregate(UINT64, STREAM_HANDLE, UPLOAD_HANDLE);
STATUS streamShutdownAggregate(UINT64, STREAM_HANDLE, BOOL);

UINT64 getCurrentTimeAggregate(UINT64);
UINT32 getRandomNumberAggregate(UINT64);
MUTEX createMutexAggregate(UINT64, BOOL);
VOID lockMutexAggregate(UINT64, MUTEX);
VOID unlockMutexAggregate(UINT64, MUTEX);
BOOL tryLockMutexAggregate(UINT64, MUTEX);
VOID freeMutexAggregate(UINT64, MUTEX);
CVAR createConditionVariableAggregate(UINT64);
STATUS signalConditionVariableAggregate(UINT64, CVAR);
STATUS broadcastConditionVariableAggregate(UINT64, CVAR);
STATUS waitConditionVariableAggregate(UINT64, CVAR, MUTEX, UINT64);
VOID freeConditionVariableAggregate(UINT64, CVAR);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_CALLBACKS_PROVIDER_INCLUDE_I__ */
