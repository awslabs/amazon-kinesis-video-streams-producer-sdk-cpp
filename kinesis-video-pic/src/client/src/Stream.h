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

    // Terminated
    UPLOAD_HANDLE_STATE_TERMINATED = (1 << 4),

    // Error terminated
    UPLOAD_HANDLE_STATE_ERROR = (1 << 5),

    // All states combined
    UPLOAD_HANDLE_STATE_ANY =  UPLOAD_HANDLE_STATE_NEW |
            UPLOAD_HANDLE_STATE_READY |
            UPLOAD_HANDLE_STATE_STREAMING |
            UPLOAD_HANDLE_STATE_TERMINATING |
            UPLOAD_HANDLE_STATE_TERMINATED |
            UPLOAD_HANDLE_STATE_ERROR,

} UPLOAD_HANDLE_STATE;

/**
 * Upload handle information struct
 */
typedef struct __UploadHandleInfo UploadHandleInfo;
struct __UploadHandleInfo {
    // The upload handle
    UPLOAD_HANDLE handle;

    // Start timestamp
    UINT64 timestamp;

    // Content view item index when the session started
    UINT64 startIndex;

    // Content view item index when the session ended
    UINT64 endIndex;

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

    // Stream Info structure
    StreamInfo streamInfo;

    // Auth info for the streaming
    AuthInfo streamingAuthInfo;

    // Streaming end point - 1 more for the null terminator
    CHAR streamingEndpoint[MAX_URI_CHAR_LEN + 1];

    // New stream timestamp which will be used as current after the rotation in relative timestamp streams.
    UINT64 newSessionTimestamp;

    // New stream item index which will be used to determine when to purge the upload stream/
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

    // Indicates whether the connection has been dropped
    BOOL connectionDropped;

    // Indicates whether the connection has been dropped
    BOOL retryingOnRotation;

    // Indicates whether the stream has been stopped
    BOOL streamStopped;

    // Whether the stream is in a grace period for the token rotation.
    BOOL gracePeriod;

    // Whether to reset the generator with the next key frame
    BOOL resetGeneratorOnKeyFrame;

    // Diagnostics information to be used with metrics
    KinesisVideoStreamDiagnostics diagnostics;

    // Connection result when the stream was dropped
    SERVICE_CALL_RESULT connectionDroppedResult;
};
typedef __KinesisVideoStream* PKinesisVideoStream;

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
 * Puts the frame into the stream. The stream will be started if it hasn't been yet.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object.
 * @param 2 UINT32 - Codec private data size.
 * @param 3 PBYTE - Codec Private Data bits.
 *
 * @return Status of the function call.
 */
STATUS streamFormatChanged(PKinesisVideoStream, UINT32, PBYTE);

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
 * Frees the previously allocated CPD if any.
 *
 * @param 1 PStreamInfo - Kinesis Video stream object.
 */
VOID freeCodecPrivateData(PKinesisVideoStream pKinesisVideoStream);

/**
 * Moves the view to the next boundary item - aka the next key frame or fragment start
 */
STATUS getNextBoundaryViewItem(PKinesisVideoStream, PViewItem*);

/**
 * Generates the packager
 */
STATUS createPackager(PStreamInfo, GetCurrentTimeFunc, UINT64, PMkvGenerator*);

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
 * Frees the upload handle info queue
 */
STATUS freeUploadInfoQueue(PKinesisVideoStream);

/**
 * Gets the current stream upload handle info if existing or NULL otherwise.
 *
 * NOTE: This doesn't dequeue the item
 */
PUploadHandleInfo getCurrentStreamUploadInfo(PKinesisVideoStream);

/**
 * Deletes and frees the specified stream upload info object
 */
VOID deleteStreamUploadInfo(PKinesisVideoStream, PUploadHandleInfo);

/**
 * Gets the first upload handle info corresponding to the state and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfoWithState(PKinesisVideoStream, UINT32);

/**
 * Gets the first upload handle info corresponding to the index and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfoWithEndIndex(PKinesisVideoStream, UINT64);

/**
 * Gets the upload handle info corresponding to the specified handle and NULL otherwise
 */
PUploadHandleInfo getStreamUploadInfo(PKinesisVideoStream, UPLOAD_HANDLE);

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
STATUS streamFragmentPersistedAck(PKinesisVideoStream, UINT64);
STATUS streamFragmentErrorAck(PKinesisVideoStream, UINT64, SERVICE_CALL_RESULT);

///////////////////////////////////////////////////////////////////////////
// Streaming event functions
///////////////////////////////////////////////////////////////////////////
STATUS getStreamData(PKinesisVideoStream, PUINT64, PBYTE, UINT32, PUINT32);

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
