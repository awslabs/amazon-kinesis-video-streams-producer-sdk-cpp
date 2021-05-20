#ifndef __KINESIS_VIDEO_STREAM_LATENCY_STATE_MACHINE_INCLUDE_I__
#define __KINESIS_VIDEO_STREAM_LATENCY_STATE_MACHINE_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define GRACE_PERIOD_STREAM_LATENCY_STATE_MACHINE        (30 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define VERIFICATION_PERIOD_STREAM_LATENCY_STATE_MACHINE (60 * HUNDREDS_OF_NANOS_IN_A_SECOND)

#define STREAM_LATENCY_STATE_MACHINE_UPDATE_TIMESTAMP(stateMachine)                                                                                  \
    {                                                                                                                                                \
        (stateMachine)->quietTime = (stateMachine)->currTime + GRACE_PERIOD_STREAM_LATENCY_STATE_MACHINE;                                            \
        (stateMachine)->backToNormalTime = (stateMachine)->quietTime + VERIFICATION_PERIOD_STREAM_LATENCY_STATE_MACHINE;                             \
    }

struct __CallbackStateMachine;

struct __StreamLatencyStateMachine {
    STREAM_CALLBACK_HANDLING_STATE currentState;
    UINT64 currTime;
    // the time period that state machine would not respond to callbacks
    UINT64 quietTime;
    // the time period after an action is taken to verify if stream status has recovered
    UINT64 backToNormalTime;

    struct __CallbackStateMachine* pCallbackStateMachine;
};

////////////////////////////////////////////////////////////////////////
// Setter
////////////////////////////////////////////////////////////////////////
STATUS setStreamLatencyStateMachine(struct __CallbackStateMachine*, STREAM_CALLBACK_HANDLING_STATE, UINT64, UINT64, UINT64);

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

STATUS streamLatencyStateMachineHandleStreamLatency(STREAM_HANDLE, PStreamLatencyStateMachine);
STATUS streamLatencyStateMachineToResetConnectionState(STREAM_HANDLE, PStreamLatencyStateMachine);
VOID streamLatencyStateMachineSetThrottlePipelineState(PStreamLatencyStateMachine);
VOID streamLatencyStateMachineSetInfiniteRetryState(PStreamLatencyStateMachine);
STATUS streamLatencyStateMachineDoInfiniteRetry(STREAM_HANDLE, PStreamLatencyStateMachine);

#ifdef __cplusplus
}
#endif

#endif //__KINESIS_VIDEO_STREAM_LATENCY_STATE_MACHINE_INCLUDE_I__
