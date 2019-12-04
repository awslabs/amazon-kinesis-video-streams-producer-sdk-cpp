/**
 * Main public include file
 */
#ifndef __KINESIS_VIDEO_CLIENT_INCLUDE__
#define __KINESIS_VIDEO_CLIENT_INCLUDE__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#include <com/amazonaws/kinesis/video/common/CommonDefs.h>
#include <com/amazonaws/kinesis/video/common/PlatformUtils.h>

// IMPORTANT! Some of the headers are not tightly packed!
////////////////////////////////////////////////////
// Public headers
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/mkvgen/Include.h>
#include <com/amazonaws/kinesis/video/view/Include.h>
#include <com/amazonaws/kinesis/video/heap/Include.h>
#include <com/amazonaws/kinesis/video/state/Include.h>

// For tight packing
#pragma pack(push, include, 1) // for byte alignment

/**
 * Device/Client state transitions
 *
 *              +---+
 *              |   |
 *              |   V
 *        [Provisioning]---+
 *              ^          |           +---+
 *              |          |           |   |
 *              |          v           |   V
 *  *[New]--->[Auth]--->[Create]--->[TagDevice]--->[Ready]
 *              |          ^
 *              |          |
 *              v          |
 *        [TokenFromCert]--+
 *            |   ^
 *            |   |
 *            +---+
 *
 */

/**
 * Stream state transitions
 *
 *            +---+
 *            |   |
 *            |   V
 *           [Create]----------+                                             +---------------------------------------------+
 *              ^              |                                             |                                             |
 *              |              |                                             |                                             |
 *              |              v                                             V                                             |
 *  *[New]--->[Describe]--->[TagStream]--->[GetStreamingEndpoint]--->[GetStreamingToken]--->[Ready]--->[PutStream]--->[Streaming]--->[Stopped]
 *              |    ^        |     ^         |             ^          |           ^         |   ^       |     ^        |     ^        |   ^
 *              |    |        |     |         |             |          |           |         |   |       |     |        |     |        |   |
 *              +----+--------+-----+---------+-------------+----------+-----------+---------+---+-------+-----+--------+-----+--------+---+
 *
 */

////////////////////////////////////////////////////
// Status return codes
////////////////////////////////////////////////////
#define STATUS_CLIENT_BASE                                                          0x52000000
#define STATUS_MAX_STREAM_COUNT                                                     STATUS_CLIENT_BASE + 0x00000001
#define STATUS_MIN_STREAM_COUNT                                                     STATUS_CLIENT_BASE + 0x00000002
#define STATUS_INVALID_DEVICE_NAME_LENGTH                                           STATUS_CLIENT_BASE + 0x00000003
#define STATUS_INVALID_DEVICE_INFO_VERSION                                          STATUS_CLIENT_BASE + 0x00000004
#define STATUS_MAX_TAG_COUNT                                                        STATUS_CLIENT_BASE + 0x00000005
#define STATUS_DEVICE_FINGERPRINT_LENGTH                                            STATUS_CLIENT_BASE + 0x00000006
#define STATUS_INVALID_CALLBACKS_VERSION                                            STATUS_CLIENT_BASE + 0x00000007
#define STATUS_INVALID_STREAM_INFO_VERSION                                          STATUS_CLIENT_BASE + 0x00000008
#define STATUS_INVALID_STREAM_NAME_LENGTH                                           STATUS_CLIENT_BASE + 0x00000009
#define STATUS_INVALID_STORAGE_SIZE                                                 STATUS_CLIENT_BASE + 0x0000000a
#define STATUS_INVALID_ROOT_DIRECTORY_LENGTH                                        STATUS_CLIENT_BASE + 0x0000000b
#define STATUS_INVALID_SPILL_RATIO                                                  STATUS_CLIENT_BASE + 0x0000000c
#define STATUS_INVALID_STORAGE_INFO_VERSION                                         STATUS_CLIENT_BASE + 0x0000000d
#define STATUS_SERVICE_CALL_CALLBACKS_MISSING                                       STATUS_CLIENT_BASE + 0x0000000f
#define STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR                                    STATUS_CLIENT_BASE + 0x00000010
#define STATUS_DESCRIBE_STREAM_CALL_FAILED                                          STATUS_CLIENT_BASE + 0x00000011
#define STATUS_INVALID_DESCRIBE_STREAM_RESPONSE                                     STATUS_CLIENT_BASE + 0x00000012
#define STATUS_STREAM_IS_BEING_DELETED_ERROR                                        STATUS_CLIENT_BASE + 0x00000013
#define STATUS_SERVICE_CALL_INVALID_ARG_ERROR                                       STATUS_CLIENT_BASE + 0x00000014
#define STATUS_SERVICE_CALL_DEVICE_NOT_FOND_ERROR                                   STATUS_CLIENT_BASE + 0x00000015
#define STATUS_SERVICE_CALL_DEVICE_NOT_PROVISIONED_ERROR                            STATUS_CLIENT_BASE + 0x00000016
#define STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR                                STATUS_CLIENT_BASE + 0x00000017
#define STATUS_INVALID_AUTH_LEN                                                     STATUS_CLIENT_BASE + 0x00000018
#define STATUS_CREATE_STREAM_CALL_FAILED                                            STATUS_CLIENT_BASE + 0x00000019
#define STATUS_GET_STREAMING_TOKEN_CALL_FAILED                                      STATUS_CLIENT_BASE + 0x0000002a
#define STATUS_GET_STREAMING_ENDPOINT_CALL_FAILED                                   STATUS_CLIENT_BASE + 0x0000002b
#define STATUS_INVALID_URI_LEN                                                      STATUS_CLIENT_BASE + 0x0000002c
#define STATUS_PUT_STREAM_CALL_FAILED                                               STATUS_CLIENT_BASE + 0x0000002d
#define STATUS_STORE_OUT_OF_MEMORY                                                  STATUS_CLIENT_BASE + 0x0000002e
#define STATUS_NO_MORE_DATA_AVAILABLE                                               STATUS_CLIENT_BASE + 0x0000002f
#define STATUS_INVALID_TAG_VERSION                                                  STATUS_CLIENT_BASE + 0x00000030
#define STATUS_SERVICE_CALL_UNKOWN_ERROR                                            STATUS_CLIENT_BASE + 0x00000031
#define STATUS_SERVICE_CALL_RESOURCE_IN_USE_ERROR                                   STATUS_CLIENT_BASE + 0x00000032
#define STATUS_SERVICE_CALL_CLIENT_LIMIT_ERROR                                      STATUS_CLIENT_BASE + 0x00000033
#define STATUS_SERVICE_CALL_DEVICE_LIMIT_ERROR                                      STATUS_CLIENT_BASE + 0x00000034
#define STATUS_SERVICE_CALL_STREAM_LIMIT_ERROR                                      STATUS_CLIENT_BASE + 0x00000035
#define STATUS_SERVICE_CALL_RESOURCE_DELETED_ERROR                                  STATUS_CLIENT_BASE + 0x00000036
#define STATUS_SERVICE_CALL_TIMEOUT_ERROR                                           STATUS_CLIENT_BASE + 0x00000037
#define STATUS_STREAM_READY_CALLBACK_FAILED                                         STATUS_CLIENT_BASE + 0x00000038
#define STATUS_DEVICE_TAGS_COUNT_NON_ZERO_TAGS_NULL                                 STATUS_CLIENT_BASE + 0x00000039
#define STATUS_INVALID_STREAM_DESCRIPTION_VERSION                                   STATUS_CLIENT_BASE + 0x0000003a
#define STATUS_INVALID_TAG_NAME_LEN                                                 STATUS_CLIENT_BASE + 0x0000003b
#define STATUS_INVALID_TAG_VALUE_LEN                                                STATUS_CLIENT_BASE + 0x0000003c
#define STATUS_TAG_STREAM_CALL_FAILED                                               STATUS_CLIENT_BASE + 0x0000003d
#define STATUS_INVALID_CUSTOM_DATA                                                  STATUS_CLIENT_BASE + 0x0000003e
#define STATUS_INVALID_CREATE_STREAM_RESPONSE                                       STATUS_CLIENT_BASE + 0x0000003f
#define STATUS_CLIENT_AUTH_CALL_FAILED                                              STATUS_CLIENT_BASE + 0x00000040
#define STATUS_GET_CLIENT_TOKEN_CALL_FAILED                                         STATUS_CLIENT_BASE + 0x00000041
#define STATUS_CLIENT_PROVISION_CALL_FAILED                                         STATUS_CLIENT_BASE + 0x00000042
#define STATUS_CREATE_CLIENT_CALL_FAILED                                            STATUS_CLIENT_BASE + 0x00000043
#define STATUS_CLIENT_READY_CALLBACK_FAILED                                         STATUS_CLIENT_BASE + 0x00000044
#define STATUS_TAG_CLIENT_CALL_FAILED                                               STATUS_CLIENT_BASE + 0x00000045
#define STATUS_INVALID_CREATE_DEVICE_RESPONSE                                       STATUS_CLIENT_BASE + 0x00000046
#define STATUS_ACK_TIMESTAMP_NOT_IN_VIEW_WINDOW                                     STATUS_CLIENT_BASE + 0x00000047
#define STATUS_INVALID_FRAGMENT_ACK_VERSION                                         STATUS_CLIENT_BASE + 0x00000048
#define STATUS_INVALID_TOKEN_EXPIRATION                                             STATUS_CLIENT_BASE + 0x00000049
#define STATUS_END_OF_STREAM                                                        STATUS_CLIENT_BASE + 0x0000004a
#define STATUS_DUPLICATE_STREAM_NAME                                                STATUS_CLIENT_BASE + 0x0000004b
#define STATUS_INVALID_RETENTION_PERIOD                                             STATUS_CLIENT_BASE + 0x0000004c
#define STATUS_INVALID_ACK_KEY_START                                                STATUS_CLIENT_BASE + 0x0000004d
#define STATUS_INVALID_ACK_DUPLICATE_KEY_NAME                                       STATUS_CLIENT_BASE + 0x0000004e
#define STATUS_INVALID_ACK_INVALID_VALUE_START                                      STATUS_CLIENT_BASE + 0x0000004f
#define STATUS_INVALID_ACK_INVALID_VALUE_END                                        STATUS_CLIENT_BASE + 0x00000050
#define STATUS_INVALID_PARSED_ACK_TYPE                                              STATUS_CLIENT_BASE + 0x00000051
#define STATUS_STREAM_HAS_BEEN_STOPPED                                              STATUS_CLIENT_BASE + 0x00000052
#define STATUS_INVALID_STREAM_METRICS_VERSION                                       STATUS_CLIENT_BASE + 0x00000053
#define STATUS_INVALID_CLIENT_METRICS_VERSION                                       STATUS_CLIENT_BASE + 0x00000054
#define STATUS_INVALID_CLIENT_READY_STATE                                           STATUS_CLIENT_BASE + 0x00000055
#define STATUS_INVALID_FRAGMENT_ACK_TYPE                                            STATUS_CLIENT_BASE + 0x00000057
#define STATUS_INVALID_STREAM_READY_STATE                                           STATUS_CLIENT_BASE + 0x00000058
#define STATUS_CLIENT_FREED_BEFORE_STREAM                                           STATUS_CLIENT_BASE + 0x00000059
#define STATUS_ALLOCATION_SIZE_SMALLER_THAN_REQUESTED                               STATUS_CLIENT_BASE + 0x0000005a
#define STATUS_VIEW_ITEM_SIZE_GREATER_THAN_ALLOCATION                               STATUS_CLIENT_BASE + 0x0000005b
#define STATUS_ACK_ERR_STREAM_READ_ERROR                                            STATUS_CLIENT_BASE + 0x0000005c
#define STATUS_ACK_ERR_FRAGMENT_SIZE_REACHED                                        STATUS_CLIENT_BASE + 0x0000005d
#define STATUS_ACK_ERR_FRAGMENT_DURATION_REACHED                                    STATUS_CLIENT_BASE + 0x0000005e
#define STATUS_ACK_ERR_CONNECTION_DURATION_REACHED                                  STATUS_CLIENT_BASE + 0x0000005f
#define STATUS_ACK_ERR_FRAGMENT_TIMECODE_NOT_MONOTONIC                              STATUS_CLIENT_BASE + 0x00000060
#define STATUS_ACK_ERR_MULTI_TRACK_MKV                                              STATUS_CLIENT_BASE + 0x00000061
#define STATUS_ACK_ERR_INVALID_MKV_DATA                                             STATUS_CLIENT_BASE + 0x00000062
#define STATUS_ACK_ERR_INVALID_PRODUCER_TIMESTAMP                                   STATUS_CLIENT_BASE + 0x00000063
#define STATUS_ACK_ERR_STREAM_NOT_ACTIVE                                            STATUS_CLIENT_BASE + 0x00000064
#define STATUS_ACK_ERR_KMS_KEY_ACCESS_DENIED                                        STATUS_CLIENT_BASE + 0x00000065
#define STATUS_ACK_ERR_KMS_KEY_DISABLED                                             STATUS_CLIENT_BASE + 0x00000066
#define STATUS_ACK_ERR_KMS_KEY_VALIDATION_ERROR                                     STATUS_CLIENT_BASE + 0x00000067
#define STATUS_ACK_ERR_KMS_KEY_UNAVAILABLE                                          STATUS_CLIENT_BASE + 0x00000068
#define STATUS_ACK_ERR_KMS_KEY_INVALID_USAGE                                        STATUS_CLIENT_BASE + 0x00000069
#define STATUS_ACK_ERR_KMS_KEY_INVALID_STATE                                        STATUS_CLIENT_BASE + 0x0000006a
#define STATUS_ACK_ERR_KMS_KEY_NOT_FOUND                                            STATUS_CLIENT_BASE + 0x0000006b
#define STATUS_ACK_ERR_STREAM_DELETED                                               STATUS_CLIENT_BASE + 0x0000006c
#define STATUS_ACK_ERR_ACK_INTERNAL_ERROR                                           STATUS_CLIENT_BASE + 0x0000006d
#define STATUS_ACK_ERR_FRAGMENT_ARCHIVAL_ERROR                                      STATUS_CLIENT_BASE + 0x0000006e
#define STATUS_ACK_ERR_UNKNOWN_ACK_ERROR                                            STATUS_CLIENT_BASE + 0x0000006f
#define STATUS_MISSING_ERR_ACK_ID                                                   STATUS_CLIENT_BASE + 0x00000070
#define STATUS_INVALID_ACK_SEGMENT_LEN                                              STATUS_CLIENT_BASE + 0x00000071
#define STATUS_AWAITING_PERSISTED_ACK                                               STATUS_CLIENT_BASE + 0x00000072
#define STATUS_PERSISTED_ACK_TIMEOUT                                                STATUS_CLIENT_BASE + 0x00000073
#define STATUS_MAX_FRAGMENT_METADATA_COUNT                                          STATUS_CLIENT_BASE + 0x00000074
#define STATUS_ACK_ERR_FRAGMENT_METADATA_LIMIT_REACHED                              STATUS_CLIENT_BASE + 0x00000075
#define STATUS_BLOCKING_PUT_INTERRUPTED_STREAM_TERMINATED                           STATUS_CLIENT_BASE + 0x00000076
#define STATUS_INVALID_METADATA_NAME                                                STATUS_CLIENT_BASE + 0x00000077
#define STATUS_END_OF_FRAGMENT_FRAME_INVALID_STATE                                  STATUS_CLIENT_BASE + 0x00000078
#define STATUS_TRACK_INFO_MISSING                                                   STATUS_CLIENT_BASE + 0x00000079
#define STATUS_MAX_TRACK_COUNT_EXCEEDED                                             STATUS_CLIENT_BASE + 0x0000007a
#define STATUS_OFFLINE_MODE_WITH_ZERO_RETENTION                                     STATUS_CLIENT_BASE + 0x0000007b
#define STATUS_ACK_ERR_TRACK_NUMBER_MISMATCH                                        STATUS_CLIENT_BASE + 0x0000007c
#define STATUS_ACK_ERR_FRAMES_MISSING_FOR_TRACK                                     STATUS_CLIENT_BASE + 0x0000007d
#define STATUS_ACK_ERR_MORE_THAN_ALLOWED_TRACKS_FOUND                               STATUS_CLIENT_BASE + 0x0000007e
#define STATUS_UPLOAD_HANDLE_ABORTED                                                STATUS_CLIENT_BASE + 0x0000007f
#define STATUS_INVALID_CERT_PATH_LENGTH                                             STATUS_CLIENT_BASE + 0x00000080
#define STATUS_DUPLICATE_TRACK_ID_FOUND                                             STATUS_CLIENT_BASE + 0x00000081
#define STATUS_INVALID_CLIENT_INFO_VERSION                                          STATUS_CLIENT_BASE + 0x00000082
#define STATUS_INVALID_CLIENT_ID_STRING_LENGTH                                      STATUS_CLIENT_BASE + 0x00000083
#define STATUS_SETTING_KEY_FRAME_FLAG_WHILE_USING_EOFR                              STATUS_CLIENT_BASE + 0x00000084
#define STATUS_MAX_FRAME_TIMESTAMP_DELTA_BETWEEN_TRACKS_EXCEEDED                    STATUS_CLIENT_BASE + 0x00000085
#define STATUS_STREAM_SHUTTING_DOWN                                                 STATUS_CLIENT_BASE + 0x00000086
#define STATUS_CLIENT_SHUTTING_DOWN                                                 STATUS_CLIENT_BASE + 0x00000087

#define IS_RECOVERABLE_ERROR(error)     ((error) == STATUS_ACK_ERR_INVALID_MKV_DATA ||          \
                                        (error) == STATUS_ACK_ERR_FRAGMENT_ARCHIVAL_ERROR ||    \
                                        (error) == STATUS_INVALID_ACK_KEY_START ||              \
                                        (error) == STATUS_INVALID_ACK_DUPLICATE_KEY_NAME ||     \
                                        (error) == STATUS_INVALID_ACK_INVALID_VALUE_START ||    \
                                        (error) == STATUS_INVALID_ACK_INVALID_VALUE_END ||      \
                                        (error) == STATUS_ACK_TIMESTAMP_NOT_IN_VIEW_WINDOW ||   \
                                        (error) == STATUS_ACK_ERR_FRAGMENT_DURATION_REACHED)    \

#define IS_RETRIABLE_ERROR(error)       ((error) == STATUS_DESCRIBE_STREAM_CALL_FAILED ||       \
                                        (error) == STATUS_CREATE_STREAM_CALL_FAILED ||          \
                                        (error) == STATUS_GET_STREAMING_TOKEN_CALL_FAILED ||    \
                                        (error) == STATUS_PUT_STREAM_CALL_FAILED ||             \
                                        (error) == STATUS_GET_STREAMING_ENDPOINT_CALL_FAILED || \
                                        (error) == STATUS_INVALID_TOKEN_EXPIRATION)             \

////////////////////////////////////////////////////
// Main defines
////////////////////////////////////////////////////
/**
 * Max device name length in chars
 */
#define MAX_DEVICE_NAME_LEN                      128

/**
 * Max stream count for sanity validation - 1M
 */
#define MAX_STREAM_COUNT                         1000000

/**
 * Max stream name length chars
 */
#define MAX_STREAM_NAME_LEN                      256

/**
 * Max update version length in chars
 */
#define MAX_UPDATE_VERSION_LEN                   128

/**
 * Max ARN len in chars
 */
#define MAX_ARN_LEN                              2048

/**
 * Max len of the auth data (STS or Cert) in bytes
 */
#define MAX_AUTH_LEN                             10000

/**
 * Max len of the fully qualified URI
 */
#define MAX_URI_CHAR_LEN                         10000

/**
 * Minimal storage allocation size = MIN heap size
 */
#define MIN_STORAGE_ALLOCATION_SIZE              MIN_HEAP_SIZE

/**
 * Max storage allocation size = 10GB
 */
#define MAX_STORAGE_ALLOCATION_SIZE              (10LLU * 1024 * 1024 * 1024)

/**
 * Max number of fragment metadatas in the segment
 */
#define MAX_SEGMENT_METADATA_COUNT               1024

/**
 * Minimal valid retention period
 */
#define MIN_RETENTION_PERIOD                     (1 * HUNDREDS_OF_NANOS_IN_AN_HOUR)

/**
 * Maximal size of the metadata queue for a fragment
 */
#define MAX_FRAGMENT_METADATA_COUNT              10

/**
 * Max length of the fragment sequence number
 */
#define MAX_FRAGMENT_SEQUENCE_NUMBER             128

/**
 * Max length of an entire ACK fragment which is longer than MAX_FRAGMENT_SEQUENCE_NUMBER
 */
#define MAX_ACK_FRAGMENT_LEN                     1024

/**
 * Min streaming token expiration duration. Currently defined as 30 seconds.
 */
#define MIN_STREAMING_TOKEN_EXPIRATION_DURATION  (30 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * The max streaming token expiration duration after which the ingestion host will force terminate the connection.
 */
#define MAX_ENFORCED_TOKEN_EXPIRATION_DURATION      (40 * HUNDREDS_OF_NANOS_IN_A_MINUTE)

/**
 * Grace period for the streaming token expiration - 3 seconds
 */
#define STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD     (3 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Default frame rate - NTSC standard
 */
#define DEFAULT_FRAME_RATE                       24

/**
 * Buffer duration in seconds for streaming
 */
#define MIN_BUFFER_DURATION_IN_SECONDS           20

/**
 * Minimal temporal buffer to keep in the view for the streaming
 */
#define MIN_VIEW_BUFFER_DURATION     (MAX(MIN_BUFFER_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, MIN_CONTENT_VIEW_BUFFER_DURATION))

/**
 * Service call default timeout - 5 seconds
 */
#define SERVICE_CALL_DEFAULT_TIMEOUT            (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Service call infinite timeout for streaming
 */
#define SERVICE_CALL_INFINITE_TIMEOUT           MAX_UINT64

/**
 * Default service call retry count
 */
#define SERVICE_CALL_MAX_RETRY_COUNT           5

/**
 * MKV packaging type string
 */
#define MKV_CONTAINER_TYPE_STRING               "video/x-matroska"

/**
 * Remaining storage pressure notification threshold in percents.
 * The client application will be notified if the storage availability
 * falls below this percentage.
 */
#define STORAGE_PRESSURE_NOTIFICATION_THRESHOLD     5

/**
 * Remaining buffer duration pressure notification threshold in percents.
 * The client application will be notified if the buffer duration availability
 * falls below this percentage.
 */
#define BUFFER_DURATION_PRESSURE_NOTIFICATION_THRESHOLD     5

/**
 * Default device name string length
 */
#define DEFAULT_DEVICE_NAME_LEN                     16

/**
 * Default stream name string length
 */
#define DEFAULT_STREAM_NAME_LEN                     16

/**
 * Max device fingerprint length
 */
#define MAX_DEVICE_FINGERPRINT_LENGTH               32

/**
 * Max client id string length
 */
#define MAX_CLIENT_ID_STRING_LENGTH                 64

/**
 * Default timecode scale sentinel value
 * The actual value of the timecode scale might be
 * different for different packaging types
 */
#define DEFAULT_TIMECODE_SCALE_SENTINEL             0

/**
 * Stream latency pressure check sentinel value.
 * If this value is specified for the stream caps
 * max latency then the check will be skipped.
 */
#define STREAM_LATENCY_PRESSURE_CHECK_SENTINEL      0

/**
 * Fragment duration sentinel value in case of I-frame fragmentation.
 */
#define FRAGMENT_KEY_FRAME_DURATION_SENTINEL                0

/**
 * Sentinel value for the duration when checking the connection
 * staleness is not required.
 */
#define CONNECTION_STALENESS_DETECTION_SENTINEL             0

/**
 * Retention period sentinel value indicating no retention is needed
 */
#define RETENTION_PERIOD_SENTINEL                           0

/**
 * Max number of tracks allowed per stream
 */
#define MAX_SUPPORTED_TRACK_COUNT_PER_STREAM                3

/**
 * Client ready timeout duration.
 **/
#define CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS            15

/**
 * Stream ready timeout duration.
 **/
#define STREAM_READY_TIMEOUT_DURATION_IN_SECONDS            30

/**
 * Stream closed timeout duration.
 */
#define STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS           120

/**
 * Default logger log level
 */
#define DEFAULT_LOGGER_LOG_LEVEL                            LOG_LEVEL_WARN

/**
 * Client clean shutdown timeout
 */
#define CLIENT_SHUTDOWN_SEMAPHORE_TIMEOUT                   (3 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Stream clean shutdown timeout
 */
#define STREAM_SHUTDOWN_SEMAPHORE_TIMEOUT                   (1 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Current versions for the public structs
 */
#define DEVICE_INFO_CURRENT_VERSION                         1
#define CALLBACKS_CURRENT_VERSION                           0
#define STREAM_INFO_CURRENT_VERSION                         1
#define SEGMENT_INFO_CURRENT_VERSION                        0
#define STORAGE_INFO_CURRENT_VERSION                        0
#define AUTH_INFO_CURRENT_VERSION                           0
#define SERVICE_CALL_CONTEXT_CURRENT_VERSION                0
#define STREAM_DESCRIPTION_CURRENT_VERSION                  1
#define FRAGMENT_ACK_CURRENT_VERSION                        0
#define STREAM_METRICS_CURRENT_VERSION                      0
#define CLIENT_METRICS_CURRENT_VERSION                      0
#define CLIENT_INFO_CURRENT_VERSION                         0

/**
 * Definition of the client handle
 */
typedef UINT64 CLIENT_HANDLE;
typedef CLIENT_HANDLE* PCLIENT_HANDLE;

/**
 * This is a sentinel indicating an invalid handle value
 */
#ifndef INVALID_CLIENT_HANDLE_VALUE
#define INVALID_CLIENT_HANDLE_VALUE ((CLIENT_HANDLE) INVALID_PIC_HANDLE_VALUE)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_CLIENT_HANDLE
#define IS_VALID_CLIENT_HANDLE(h) ((h) != INVALID_CLIENT_HANDLE_VALUE)
#endif

/**
 * Definition of the stream handle
 */
typedef UINT64 STREAM_HANDLE;
typedef STREAM_HANDLE* PSTREAM_HANDLE;

/**
 * This is a sentinel indicating an invalid handle value
 */
#ifndef INVALID_STREAM_HANDLE_VALUE
#define INVALID_STREAM_HANDLE_VALUE ((STREAM_HANDLE) INVALID_PIC_HANDLE_VALUE)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_STREAM_HANDLE
#define IS_VALID_STREAM_HANDLE(h) ((h) != INVALID_STREAM_HANDLE_VALUE)
#endif

/**
 * Definition of the upload handle
 */
typedef UINT64 UPLOAD_HANDLE;
typedef UPLOAD_HANDLE* PUPLOAD_HANDLE;

/**
 * This is a sentinel indicating an invalid upload stream handle value
 */
#ifndef INVALID_UPLOAD_HANDLE_VALUE
#define INVALID_UPLOAD_HANDLE_VALUE ((UPLOAD_HANDLE) 0xFFFFFFFFFFFFFFFFULL)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_UPLOAD_HANDLE
#define IS_VALID_UPLOAD_HANDLE(h) ((h) != INVALID_UPLOAD_HANDLE_VALUE)
#endif

/**
 * This is a sentinel indicating an invalid timestamp value
 */
#ifndef INVALID_TIMESTAMP_VALUE
#define INVALID_TIMESTAMP_VALUE ((UINT64) 0xFFFFFFFFFFFFFFFFULL)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_TIMESTAMP
#define IS_VALID_TIMESTAMP(h) ((h) != INVALID_TIMESTAMP_VALUE)
#endif

////////////////////////////////////////////////////
// Main structure declarations
////////////////////////////////////////////////////

/**
 * Streaming type definition
 */
typedef enum {
    // Realtime mode for minimal latency
    STREAMING_TYPE_REALTIME,

    // Near-realtime mode for predefined latency
    STREAMING_TYPE_NEAR_REALTIME,

    // Offline upload mode
    STREAMING_TYPE_OFFLINE,
} STREAMING_TYPE;

/**
 * Whether the streaming mode is offline
 */
#define IS_OFFLINE_STREAMING_MODE(mode)     ((mode) == STREAMING_TYPE_OFFLINE)

/**
 * Device storage types
 */
typedef enum {
    // In-memory storage type
    DEVICE_STORAGE_TYPE_IN_MEM,

    // File based storage type
    DEVICE_STORAGE_TYPE_HYBRID_FILE,

    // In-memory storage type with all allocations from the content store
    DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC,
} DEVICE_STORAGE_TYPE;

/**
 * Stream status as reported by the service
 */
typedef enum {
    // Stream is being created
    STREAM_STATUS_CREATING,

    // Stream has been created and is active
    STREAM_STATUS_ACTIVE,

    // Stream is being updated
    STREAM_STATUS_UPDATING,

    // Stream is being deleted
    STREAM_STATUS_DELETING,
} STREAM_STATUS;

/**
 * Stream access mode
 */
typedef enum {
    // Stream access mode for reading
    STREAM_ACCESS_MODE_READ,

    // Stream access mode for writing
    STREAM_ACCESS_MODE_WRITE,
} STREAM_ACCESS_MODE;

/**
* Service call result
*/
typedef enum {
    // Not defined
    SERVICE_CALL_RESULT_NOT_SET = 0,

    // All OK
    SERVICE_CALL_RESULT_OK = 200,

    // Invalid params error
    SERVICE_CALL_INVALID_ARG = 406,

    // Resource not found exception
    SERVICE_CALL_RESOURCE_NOT_FOUND = 404,

    // Client limit exceeded error
    SERVICE_CALL_CLIENT_LIMIT = 10000,

    // Device limit exceeded error
    SERVICE_CALL_DEVICE_LIMIT = 10001,

    // Stream limit exception
    SERVICE_CALL_STREAM_LIMIT = 10002,

    // Resource in use exception
    SERVICE_CALL_RESOURCE_IN_USE = 10003,

    // Bad request
    SERVICE_CALL_BAD_REQUEST = 400,

    // Forbidden
    SERVICE_CALL_FORBIDDEN = 403,

    // Device not provisioned
    SERVICE_CALL_DEVICE_NOT_PROVISIONED = 10004,

    // Device not found
    SERVICE_CALL_DEVICE_NOT_FOUND = 10005,

    // Security error
    SERVICE_CALL_NOT_AUTHORIZED = 401,

    // Request timeout
    SERVICE_CALL_REQUEST_TIMEOUT = 408,

    // Gateway timeout
    SERVICE_CALL_GATEWAY_TIMEOUT = 504,

    // Network read timeout
    SERVICE_CALL_NETWORK_READ_TIMEOUT = 598,

    // Network connection timeout
    SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT = 599,

    // Resource deleted exception
    SERVICE_CALL_RESOURCE_DELETED = 10400,

    // The stream authorization is in a grace period
    SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD = 10401,

    // Not implemented
    SERVICE_CALL_NOT_IMPLEMENTED = 501,

    // Internal server error
    SERVICE_CALL_INTERNAL_ERROR = 500,

    // Service unavailable
    SERVICE_CALL_SERVICE_UNAVAILABLE = 503,

    // Other errors
    SERVICE_CALL_UNKNOWN = 10006,

    // ACK errors
    // Error when reading the input stream
    SERVICE_CALL_RESULT_STREAM_READ_ERROR = 4000,

    // Fragment size is greater than the limit
    SERVICE_CALL_RESULT_FRAGMENT_SIZE_REACHED = 4001,

    // Fragment duration is greater than the limit
    SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED = 4002,

    // Connection duration is greater than allowed threshold
    SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED = 4003,

    // Fragment timecode is not monotonically increasing
    SERVICE_CALL_RESULT_FRAGMENT_TIMECODE_NOT_MONOTONIC = 4004,

    // Multi-track MKV is not supported
    SERVICE_CALL_RESULT_MULTI_TRACK_MKV = 4005,

    // MKV parsing error
    SERVICE_CALL_RESULT_INVALID_MKV_DATA = 4006,

    // Invalid producer timestamp
    SERVICE_CALL_RESULT_INVALID_PRODUCER_TIMESTAMP = 4007,

    // Inactive stream
    SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE = 4008,

    // Fragment metadata name/value/count limit reached
    SERVICE_CALL_RESULT_FRAGMENT_METADATA_LIMIT_REACHED = 4009,

    // Track number in simple block doesn't match the track number in TrackInfo
    SERVICE_CALL_RESULT_TRACK_NUMBER_MISMATCH = 4010,

    // Frames (simple block) missing for at least of one of the Tracks specified in TrackInfo
    SERVICE_CALL_RESULT_FRAMES_MISSING_FOR_TRACK = 4011,

    // KVS doesn't accept MKV input with more than 3 tracks (limit can be changed via SDC config)
    SERVICE_CALL_RESULT_MORE_THAN_ALLOWED_TRACKS_FOUND = 4012,

    // KMS specific error - KMS access denied while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_ACCESS_DENIED = 4500,

    // KMS specific error - KMS key is disabled while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_DISABLED = 4501,

    // KMS specific error - KMS throws validation error while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_VALIDATION_ERROR = 4502,

    // KMS specific error - KMS key unavailable while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_UNAVAILABLE = 4503,

    // KMS specific error - Invalid usage of the KMS key while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_INVALID_USAGE = 4504,

    // KMS specific error - Invalid state of the KMS key while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_INVALID_STATE = 4505,

    // KMS specific error - KMS key not found while encrypting data
    SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND = 4506,

    // Stream has been/is being deleted
    SERVICE_CALL_RESULT_STREAM_DELETED = 4507,

    // Internal error
    SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR = 5000,

    // Fragment archiving error in the persistent storage
    SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR = 5001,

    // Go Away result
    SERVICE_CALL_RESULT_SIGNALING_GO_AWAY = 6000,

    // Reconnect ICE Server
    SERVICE_CALL_RESULT_SIGNALING_RECONNECT_ICE = 6001,

    // Unknown ACK error
    SERVICE_CALL_RESULT_UNKNOWN_ACK_ERROR = 7000,
} SERVICE_CALL_RESULT;

/**
 * Fragment ACK type enum
 */
typedef enum {
    // Undefined
    FRAGMENT_ACK_TYPE_UNDEFINED,

    // Started buffering the fragment
    FRAGMENT_ACK_TYPE_BUFFERING,

    // Fragment has been received OK
    FRAGMENT_ACK_TYPE_RECEIVED,

    // Fragment has been persisted OK - only when persistence is enabled
    FRAGMENT_ACK_TYPE_PERSISTED,

    // Fragment transmission error.
    FRAGMENT_ACK_TYPE_ERROR,

    // Special ack to keep the connection alive.
    FRAGMENT_ACK_TYPE_IDLE,
} FRAGMENT_ACK_TYPE;

/**
 * Authentication info type
 */
typedef enum {
    // Undefined
    AUTH_INFO_UNDEFINED,

    // Certificate authentication
    AUTH_INFO_TYPE_CERT,

    // STS token authentication
    AUTH_INFO_TYPE_STS,

    // No authentication is needed
    AUTH_INFO_NONE,
} AUTH_INFO_TYPE;

/**
 * Fragment ACK declaration
 */
typedef struct __FragmentAck FragmentAck;
struct __FragmentAck {
    // Version of the struct
    UINT32 version;

    // Ack type
    FRAGMENT_ACK_TYPE ackType;

    // Fragment timecode - the exact one as was extracted from the stream. NOTE: For MKV streaming
    // this will be the Cluster timecode which can be absolute or relative in which case it's still
    // unique for the uninterrupted stream. This is enough to identify the start frame of the fragment.
    UINT64 timestamp;

    // Fragment sequence number
    CHAR sequenceNumber[MAX_FRAGMENT_SEQUENCE_NUMBER + 1];

    // Reporting the ack error type. For non-error acks this is ignored but should be set to SERVICE_CALL_RESULT_OK
    SERVICE_CALL_RESULT result;
};

typedef struct __FragmentAck* PFragmentAck;

/**
 * NAL adaptation types enum. The bit flags correspond to the ones defined in the
 * mkvgen public header enumeration for simple copy forward.
 */
typedef enum {
    /**
     * No flags are set - no adaptation
     */
    NAL_ADAPTATION_FLAG_NONE                    = 0,

    /**
     * Whether to adapt Annex-B NALUs to Avcc NALUs
     */
    NAL_ADAPTATION_ANNEXB_NALS                  = (1 << 3),

    /**
     * Whether to adapt Avcc NALUs to Annex-B NALUs
     */
    NAL_ADAPTATION_AVCC_NALS                    = (1 << 4),

    /**
     * Whether to adapt Annex-B NALUs for the codec private data to Avcc format NALUs
     */
    NAL_ADAPTATION_ANNEXB_CPD_NALS              = (1 << 5),
} NAL_ADAPTATION_FLAGS;

typedef enum {
    /**
     * When in FRAME_ORDER_MODE_PASS_THROUGH, when putKinesisVideoFrame is called, the frame is submitted immediately
     */
    FRAME_ORDER_MODE_PASS_THROUGH                           = 0,
    /**
     * When in FRAME_ORDERING_MODE_MULTI_TRACK_AV, frames are submitted in the order of their dts. In case of two frames
     * having the same mkv timestamp, and one of them being key frame, the key frame flag is moved to the earliest frame
     * to make sure we dont have cluster end timestamp being equal to the next cluster beginning timestamp.
     */
    FRAME_ORDERING_MODE_MULTI_TRACK_AV                      = 1,

    /**
     * If frames from different tracks have dts difference less than mkv timecode scale, then add 1 unit of mkv timecode
     * scale to the latter frame to avoid backend reporting fragment overlap. This will be deprecated once backend is
     * fixed.
     */
    FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_DTS_ONE_MS_COMPENSATE    = 2,

    /**
     * same as the dts counter part, but compares pts instead.
     */
    FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE    = 3,
} FRAME_ORDER_MODE;

/**
 * Stream capabilities declaration
 */
typedef struct __StreamCaps StreamCaps;
struct __StreamCaps {
    // Streaming type
    STREAMING_TYPE streamingType;

    // Stream content type - null terminated.
    CHAR contentType[MAX_CONTENT_TYPE_LEN + 1];

    // Whether the bitrate can change in mid-stream.
    BOOL adaptive;

    // Max latency tolerance in time units. Can be STREAM_LATENCY_PRESSURE_CHECK_SENTINEL for realtime streaming.
    UINT64 maxLatency;

    // Duration of the fragment/cluster. Can be FRAGMENT_KEY_FRAME_DURATION_SENTINEL if based on IDR/key frames.
    UINT64 fragmentDuration;

    // Whether to create fragments on the IDR/key frame boundary or based on duration.
    BOOL keyFrameFragmentation;

    // Whether to use frame timecodes.
    BOOL frameTimecodes;

    // Whether the clusters will have absolute or relative timecodes
    BOOL absoluteFragmentTimes;

    // Whether the application ACKs are required
    BOOL fragmentAcks;

    // Whether to recover after an error occurred
    BOOL recoverOnError;

    // Specify the NALs adaptation flags as defined in NAL_ADAPTATION_FLAGS enumeration
    // The adaptation will be applied to all tracks
    UINT32 nalAdaptationFlags;

    // Average stream bandwidth requirement in bits per second
    UINT32 avgBandwidthBps;

    // Number of frames per second. Will use the defaults if 0.
    UINT32 frameRate;

    // Duration of content to keep in 100ns in storage before purging.
    // 0 for default values to be calculated based on replay buffer, etc..
    UINT64 bufferDuration;

    // Duration of content in 100ns to re-transmit after reconnection.
    // 0 if the latest frame is to be re-transmitted in Realtime mode
    // For Near-Realtime mode or offline mode it will be ignored.
    // If we receive non "dead host" error in connection termination event
    // and the ACKs are enabled then we can rollback to an earlier timestamp
    // as the host has already received the fragment.
    UINT64 replayDuration;

    // Duration to check back for the connection staleness.
    // Can be CONNECTION_STALENESS_DETECTION_SENTINEL to skip the check.
    // If we haven't received any buffering ACKs and the delta between the current
    // and the last buffering ACK is greater than this duration the customer
    // provided optional callback will be executed.
    UINT64 connectionStalenessDuration;

    // Timecode scale to use generating the packaging.
    // NOTE: Specifying DEFAULT_TIMECODE_SCALE_SENTINEL will imply using
    // default timecode for the packaging.
    UINT64 timecodeScale;

    // Whether to recalculate metrics at runtime with slight increasing performance hit.
    BOOL recalculateMetrics;

    // Segment UUID. If specified it should be MKV_SEGMENT_UUID_LEN long. Specifying NULL will generate random UUID
    PBYTE segmentUuid;

    // Array of TrackInfo containing track metadata
    PTrackInfo trackInfoList;

    // Number of TrackInfo in trackInfoList
    UINT32 trackInfoCount;

    // ------------------------------- V0 compat ----------------------

    // How incoming frames are reordered
    FRAME_ORDER_MODE frameOrderingMode;
};

typedef struct __StreamCaps* PStreamCaps;

/**
 * Stream info declaration
 */
typedef struct __StreamInfo StreamInfo;
struct __StreamInfo {
    // Version of the struct
    UINT32 version;

    // Stream name - human readable. Null terminated.
    // Should be unique per AWS account.
    CHAR name[MAX_STREAM_NAME_LEN + 1];

    // Number of tags associated with the stream
    UINT32 tagCount;

    // Stream tags array
    PTag tags;

    // Stream retention period in 100ns - 0 for no retention. Retention should be greater than 1 hour as
    // the service-side accepts the retention policy in an hour units.
    UINT64 retention;

    // KMS key id ARN
    CHAR kmsKeyId[MAX_ARN_LEN + 1];

    // Stream capabilities
    StreamCaps streamCaps;
};

typedef struct __StreamInfo* PStreamInfo;
typedef struct __StreamInfo** PPStreamInfo;

/**
 * Storage info declaration
 */
typedef struct __StorageInfo StorageInfo;
struct __StorageInfo {
    // Version of the struct
    UINT32 version;

    // Storage type
    DEVICE_STORAGE_TYPE storageType;

    // Max storage allocation
    UINT64 storageSize;

    // Spillover ratio for the hybrid heaps in 0 - 100%
    UINT32 spillRatio;

    // File location in case of the file based storage
    CHAR rootDirectory[MAX_PATH_LEN + 1];
};

typedef struct __StorageInfo* PStorageInfo;

/**
 * Client Info
 */
typedef struct __ClientInfo {
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
} ClientInfo, *PClientInfo;

/**
 * Device info declaration
 */
typedef struct __DeviceInfo {
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

    // ------------------------------- V0 compat ----------------------

    // Client ID used as an identifier when generating MKV header
    CHAR clientId[MAX_CLIENT_ID_STRING_LENGTH + 1];

    // Client info
    ClientInfo clientInfo;
} DeviceInfo, *PDeviceInfo;

/**
 * Client metrics
 */
typedef struct __ClientMetrics ClientMetrics;
struct __ClientMetrics {
    // Version of the struct
    UINT32 version;

    // Overall content store allocation size
    UINT64 contentStoreSize;

    // Available bytes in the content store
    UINT64 contentStoreAvailableSize;

    // Content store allocated size
    UINT64 contentStoreAllocatedSize;

    // Content view allocation size
    UINT64 totalContentViewsSize;

    // Overall frame rate across the streams
    UINT64 totalFrameRate;

    // Overall transfer rate across the streams
    UINT64 totalTransferRate;
};

typedef struct __ClientMetrics* PClientMetrics;

/**
 * Stream metrics
 */
typedef struct __StreamMetrics StreamMetrics;
struct __StreamMetrics {
    // Version of the struct
    UINT32 version;

    // Duration from the current to the head in 100ns
    UINT64 currentViewDuration;

    // Overall duration from the tail to the head in 100ns
    UINT64 overallViewDuration;

    // Size from the current to the head in bytes
    UINT64 currentViewSize;

    // Overall size from the tail to the head in bytes
    UINT64 overallViewSize;

    // Last measured put frame rate in bytes per second
    DOUBLE currentFrameRate;

    // Last measured transfer rate in bytes per second
    UINT64 currentTransferRate;
};

typedef struct __StreamMetrics* PStreamMetrics;

/**
 * Fragment metadata declaration
 */
typedef struct __FragmentMetadata FragmentMetadata;
struct __FragmentMetadata {
    // Offset of the fragment in the stream in bytes
    UINT64 offset;

    // Length of the fragment in bytes
    UINT64 length;

    // Fragment in-stream timecode in 100ns
    UINT64 timecode;

    // Fragment duration in 100ns
    UINT64 duration;
};

typedef struct __FragmentMetadata* PFragmentMetadata;

/**
 * Segment info declaration
 */
typedef struct __SegmentInfo SegmentInfo;
struct __SegmentInfo {
    // Version of the struct
    UINT32 version;

    // Segment data size in byte
    UINT32 segmentDataSize;

    // Optional flat data that's shared for the entire segment (i.e. Init fragment)
    PBYTE segmentData;

    // Fragment count in the segment
    UINT32 fragmentCount;

    // Actual fragments buffer
    PBYTE fragmentData;
};

typedef struct __SegmentInfo* PSegmentInfo;

/**
 * Stream description declaration
 */
typedef struct __StreamDescription StreamDescription;
struct __StreamDescription {
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

    // ------------------------------- V0 compat ----------------------

    // Data retention in hours
    UINT64 retention;

    // KMS key id ARN
    CHAR kmsKeyId[MAX_ARN_LEN + 1];
};

typedef struct __StreamDescription* PStreamDescription;

/**
 * Auth info - either STS token or a Certificate
 */
typedef struct __AuthInfo AuthInfo;
struct __AuthInfo {
    // the version of the structure
    UINT32 version;

    // Auth type
    AUTH_INFO_TYPE type;

    // The bits of the auth
    BYTE data[MAX_AUTH_LEN];

    // Size of the auth in bytes
    UINT32 size;

    // Expiration of the auth
    UINT64 expiration;
};

typedef struct __AuthInfo* PAuthInfo;

/**
 * Service API call context
 */
typedef struct __ServiceCallContext ServiceCallContext;
struct __ServiceCallContext {
    // Version of the struct
    UINT32 version;

    // Call after this absolute time
    UINT64 callAfter;

    // The timeout for the operation in 100ns.
    UINT64 timeout;

    // Custom data to be passed back with the event
    UINT64 customData;

    // Authentication info
    PAuthInfo pAuthInfo;
};

typedef struct __ServiceCallContext* PServiceCallContext;

////////////////////////////////////////////////////
// General callbacks definitions
////////////////////////////////////////////////////

/**
 * Gets the device certificate and the private key.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PBYTE* - Device certificate bits.
 * @param 3 PUINT32 - Device certificate bits size.
 * @param 4 PUINT64 - Device certificate expiration - in 100ns - absolute time.
 *
 * @return Status of the callback
 */
typedef STATUS (*GetDeviceCertificateFunc)(UINT64,
                                           PBYTE*,
                                           PUINT32,
                                           PUINT64);

/**
 * Gets the device security token.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PBYTE* - Device STS token bits.
 * @param 3 PUINT32 - Device STS token bits size.
 * @param 4 PUINT64 - Device token expiration - in 100ns - absolute time.
 *
 * @return Status of the callback
 */
typedef STATUS (*GetSecurityTokenFunc)(UINT64,
                                       PBYTE*,
                                       PUINT32,
                                       PUINT64);

/**
 * Gets the device fingerprint.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR* - Device fingerprint - NULL terminated.
 *
 * @return Status of the callback
 */
typedef STATUS (*GetDeviceFingerprintFunc)(UINT64,
                                           PCHAR*);

/**
 * Reports an underflow for the stream.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - Reporting for this stream.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamUnderflowReportFunc)(UINT64,
                                            STREAM_HANDLE);

/**
 * Reports storage pressure.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 UINT64 - Remaining bytes.
 *
 * @return Status of the callback
 */
typedef STATUS (*StorageOverflowPressureFunc)(UINT64,
                                              UINT64);

/**
 * Reports temporal buffer pressure.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - Reporting for this stream.
 * @param 3 UINT64 - Remaining duration in hundreds of nanos.
 *
 * @return Status of the callback
 */
typedef STATUS (*BufferDurationOverflowPressureFunc)(UINT64,
                                                     STREAM_HANDLE,
                                                     UINT64);

/**
 * Reports stream latency excess.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UINT64 - The current buffer duration/depth in 100ns.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamLatencyPressureFunc)(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);

/**
 * Reports stream staleness as the last buffering ack is greater than
 * a specified duration in the stream caps.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UINT64 - Duration of the last buffering ACK received in 100ns.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamConnectionStaleFunc)(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);

/**
 * Reports a dropped frame for the stream.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UINT64 - The timecode of the dropped frame in 100ns.
 *
 * @return Status of the callback
 */
typedef STATUS (*DroppedFrameReportFunc)(UINT64,
                                         STREAM_HANDLE,
                                         UINT64);

/**
 * Reports a dropped fragment for the stream.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UINT64 - The timecode of the dropped fragment in 100ns.
 *
 * @return Status of the callback
 */
typedef STATUS (*DroppedFragmentReportFunc)(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);

/**
 * Reports a stream error due to an error ACK. The PIC will initiate the termination
 * of the stream itself as the Inlet host has/will close the connection regardless.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UPLOAD_HANDLE - Client upload handle.
 * @param 4 UINT64 - The timecode of the errored fragment in 100ns.
 * @param 5 STATUS - The status code of the error to report.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamErrorReportFunc)(UINT64,
                                        STREAM_HANDLE,
                                        UPLOAD_HANDLE,
                                        UINT64,
                                        STATUS);

/**
 * Reports a received fragment ack for the stream.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UPLOAD_HANDLE - Client upload handle.
 * @param 4 PFragmentAck - The constructed fragment ack.
 *
 * @return Status of the callback
 */
typedef STATUS (*FragmentAckReceivedFunc)(UINT64,
                                          STREAM_HANDLE,
                                          UPLOAD_HANDLE,
                                          PFragmentAck);

///////////////////////////////////////////////////////////////
// State transition callbacks
///////////////////////////////////////////////////////////////
/**
 * Reports a ready state for the client.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CLIENT_HANDLE - The client handle.
 *
 * @return Status of the callback
 */
typedef STATUS (*ClientReadyFunc)(UINT64,
                                  CLIENT_HANDLE);

/**
 * Reports a ready state for the stream.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamReadyFunc)(UINT64,
                                  STREAM_HANDLE);

/**
 * Reports an EOS
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 UPLOAD_HANDLE - Client upload handle.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamClosedFunc)(UINT64,
                                   STREAM_HANDLE,
                                   UPLOAD_HANDLE);

/**
 * Notifies that a given stream has data available.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to report for.
 * @param 3 PCHAR - Stream name.
 * @param 4 UPLOAD_HANDLE - Current client stream upload handle passed by the caller.
 * @param 5 UINT64 - The duration of content currently available in 100ns.
 * @param 6 UINT64 - The size of content in bytes currently available.
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamDataAvailableFunc)(UINT64,
                                          STREAM_HANDLE,
                                          PCHAR,
                                          UPLOAD_HANDLE,
                                          UINT64,
                                          UINT64);

///////////////////////////////////////////////////////////////
// Synchronization callbacks - Locking
///////////////////////////////////////////////////////////////
/**
 * Creates a mutex
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 BOOL - Whether the mutex is reentrant.
 *
 * @return MUTEX object to use
 */
typedef MUTEX (*CreateMutexFunc)(UINT64,
                                 BOOL);

/**
 * Lock the mutex
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 MUTEX - The mutex to lock.
 *
 */
typedef VOID (*LockMutexFunc)(UINT64,
                              MUTEX);

/**
 * Unlock the mutex
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 MUTEX - The mutex to unlock.
 *
 */
typedef VOID (*UnlockMutexFunc)(UINT64,
                                MUTEX);

/**
 * Try to lock the mutex
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 MUTEX - The mutex to try to lock.
 *
 */
typedef BOOL (*TryLockMutexFunc)(UINT64,
                                 MUTEX);

/**
 * Free the mutex
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 MUTEX - The mutex to free.
 *
 */
typedef VOID (*FreeMutexFunc)(UINT64,
                              MUTEX);

///////////////////////////////////////////////////////////////
// Synchronization callbacks - Conditional variables
///////////////////////////////////////////////////////////////
/**
 * Creates a condition variable
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 *
 * @return CVAR object to use
 */
typedef CVAR (*CreateConditionVariableFunc)(UINT64);

/**
 * Signals a single listener for a condition variable
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CVAR - The conditional variable to signal.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*SignalConditionVariableFunc)(UINT64, CVAR);

/**
 * Broadcasts to all listeners for a condition variable
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CVAR - The conditional variable to signal.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*BroadcastConditionVariableFunc)(UINT64, CVAR);

/**
 * Waits for a condition variable to become signalled
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CVAR - The conditional variable to wait for until signalled.
 * @param 3 MUTEX - The lock to use for signalling.
 * @param 4 UINT64 - The timeout to use. INFINITE_TIME_VALUE will wait infinitely for the signal.
 *
 * @return STATUS code of the operations
 */
typedef STATUS (*WaitConditionVariableFunc)(UINT64, CVAR, MUTEX, UINT64);

/**
 * Frees a condition variable
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CVAR - The conditional variable to free.
 *
 * @return STATUS code of the operations
 */
typedef VOID (*FreeConditionVariableFunc)(UINT64, CVAR);

///////////////////////////////////////////////////////////////
// Pseudo-random number generator callbacks
///////////////////////////////////////////////////////////////

/**
 * Get a random number between 0 and RAND_MAX (included)
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 *
 * @return Pseudo-random number
 */
typedef UINT32 (*GetRandomNumberFunc)(UINT64);

///////////////////////////////////////////////////////////////
// Logging callbacks
///////////////////////////////////////////////////////////////

/**
 * Logs a line of text with the tag and the log level - see PlatformUtils.h for more info
 */
typedef logPrintFunc LogPrintFunc;

///////////////////////////////////////////////////////////////
// Service call callbacks
///////////////////////////////////////////////////////////////

/**
 * Create device callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Device name.
 * @param 3 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*CreateDeviceFunc)(UINT64,
                                   PCHAR,
                                   PServiceCallContext);

/**
 * Get device token from certificate callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Device name.
 * @param 3 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*DeviceCertToTokenFunc)(UINT64,
                                        PCHAR,
                                        PServiceCallContext);

/**
 * Create stream callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Device name.
 * @param 3 PCHAR - Stream name.
 * @param 4 PCHAR - Content type.
 * @param 5 PCHAR - Kms Key ID Arn - Optional.
 * @param 6 UINT64 - Stream retention period in 100ns.
 * @param 7 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*CreateStreamFunc)(UINT64,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   UINT64,
                                   PServiceCallContext);

/**
 * Describe stream callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Stream name.
 * @param 3 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*DescribeStreamFunc)(UINT64,
                                     PCHAR,
                                     PServiceCallContext);

/**
 * Get streaming endpoint callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Stream name.
 * @param 3 PCHAR - API name to call. Currently only PUT_MEDIA is supported
 * @param 4 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*GetStreamingEndpointFunc)(UINT64,
                                           PCHAR,
                                           PCHAR,
                                           PServiceCallContext);

/**
 * Get streaming token callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Stream name.
 * @param 3 STREAM_ACCESS_MODE - Stream access mode - read/write.
 * @param 4 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*GetStreamingTokenFunc)(UINT64,
                                        PCHAR,
                                        STREAM_ACCESS_MODE,
                                        PServiceCallContext);

/**
 * Put stream callback function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - Stream name.
 * @param 3 PCHAR - Container type enum.
 * @param 4 UINT64 - Stream start timestamp.
 * @param 5 BOOL - Whether the stream fragment time stamps are absolute or relative.
 * @param 6 BOOL - Application level ACK required.
 * @param 7 PCHAR - Streaming endpoint URI
 * @param 8 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*PutStreamFunc)(UINT64,
                                PCHAR,
                                PCHAR,
                                UINT64,
                                BOOL,
                                BOOL,
                                PCHAR,
                                PServiceCallContext);

/**
 * Tag a resource function.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 PCHAR - ARN of the resource to tag.
 * @param 3 UINT32 - Number of tags that follows.
 * @param 4 PTag - Tag array.
 * @param 5 PServiceCallContext - Service call context.
 *
 * @return Status of the callback
 */
typedef STATUS (*TagResourceFunc)(UINT64,
                                  PCHAR,
                                  UINT32,
                                  PTag,
                                  PServiceCallContext);

/**
 * Client shutdown function.
 *
 * NOTE: No more PIC API calls should be made for the client object from this moment on, including calling
 * PIC API from within the shutdown callback.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 CLIENT_HANDLE - The client handle.
 *
 * @return Status of the callback
 */
typedef STATUS (*ClientShutdownFunc)(UINT64, CLIENT_HANDLE);

/**
 * Stream shutdown function.
 *
 * NOTE: No more PIC API calls should be made for the stream object from this moment on, including calling
 * PIC API from within the shutdown callback.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 * @param 2 STREAM_HANDLE - The stream to shutdown.
 * @param 3 BOOL - Whether {@link kinesisVideoStreamResetStream} or {@link freeKinesisVideoStream} is called
 *
 * @return Status of the callback
 */
typedef STATUS (*StreamShutdownFunc)(UINT64, STREAM_HANDLE, BOOL);

/**
 * The callbacks structure to be passed in to the client
 */
typedef struct __ClientCallbacks ClientCallbacks;
struct __ClientCallbacks {
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
    GetDeviceCertificateFunc getDeviceCertificateFn;
    GetSecurityTokenFunc getSecurityTokenFn;
    GetDeviceFingerprintFunc getDeviceFingerprintFn;
    StreamUnderflowReportFunc streamUnderflowReportFn;
    StorageOverflowPressureFunc storageOverflowPressureFn;
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
    CreateStreamFunc createStreamFn;
    DescribeStreamFunc describeStreamFn;
    GetStreamingEndpointFunc getStreamingEndpointFn;
    GetStreamingTokenFunc getStreamingTokenFn;
    PutStreamFunc putStreamFn;
    TagResourceFunc tagResourceFn;
    CreateDeviceFunc createDeviceFn;
    DeviceCertToTokenFunc deviceCertToTokenFn;
    ClientReadyFunc clientReadyFn;
    LogPrintFunc logPrintFn;
    ClientShutdownFunc clientShutdownFn;
    StreamShutdownFunc streamShutdownFn;
};
typedef struct __ClientCallbacks* PClientCallbacks;

////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////

/**
 * Create a client object
 *
 * @PDeviceInfo - Device info structure
 * @PClientCallbacks - Client callbacks structure with the function pointers
 * @PCLIENT_HANDLE - OUT - returns the newly created objects handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createKinesisVideoClient(PDeviceInfo, PClientCallbacks, PCLIENT_HANDLE);

/**
 * Create a client object synchronously awaiting for the Ready state.
 *
 * @PDeviceInfo - Device info structure
 * @PClientCallbacks - Client callbacks structure with the function pointers
 * @PCLIENT_HANDLE - OUT - returns the newly created objects handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createKinesisVideoClientSync(PDeviceInfo, PClientCallbacks, PCLIENT_HANDLE);

/**
 * Frees and de-allocates the memory of the client and it's sub-objects
 *
 * NOTE: This function is idempotent - can be called at various stages of construction.
 *
 * @PCLIENT_HANDLE - the pointer to the object to free
 *
 * @return - The status of the function call.
 */
PUBLIC_API STATUS freeKinesisVideoClient(PCLIENT_HANDLE);

/**
 * Stops all stream processing
 *
 * @param 1 CLIENT_HANDLE - the client handle.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS stopKinesisVideoStreams(CLIENT_HANDLE);

/**
 * Create a stream object
 *
 * @CLIENT_HANDLE - Client objects handle
 * @PStreamInfo - Stream info object
 * @PSTREAM_HANDLE - OUT - returns the newly created stream objects handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createKinesisVideoStream(CLIENT_HANDLE, PStreamInfo, PSTREAM_HANDLE);

/**
 * Create a stream object syncronously awaiting for the stream to enter Ready state.
 *
 * @CLIENT_HANDLE - Client objects handle
 * @PStreamInfo - Stream info object
 * @PSTREAM_HANDLE - OUT - returns the newly created stream objects handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createKinesisVideoStreamSync(CLIENT_HANDLE, PStreamInfo, PSTREAM_HANDLE);

/**
 * Stops and frees the stream
 *
 * @param 1 PSTREAM_HANDLE - the stream handle pointer.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS freeKinesisVideoStream(PSTREAM_HANDLE);

/**
 * Stops the stream processing. This is an ASYNC api and will continue streaming until the buffer is emptied.
 *
 * @param 1 STREAM_HANDLE - the client handle.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS stopKinesisVideoStream(STREAM_HANDLE);

/**
 * Stops the stream processing. This is a SYNC api and the call will block until the last bits
 * of the buffer are streamed out and the stream stopped notification is called.
 *
 * @param 1 STREAM_HANDLE - the client handle.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS stopKinesisVideoStreamSync(STREAM_HANDLE);

/**
 * Puts a frame into the stream
 *
 * @param 1 STREAM_HANDLE - the stream handle.
 * @param 2 PFrame - the frame to process.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS putKinesisVideoFrame(STREAM_HANDLE, PFrame);

/**
 * Gets the data for the stream.
 *
 * NOTE: The function will try to fill as much buffer as available to fill
 * and will return a STATUS_NO_MORE_DATA_AVAILABLE status code. The caller
 * should check the returned filled size for partially filled buffers.
 *
 * @param 1 STREAM_HANDLE - the stream handle.
 * @param 2 UPLOAD_HANDLE - Client stream upload handle.
 * @param 3 PBYTE - Buffer to fill in.
 * @param 4 UINT32 - Size of the buffer to fill up-to.
 * @param 5 PUINT32 - Actual size filled.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS getKinesisVideoStreamData(STREAM_HANDLE, UPLOAD_HANDLE, PBYTE, UINT32, PUINT32);

/**
 * Inserts a "metadata" - a key/value string pair into the stream.
 *
 * NOTE: The metadata are modelled as MKV tags and are not immediately put into the stream as
 * it might break the fragment.
 * This is a limitation of MKV format as Tags are level 1 elements.
 * Instead, they will be accumulated and inserted in-between the fragments and at the end of the stream.
 *
 * MKV spec is available at: https://matroska.org/technical/specs/index.html
 *
 * Putting a "persistent" metadata will result in the metadata being inserted before every fragment.
 * The metadata can be changed by calling this function with the same name and a different value.
 * Specifying an empty string for the value for a persistent metadata will clear it and it won't
 * be applied to the consecutive fragments.
 *
 * @param 1 STREAM_HANDLE - the stream handle.
 * @param 2 PCHAR - the metadata name.
 * @param 3 PCHAR - the metadata value.
 * @param 4 BOOL - Whether to keep applying the metadata to following fragments.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS putKinesisVideoFragmentMetadata(STREAM_HANDLE, PCHAR, PCHAR, BOOL);

////////////////////////////////////////////////////
// Diagnostics functions
////////////////////////////////////////////////////

/**
 * Gets information about the storage availability.
 *
 * @param 1 CLIENT_HANDLE - the client object handle.
 * @param 2 PClientMetrics - OUT - Client object metrics to be filled.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS getKinesisVideoMetrics(CLIENT_HANDLE, PClientMetrics);

/**
 * Gets information about the stream content view.
 *
 * @param 1 STREAM_HANDLE - the stream object handle.
 * @param 2 PStreamMetrics - Stream metrics to fill.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS getKinesisVideoStreamMetrics(STREAM_HANDLE, PStreamMetrics);

////////////////////////////////////////////////////
// Public Service call event functions
////////////////////////////////////////////////////

/**
 * Create device API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PCHAR - Device ARN.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS createDeviceResultEvent(UINT64, SERVICE_CALL_RESULT, PCHAR);

/**
 * Device certificate to token exchange API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PBYTE - Device token bits.
 * @param 4 UINT32 - Device token bits size in bytes.
 * @param 5 UINT64 - Token expiration.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS deviceCertToTokenResultEvent(UINT64, SERVICE_CALL_RESULT, PBYTE, UINT32, UINT64);

/**
 * Create stream API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PCHAR - Stream ARN.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS createStreamResultEvent(UINT64, SERVICE_CALL_RESULT, PCHAR);

/**
 * Describe stream API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PStreamDescription - Stream description object result.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS describeStreamResultEvent(UINT64, SERVICE_CALL_RESULT, PStreamDescription);

/**
 * Get streaming token API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PBYTE - Streaming token.
 * @param 4 UINT32 - Size of the streaming token bits.
 * @param 5 UINT64 - Streaming token expiration in 100ns - absolute time.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS getStreamingTokenResultEvent(UINT64, SERVICE_CALL_RESULT, PBYTE, UINT32, UINT64);

/**
 * Get streaming endpoint API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 PCHAR - Streaming endpoint.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS getStreamingEndpointResultEvent(UINT64, SERVICE_CALL_RESULT, PCHAR);

/**
 * Put stream API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 * @param 3 UPLOAD_HANDLE - Client stream upload handle.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS putStreamResultEvent(UINT64, SERVICE_CALL_RESULT, UPLOAD_HANDLE);

/**
 * Tag resource API call result event
 *
 * @param 1 UINT64 - the custom data passed to the callback by Kinesis Video.
 * @param 2 SERVICE_CALL_RESULT - Service call result.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS tagResourceResultEvent(UINT64, SERVICE_CALL_RESULT);

////////////////////////////////////////////////////
// Public streaming event functions
////////////////////////////////////////////////////

/**
 * Updates the codec private data associated with the stream.
 *
 * NOTE: Many encoders provide CPD after they have been initialized.
 * This update should happen in states other than STREAMING state.
 *
 * @param 1 STREAM_HANDLE - the stream handle.
 * @param 2 UINT32 - Codec Private Data size
 * @param 3 PBYTE - Codec Private Data bits.
 * @param 4 UINT64 - Id of TrackInfo in trackInfoList
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamFormatChanged(STREAM_HANDLE, UINT32, PBYTE, UINT64);

/**
 * Streaming has been terminated unexpectedly
 *
 * @param 1 STREAM_HANDLE - the stream handle.
 * @param 2 UPLOAD_HANDLE - Stream upload handle returned by the client.
 * @param 3 SERVICE_CALL_RESULT - Result returned.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamTerminated(STREAM_HANDLE, UPLOAD_HANDLE, SERVICE_CALL_RESULT);

/**
 * Stream fragment ACK received.
 *
 * Processing the following cases:
 * 1) In case of a fragment buffering ack we store the timestamp for staleness detection.
 * 2) In case of a fragment received ack we store the timestamp for staleness detection and ensure
 *       we re-stream from the next fragment start timestamp in case of a non-host dead termination.
 * 3) In case of a fragment persisted ack we trim the tail of the view to the next fragment timestamp.
 * 4) In case of an error we re-set the current to the fragment start frame timestamp.
 *
 * @param 1 STREAM_HANDLE - The stream handle to report ACKs for
 * @param 2 UPLOAD_HANDLE - Stream upload handle.
 * @param 3 PFragmentAck - Reported ack.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamFragmentAck(STREAM_HANDLE,
                                                UPLOAD_HANDLE,
                                                PFragmentAck);

/**
 * Parse and consume a string representing a fragment ACK.
 * The API is useful to route the networking stream to so it can parse and respond. The ack segment can have
 * partial ACK or multiple ACKs.
 *
 * NOTE: See kinesisVideoStreamFragmentAck for more information about the successfully received ACK processing
 *
 * @param 1 STREAM_HANDLE - The stream handle to parse the ACKs for
 * @param 2 UPLOAD_HANDLE - Stream upload handle.
 * @param 3 PCHAR - Reported ack segment - can be non-NULL terminated.
 * @param 4 UINT32 - Reported ack segment size in chars.
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamParseFragmentAck(STREAM_HANDLE,
                                                     UPLOAD_HANDLE,
                                                     PCHAR,
                                                     UINT32);

/**
 * Get the streamInfo object belonging to stream with STREAM_HANDLE
 *
 * @param 1 STREAM_HANDLE - The stream handle to get the streamInfo
 * @param 2 PStreamInfo - Returned streamInfo
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamGetStreamInfo(STREAM_HANDLE,
                                                  PPStreamInfo);

/**
 * Restart/Reset a stream by dropping remaining data and reset stream state machine.
 * Note: The buffered frames will be deleted without being sent.
 *
 * @param 1 STREAM_HANDLE - The stream handle to reset
 *
 * @return - The status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamResetStream(STREAM_HANDLE);

/**
 * Reset connection for stream, continue sending existing data in buffer with new connection
 *
 * @param 1 STREAM_HANDLE - The stream handle to reset
 *
 * @return - The status of the function call.
 */
PUBLIC_API STATUS kinesisVideoStreamResetConnection(STREAM_HANDLE);

///////////////////////////////////////////////////
// Default implementations of the platform callbacks
///////////////////////////////////////////////////
PUBLIC_API UINT64 kinesisVideoStreamDefaultGetCurrentTime(UINT64 customData);
PUBLIC_API UINT32 kinesisVideoStreamDefaultGetRandomNumber(UINT64 customData);
PUBLIC_API MUTEX kinesisVideoStreamDefaultCreateMutex(UINT64 customData, BOOL reentrant);
PUBLIC_API VOID kinesisVideoStreamDefaultLockMutex(UINT64 customData, MUTEX mutex);
PUBLIC_API VOID kinesisVideoStreamDefaultUnlockMutex(UINT64 customData, MUTEX mutex);
PUBLIC_API BOOL kinesisVideoStreamDefaultTryLockMutex(UINT64 customData, MUTEX mutex);
PUBLIC_API VOID kinesisVideoStreamDefaultFreeMutex(UINT64 customData, MUTEX mutex);
PUBLIC_API CVAR kinesisVideoStreamDefaultCreateConditionVariable(UINT64 customData);
PUBLIC_API STATUS kinesisVideoStreamDefaultSignalConditionVariable(UINT64 customData, CVAR cvar);
PUBLIC_API STATUS kinesisVideoStreamDefaultBroadcastConditionVariable(UINT64 customData, CVAR cvar);
PUBLIC_API STATUS kinesisVideoStreamDefaultWaitConditionVariable(UINT64 customData, CVAR cvar, MUTEX mutex,
                                                                 UINT64 timeout);
PUBLIC_API VOID kinesisVideoStreamDefaultFreeConditionVariable(UINT64 customData, CVAR cvar);
PUBLIC_API STATUS kinesisVideoStreamDefaultStreamReady(UINT64 customData, STREAM_HANDLE streamHandle);
PUBLIC_API STATUS kinesisVideoStreamDefaultEndOfStream(UINT64 customData, STREAM_HANDLE streamHandle,
                                                       UPLOAD_HANDLE streamUploadHandle);
PUBLIC_API STATUS kinesisVideoStreamDefaultClientReady(UINT64 customData, CLIENT_HANDLE clientHandle);
PUBLIC_API STATUS kinesisVideoStreamDefaultStreamDataAvailable(UINT64 customData, STREAM_HANDLE streamHandle,
                                                               PCHAR streamName, UINT64 uploadHandle, UINT64 duration,
                                                               UINT64 size);
PUBLIC_API STATUS kinesisVideoStreamDefaultClientShutdown(UINT64 customData, CLIENT_HANDLE clientHandle);
PUBLIC_API STATUS kinesisVideoStreamDefaultStreamShutdown(UINT64 customData, STREAM_HANDLE streamHandle, BOOL resetStream);

///////////////////////////////////////////////////
// Auxiliary functionality
///////////////////////////////////////////////////

/*
 * Maps the service call result to a status.
 *
 * @param - SERVICE_CALL_RESULT - IN - Service call result to convert
 *
 * @return - STATUS Mapped status
 */
PUBLIC_API STATUS serviceCallResultCheck(SERVICE_CALL_RESULT);

#pragma pack(pop, include)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_CLIENT_INCLUDE__ */
