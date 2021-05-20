#ifndef __KINESIS_VIDEO__CONNECTION_STALE_STATE_MACHINE_INCLUDE_I__
#define __KINESIS_VIDEO__CONNECTION_STALE_STATE_MACHINE_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define GRACE_PERIOD_CONNECTION_STALE_STATE_MACHINE        (30 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define VERIFICATION_PERIOD_CONNECTION_STALE_STATE_MACHINE (60 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define CONNECTION_STALE_STATE_MACHINE_UPDATE_TIMESTAMP(stateMachine)                                                                                \
    {                                                                                                                                                \
        (stateMachine)->quietTime = (stateMachine)->currTime + GRACE_PERIOD_CONNECTION_STALE_STATE_MACHINE;                                          \
        (stateMachine)->backToNormalTime = (stateMachine)->quietTime + VERIFICATION_PERIOD_CONNECTION_STALE_STATE_MACHINE;                           \
    }

struct __CallbackStateMachine;

struct __ConnectionStaleStateMachine {
    STREAM_CALLBACK_HANDLING_STATE currentState;
    // the time period that state machine would not respond to callbacks
    UINT64 quietTime;
    // the time period after an action is taken to verify if stream status has recovered
    UINT64 backToNormalTime;
    UINT64 currTime;

    struct __CallbackStateMachine* pCallbackStateMachine;
};

////////////////////////////////////////////////////////////////////////
// Setter
////////////////////////////////////////////////////////////////////////
STATUS setConnectionStaleStateMachine(struct __CallbackStateMachine*, STREAM_CALLBACK_HANDLING_STATE, UINT64, UINT64, UINT64);

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////
STATUS connectionStaleStateMachineSetResetConnectionState(STREAM_HANDLE, PConnectionStaleStateMachine);
STATUS connectionStaleStateMachineHandleConnectionStale(STREAM_HANDLE, PConnectionStaleStateMachine);

#ifdef __cplusplus
}
#endif

#endif //__KINESIS_VIDEO__CONNECTION_STALE_STATE_MACHINE_INCLUDE_I__
