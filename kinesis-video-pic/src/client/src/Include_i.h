/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_CLIENT_INCLUDE_I__
#define __KINESIS_VIDEO_CLIENT_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include "com/amazonaws/kinesis/video/client/Include.h"

#define MAX_PIC_REENTRANCY_COUNT (1024 * 1024)

/**
 * Forward declarations
 */
typedef struct __KinesisVideoStream KinesisVideoStream;
typedef struct __KinesisVideoStream* PKinesisVideoStream;

/**
 * Kinesis Video client base internal structure
 */
typedef struct __KinesisVideoBase KinesisVideoBase;
struct __KinesisVideoBase {
    // Identifier of the structure
    UINT32 identifier;

    // Version of the structure
    UINT32 version;

    // ARN - 1 more for the null terminator
    CHAR arn[MAX_ARN_LEN + 1];

    // The state machine
    PStateMachine pStateMachine;

    // Service call context
    ServiceCallContext serviceCallContext;

    // Current service call result
    SERVICE_CALL_RESULT result;

    // Sync mutex for fine grained interlocking the calls
    MUTEX lock;

    // Lock needed to create/free a stream + iterating over a stream
    MUTEX streamListLock;

    // Conditional variable for Ready state
    CVAR ready;

    // Indicating whether shutdown has been triggered
    BOOL shutdown;

    // Shutdown sequencing semaphore
    SEMAPHORE_HANDLE shutdownSemaphore;
};

typedef struct __KinesisVideoBase* PKinesisVideoBase;

/**
 * Token return definition
 */
typedef struct __SecurityTokenInfo SecurityTokenInfo;
struct __SecurityTokenInfo {
    UINT32 size;
    PBYTE token;
};
typedef struct __SecurityTokenInfo* PSecurityTokenInfo;

/**
 * Endpoint return definition
 */
typedef struct __EndpointInfo EndpointInfo;
struct __EndpointInfo {
    UINT32 length;
    PCHAR endpoint;
};
typedef struct __EndpointInfo* PEndpointInfo;

// Testability hooks functions
typedef STATUS (*KinesisVideoClientCallbackHookFunc)(UINT64);

/**
 * The rest of the internal include files
 */

#include "InputValidator.h"
#include "AckParser.h"
#include "FrameOrderCoordinator.h"
#include "Stream.h"

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

/**
 * Client structure current version
 */
#define KINESIS_VIDEO_CLIENT_CURRENT_VERSION 0

/**
 * Kinesis Video client states definitions
 */
#define CLIENT_STATE_NONE       ((UINT64) 0)
#define CLIENT_STATE_NEW        ((UINT64)(1 << 0))
#define CLIENT_STATE_AUTH       ((UINT64)(1 << 1))
#define CLIENT_STATE_PROVISION  ((UINT64)(1 << 2))
#define CLIENT_STATE_GET_TOKEN  ((UINT64)(1 << 3))
#define CLIENT_STATE_CREATE     ((UINT64)(1 << 4))
#define CLIENT_STATE_TAG_CLIENT ((UINT64)(1 << 5))
#define CLIENT_STATE_READY      ((UINT64)(1 << 6))

/**
 * Object identifier enum - these will serve as sentinel values to identify
 * an object type with the events.
 */
// Client object identifier
#define KINESIS_VIDEO_OBJECT_IDENTIFIER_CLIENT 0x12345678

// Stream identifier
#define KINESIS_VIDEO_OBJECT_IDENTIFIER_STREAM 0xabcdabcd

// Any object
#define KINESIS_VIDEO_OBJECT_IDENTIFIER_ANY 0x55ff55ff

/**
 * Macros to convert to and from handle
 */
#define TO_CLIENT_HANDLE(p)   ((CLIENT_HANDLE)(p))
#define FROM_CLIENT_HANDLE(h) (IS_VALID_CLIENT_HANDLE(h) ? (PKinesisVideoClient)(h) : NULL)

#define TO_STREAM_HANDLE(p)   (toStreamHandle(p))
#define FROM_STREAM_HANDLE(h) (fromStreamHandle(h))

#define TO_CUSTOM_DATA(p)          ((UINT64)(p))
#define STREAM_FROM_CUSTOM_DATA(h) ((PKinesisVideoStream)(h))
#define CLIENT_FROM_CUSTOM_DATA(h) ((PKinesisVideoClient)(h))

#define KINESIS_VIDEO_OBJECT_IDENTIFIER_FROM_CUSTOM_DATA(h) ((UINT32) * (PUINT32)(h))

/**
 * Default heap flags
 */
#define MEMORY_BASED_HEAP_FLAGS FLAGS_USE_AIV_HEAP
#define FILE_BASED_HEAP_FLAGS   (FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP)

/**
 * Checks whether the dropped connection can be due to host issues
 */
#define CONNECTION_DROPPED_HOST_ALIVE(r)                                                                                                             \
    ((r) == SERVICE_CALL_RESULT_NOT_SET || (r) == SERVICE_CALL_RESULT_OK || (r) == SERVICE_CALL_CLIENT_LIMIT || (r) == SERVICE_CALL_DEVICE_LIMIT ||  \
     (r) == SERVICE_CALL_STREAM_LIMIT || (r) == SERVICE_CALL_RESULT_STREAM_READ_ERROR || (r) == SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED ||   \
     (r) == SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE || (r) == SERVICE_CALL_RESULT_KMS_KEY_ACCESS_DENIED ||                                             \
     (r) == SERVICE_CALL_RESULT_KMS_KEY_DISABLED || (r) == SERVICE_CALL_RESULT_KMS_KEY_VALIDATION_ERROR ||                                           \
     (r) == SERVICE_CALL_RESULT_KMS_KEY_UNAVAILABLE || (r) == SERVICE_CALL_RESULT_KMS_KEY_INVALID_USAGE ||                                           \
     (r) == SERVICE_CALL_RESULT_KMS_KEY_INVALID_STATE || (r) == SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND ||                                             \
     (r) == SERVICE_CALL_RESULT_STREAM_DELETED || (r) == SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR ||                                                   \
     (r) == SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR || (r) == SERVICE_CALL_NOT_AUTHORIZED)

/**
 * Returns whether the service call result is a timeout
 */
#define SERVICE_CALL_RESULT_IS_A_TIMEOUT(r)                                                                                                          \
    ((r) == SERVICE_CALL_REQUEST_TIMEOUT || (r) == SERVICE_CALL_GATEWAY_TIMEOUT || (r) == SERVICE_CALL_NETWORK_READ_TIMEOUT ||                       \
     (r) == SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT)

/**
 * The Realtime put media API name for GetDataEndpoint API call
 */
#define GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME "PUT_MEDIA"

/**
 * Measuring the transfer rate after this time elapses to avoid precision issues with burst reads into the
 * networking clients transfer buffer.
 */
#define TRANSFER_RATE_MEASURING_INTERVAL_EPSILON (DOUBLE) 0.2

/**
 * Multiplier factor used to compensate for the fragmentation
 */
#define FRAME_ALLOC_FRAGMENTATION_FACTOR (DOUBLE) 1.8

/**
 * Definition that controls whether we enabled the persist ACK awaiting for the last cluster or not.
 * This needs to be changed to TRUE when the backend processing is enabled or the value should be
 * removed entirely from the WAIT_FOR_PERSISTED_ACK macro.
 */
#define AWAIT_FOR_PERSISTED_ACK TRUE

/**
 * Macro that checks whether to wait for the persistent ACK or not
 */
#define WAIT_FOR_PERSISTED_ACK(x)                                                                                                                    \
    (AWAIT_FOR_PERSISTED_ACK && (x)->streamInfo.streamCaps.fragmentAcks && (x)->streamInfo.retention != RETENTION_PERIOD_SENTINEL)

/**
 * Controls whether we enable auth info expiration randomization
 */
#define ENABLE_AUTH_INFO_EXPIRATION_RANDOMIZATION TRUE

/**
 * The threshold beyond which we won't do any auth info expiration randomization
 */
#define AUTH_INFO_EXPIRATION_RANDOMIZATION_DURATION_THRESHOLD (5 * HUNDREDS_OF_NANOS_IN_A_MINUTE)

/**
 * Max randomization value to be added
 */
#define MAX_AUTH_INFO_EXPIRATION_RANDOMIZATION (3 * HUNDREDS_OF_NANOS_IN_A_MINUTE)

/**
 * Ratio of the expiration to use for jitter
 */
#define AUTH_INFO_EXPIRATION_JITTER_RATIO 0.1L

/**
 * How often the callback is invoked to check all video streams for past max timeout
 */
#define INTERMITTENT_PRODUCER_PERIOD_DEFAULT (5000LL * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

/**
 * Initial time to delay before firing first callback for automatic intermittent producer
 */
#define INTERMITTENT_PRODUCER_TIMER_START_DELAY (3 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Time after which if no frames have been received we submit EoFR to close out the session
 */
#define INTERMITTENT_PRODUCER_MAX_TIMEOUT (20LL * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Kinesis Video client internal structure
 */
typedef struct __KinesisVideoClient {
    // Base object
    KinesisVideoBase base;

    // Device Info structure.
    DeviceInfo deviceInfo;

    // Client callbacks
    ClientCallbacks clientCallbacks;

    // Client storage
    PHeap pHeap;

    // Current number of the streams
    UINT32 streamCount;

    // The array of streams objects
    PKinesisVideoStream* streams;

    // Authentication info - Token and Cert
    AuthInfo tokenAuthInfo;
    AuthInfo certAuthInfo;

    // Sets to true when the client object is ready
    BOOL clientReady;

    // Device fingerprint storage if needed for provisioning including null terminator
    CHAR deviceFingerprint[MAX_DEVICE_FINGERPRINT_LENGTH + 1];

    // Total memory allocation tracker
    UINT64 totalAllocationSize;

    // Timer Queue/Callback func for Automatic Intermittent Producer
    TIMER_QUEUE_HANDLE timerQueueHandle;
    TimerCallbackFunc timerCallbackFunc;

    KinesisVideoClientCallbackHookFunc timerCallbackPreHookFunc;
    UINT64 hookCustomData;

    // ID for timer created to wake and check if streams have incoming data
    UINT32 timerId;

    // Stored function pointers to reset on exit
    memAlloc storedMemAlloc;
    memAlignAlloc storedMemAlignAlloc;
    memCalloc storedMemCalloc;
    memFree storedMemFree;
} KinesisVideoClient, *PKinesisVideoClient;

/**
 * Internal declarations of backwards compat device info
 */
typedef struct __DeviceInfo_V0 DeviceInfo_V0;
struct __DeviceInfo_V0 {
    // Version of the struct
    UINT32 version;

    // Device name - human readable. Null terminated.
    // Should be unique per AWS account.
    CHAR name[MAX_DEVICE_NAME_LEN + 1];

    // Number of tags associated with the device.
    UINT32 tagCount;

    // Device tags array
    PTag tags;

    // Storage configuration information
    StorageInfo storageInfo;

    // Number of declared streams.
    UINT32 streamCount;
};
typedef struct __DeviceInfo_V0* PDeviceInfo_V0;

/**
 * Internal declarations of backwards compat client info
 */
typedef struct __ClientInfo_V0 ClientInfo_V0;
struct __ClientInfo_V0 {
    // Version of the struct
    UINT32 version;

    // Client sync creation timeout. 0 or INVALID_TIMESTAMP_VALUE = use default
    UINT64 createClientTimeout;

    // Stream sync creation timeout. 0 or INVALID_TIMESTAMP_VALUE= use default
    UINT64 createStreamTimeout;

    // Stream sync stopping timeout. 0 or INVALID_TIMESTAMP_VALUE= use default
    UINT64 stopStreamTimeout;

    // Offline mode wait for buffer availability timeout. 0 or INVALID_TIMESTAMP_VALUE= use default
    UINT64 offlineBufferAvailabilityTimeout;

    // Logger log level. 0 = use default
    UINT32 loggerLogLevel;

    // whether to log metric or not
    BOOL logMetric;
};
typedef struct __ClientInfo_V0* PClientInfo_V0;

/**
 * Internal declarations of backwards compat stream description
 */
typedef struct __StreamDescription_V0 StreamDescription_V0;
struct __StreamDescription_V0 {
    // Version of the struct
    UINT32 version;

    // Device name - human readable. Null terminated.
    // Should be unique per AWS account.
    CHAR deviceName[MAX_DEVICE_NAME_LEN + 1];

    // Stream name - human readable. Null terminated.
    // Should be unique per AWS account.
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];

    // Stream content type - nul terminated.
    CHAR contentType[MAX_CONTENT_TYPE_LEN + 1];

    // Update version.
    CHAR updateVersion[MAX_UPDATE_VERSION_LEN + 1];

    // Stream ARN
    CHAR streamArn[MAX_ARN_LEN + 1];

    // Current stream status
    STREAM_STATUS streamStatus;

    // Stream creation time
    UINT64 creationTime;
};
typedef struct __StreamDescription_V0* PStreamDescription_V0;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Converts the stream to a stream handle
 */
STREAM_HANDLE toStreamHandle(PKinesisVideoStream);

/**
 * Converts handle to a stream
 */
PKinesisVideoStream fromStreamHandle(STREAM_HANDLE);

/**
 * Internal function to free the client object
 */
STATUS freeKinesisVideoClientInternal(PKinesisVideoClient, STATUS);

/**
 * Callback function which is invoked when an item gets purged from the view
 */
VOID viewItemRemoved(PContentView, UINT64, PViewItem, BOOL);

/**
 * Creates a random alpha num name
 */
VOID createRandomName(PCHAR, UINT32, GetRandomNumberFunc, UINT64);

/**
 * Gets the authentication information from the client application
 */
STATUS getAuthInfo(PKinesisVideoClient);

/**
 * Gets the current authentication expiration from the client object
 */
UINT64 getCurrentAuthExpiration(PKinesisVideoClient);

/**
 * Performs a provisioning for the device
 */
STATUS provisionKinesisVideoProducer(PKinesisVideoClient);

/**
 * Returns the current auth integration type
 */
AUTH_INFO_TYPE getCurrentAuthType(PKinesisVideoClient);

/**
 * Randomizing or adding a jitter to the auth info expiration
 */
UINT64 randomizeAuthInfoExpiration(PKinesisVideoClient, UINT64, UINT64);

/**
 * Set content store as default allocator
 */
STATUS setContentStoreAllocator(PKinesisVideoClient);

/**
 * Helper to step the client state machine
 */
STATUS stepClientStateMachine(PKinesisVideoClient);

///////////////////////////////////////////////////////////////////////////
// Client service call event functions
///////////////////////////////////////////////////////////////////////////
STATUS createDeviceResult(PKinesisVideoClient, SERVICE_CALL_RESULT, PCHAR);
STATUS deviceCertToTokenResult(PKinesisVideoClient, SERVICE_CALL_RESULT, PBYTE, UINT32, UINT64);
STATUS tagClientResult(PKinesisVideoClient, SERVICE_CALL_RESULT);

///////////////////////////////////////////////////////////////////////////
// Client state machine callback functionality
///////////////////////////////////////////////////////////////////////////
STATUS fromNewClientState(UINT64, PUINT64);
STATUS fromAuthClientState(UINT64, PUINT64);
STATUS fromGetTokenClientState(UINT64, PUINT64);
STATUS fromProvisionClientState(UINT64, PUINT64);
STATUS fromCreateClientState(UINT64, PUINT64);
STATUS fromTagClientState(UINT64, PUINT64);
STATUS fromReadyClientState(UINT64, PUINT64);

STATUS executeNewClientState(UINT64, UINT64);
STATUS executeAuthClientState(UINT64, UINT64);
STATUS executeGetTokenClientState(UINT64, UINT64);
STATUS executeProvisionClientState(UINT64, UINT64);
STATUS executeCreateClientState(UINT64, UINT64);
STATUS executeTagClientState(UINT64, UINT64);
STATUS executeReadyClientState(UINT64, UINT64);

STATUS checkIntermittentProducerCallback(UINT32, UINT64, UINT64);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_CLIENT_INCLUDE_I__ */
