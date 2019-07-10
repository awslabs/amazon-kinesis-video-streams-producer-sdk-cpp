/*******************************************
* Kinesis Video client state include file
*******************************************/
#ifndef __KINESIS_VIDEO_FSA_STATE_H__
#define __KINESIS_VIDEO_FSA_STATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

// Indicates infinite retries
#define INFINITE_RETRY_COUNT_SENTINEL               0

/**
 * State transition function definitions
 *
 * @param 1 UINT64 - Custom data passed in
 * @param 2 PUINT64 - OUT - Returned next state on success
 *
 * @return Status of the callback
 */
typedef STATUS (*GetNextStateFunc)(UINT64, PUINT64);

/**
 * State execution function definitions
 *
 * @param 1 - Custom data passed in
 * @param 2 - Time after which to execute the function
 *
 * @return Status of the callback
 */
typedef STATUS (*ExecuteStateFunc)(UINT64, UINT64);

/**
 * State Machine state
 */
typedef struct __StateMachineState StateMachineState;
struct __StateMachineState {
    UINT64 state;
    UINT64 acceptStates;
    GetNextStateFunc getNextStateFn;
    ExecuteStateFunc executeStateFn;
    UINT32 retry;
    STATUS status;
};
typedef struct __StateMachineState* PStateMachineState;

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

/**
 * State Machine context
 */
typedef struct __StateMachineContext StateMachineContext;
struct __StateMachineContext {
    PStateMachineState pCurrentState;
    UINT32 retryCount;
    UINT64 time;
};
typedef struct __StateMachineContext* PStateMachineContext;

/**
 * State Machine definition
 */
struct __StateMachine {
    GetCurrentTimeFunc getCurrentTimeFunc;
    UINT64 getCurrentTimeFuncCustomData;
    UINT64 customData;
    StateMachineContext context;
    UINT32 stateCount;
    PStateMachineState states;
};

///////////////////////////////////////////////////////////////////////////////////////
// Functionality
///////////////////////////////////////////////////////////////////////////////////////
STATUS createStateMachine(PStateMachineState, UINT32, UINT64, GetCurrentTimeFunc, UINT64, PStateMachine*);
STATUS freeStateMachine(PStateMachine);
STATUS getStateMachineState(PStateMachine, UINT64, PStateMachineState*);
STATUS stepStateMachine(PStateMachine);
STATUS acceptStateMachineState(PStateMachine, UINT64);

#ifdef __cplusplus
}
#endif

#endif // __KINESIS_VIDEO_FSA_STATE_H__
