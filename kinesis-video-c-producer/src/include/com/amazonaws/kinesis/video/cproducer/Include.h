/**
 * Main public include file
 */
#ifndef __KINESIS_VIDEO_PRODUCER_INCLUDE__
#define __KINESIS_VIDEO_PRODUCER_INCLUDE__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Public headers
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/client/Include.h>
#include <com/amazonaws/kinesis/video/common/Include.h>

////////////////////////////////////////////////////
/// Producer base return codes
////////////////////////////////////////////////////

/*! \addtogroup ProducerBaseStatusCodes
 *  @{
 */
#define STATUS_PRODUCER_BASE                            0x15000000
#define STATUS_STOP_CALLBACK_CHAIN                      STATUS_PRODUCER_BASE + 0x00000001
#define STATUS_MAX_CALLBACK_CHAIN                       STATUS_PRODUCER_BASE + 0x00000002
#define STATUS_INVALID_PLATFORM_CALLBACKS_VERSION       STATUS_PRODUCER_BASE + 0x00000003
#define STATUS_INVALID_PRODUCER_CALLBACKS_VERSION       STATUS_PRODUCER_BASE + 0x00000004
#define STATUS_INVALID_STREAM_CALLBACKS_VERSION         STATUS_PRODUCER_BASE + 0x00000005
#define STATUS_INVALID_AUTH_CALLBACKS_VERSION           STATUS_PRODUCER_BASE + 0x00000006
#define STATUS_INVALID_API_CALLBACKS_VERSION            STATUS_PRODUCER_BASE + 0x00000007
#define STATUS_INVALID_DESCRIBE_STREAM_RETURN_JSON      STATUS_PRODUCER_BASE + 0x0000000f
#define STATUS_MAX_USER_AGENT_NAME_POSTFIX_LEN_EXCEEDED STATUS_PRODUCER_BASE + 0x00000013
#define STATUS_MAX_CUSTOM_USER_AGENT_LEN_EXCEEDED       STATUS_PRODUCER_BASE + 0x00000014
#define STATUS_INVALID_ENDPOINT_CACHING_PERIOD          STATUS_PRODUCER_BASE + 0x00000016
#define STATUS_DUPLICATE_PRODUCER_CALLBACK_FREE_FUNC    STATUS_PRODUCER_BASE + 0x00000019
#define STATUS_DUPLICATE_STREAM_CALLBACK_FREE_FUNC      STATUS_PRODUCER_BASE + 0x0000001a
#define STATUS_DUPLICATE_AUTH_CALLBACK_FREE_FUNC        STATUS_PRODUCER_BASE + 0x0000001b
#define STATUS_DUPLICATE_API_CALLBACK_FREE_FUNC         STATUS_PRODUCER_BASE + 0x0000001c
#define STATUS_FILE_LOGGER_INDEX_FILE_TOO_LARGE         STATUS_PRODUCER_BASE + 0x0000001d
#define STATUS_STREAM_BEING_SHUTDOWN                    STATUS_PRODUCER_BASE + 0x00000025
#define STATUS_CLIENT_BEING_SHUTDOWN                    STATUS_PRODUCER_BASE + 0x00000026
#define STATUS_CONTINUOUS_RETRY_RESET_FAILED            STATUS_PRODUCER_BASE + 0x00000027
/*!@} */

/**
 * Macro for checking whether the status code should be retried by the continuous retry logic
 */
#define IS_RETRIABLE_PRODUCER_ERROR(error)                                                                                                           \
    ((error) == STATUS_INVALID_DESCRIBE_STREAM_RETURN_JSON || (error) == STATUS_STREAM_BEING_SHUTDOWN || (error) == STATUS_CLIENT_BEING_SHUTDOWN)

/**
 * Maximum callbacks in the processing chain
 */
#define MAX_CALLBACK_CHAIN_COUNT 20

/**
 * Default number of the callbacks in the chain
 */
#define DEFAULT_CALLBACK_CHAIN_COUNT 5

/////////////////////////////////////////////////////
/// Current versions for the public structs
/////////////////////////////////////////////////////

/*! \addtogroup CallbackStructsVersion
 * Callback structure versions
 *  @{
 */

/**
 * Producer callback struct current version
 */
#define PRODUCER_CALLBACKS_CURRENT_VERSION 0
/**
 * Platform callback struct current version
 */
#define PLATFORM_CALLBACKS_CURRENT_VERSION 0
/**
 * Stream callback struct current version
 */
#define STREAM_CALLBACKS_CURRENT_VERSION 0
/**
 * Auth callback struct current version
 */
#define AUTH_CALLBACKS_CURRENT_VERSION 0
/**
 * API callback struct current version
 */
#define API_CALLBACKS_CURRENT_VERSION 0
/*!@} */

////////////////////////////////////////////////////
/// Extra callbacks definitions
////////////////////////////////////////////////////

/*! \addtogroup Callbacks
 * Callback definitions
 *  @{
 */

/**
 * @brief Frees platform callbacks
 *
 * @param PUINT64 Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreePlatformCallbacksFunc)(PUINT64);

/**
 * @brief Frees producer callbacks
 *
 * @param PUINT64 Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeProducerCallbacksFunc)(PUINT64);

/**
 * @brief Frees stream callbacks
 *
 * @param PUINT64 Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeStreamCallbacksFunc)(PUINT64);

/**
 * @brief Frees auth callbacks
 *
 * @param PUINT64 Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeAuthCallbacksFunc)(PUINT64);

/**
 * @brief Frees API callbacks
 *
 * @param PUINT64 Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeApiCallbacksFunc)(PUINT64);
/*!@} */

////////////////////////////////////////////////////
/// Main structure declarations
////////////////////////////////////////////////////

/*! \addtogroup PublicStructures
 *
 * @{
 */

/**
 * @brief The Platform specific callbacks
 */
typedef struct __PlatformCallbacks PlatformCallbacks;
struct __PlatformCallbacks {
    UINT32 version;                                              //!< Version
    UINT64 customData;                                           //!< Custom data to be passed back to the caller
    GetCurrentTimeFunc getCurrentTimeFn;                         //!< The GetCurrentTimeFunc callback function
    GetRandomNumberFunc getRandomNumberFn;                       //!< The GetRandomNumberFunc callback function
    CreateMutexFunc createMutexFn;                               //!< The CreateMutexFunc callback function
    LockMutexFunc lockMutexFn;                                   //!< The LockMutexFunc callback function
    UnlockMutexFunc unlockMutexFn;                               //!< The UnlockMutexFunc callback function
    TryLockMutexFunc tryLockMutexFn;                             //!< The TryLockMutexFunc callback function
    FreeMutexFunc freeMutexFn;                                   //!< The FreeMutexFunc callback function
    CreateConditionVariableFunc createConditionVariableFn;       //!< The CreateConditionVariableFunc callback function
    SignalConditionVariableFunc signalConditionVariableFn;       //!< The SignalConditionVariableFunc callback function
    BroadcastConditionVariableFunc broadcastConditionVariableFn; //!< The BroadcastConditionVariableFunc callback function
    WaitConditionVariableFunc waitConditionVariableFn;           //!< The WaitConditionVariableFunc callback function
    FreeConditionVariableFunc freeConditionVariableFn;           //!< The FreeConditionVariableFunc callback function
    LogPrintFunc logPrintFn;                                     //!< The LogPrintFunc callback function
    FreePlatformCallbacksFunc freePlatformCallbacksFn;           //!< Specialized cleanup callback
};
typedef struct __PlatformCallbacks* PPlatformCallbacks;

/**
 * @brief The Producer object specific callbacks
 */
typedef struct __ProducerCallbacks ProducerCallbacks;
struct __ProducerCallbacks {
    UINT32 version;                                        //!< Version
    UINT64 customData;                                     //!< Custom data to be passed back to the caller
    StorageOverflowPressureFunc storageOverflowPressureFn; //!< The StorageOverflowPressureFunc callback function
    ClientReadyFunc clientReadyFn;                         //!< The ClientReadyFunc callback function
    ClientShutdownFunc clientShutdownFn;                   //!< The ClientShutdownFunc callback function
    FreeProducerCallbacksFunc freeProducerCallbacksFn;     //!< Specialized cleanup callback
};
typedef struct __ProducerCallbacks* PProducerCallbacks;

/**
 * @brief The Stream specific callbacks
 */
typedef struct __StreamCallbacks StreamCallbacks;
struct __StreamCallbacks {
    UINT32 version;                                                      //!< Version
    UINT64 customData;                                                   //!< Custom data to be passed back to the caller
    StreamUnderflowReportFunc streamUnderflowReportFn;                   //!< The StreamUnderflowReportFunc callback function
    BufferDurationOverflowPressureFunc bufferDurationOverflowPressureFn; //!< The BufferDurationOverflowPressureFunc callback function
    StreamLatencyPressureFunc streamLatencyPressureFn;                   //!< The StreamLatencyPressureFunc callback function
    StreamConnectionStaleFunc streamConnectionStaleFn;                   //!< The StreamConnectionStaleFunc callback function
    DroppedFrameReportFunc droppedFrameReportFn;                         //!< The DroppedFrameReportFunc callback function
    DroppedFragmentReportFunc droppedFragmentReportFn;                   //!< The DroppedFragmentReportFunc callback function
    StreamErrorReportFunc streamErrorReportFn;                           //!< The StreamErrorReportFunc callback function
    FragmentAckReceivedFunc fragmentAckReceivedFn;                       //!< The FragmentAckReceivedFunc callback function
    StreamDataAvailableFunc streamDataAvailableFn;                       //!< The StreamDataAvailableFunc callback function
    StreamReadyFunc streamReadyFn;                                       //!< The StreamReadyFunc callback function
    StreamClosedFunc streamClosedFn;                                     //!< The StreamClosedFunc callback function
    StreamShutdownFunc streamShutdownFn;                                 //!< The StreamShutdownFunc callback function

    // Specialized cleanup callback
    FreeStreamCallbacksFunc freeStreamCallbacksFn;
};
typedef struct __StreamCallbacks* PStreamCallbacks;

/**
 * @brief The Authentication specific callbacks
 */
typedef struct __AuthCallbacks AuthCallbacks;
struct __AuthCallbacks {
    UINT32 version;                                  //!< Version
    UINT64 customData;                               //!< Custom data to be passed back to the caller
    GetSecurityTokenFunc getSecurityTokenFn;         //!< The GetSecurityTokenFunc callback function
    GetDeviceCertificateFunc getDeviceCertificateFn; //!< The GetDeviceCertificateFunc callback function
    DeviceCertToTokenFunc deviceCertToTokenFn;       //!< The DeviceCertToTokenFunc callback function
    GetDeviceFingerprintFunc getDeviceFingerprintFn; //!< The GetDeviceFingerprintFunc callback function
    GetStreamingTokenFunc getStreamingTokenFn;       //!< The GetStreamingTokenFunc callback function

    //!< Specialized cleanup callback
    FreeAuthCallbacksFunc freeAuthCallbacksFn;
};
typedef struct __AuthCallbacks* PAuthCallbacks;

/**
 * @brief The KVS backend specific callbacks
 */
typedef struct __ApiCallbacks ApiCallbacks;
struct __ApiCallbacks {
    UINT32 version;                                  //!< Version
    UINT64 customData;                               //!< Custom data to be passed back to the caller
    CreateStreamFunc createStreamFn;                 //!< The CreateStreamFunc callback function
    DescribeStreamFunc describeStreamFn;             //!< The DescribeStreamFunc callback function
    GetStreamingEndpointFunc getStreamingEndpointFn; //!< The GetStreamingEndpointFunc callback function
    PutStreamFunc putStreamFn;                       //!< The PutStreamFunc callback function
    TagResourceFunc tagResourceFn;                   //!< The TagResourceFunc callback function
    CreateDeviceFunc createDeviceFn;                 //!< The CreateDeviceFunc callback function
    FreeApiCallbacksFunc freeApiCallbacksFn;         //!< Specialized cleanup callback
};
typedef struct __ApiCallbacks* PApiCallbacks;
/*!@} */

////////////////////////////////////////////////////
/// Main enum declarations
////////////////////////////////////////////////////
/*! \addtogroup PubicEnums
 *
 * @{
 */

/**
 * Type of caching implementation to use with the callbacks provider
 */
typedef enum {
    //!< No caching. The callbacks provider will make backend API calls every time PIC requests.
    API_CALL_CACHE_TYPE_NONE,

    //!< The backend API calls are emulated for all of the API calls with the exception of GetStreamingEndpoint
    //!< and PutMedia (for actual streaming). The result of the GetStreamingEndpoint is cached and the cached
    //!< value is returned next time PIC makes a request to call GetStreamingEndpoint call.
    //!< NOTE: The stream should be pre-existing as DescribeStream API call will be emulated and will
    //!< return a success with stream being Active.
    API_CALL_CACHE_TYPE_ENDPOINT_ONLY,

    //!< Cache all of the backend API calls with the exception of PutMedia (for actual data streaming).
    //!< In this mode, the actual backend APIs will be called once and the information will be cached.
    //!< The cached result will be returned afterwards when the PIC requests the provider to make the
    //!< backend API call.
    //!< This mode is the recommended mode for most of the streaming scenarios and is the default in the samples.
    //!<
    //!< NOTE: This mode is most suitable for streaming scenarios when the stream is not dynamically deleted
    //!< or modified by another entity so the cached value of the API calls are static.
    //!< NOTE: This mode will work for non-pre-created streams as the first call will not be cached for
    //!< DescribeStream API call and if the stream is not created then the state machine will attempt
    //!< to create it by calling CreateStream API call which would evaluate to the actual backend call
    //!< to create the stream as it's not cached.
    API_CALL_CACHE_TYPE_ALL,
} API_CALL_CACHE_TYPE;
/*!@} */

////////////////////////////////////////////////////
/// Public functions
////////////////////////////////////////////////////

/*! \addtogroup PublicMemberFunctions
 * @{
 */

/**
 * Creates a default callbacks provider based on static AWS credentials
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param[in] PCHAR Access Key Id
 * @param[in] PCHAR Secret Key
 * @param[in,opt] PCHAR Session Token
 * @param[in] UINT64 Expiration of the token. MAX_UINT64 if non-expiring
 * @param[in,opt] PCHAR AWS region
 * @param[in,opt] PCHAR CA Cert Path
 * @param[in,opt] PCHAR User agent name (Use NULL)
 * @param[in,out] PCHAR Custom user agent to be used in the API calls
 * @param[out] PClientCallbacks* Returned pointer to callbacks provider
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithAwsCredentials(PCHAR, PCHAR, PCHAR, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses iot certificate as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param[in] PCHAR IoT endpoint to use for the auth
 * @param[in] PCHAR Credential cert path
 * @param[in] PCHAR Private key path
 * @param[in,opt] PCHAR CA Cert path
 * @param[in] PCHAR Role alias name
 * @param[in] PCHAR IoT Thing name
 * @param[in,opt] PCHAR AWS region
 * @param[in,opt] PCHAR User agent name (Use NULL)
 * @param[in,opt] PCHAR Custom user agent to be used in the API calls
 * @param[out] PClientCallbacks* Returned pointer to callbacks provider
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithIotCertificate(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses file-based certificate as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param[in] PCHAR Credential file path
 * @param[in,opt] PCHAR AWS region
 * @param[in,opt] PCHAR CA Cert path
 * @param[in,opt] PCHAR User agent name (Use NULL)
 * @param[in,opt] PCHAR Custom user agent to be used in the API calls
 * @param[out] PClientCallbacks* Returned pointer to callbacks provider
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithFileAuth(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses auth callbacks as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param[in] PAuthCallbacks Auth Callback for the auth
 * @param[in,opt] PCHAR AWS region
 * @param[in,opt] PCHAR CA Cert path
 * @param[in,opt] PCHAR User agent name (Use NULL)
 * @param[in,opt] PCHAR Custom user agent to be used in the API calls
 * @param[out] PClientCallbacks* Returned pointer to callbacks provider
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithAuthCallbacks(PAuthCallbacks, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Releases and frees the callbacks provider structure.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in,out] PClientCallbacks* Pointer to callbacks provider to free
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS freeCallbacksProvider(PClientCallbacks*);

/**
 * Sets the Platform callbacks.
 *
 * NOTE: The existing Platform callbacks are overwritten.
 *
 * @param[in] PClientCallbacks Pointer to client callbacks
 * @param[in] @PPlatformCallbacks Pointer to platform callbacks to set
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS setPlatformCallbacks(PClientCallbacks, PPlatformCallbacks);

/**
 * Appends Producer callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param[in] PClientCallbacks Pointer to client callbacks
 * @param[in] PProducerCallbacks Pointer to producer callbacks to add
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS addProducerCallbacks(PClientCallbacks, PProducerCallbacks);

/**
 * Appends Stream callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param[in] PClientCallbacks Pointer to client callbacks
 * @param[in] PStreamCallbacks Pointer to stream callbacks to use
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS addStreamCallbacks(PClientCallbacks, PStreamCallbacks);

/**
 * Appends Auth callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param[in] PClientCallbacks Pointer to client callbacks
 * @param[in] PAuthCallbacks Pointer to auth callbacks to use
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS addAuthCallbacks(PClientCallbacks, PAuthCallbacks);

/**
 * Appends Api callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param[in] PClientCallbacks Pointer to client callbacks
 * @param[in] PApiCallbacks Pointer to api callbacks to use
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS addApiCallbacks(PClientCallbacks, PApiCallbacks);

/**
 * Creates Stream Info for RealTime Streaming Scenario using default values.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in,opt] PCHAR stream name
 * @param[in] UINT64 retention in 100ns time unit
 * @param[in] UINT64 buffer duration in 100ns time unit
 * @param[out] PStreamInfo* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createRealtimeVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for Offline Video Streaming Scenario.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in,opt] PCHAR stream name
 * @param[in] UINT64 etention in 100ns time unit. Should be greater than 0.
 * @param[in] UINT64 buffer duration in 100ns time unit
 * @param[out] PStreamInfo* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createOfflineVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for RealTime Audio Video Streaming Scenario
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in,opt] PCHAR stream name
 * @param[in] UINT64 retention in 100ns time unit
 * @param[in] UINT64 buffer duration in 100ns time unit
 * @param[out] PStreamInfo* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createRealtimeAudioVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for Offline Audio Video Streaming Scenario
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in,opt] PCHAR stream name
 * @param[in] UINT64 retention in 100ns time unit
 * @param[in] UINT64 buffer duration in 100ns time unit
 * @param[out] PStreamInfo* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createOfflineAudioVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Configure streaminfo based on given storage size and average bitrate
 * Will change buffer duration, stream latency duration.
 *
 * @param[in] UINT32 Storage size in bytes
 * @param[in] UINT64 Total average bitrate for all tracks. Unit: bits per second
 * @param[in] UINT32 Total number of streams per kinesisVideoStreamClient
 * @param[out] PStreamInfo
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS setStreamInfoBasedOnStorageSize(UINT32, UINT64, UINT32, PStreamInfo);

/*
 * Frees the StreamInfo provider object.
 *
 * @param[in,out] PStreamInfo* StreamInfo provider object to be freed
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS freeStreamInfoProvider(PStreamInfo*);

/**
 * Create deviceInfo with DEFAULT_STORAGE_SIZE amount of storage.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[out] PDeviceInfo* pointer to PDeviceInfo that will point to the created deviceInfo
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS createDefaultDeviceInfo(PDeviceInfo*);

/**
 * Frees the DeviceInfo provider object.
 *
 * @param[in,out] PDeviceInfo* pointer to PDeviceInfo provider that will be freed
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS freeDeviceInfo(PDeviceInfo*);

/**
 * Can be called after createDefaultDeviceInfo, set the storage size to storageSize.
 *
 * @param[in] PDeviceInfo pointer to the target object
 * @param[in] UINT64 Buffer storage size in bytes
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS setDeviceInfoStorageSize(PDeviceInfo, UINT64);

/**
 *  Can be called after createDefaultDeviceInfo, calculate the storage size based on averageBitRate and bufferDuration,
 *  the call initDeviceInfoStorageInMemoryFixedSize to set storage size.
 *
 * @param[in] PDeviceInfo pointer to the target object
 * @param[in] UINT64 bitrate in bits per second
 * @param[in] UINT64 buffer duration in 100ns
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS setDeviceInfoStorageSizeBasedOnBitrateAndBufferDuration(PDeviceInfo, UINT64, UINT64);

/*
 * Creates the Iot Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in] PCallbacksProvider Pointer to callback provider
 * @param[in] PCHAR iot credentials endpoint
 * @param[in] PCHAR kvs iot certificate file path
 * @param[in] PCHAR private key file path
 * @param[in] PCHAR CA cert path
 * @param[in] PCHAR iot role alias
 * @param[in] PCHAR IoT thing name
 *
 * @param[in,out] PAuthCallbacks* Pointer to pointer to AuthCallback struct
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS createIotAuthCallbacks(PClientCallbacks, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PAuthCallbacks*);

/*
 * Frees the Iot Credential auth callbacks
 *
 * @param[in,out] PAuthCallbacks* pointer to AuthCallback provider object
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS freeIotAuthCallbacks(PAuthCallbacks*);

/*
 * Creates the File Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in] PCallbacksProvider Pointer to callback provider
 * @param[in] PCHAR file path for the credentials file
 * @param[in,out] PAuthCallbacks* Pointer to pointer to AuthCallback struct
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS createFileAuthCallbacks(PClientCallbacks, PCHAR, PAuthCallbacks*);

/*
 * Frees the File Credential auth callbacks
 *
 * @param[in,out] PAuthCallbacks* pointer to AuthCallback provider object
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS freeFileAuthCallbacks(PAuthCallbacks*);

/*
 * Creates a Static Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[in] PCallbacksProvider Pointer to callback provider
 * @param[in] PCHAR Access key id
 * @param[in] PCHAR Secret key
 * @param[in,opt] PCHAR Session token
 * @param[in] UINT64 Expiration absolute time in 100ns
 * @param[in,out] PAuthCallbacks* Pointer to pointer to AuthCallback struct
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS createStaticAuthCallbacks(PClientCallbacks, PCHAR, PCHAR, PCHAR, UINT64, PAuthCallbacks*);

/*
 * Frees the Static Credential auth callback
 *
 * @param[in,out] PAuthCallbacks* pointer to AuthCallback provider object
 *
 * @return STATUS status of operation
 */
PUBLIC_API STATUS freeStaticAuthCallbacks(PAuthCallbacks*);

/**
 * Creates StreamCallbacks struct
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param[out] PStreamCallbacks* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createStreamCallbacks(PStreamCallbacks*);

/**
 * Frees StreamCallbacks struct
 *
 * @param[in,out] PStreamCallbacks* the object to be freed
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS freeStreamCallbacks(PStreamCallbacks*);

/**
 * Creates a stream callbacks object allowing continuous retry on failures,
 * reset logic on staleness and latency.
 *
 * https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/producer-reference-callbacks.html
 *
 * @param[in] PCallbacksProvider Pointer to callback provider
 * @param[out] PStreamCallbacks* Constructed object
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createContinuousRetryStreamCallbacks(PClientCallbacks, PStreamCallbacks*);

/**
 * Frees a previously constructed continuous stream callbacks object
 *
 * @param[in,out,opt] PStreamCallbacks* Object to be destroyed.
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS freeContinuousRetryStreamCallbacks(PStreamCallbacks*);

/**
 * Create abstract callback provider that can hook with other callbacks
 *
 * @param[in] UINT32 Length of callback provider calling chain
 * @param[in] API_CALL_CACHE_TYPE Backend API call caching mode
 * @param[in] UINT64 Cached endpoint update period
 * @param[in,opt] PCHAR AWS region
 * @param[in] PCHAR Specific Control Plane Uri as endpoint to be called
 * @param[in,opt] PCHAR CA Cert path
 * @param[in,opt] PCHAR User agent name (Use NULL)
 * @param[in,opt] PCHAR Custom user agent to be used in the API calls
 * @param[out] PClientCallbacks* Returned pointer to callbacks provider
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS createAbstractDefaultCallbacksProvider(UINT32, API_CALL_CACHE_TYPE, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Use file logger instead of default logger which log to stdout. The underlying objects are automatically freed
 * when PClientCallbacks is freed.
 *
 * @param[in] PClientCallbacks The callback provider whose logPrintFn will be replaced with file logger log printing function
 * @param[in] UINT64 Size of string buffer in file logger. When the string buffer is full the logger will flush everything into a new file
 * @param[in] UINT64 Max number of log file. When exceeded, the oldest file will be deleted when new one is generated
 * @param[in] PCHAR Directory in which the log file will be generated
 * @param[in] BOOL print log to std out too
 *
 * @return STATUS code of the execution
 */
PUBLIC_API STATUS addFileLoggerPlatformCallbacksProvider(PClientCallbacks, UINT64, UINT64, PCHAR, BOOL);
/*!@} */

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_PRODUCER_INCLUDE__ */
