#ifndef __KINESIS_VIDEO_CONTINUOUS_RETRY_INCLUDE_I__
#define __KINESIS_VIDEO_CONTINUOUS_RETRY_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing


struct __CallbackStateMachine;
struct __CallbacksProvider;

////////////////////////////////////////////////////////////////////////
// Struct definition
////////////////////////////////////////////////////////////////////////
typedef struct __ContinuousRetryStreamCallbacks ContinuousRetryStreamCallbacks;
struct __ContinuousRetryStreamCallbacks {
    // First member should be the stream callbacks
    StreamCallbacks streamCallbacks;

    // Back pointer to the callback provider object
    struct __CallbacksProvider* pCallbacksProvider;

    // Lock guarding the mapping table
    MUTEX mappingLock;

    // Streams state machine table stream handle -> callback state machine
    PHashTable pStreamMapping;
};
typedef struct __ContinuousRetryStreamCallbacks* PContinuousRetryStreamCallbacks;

struct __StreamLatencyStateMachine;
struct __ConnectionStaleStateMachine;

typedef struct __CallbackStateMachine {
    struct __StreamLatencyStateMachine streamLatencyStateMachine;
    struct __ConnectionStaleStateMachine connectionStaleStateMachine;
    volatile BOOL streamReady;
    PContinuousRetryStreamCallbacks pContinuousRetryStreamCallbacks;
} CallbackStateMachine, *PCallbackStateMachine;

////////////////////////////////////////////////////////////////////////
// Auxiliary functionality
////////////////////////////////////////////////////////////////////////
STATUS removeMappingEntryCallback(UINT64, PHashEntry);
STATUS freeStreamMapping(PContinuousRetryStreamCallbacks, STREAM_HANDLE, BOOL);
STATUS getStreamMapping(PContinuousRetryStreamCallbacks, STREAM_HANDLE, PCallbackStateMachine*);
PVOID continuousRetryStreamRestartHandler(PVOID);

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////
STATUS continuousRetryStreamConnectionStaleHandler(UINT64, STREAM_HANDLE, UINT64);
STATUS continuousRetryStreamErrorReportHandler(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, STATUS);
STATUS continuousRetryStreamLatencyPressureHandler(UINT64, STREAM_HANDLE, UINT64);
STATUS continuousRetryStreamReadyHandler(UINT64, STREAM_HANDLE);
STATUS continuousRetryStreamFreeHandler(PUINT64);
STATUS continuousRetryStreamShutdownHandler(UINT64, STREAM_HANDLE, BOOL);
STATUS continuousRetryStreamClosedHandler(UINT64, STREAM_HANDLE, UPLOAD_HANDLE);



#ifdef  __cplusplus
}
#endif

#endif //__KINESIS_VIDEO_CONTINUOUS_RETRY_INCLUDE_I__
