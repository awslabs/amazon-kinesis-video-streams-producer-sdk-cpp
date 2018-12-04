/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_CLIENT_INCLUDE_I__
#define __KINESIS_VIDEO_CLIENT_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include "com/amazonaws/kinesis/video/client/Include.h"

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
 * Forward declarations
 */
typedef struct __KinesisVideoStream KinesisVideoStream;
typedef __KinesisVideoStream* PKinesisVideoStream;

typedef struct __StateMachine StateMachine;
typedef __StateMachine* PStateMachine;

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

    // Sync mutex
    MUTEX lock;
};

typedef __KinesisVideoBase* PKinesisVideoBase;

/**
 * The rest of the internal include files
 */
#include "InputValidator.h"
#include "State.h"
#include "AckParser.h"
#include "Stream.h"

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

/**
 * Client structure current version
 */
#define KINESIS_VIDEO_CLIENT_CURRENT_VERSION                   0

/**
 * Kinesis Video client states definitions
 */
#define CLIENT_STATE_NONE                               ((UINT64) 0)
#define CLIENT_STATE_NEW                                ((UINT64) (1 << 0))
#define CLIENT_STATE_AUTH                               ((UINT64) (1 << 1))
#define CLIENT_STATE_PROVISION                          ((UINT64) (1 << 2))
#define CLIENT_STATE_GET_TOKEN                          ((UINT64) (1 << 3))
#define CLIENT_STATE_CREATE                             ((UINT64) (1 << 4))
#define CLIENT_STATE_TAG_CLIENT                         ((UINT64) (1 << 5))
#define CLIENT_STATE_READY                              ((UINT64) (1 << 6))

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
#define TO_CLIENT_HANDLE(p) ((CLIENT_HANDLE) (p))
#define FROM_CLIENT_HANDLE(h) (IS_VALID_CLIENT_HANDLE(h) ? (PKinesisVideoClient) (h) : NULL)

#define TO_STREAM_HANDLE(p) (toStreamHandle(p))
#define FROM_STREAM_HANDLE(h) (fromStreamHandle(h))

#define TO_CUSTOM_DATA(p) ((UINT64) (p))
#define STREAM_FROM_CUSTOM_DATA(h) ((PKinesisVideoStream) (h))
#define CLIENT_FROM_CUSTOM_DATA(h) ((PKinesisVideoClient) (h))

#define KINESIS_VIDEO_OBJECT_IDENTIFIER_FROM_CUSTOM_DATA(h) ((UINT32) *(PUINT32)(h))

/**
 * Default heap flags
 */
#define MEMORY_BASED_HEAP_FLAGS     FLAGS_USE_AIV_HEAP
#define FILE_BASED_HEAP_FLAGS       (FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP)

/**
 * Defines the full tag structure length when the pointers to the strings are allocated after the
 * main struct. We will add 2 for NULL terminators
 */
#define TAG_FULL_LENGTH             (SIZEOF(Tag) + (MAX_TAG_NAME_LEN + MAX_TAG_VALUE_LEN + 2) * SIZEOF(CHAR))

/**
 * Calculates the next service call retry time
 */
#define NEXT_SERVICE_CALL_RETRY_DELAY(r)        (((UINT64)1 << (r)) * SERVICE_CALL_RETRY_TIMEOUT)

/**
 * Checks whether the dropped connection can be due to host issues
 */
#define CONNECTION_DROPPED_HOST_ALIVE(r)        ((r) == SERVICE_CALL_RESULT_NOT_SET || \
                                                (r) == SERVICE_CALL_RESULT_OK || \
                                                (r) == SERVICE_CALL_CLIENT_LIMIT || \
                                                (r) == SERVICE_CALL_DEVICE_LIMIT || \
                                                (r) == SERVICE_CALL_STREAM_LIMIT || \
                                                (r) == SERVICE_CALL_RESULT_STREAM_READ_ERROR || \
                                                (r) == SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED || \
                                                (r) == SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_ACCESS_DENIED || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_DISABLED || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_VALIDATION_ERROR || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_UNAVAILABLE || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_INVALID_USAGE || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_INVALID_STATE || \
                                                (r) == SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND || \
                                                (r) == SERVICE_CALL_RESULT_STREAM_DELETED || \
                                                (r) == SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR || \
                                                (r) == SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR || \
                                                (r) == SERVICE_CALL_NOT_AUTHORIZED)

/**
 * The Realtime put media API name for GetDataEndpoint API call
 */
#define GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME            "PUT_MEDIA"

/**
 * EMA (Exponential Moving Average) alpha value and 1-alpha value - over appx 20 samples
 */
#define EMA_ALPHA_VALUE                 ((DOUBLE) 0.05)
#define ONE_MINUS_EMA_ALPHA_VALUE       ((DOUBLE) (1 - EMA_ALPHA_VALUE))

/**
 * Calculates the EMA (Exponential Moving Average) accumulator value
 *
 * a - Accumulator value
 * v - Next sample point
 *
 * @return the new Accumulator value
 */
#define EMA_ACCUMULATOR_GET_NEXT(a, v)    (DOUBLE) (EMA_ALPHA_VALUE * (v) + ONE_MINUS_EMA_ALPHA_VALUE * (a))

/**
 * Measuring the transfer rate after this time elapses to avoid precision issues with burst reads into the
 * networking clients transfer buffer.
 */
#define TRANSFER_RATE_MEASURING_INTERVAL_EPSILON               (DOUBLE) 0.2

/*
 * Definition that controls whether we enabled the persist ACK awaiting for the last cluster or not.
 * This needs to be changed to TRUE when the backend processing is enabled or the value should be
 * removed entirely from the WAIT_FOR_PERSISTED_ACK macro.
 */
#define AWAIT_FOR_PERSISTED_ACK                 FALSE

/**
 * Macro that checks whether to wait for the persistent ACK or not
 */
#define WAIT_FOR_PERSISTED_ACK(x)               (AWAIT_FOR_PERSISTED_ACK && \
                                                    (x)->streamInfo.streamCaps.fragmentAcks && \
                                                    (x)->streamInfo.retention != RETENTION_PERIOD_SENTINEL)

/**
 * Kinesis Video client internal structure
 */
typedef struct __KinesisVideoClient KinesisVideoClient;
struct __KinesisVideoClient {
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
};

typedef __KinesisVideoClient* PKinesisVideoClient;

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
 * Default implementations of some of the callbacks if the caller hasn't specified them
 */
UINT64 defaultGetCurrentTime(UINT64);
UINT32 defaultGetRandomNumber(UINT64);
MUTEX defaultCreateMutex(UINT64, BOOL);
VOID defaultLockMutex(UINT64, MUTEX);
VOID defaultUnlockMutex(UINT64, MUTEX);
BOOL defaultTryLockMutex(UINT64, MUTEX);
VOID defaultFreeMutex(UINT64, MUTEX);
STATUS defaultStreamReady(UINT64, STREAM_HANDLE);
STATUS defaultEndOfStream(UINT64, STREAM_HANDLE, UPLOAD_HANDLE);
STATUS defaultClientReady(UINT64, CLIENT_HANDLE);
STATUS defaultStreamDataAvailable(UINT64, STREAM_HANDLE, PCHAR, UINT64, UINT64, UINT64);

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


#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_CLIENT_INCLUDE_I__ */
