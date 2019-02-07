/*******************************************
* Kinesis Video Stream include file
*******************************************/
#ifndef __KINESIS_VIDEO_STREAM_H__
#define __KINESIS_VIDEO_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////
/**
 * Kinesis Video stream states definitions
 */
#define STREAM_STATE_NONE                               ((UINT64) 0)
#define STREAM_STATE_NEW                                ((UINT64) (1 << 0))
#define STREAM_STATE_DESCRIBE                           ((UINT64) (1 << 1))
#define STREAM_STATE_CREATE                             ((UINT64) (1 << 2))
#define STREAM_STATE_TAG_STREAM                         ((UINT64) (1 << 3))
#define STREAM_STATE_GET_TOKEN                          ((UINT64) (1 << 4))
#define STREAM_STATE_GET_ENDPOINT                       ((UINT64) (1 << 5))
#define STREAM_STATE_READY                              ((UINT64) (1 << 6))
#define STREAM_STATE_PUT_STREAM                         ((UINT64) (1 << 7))
#define STREAM_STATE_STREAMING                          ((UINT64) (1 << 8))
#define STREAM_STATE_STOPPED                            ((UINT64) (1 << 9))

/**
 * Stream object internal version
 */
#define STREAM_CURRENT_VERSION                  0

/**
 * Forward declarations
 */
typedef struct __StreamCaps StreamCaps;
typedef __StreamCaps* PStreamCaps;

typedef struct __StreamInfo StreamInfo;
typedef __StreamInfo* PStreamInfo;

typedef struct __StateMachine StateMachine;
typedef __StateMachine* PStateMachine;

typedef struct __StateMachineState StateMachineState;
typedef __StateMachineState* PStateMachineState;

typedef struct __KinesisVideoBase KinesisVideoBase;
typedef __KinesisVideoBase* PKinesisVideoBase;

typedef struct __KinesisVideoClient KinesisVideoClient;
typedef __KinesisVideoClient* PKinesisVideoClient;

typedef struct __FragmentAckParser FragmentAckParser;
typedef __FragmentAckParser* PFragmentAckParser;

/**
 * Default MKV timecode scale - 1ms.
 * MKV basic unit of time is 1ns.
 * The frame timecode is represented in 2 bytes
 * so for the default value we choose 1ms to be
 * able to represent a larger timecode values
 * by multiplying the value with the timecode
 * scale which we define as 1ms.
 */
#define DEFAULT_MKV_TIMECODE_SCALE      (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

/**
 * End-of-fragment metadata name
 */
#define EOFR_METADATA_NAME                   "AWS_KINESISVIDEO_END_OF_FRAGMENT"

/**
 * Internal AWS metadata prefix
 */
#define AWS_INTERNAL_METADATA_PREFIX         "AWS"

/**
 * Max packaged size for the metadata including metadata name len + value + delta for packaging. The delta is
 * the maximal "bloat" size the MKV packaging could add. The actual packaged size will be calculated properly.
 */
#define MAX_PACKAGED_METADATA_LEN             (MKV_MAX_TAG_NAME_LEN + MAX(MKV_MAX_TAG_VALUE_LEN, MAX_STREAM_NAME_LEN) + 256)

/**
 * Maximal overhead for an allocation
 */
#define MAX_ALLOCATION_OVERHEAD_SIZE        100

/**
 * Max wait time for the blocking put frame
 */
#define MAX_BLOCKING_PUT_WAIT               INFINITE_TIME_VALUE

/**
 * Valid status codes from get stream data
 */
#define IS_VALID_GET_STREAM_DATA_STATUS(s) ((s) == STATUS_SUCCESS || \
                                            (s) == STATUS_NO_MORE_DATA_AVAILABLE || \
                                            (s) == STATUS_AWAITING_PERSISTED_ACK || \
                                            (s) == STATUS_END_OF_STREAM)

/**
 * Kinesis Video stream diagnostics information accumulator
 */
typedef struct __KinesisVideoStreamDiagnostics KinesisVideoStreamDiagnostics;
struct __KinesisVideoStreamDiagnostics {
    // Current FPS value
    DOUBLE currentFrameRate;

    // Last time we took a measurement for the frame rate
    UINT64 lastFrameRateTimestamp;

    // Current transfer bytes-per-second
    UINT64 currentTransferRate;

    // Accumuted byte count for the transfer rate calculation
    UINT64 accumulatedByteCount;

    // Last time we took a measurement for the transfer rate
    UINT64 lastTransferRateTimestamp;
};
typedef __KinesisVideoStreamDiagnostics* PKinesisVideoStreamDiagnostics;

/**
 * Wrapper around ViewItem that has the consumed data offset information.
 */
typedef struct __CurrentViewItem CurrentViewItem;
struct __CurrentViewItem {
    // The wrapped ViewItem
    ViewItem viewItem;

    // Consumed data offset
    UINT32 offset;
};
typedef __CurrentViewItem* PCurrentViewItem;

/**
 * Helper structure storing and tracking metadata.
 */
typedef struct __MetadataTracker MetadataTracker;
struct __MetadataTracker {
    // Whether we are sending metadata
    BOOL send;

    // Consumed data offset
    UINT32 offset;

    // Packaged metadata size
    UINT32 size;

    // Storage for the packaged metadata.
    PBYTE data;
};
typedef __MetadataTracker* PMetadataTracker;

/**
 * Streaming type definition
 */
typedef enum {
    // State none is a sentinel value for comparison
    UPLOAD_HANDLE_STATE_NONE = (UINT32) 0,

    // New handle returned by put stream result event
    UPLOAD_HANDLE_STATE_NEW = (1 << 0),

    // Handle that's been put to rotation after moving from put stream state
    UPLOAD_HANDLE_STATE_READY = (1 << 1),

    // Handle that's been already used to stream some data
    UPLOAD_HANDLE_STATE_STREAMING = (1 << 2),

    // Terminating
    UPLOAD_HANDLE_STATE_TERMINATING = (1 << 3),

    // Terminating - awaiting ACK
    UPLOAD_HANDLE_STATE_AWAITING_ACK = (1 << 4),

    // Terminating - awaiting ACK received
    UPLOAD_HANDLE_STATE_ACK_RECEIVED = (1 << 5),

    // Terminated
    UPLOAD_HANDLE_STATE_TERMINATED = (1 << 6),

    // Error terminated
    UPLOAD_HANDLE_STATE_ERROR = (1 << 7),

    // Not-yet put to use handle
    UPLOAD_HANDLE_STATE_NOT_IN_USE = UPLOAD_HANDLE_STATE_NEW |
                                     UPLOAD_HANDLE_STATE_READY,

    // Active states
    UPLOAD_HANDLE_STATE_ACTIVE = UPLOAD_HANDLE_STATE_NEW |
                                 UPLOAD_HANDLE_STATE_READY |
                                 UPLOAD_HANDLE_STATE_STREAMING |
                                 UPLOAD_HANDLE_STATE_TERMINATING |
                                 UPLOAD_HANDLE_STATE_AWAITING_ACK |
                                 UPLOAD_HANDLE_STATE_ACK_RECEIVED,

    // Session stopping state
    UPLOAD_HANDLE_STATE_SEND_EOS = UPLOAD_HANDLE_STATE_TERMINATING |
                                   UPLOAD_HANDLE_STATE_TERMINATED,

    // States where all items are sent. Currently including UPLOAD_HANDLE_STATE_ERROR because
    // upload handle has to get to UPLOAD_HANDLE_STATE_TERMINATED first then UPLOAD_HANDLE_STATE_ERROR
    UPLOAD_HANDLE_STATE_READY_TO_TRIM = UPLOAD_HANDLE_STATE_ACK_RECEIVED |
                                        UPLOAD_HANDLE_STATE_TERMINATED |
                                        UPLOAD_HANDLE_STATE_ERROR,

    // All states combined
    UPLOAD_HANDLE_STATE_ANY = UPLOAD_HANDLE_STATE_NEW |
                              UPLOAD_HANDLE_STATE_READY |
                              UPLOAD_HANDLE_STATE_STREAMING |
                              UPLOAD_HANDLE_STATE_TERMINATING |
                              UPLOAD_HANDLE_STATE_AWAITING_ACK |
                              UPLOAD_HANDLE_STATE_ACK_RECEIVED |
                              UPLOAD_HANDLE_STATE_TERMINATED |
                              UPLOAD_HANDLE_STATE_ERROR,

} UPLOAD_HANDLE_STATE;

/**
 * Upload connection state enum type definition
 */
typedef enum {
    // No dropped connection
    UPLOAD_CONNECTION_STATE_NONE = (UINT32) 0,

    // Connection state is OK
    UPLOAD_CONNECTION_STATE_OK = (1 << 0),

    // Connection dropped on a new/unused upload handle
    UPLOAD_CONNECTION_STATE_NOT_IN_USE = (1 << 1),

    // Connection dropped on a connection in use
    UPLOAD_CONNECTION_STATE_IN_USE = (1 << 2),
} UPLOAD_CONNECTION_STATE;

/**
 * Whether the upload handle is in sending EoS state
 */
#define IS_UPLOAD_HANDLE_IN_SENDING_EOS_STATE(p)    ((((PUploadHandleInfo)(p))->state & UPLOAD_HANDLE_STATE_SEND_EOS) != UPLOAD_HANDLE_STATE_NONE)

/**
 * Whether the upload handle has sent all its items, including eos.
 */
#define IS_UPLOAD_HANDLE_READY_TO_TRIM(p)    ((((PUploadHandleInfo)(p))->state & UPLOAD_HANDLE_STATE_READY_TO_TRIM) != UPLOAD_HANDLE_STATE_NONE)

/**
 * Upload handle information struct
 */
typedef struct __UploadHandleInfo UploadHandleInfo;
struct __UploadHandleInfo {
    // The upload handle
    UPLOAD_HANDLE handle;

    // Start timestamp
    UINT64 timestamp;

    // Last sent key frame timestamp for tracking last ACKs
    UINT64 lastFragmentTs;

    // Last received persisted ack timestamp
    UINT64 lastPersistedAckTs;

    // Handle state
    UPLOAD_HANDLE_STATE state;
};
typedef __UploadHandleInfo* PUploadHandleInfo;

/**
 * Kinesis Video stream internal structure
 */
typedef struct __KinesisVideoStream KinesisVideoStream;
struct __KinesisVideoStream {
    // Base object
    __KinesisVideoBase base;

    // The reference to the parent object
    PKinesisVideoClient pKinesisVideoClient;

    // The id of the stream
    UINT32 streamId;

    // Content view object for the stream
    PContentView pView;

    // MKV stream generator
    PMkvGenerator pMkvGenerator;

    // Upload handle queue
    PStackQueue pUploadInfoQueue;

    // Stored metadata queue
    PStackQueue pMetadataQueue;

    // Stream Info structure
    StreamInfo streamInfo;

    // Auth info for the streaming
    AuthInfo streamingAuthInfo;

    // Streaming end point - 1 more for the null terminator
    CHAR streamingEndpoint[MAX_URI_CHAR_LEN + 1];

    // New stream timestamp which will be used as current after the rotation in relative timestamp streams.
    UINT64 newSessionTimestamp;

    // New stream item index which will be used to determine when to purge the upload stream
    UINT64 newSessionIndex;

    // The current session index which will be used to get the upload handle map.
    UINT64 curSessionIndex;

    // Describe stream returned status
    STREAM_STATUS streamStatus;

    // Stream state for running/stopping
    UINT64 streamState;

    // Fragment ACK parser for streaming ACKs
    FragmentAckParser fragmentAckParser;

    // Current view item
    CurrentViewItem curViewItem;

    // Current EOS sending tracker
    MetadataTracker eosTracker;

    // Packaged metadata sending tracker.
    MetadataTracker metadataTracker;

    // Indicates whether the connection has been dropped
    UPLOAD_CONNECTION_STATE connectionState;

    // Indicates whether the stream has been stopped
    BOOL streamStopped;

    // Whether the stream is in a grace period for the token rotation.
    BOOL gracePeriod;

    // Flag determining if the last frame was a EoFr
    BOOL eofrFrame;

    // Whether to reset the generator with the next key frame
    BOOL resetGeneratorOnKeyFrame;

    // Time after which should reset the generator with the next key frame
    UINT64 resetGeneratorTime;

    // Diagnostics information to be used with metrics
    KinesisVideoStreamDiagnostics diagnostics;

    // Connection result when the stream was dropped
    SERVICE_CALL_RESULT connectionDroppedResult;

    // Conditional lock for blocking put
    MUTEX bufferAvailabilityLock;

    // Conditional variable for the blocking put
    CVAR bufferAvailabilityCondition;

    // Maximum size of frame observed
    UINT64 maxFrameSizeSeen;
};
typedef __KinesisVideoStream* PKinesisVideoStream;

/**
 * Wrapper around an allocation which stores the application supplied metadata.
 */
typedef struct __SerializedMetadata SerializedMetadata;
struct __SerializedMetadata {
    // Packaged size
    UINT32 packagedSize;

    // Stored name of the metadata
    PCHAR name;

    // Stored value of the metadata
    PCHAR value;

    // Whether the metadata is persistent or not
    BOOL persistent;

    // Whether the metadata has been already "applied". This makes sense only for persistent metadata.
    BOOL applied;

    // The actual strings are stored following the structure
};
typedef SerializedMetadata* PSerializedMetadata;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////

/**
 * Creates a new stream.
 *
 * @param 1 PKinesisVideoClient - Kinesis Video clientobject.
 * @param 2 PStreamInfo - Stream information object.
 * @param 3 PKinesisVideoStream* - OUT - the newly created stream object.
 *
 * @return Status of the function call.
 */
STATUS createStream(PKinesisVideoClient, PStreamInfo, PKinesisVideoStream*);

/**
 * Frees the given stream and deallocates all the objects
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 *
 * @return Status of the function call.
 */
STATUS freeStream(PKinesisVideoStream);

/**
 * Stops the given stream
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 *
 * @return Status of the function call.
 */
STATUS stopStream(PKinesisVideoStream);

/**
 * Puts the frame into the stream. The stream will be started if it hasn't been yet.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 * @param 2 PFrame - The frame to process.
 *
 * @return Status of the function call.
 */
STATUS putFrame(PKinesisVideoStream, PFrame);

/**
 * Puts a metadata into the stream.
 *
 * The metadata will be accumulated and
 * prepended to the cluster that follows.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 * @param 2 PCHAR - The name of the metadata.
 * @param 3 PCHAR - The value of the metadata.
 * @param 4 BOOL - Whether to persist/apply the metadata for all fragments that follow.
 *
 * @return Status of the function call.
 */
STATUS putFragmentMetadata(PKinesisVideoStream, PCHAR, PCHAR, BOOL);

/**
 * Puts the frame into the stream. The stream will be started if it hasn't been yet.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 * @param 2 UINT32 - Codec private data size.
 * @param 3 PBYTE - Codec Private Data bits.
 * @param 4 UINT64 - TrackId from TrackInfo.
 *
 * @return Status of the function call.
 */
STATUS streamFormatChanged(PKinesisVideoStream, UINT32, PBYTE, UINT64);

/**
 * Extracts and returns the stream metrics.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 * @param 2 PStreamMetrics - OUT - Stream metrics to return.
 *
 * @return Status of the function call.
 */
STATUS getStreamMetrics(PKinesisVideoStream, PStreamMetrics);

/**
 * Calculates the max number of items in the content view
 *
 * @param 1 PStreamInfo - Kinesis Video stream info object.
 *
 * @return The max item count in the view.
 */
UINT32 calculateViewItemCount(PStreamInfo);

/**
 * Calculates the buffer duration
 *
 * @param 1 PStreamInfo - Kinesis Video stream object.
 *
 * @return The buffer duration for the view.
 */
UINT64 calculateViewBufferDuration(PStreamInfo);

/**
 * Frees the previously allocated metadata tracking info
 *
 * @param 1 PMetadataTracker - Metadata tracker object.
 */
VOID freeMetadataTracker(PMetadataTracker);

/**
 * Moves the view to the next boundary item - aka the next key frame or fragment start
 */
STATUS getNextBoundaryViewItem(PKinesisVideoStream, PViewItem*);

/**
 * Generates the packager
 */
STATUS createPackager(PKinesisVideoStream, PMkvGenerator*);

/**
 * Checks for the connection staleness
 */
STATUS checkForConnectionStaleness(PKinesisVideoStream, PViewItem);

/**
 * Checks for the streaming token expiration. If the stream token is present and is in the grace period
 * then we will move the state machinery to the get endpoint state.
 */
STATUS checkStreamingTokenExpiration(PKinesisVideoStream);

/**
 * Fixes up the current view item to be a stream start.
 */
STATUS streamStartFixupOnReconnect(PKinesisVideoStream);

/**
 * Fixes up the current view item to remove stream start.
 */
STATUS resetCurrentViewItemStreamStart(PKinesisVideoStream);

/**
 * Frees the specified queue by removing and freeing the data before freeing the queue
 */
STATUS freeStackQueue(PStackQueue);

/**
 * Gets the current stream upload handle info if existing or NULL otherwise.
 *
 * NOTE: This doesn't dequeue the item
 */
PUploadHandleInfo getCurrentStreamUploadInfo(PKinesisVideoStream);

/**
 * Gets the earliest stream info which is in an awaiting ack received state.
 *
 * NOTE: This doesn't dequeue the item
 */
PUploadHandleInfo getAckReceivedStreamUploadInfo(PKinesisVideoStream);

/**
 * Deletes and frees the specified stream upload info object
 */
VOID deleteStreamUploadInfo(PKinesisVideoStream, PUploadHandleInfo);

/**
 * Gets the first upload handle info corresponding to the state and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfoWithState(PKinesisVideoStream, UINT32);

/**
 * Gets the upload handle info corresponding to the specified handle and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfo(PKinesisVideoStream, UPLOAD_HANDLE);

/**
 * Returns the available duration and byte size for streaming, accounting for the partial frame sent and potential EoS
 */
STATUS getAvailableViewSize(PKinesisVideoStream, PUINT64, PUINT64);

/**
 * Await for the frame availability in OFFLINE mode
 *
 * @param 1 - IN - KVS stream object
 * @param 2 - IN - Size of the overall packaged allocation
 * @return Status code of the operation
 */
STATUS waitForAvailability(PKinesisVideoStream, UINT32);

/**
 * Checks whether space is available in the content store and
 * if there is enough duration available in the content view
 */
STATUS checkForAvailability(PKinesisVideoStream, UINT32, PBOOL);

/**
 * Packages the stream metadata.
 *
 * @param 1 - IN - KVS object
 * @param 2 - IN - Current state of the packager/generator
 * @param 3 - IN - Whether to package only the not yet sent metadata
 * @param 4 - IN/OPT - Optional buffer to package to. If NULL then the size will be returned
 * @param 5 - IN/OUT - Size of the buffer when buffer is not NULL, otherwise will return the required size.
 * @return Status code of the operation
 */
STATUS packageStreamMetadata(PKinesisVideoStream, MKV_STREAM_STATE, BOOL, PBYTE, PUINT32);

/**
 * Generates and packages the EOS metadata
 *
 * @param 1 - IN - KVS object
 *
 * @return Status code of the operation
 */
STATUS generateEosMetadata(PKinesisVideoStream);

/**
 * Checks whether there is still metadata that needs to be sent.
 *
 * @param 1 - IN - KVS object
 * @param 2 - OUT - Whether we still have metadata that needs to be sent.
 *
 * @return Status code of the operation
 */
STATUS checkForNotSentMetadata(PKinesisVideoStream, PBOOL);

/**
 * Packages the metadata that hasn't been sent yet.
 *
 * @param 1 - IN - KVS object
 *
 * @return Status code of the operation
 */
STATUS packageNotSentMetadata(PKinesisVideoStream);

/**
 * Appends validated metadata name/value to the queue
 *
 * @param 1 - IN - KVS object
 * @param 2 - IN - Metadata name
 * @param 3 - IN - Metadata value
 * @param 4 - IN - Whether persistent
 * @param 5 - IN - Packaged size of the metadata
 *
 * @return Status code of the operation
 */
STATUS appendValidatedMetadata(PKinesisVideoStream, PCHAR, PCHAR, BOOL, UINT32);

///////////////////////////////////////////////////////////////////////////
// Service call event functions
///////////////////////////////////////////////////////////////////////////
STATUS describeStreamResult(PKinesisVideoStream, SERVICE_CALL_RESULT, PStreamDescription);
STATUS createStreamResult(PKinesisVideoStream, SERVICE_CALL_RESULT, PCHAR);
STATUS getStreamingTokenResult(PKinesisVideoStream, SERVICE_CALL_RESULT, PBYTE, UINT32, UINT64);
STATUS getStreamingEndpointResult(PKinesisVideoStream, SERVICE_CALL_RESULT, PCHAR);
STATUS putStreamResult(PKinesisVideoStream, SERVICE_CALL_RESULT, UPLOAD_HANDLE);
STATUS tagStreamResult(PKinesisVideoStream, SERVICE_CALL_RESULT);
STATUS streamTerminatedEvent(PKinesisVideoStream, UPLOAD_HANDLE, SERVICE_CALL_RESULT);
STATUS serviceCallResultCheck(SERVICE_CALL_RESULT);


///////////////////////////////////////////////////////////////////////////
// ACK related functions
///////////////////////////////////////////////////////////////////////////
STATUS streamFragmentAckEvent(PKinesisVideoStream, UPLOAD_HANDLE, PFragmentAck);
STATUS streamFragmentBufferingAck(PKinesisVideoStream, UINT64);
STATUS streamFragmentReceivedAck(PKinesisVideoStream, UINT64);
STATUS streamFragmentPersistedAck(PKinesisVideoStream, UINT64, PUploadHandleInfo);
STATUS streamFragmentErrorAck(PKinesisVideoStream, UINT64, SERVICE_CALL_RESULT);

///////////////////////////////////////////////////////////////////////////
// Streaming event functions
///////////////////////////////////////////////////////////////////////////
STATUS getStreamData(PKinesisVideoStream, UPLOAD_HANDLE, PBYTE, UINT32, PUINT32);

///////////////////////////////////////////////////////////////////////////
// State machine callback functionality
///////////////////////////////////////////////////////////////////////////
STATUS fromNewStreamState(UINT64, PUINT64);
STATUS fromDescribeStreamState(UINT64, PUINT64);
STATUS fromCreateStreamState(UINT64, PUINT64);
STATUS fromGetEndpointStreamState(UINT64, PUINT64);
STATUS fromGetTokenStreamState(UINT64, PUINT64);
STATUS fromReadyStreamState(UINT64, PUINT64);
STATUS fromPutStreamState(UINT64, PUINT64);
STATUS fromStreamingStreamState(UINT64, PUINT64);
STATUS fromStoppedStreamState(UINT64, PUINT64);
STATUS fromTagStreamState(UINT64, PUINT64);

STATUS executeNewStreamState(UINT64, UINT64);
STATUS executeDescribeStreamState(UINT64, UINT64);
STATUS executeGetEndpointStreamState(UINT64, UINT64);
STATUS executeGetTokenStreamState(UINT64, UINT64);
STATUS executeCreateStreamState(UINT64, UINT64);
STATUS executeReadyStreamState(UINT64, UINT64);
STATUS executePutStreamState(UINT64, UINT64);
STATUS executeStreamingStreamState(UINT64, UINT64);
STATUS executeStoppedStreamState(UINT64, UINT64);
STATUS executeTagStreamState(UINT64, UINT64);

#ifdef __cplusplus
}
#endif

#endif // __KINESIS_VIDEO_STREAM_H__
