/**
 * Main public include file
 */
#ifndef __KINESIS_VIDEO_PRODUCER_INCLUDE__
#define __KINESIS_VIDEO_PRODUCER_INCLUDE__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Public headers
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/client/Include.h>

// For tight packing
#pragma pack(push, include, 1) // for byte alignment

////////////////////////////////////////////////////
// Status return codes
////////////////////////////////////////////////////
#define STATUS_PRODUCER_BASE                                                        0x15000000
#define STATUS_STOP_CALLBACK_CHAIN                                                  STATUS_PRODUCER_BASE + 0x00000001
#define STATUS_MAX_CALLBACK_CHAIN                                                   STATUS_PRODUCER_BASE + 0x00000002
#define STATUS_INVALID_PLATFORM_CALLBACKS_VERSION                                   STATUS_PRODUCER_BASE + 0x00000003
#define STATUS_INVALID_PRODUCER_CALLBACKS_VERSION                                   STATUS_PRODUCER_BASE + 0x00000004
#define STATUS_INVALID_STREAM_CALLBACKS_VERSION                                     STATUS_PRODUCER_BASE + 0x00000005
#define STATUS_INVALID_AUTH_CALLBACKS_VERSION                                       STATUS_PRODUCER_BASE + 0x00000006
#define STATUS_INVALID_API_CALLBACKS_VERSION                                        STATUS_PRODUCER_BASE + 0x00000007
#define STATUS_INVALID_AWS_CREDENTIALS_VERSION                                      STATUS_PRODUCER_BASE + 0x00000008
#define STATUS_MAX_REQUEST_HEADER_COUNT                                             STATUS_PRODUCER_BASE + 0x00000009
#define STATUS_MAX_REQUEST_HEADER_NAME_LEN                                          STATUS_PRODUCER_BASE + 0x0000000a
#define STATUS_MAX_REQUEST_HEADER_VALUE_LEN                                         STATUS_PRODUCER_BASE + 0x0000000b
#define STATUS_INVALID_API_CALL_RETURN_JSON                                         STATUS_PRODUCER_BASE + 0x0000000c
#define STATUS_CURL_INIT_FAILED                                                     STATUS_PRODUCER_BASE + 0x0000000d
#define STATUS_CURL_LIBRARY_INIT_FAILED                                             STATUS_PRODUCER_BASE + 0x0000000e
#define STATUS_INVALID_DESCRIBE_STREAM_RETURN_JSON                                  STATUS_PRODUCER_BASE + 0x0000000f
#define STATUS_HMAC_GENERATION_ERROR                                                STATUS_PRODUCER_BASE + 0x00000010
#define STATUS_IOT_FAILED                                                           STATUS_PRODUCER_BASE + 0x00000011
#define STATUS_MAX_ROLE_ALIAS_LEN_EXCEEDED                                          STATUS_PRODUCER_BASE + 0x00000012
#define STATUS_MAX_USER_AGENT_NAME_POSTFIX_LEN_EXCEEDED                             STATUS_PRODUCER_BASE + 0x00000013
#define STATUS_MAX_CUSTOM_USER_AGENT_LEN_EXCEEDED                                   STATUS_PRODUCER_BASE + 0x00000014
#define STATUS_INVALID_USER_AGENT_LENGTH                                            STATUS_PRODUCER_BASE + 0x00000015
#define STATUS_INVALID_ENDPOINT_CACHING_PERIOD                                      STATUS_PRODUCER_BASE + 0x00000016
#define STATUS_IOT_EXPIRATION_OCCURS_IN_PAST                                        STATUS_PRODUCER_BASE + 0x00000017
#define STATUS_IOT_EXPIRATION_PARSING_FAILED                                        STATUS_PRODUCER_BASE + 0x00000018
#define STATUS_DUPLICATE_PRODUCER_CALLBACK_FREE_FUNC                                STATUS_PRODUCER_BASE + 0x00000019
#define STATUS_DUPLICATE_STREAM_CALLBACK_FREE_FUNC                                  STATUS_PRODUCER_BASE + 0x0000001a
#define STATUS_DUPLICATE_AUTH_CALLBACK_FREE_FUNC                                    STATUS_PRODUCER_BASE + 0x0000001b
#define STATUS_DUPLICATE_API_CALLBACK_FREE_FUNC                                     STATUS_PRODUCER_BASE + 0x0000001c
#define STATUS_FILE_LOGGER_INDEX_FILE_TOO_LARGE                                     STATUS_PRODUCER_BASE + 0x0000001d


/**
 * Maximum callbacks in the processing chain
 */
#define MAX_CALLBACK_CHAIN_COUNT                                                20

/**
 * Default number of the callbacks in the chain
 */
#define DEFAULT_CALLBACK_CHAIN_COUNT                                            5

/**
 * Max region name
 */
#define MAX_REGION_NAME_LEN                                                     128

/**
 * Max user agent string
 */
#define MAX_USER_AGENT_LEN                                                      256

/**
 * Max custom user agent string
 */
#define MAX_CUSTOM_USER_AGENT_LEN                                               128

/**
 * Max custom user agent name postfix string
 */
#define MAX_CUSTOM_USER_AGENT_NAME_POSTFIX_LEN                                  32

/**
 * Default Video track ID to be used
 */
#define DEFAULT_VIDEO_TRACK_ID                                                  1

/**
 * Default Audio track ID to be used
 */
#define DEFAULT_AUDIO_TRACK_ID                                                  2

/*
 * Max access key id length https://docs.aws.amazon.com/STS/latest/APIReference/API_Credentials.html
 */
#define MAX_ACCESS_KEY_LEN                                                      128

/*
 * Max secret access key length
 */
#define MAX_SECRET_KEY_LEN                                                      128

/*
 * Max session token string length
 */
#define MAX_SESSION_TOKEN_LEN                                                   2048

/*
 * Max expiration string length
 */
#define MAX_EXPIRATION_LEN                                                      128

/*
 * Max role alias length https://docs.aws.amazon.com/iot/latest/apireference/API_UpdateRoleAlias.html
 */
#define MAX_ROLE_ALIAS_LEN                                                      128

/**
 * Default period for the cached endpoint update
 */
#define DEFAULT_ENDPOINT_CACHE_UPDATE_PERIOD                                    (40 * HUNDREDS_OF_NANOS_IN_A_MINUTE)

/**
 * Sentinel value indicating to use default update period
 */
#define ENDPOINT_UPDATE_PERIOD_SENTINEL_VALUE                                   0

/**
 * Max period for the cached endpoint update
 */
#define MAX_ENDPOINT_CACHE_UPDATE_PERIOD                                        (24 * HUNDREDS_OF_NANOS_IN_AN_HOUR)

/**
 * AWS credential environment variable name
 */
#define ACCESS_KEY_ENV_VAR                                                      ((PCHAR) "AWS_ACCESS_KEY_ID")
#define SECRET_KEY_ENV_VAR                                                      ((PCHAR) "AWS_SECRET_ACCESS_KEY")
#define SESSION_TOKEN_ENV_VAR                                                   ((PCHAR) "AWS_SESSION_TOKEN")
#define DEFAULT_REGION_ENV_VAR                                                  ((PCHAR) "AWS_DEFAULT_REGION")
#define CACERT_PATH_ENV_VAR                                                     ((PCHAR) "AWS_KVS_CACERT_PATH")

////////////////////////////////////////////////////
// Main defines
////////////////////////////////////////////////////
/**
 * Current versions for the public structs
 */
#define PRODUCER_CALLBACKS_CURRENT_VERSION                                      0
#define PLATFORM_CALLBACKS_CURRENT_VERSION                                      0
#define STREAM_CALLBACKS_CURRENT_VERSION                                        0
#define AUTH_CALLBACKS_CURRENT_VERSION                                          0
#define API_CALLBACKS_CURRENT_VERSION                                           0
#define AWS_CREDENTIALS_CURRENT_VERSION                                         0

/**
 * Maximal length of the credentials file
 */
#define MAX_CREDENTIAL_FILE_LEN                                                 MAX_AUTH_LEN

////////////////////////////////////////////////////
// Extra callbacks definitions
////////////////////////////////////////////////////
/**
 * Frees platform callbacks
 *
 * @param 1 PUINT64 - Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreePlatformCallbacksFunc)(PUINT64);

/**
 * Frees producer callbacks
 *
 * @param 1 PUINT64 - Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeProducerCallbacksFunc)(PUINT64);

/**
 * Frees stream callbacks
 *
 * @param 1 PUINT64 - Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeStreamCallbacksFunc)(PUINT64);

/**
 * Frees auth callbacks
 *
 * @param 1 PUINT64 - Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeAuthCallbacksFunc)(PUINT64);

/**
 * Frees API callbacks
 *
 * @param 1 PUINT64 - Pointer to custom handle passed by the caller.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*FreeApiCallbacksFunc)(PUINT64);

////////////////////////////////////////////////////
// Main structure declarations
////////////////////////////////////////////////////
/**
 * The Platform specific callbacks
 */
typedef struct __PlatformCallbacks PlatformCallbacks;
struct __PlatformCallbacks {
    // Version
    UINT32 version;

    // Custom data to be passed back to the caller
    UINT64 customData;

    // The callback functions
    GetCurrentTimeFunc getCurrentTimeFn;
    GetRandomNumberFunc getRandomNumberFn;
    CreateMutexFunc createMutexFn;
    LockMutexFunc lockMutexFn;
    UnlockMutexFunc unlockMutexFn;
    TryLockMutexFunc tryLockMutexFn;
    FreeMutexFunc freeMutexFn;
    CreateConditionVariableFunc createConditionVariableFn;
    SignalConditionVariableFunc signalConditionVariableFn;
    BroadcastConditionVariableFunc broadcastConditionVariableFn;
    WaitConditionVariableFunc waitConditionVariableFn;
    FreeConditionVariableFunc freeConditionVariableFn;
    LogPrintFunc logPrintFn;

    // Specialized cleanup callback
    FreePlatformCallbacksFunc freePlatformCallbacksFn;
};
typedef struct __PlatformCallbacks* PPlatformCallbacks;

/**
 * The Producer object specific callbacks
 */
typedef struct __ProducerCallbacks ProducerCallbacks;
struct __ProducerCallbacks {
    // Version
    UINT32 version;

    // Custom data to be passed back to the caller
    UINT64 customData;

    // The callback functions
    StorageOverflowPressureFunc storageOverflowPressureFn;
    ClientReadyFunc clientReadyFn;
    ClientShutdownFunc clientShutdownFn;

    // Specialized cleanup callback
    FreeProducerCallbacksFunc freeProducerCallbacksFn;
};
typedef struct __ProducerCallbacks* PProducerCallbacks;

/**
 * The Stream specific callbacks
 */
typedef struct __StreamCallbacks StreamCallbacks;
struct __StreamCallbacks {
    // Version
    UINT32 version;

    // Custom data to be passed back to the caller
    UINT64 customData;

    // The callback functions
    StreamUnderflowReportFunc streamUnderflowReportFn;
    BufferDurationOverflowPressureFunc bufferDurationOverflowPressureFn;
    StreamLatencyPressureFunc streamLatencyPressureFn;
    StreamConnectionStaleFunc streamConnectionStaleFn;
    DroppedFrameReportFunc droppedFrameReportFn;
    DroppedFragmentReportFunc droppedFragmentReportFn;
    StreamErrorReportFunc streamErrorReportFn;
    FragmentAckReceivedFunc fragmentAckReceivedFn;
    StreamDataAvailableFunc streamDataAvailableFn;
    StreamReadyFunc streamReadyFn;
    StreamClosedFunc streamClosedFn;
    StreamShutdownFunc streamShutdownFn;

    // Specialized cleanup callback
    FreeStreamCallbacksFunc freeStreamCallbacksFn;
};
typedef struct __StreamCallbacks* PStreamCallbacks;

/**
 * The Authentication specific callbacks
 */
typedef struct __AuthCallbacks AuthCallbacks;
struct __AuthCallbacks {
    // Version
    UINT32 version;

    // Custom data to be passed back to the caller
    UINT64 customData;

    // The callback functions
    GetSecurityTokenFunc getSecurityTokenFn;
    GetDeviceCertificateFunc getDeviceCertificateFn;
    DeviceCertToTokenFunc deviceCertToTokenFn;
    GetDeviceFingerprintFunc getDeviceFingerprintFn;
    GetStreamingTokenFunc getStreamingTokenFn;

    // Specialized cleanup callback
    FreeAuthCallbacksFunc freeAuthCallbacksFn;
};
typedef struct __AuthCallbacks* PAuthCallbacks;

/**
 * The KVS backend specific callbacks
 */
typedef struct __ApiCallbacks ApiCallbacks;
struct __ApiCallbacks {
    // Version
    UINT32 version;

    // Custom data to be passed back to the caller
    UINT64 customData;

    // The callback functions
    CreateStreamFunc createStreamFn;
    DescribeStreamFunc describeStreamFn;
    GetStreamingEndpointFunc getStreamingEndpointFn;
    PutStreamFunc putStreamFn;
    TagResourceFunc tagResourceFn;
    CreateDeviceFunc createDeviceFn;

    // Specialized cleanup callback
    FreeApiCallbacksFunc freeApiCallbacksFn;
};
typedef struct __ApiCallbacks* PApiCallbacks;

/**
 * AWS Credentials declaration
 */
typedef struct __AwsCredentials AwsCredentials;
struct __AwsCredentials {
    // Version
    UINT32 version;

    // Size of the entire structure in bytes including the struct itself
    UINT32 size;

    // Access Key ID - NULL terminated
    PCHAR accessKeyId;

    // Length of the access key id - not including NULL terminator
    UINT32 accessKeyIdLen;

    // Secret Key - NULL terminated
    PCHAR secretKey;

    // Length of the secret key - not including NULL terminator
    UINT32 secretKeyLen;

    // Session token - NULL terminated
    PCHAR sessionToken;

    // Length of the session token - not including NULL terminator
    UINT32 sessionTokenLen;

    // Expiration in absolute time in 100ns.
    UINT64 expiration;

    // The rest of the data might follow the structure
};
typedef struct __AwsCredentials* PAwsCredentials;

////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////

/**
 * Creates a default callbacks provider based on static AWS credentials
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param - PCHAR - IN - Access Key Id
 * @param - PCHAR - IN - Secret Key
 * @param - PCHAR - IN/OPT - Session Token
 * @param - UINT64 - IN - Expiration of the token. MAX_UINT64 if non-expiring
 * @param - PCHAR - IN/OPT - AWS region
 * @param - PCHAR - IN/OPT - CA Cert Path
 * @param - PCHAR - IN/OPT - User agent name (Use NULL)
 * @param - PCHAR - IN/IOT - Custom user agent to be used in the API calls
 * @param - BOOL - IN - Whether to create caching endpoint only callback provider
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithAwsCredentials(PCHAR, PCHAR, PCHAR, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, BOOL, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses iot certificate as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param - PCHAR - IN - IoT endpoint to use for the auth
 * @param - PCHAR - IN - Credential cert path
 * @param - PCHAR - IN - Private key path
 * @param - PCHAR - IN/OPT - CA Cert path
 * @param - PCHAR - IN - Role alias name
 * @param - PCHAR - IN - IoT Thing name
 * @param - PCHAR - IN/OPT - AWS region
 * @param - PCHAR - IN/OPT - User agent name (Use NULL)
 * @param - PCHAR - IN/OPT - Custom user agent to be used in the API calls
 * @param - BOOL - IN - Whether to create caching endpoint only callback provider
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithIotCertificate(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, BOOL, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses file-based certificate as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param - PCHAR - IN - Credential file path
 * @param - PCHAR - IN/OPT - AWS region
 * @param - PCHAR - IN/OPT - CA Cert path
 * @param - PCHAR - IN/OPT - User agent name (Use NULL)
 * @param - PCHAR - IN/OPT - Custom user agent to be used in the API calls
 * @param - BOOL - IN - Whether to create caching endpoint only callback provider
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithFileAuth(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, BOOL, PClientCallbacks*);

/**
 * Creates a default callbacks provider that uses auth callbacks as auth method.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding {@link freeCallbackProvider} API.
 *
 * @param - PAuthCallbacks - IN - Auth Callback for the auth
 * @param - PCHAR - IN/OPT - AWS region
 * @param - PCHAR - IN/OPT - CA Cert path
 * @param - PCHAR - IN/OPT - User agent name (Use NULL)
 * @param - PCHAR - IN/OPT - Custom user agent to be used in the API calls
 * @param - BOOL - IN - Whether to create caching endpoint only callback provider
 * @param - BOOL - IN - Whether to create continuous retry callback provider
 * @param - UINT64 - IN - Cached endpoint update period
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createDefaultCallbacksProviderWithAuthCallbacks(PAuthCallbacks, PCHAR, PCHAR, PCHAR, PCHAR, BOOL, BOOL, UINT64, PClientCallbacks*);

/**
 * Releases and frees the callbacks provider structure.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PClientCallbacks* - IN/OUT - Pointer to callbacks provider to free
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS freeCallbacksProvider(PClientCallbacks*);

/**
 * Sets the Platform callbacks.
 *
 * NOTE: The existing Platform callbacks are overwritten.
 *
 * @param - PClientCallbacks - IN - Pointer to client callbacks
 * @param - @PPlatformCallbacks - IN - Pointer to platform callbacks to set
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS setPlatformCallbacks(PClientCallbacks, PPlatformCallbacks);

/**
 * Appends Producer callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param - PClientCallbacks - IN - Pointer to client callbacks
 * @param - PProducerCallbacks - IN - Pointer to producer callbacks to add
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS addProducerCallbacks(PClientCallbacks, PProducerCallbacks);

/**
 * Appends Stream callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param - PClientCallbacks - IN - Pointer to client callbacks
 * @PStreamCallbacks - IN - Pointer to stream callbacks to use
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS addStreamCallbacks(PClientCallbacks, PStreamCallbacks);

/**
 * Appends Auth callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param - PClientCallbacks - IN - Pointer to client callbacks
 * @PAuthCallbacks - IN - Pointer to auth callbacks to use
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS addAuthCallbacks(PClientCallbacks, PAuthCallbacks);

/**
 * Appends Api callbacks
 *
 * NOTE: The callbacks are appended at the end of the chain.
 *
 * @param - PClientCallbacks - IN - Pointer to client callbacks
 * @PApiCallbacks - IN - Pointer to api callbacks to use
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS addApiCallbacks(PClientCallbacks, PApiCallbacks);

/**
 * Creates Stream Info for RealTime Streaming Scenario using default values.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCHAR - IN/OPT - stream name
 * @param - UINT64 - IN - retention in 100ns time unit
 * @param - UINT64 - IN - buffer duration in 100ns time unit
 * @param - PStreamInfo* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createRealtimeVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for Offline Video Streaming Scenario.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCHAR - IN/OPT - stream name
 * @param - UINT64 - IN - retention in 100ns time unit. Should be greater than 0.
 * @param - UINT64 - IN - buffer duration in 100ns time unit
 * @param - PStreamInfo* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
*/
PUBLIC_API STATUS createOfflineVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for RealTime Audio Video Streaming Scenario
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCHAR - IN/OPT - stream name
 * @param - UINT64 - IN - retention in 100ns time unit
 * @param - UINT64 - IN - buffer duration in 100ns time unit
 * @param - PStreamInfo* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createRealtimeAudioVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Creates Stream Info for Offline Audio Video Streaming Scenario
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCHAR - IN/OPT - stream name
 * @param - UINT64 - IN - retention in 100ns time unit
 * @param - UINT64 - IN - buffer duration in 100ns time unit
 * @param - PStreamInfo* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
*/
PUBLIC_API STATUS createOfflineAudioVideoStreamInfoProvider(PCHAR, UINT64, UINT64, PStreamInfo*);

/*
 * Frees the StreamInfo provider object.
 *
 * @param - PStreamInfo* - IN/OUT - StreamInfo provider object to be freed
 */
PUBLIC_API STATUS freeStreamInfoProvider(PStreamInfo*);

/**
 * Create deviceInfo with DEFAULT_STORAGE_SIZE amount of storage.
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PDeviceInfo* - OUT - pointer to PDeviceInfo that will point to the created deviceInfo
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS createDefaultDeviceInfo(PDeviceInfo*);

/**
 * Frees the DeviceInfo provider object.
 *
 * @param - PDeviceInfo* - IN/OUT - pointer to PDeviceInfo provider that will be freed
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS freeDeviceInfo(PDeviceInfo*);

/**
 * Can be called after createDefaultDeviceInfo, set the storage size to storageSize.
 *
 * @param - PDeviceInfo - IN - pointer to the target object
 * @param - UINT64 - IN - Buffer storage size in bytes
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS setDeviceInfoStorageSize(PDeviceInfo, UINT64);

/**
 *  Can be called after createDefaultDeviceInfo, calculate the storage size based on averageBitRate and bufferDuration,
 *  the call initDeviceInfoStorageInMemoryFixedSize to set storage size.
 *
 * @param - PDeviceInfo - IN - pointer to the target object
 * @param - UINT64 - IN - bitrate in bits per second
 * @param - UINT64 - IN - buffer duration in 100ns
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS setDeviceInfoStorageSizeBasedOnBitrateAndBufferDuration(PDeviceInfo, UINT64, UINT64);

/*
 * Creates the Iot Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCallbacksProvider - IN - Pointer to callback provider
 * @param - PCHAR - IN - iot credentials endpoint
 * @param - PCHAR - IN - kvs iot certificate file path
 * @param - PCHAR - IN - private key file path
 * @param - PCHAR - IN - CA cert path
 * @param - PCHAR - IN - iot role alias
 * @param - PCHAR - IN - IoT thing name
 *
 * @param - PAuthCallbacks* - OUT - Pointer to pointer to AuthCallback struct
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS createIotAuthCallbacks(PClientCallbacks, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PAuthCallbacks*);

/*
 * Frees the Iot Credential auth callbacks
 *
 * @param - PAuthCallbacks* - IN/OUT - pointer to AuthCallback provider object
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS freeIotAuthCallbacks(PAuthCallbacks*);

/*
 * Creates the File Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCallbacksProvider - IN - Pointer to callback provider
 * @param - PCHAR - IN - file path for the credentials file
 * @param - PAuthCallbacks* - OUT - Pointer to pointer to AuthCallback struct
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS createFileAuthCallbacks(PClientCallbacks, PCHAR, PAuthCallbacks*);

/*
 * Frees the File Credential auth callbacks
 *
 * @param - PAuthCallbacks* - IN/OUT - pointer to AuthCallback provider object
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS freeFileAuthCallbacks(PAuthCallbacks*);

/*
 * Creates a Static Credentials auth callbacks
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PCallbacksProvider - IN - Pointer to callback provider
 * @param - PCHAR - IN - Access key id
 * @param - PCHAR - IN - Secret key
 * @param - PCHAR - IN/OPT - Session token
 * @param - UINT64 - IN - Expiration absolute time in 100ns
 * @param - PAuthCallbacks* - OUT - Pointer to pointer to AuthCallback struct
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS createStaticAuthCallbacks(PClientCallbacks, PCHAR, PCHAR, PCHAR, UINT64, PAuthCallbacks*);

/*
 * Frees the Static Credential auth callback
 *
 * @param - PAuthCallbacks* - IN/OUT - pointer to AuthCallback provider object
 *
 * @return - STATUS - status of operation
 */
PUBLIC_API STATUS freeStaticAuthCallbacks(PAuthCallbacks*);

/**
 * Creates StreamCallbacks struct
 *
 * NOTE: The caller is responsible for releasing the structure by calling
 * the corresponding free API.
 *
 * @param - PStreamCallbacks* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
*/
PUBLIC_API STATUS createStreamCallbacks(PStreamCallbacks*);

/**
 * Frees StreamCallbacks struct
 *
 * @param - PStreamCallbacks* - IN/OUT - the object to be freed
 *
 * @return - STATUS code of the execution
*/
PUBLIC_API STATUS freeStreamCallbacks(PStreamCallbacks*);

/**
 * Creates an AWS credentials object
 *
 * @param - PCHAR - IN - Access Key Id
 * @param - UINT32 - IN - Access Key Id Length excluding NULL terminator or 0 to calculate
 * @param - PCHAR - IN - Secret Key
 * @param - UINT32 - IN - Secret Key Length excluding NULL terminator or 0 to calculate
 * @param - PCHAR - IN/OPT - Session Token
 * @param - UINT32 - IN/OPT - Session Token Length excluding NULL terminator or 0 to calculate
 * @param - UINT64 - IN - Expiration in 100ns absolute time
 * @param - PAwsCredentials* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createAwsCredentials(PCHAR, UINT32, PCHAR, UINT32, PCHAR, UINT32, UINT64, PAwsCredentials*);

/**
 * Frees an Aws credentials object
 *
 * @param - PAwsCredentials* - IN/OUT/OPT - Object to be destroyed.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS freeAwsCredentials(PAwsCredentials*);

/**
 * Creates a stream callbacks object allowing continuous retry on failures,
 * reset logic on staleness and latency.
 *
 * https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/producer-reference-callbacks.html
 *
 * @param - PCallbacksProvider - IN - Pointer to callback provider
 * @param - PStreamCallbacks* - OUT - Constructed object
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createContinuousRetryStreamCallbacks(PClientCallbacks, PStreamCallbacks*);

/**
 * Frees a previously constructed continuous stream callbacks object
 *
 * @param - PStreamCallbacks* - IN/OUT/OPT - Object to be destroyed.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS freeContinuousRetryStreamCallbacks(PStreamCallbacks*);

/**
 * Create abstract callback provider that can hook with other callbacks
 *
 * @param - UINT32 - IN - Length of callback provider calling chain
 * @param - BOOL - IN - Whether to create caching endpoint only callback provider
 * @param - UINT64 - IN - Cached endpoint update period
 * @param - PCHAR - IN/OPT - AWS region
 * @param - PCHAR - IN - Specific Control Plane Uri as endpoint to be called
 * @param - PCHAR - IN/OPT - CA Cert path
 * @param - PCHAR - IN/OPT - User agent name (Use NULL)
 * @param - PCHAR - IN/OPT - Custom user agent to be used in the API calls
 * @param - PClientCallbacks* - OUT - Returned pointer to callbacks provider
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createAbstractDefaultCallbacksProvider(UINT32, BOOL, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, PCHAR, PClientCallbacks*);

/**
 * Use file logger instead of default logger which log to stdout. The underlying objects are automatically freed
 * when PClientCallbacks is freed.
 *
 * @param - PClientCallbacks - IN - The callback provider whose logPrintFn will be replaced with file logger log printing function
 * @param - UINT64 - IN - Size of string buffer in file logger. When the string buffer is full the logger will flush everything into a new file
 * @param - UINT64 - IN - Max number of log file. When exceeded, the oldest file will be deleted when new one is generated
 * @param - PCHAR - IN - Directory in which the log file will be generated
 * @param - BOOL - IN - print log to std out too
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS addFileLoggerPlatformCallbacksProvider(PClientCallbacks, UINT64, UINT64, PCHAR, BOOL);

#pragma pack(pop, include)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_PRODUCER_INCLUDE__ */
