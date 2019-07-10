/**
 * State machine functionality
 */

#define LOG_CLASS "State"
#include "Include_i.h"

/**
 * Creates a new state machine
 */
STATUS createStateMachine(PStateMachineState pStates,
                          UINT32 stateCount,
                          UINT64 customData,
                          GetCurrentTimeFunc getCurrentTimeFunc,
                          UINT64 getCurrentTimeFuncCustomData,
                          PStateMachine* ppStateMachine)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachine pStateMachine = NULL;
    UINT32 allocationSize = 0;

    CHK(pStates != NULL && ppStateMachine != NULL && getCurrentTimeFunc != NULL, STATUS_NULL_ARG);
    CHK(stateCount > 0, STATUS_INVALID_ARG);

    // Allocate the main struct with an array of stream pointers at the end
    // NOTE: The calloc will Zero the fields
    allocationSize = SIZEOF(StateMachine) + SIZEOF(StateMachineState) * stateCount;
    pStateMachine = (PStateMachine) MEMCALLOC(1, allocationSize);
    CHK(pStateMachine != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the values
    pStateMachine->getCurrentTimeFunc = getCurrentTimeFunc;
    pStateMachine->getCurrentTimeFuncCustomData = getCurrentTimeFuncCustomData;
    pStateMachine->stateCount = stateCount;
    pStateMachine->customData = customData;

    // Set the states pointer and copy the globals
    pStateMachine->states = (PStateMachineState)(pStateMachine + 1);

    // Copy the states over
    MEMCPY(pStateMachine->states, pStates, SIZEOF(StateMachineState) * stateCount);

    // NOTE: Set the initial state as the first state.
    pStateMachine->context.pCurrentState = pStateMachine->states;

    // Assign the created object
    *ppStateMachine = pStateMachine;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeStateMachine(pStateMachine);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the state machine object
 */
STATUS freeStateMachine(PStateMachine pStateMachine)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Call is idempotent
    CHK(pStateMachine != NULL, retStatus);

    // Release the object
    MEMFREE(pStateMachine);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets a pointer to the state object given it's state
 */
STATUS getStateMachineState(PStateMachine pStateMachine, UINT64 state, PStateMachineState* ppState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState = NULL;
    UINT32 i;

    CHK(pStateMachine != NULL && ppState, STATUS_NULL_ARG);

    // Iterate over and find the first state
    for (i = 0; pState == NULL && i < pStateMachine->stateCount; i++) {
        if (pStateMachine->states[i].state == state) {
            pState = &pStateMachine->states[i];
        }
    }

    // Check if found
    CHK(pState != NULL, STATUS_STATE_MACHINE_STATE_NOT_FOUND);

    // Assign the object which might be NULL if we didn't find any
    *ppState = pState;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Transition the state machine given it's context
 */
STATUS stepStateMachine(PStateMachine pStateMachine)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState = NULL;
    UINT64 nextState, time;
    UINT64 customData = pStateMachine->customData;

    CHK(pStateMachine != NULL, STATUS_NULL_ARG);

    // Get the next state
    CHK(pStateMachine->context.pCurrentState->getNextStateFn != NULL, STATUS_NULL_ARG);
    CHK_STATUS(pStateMachine->context.pCurrentState->getNextStateFn(pStateMachine->customData, &nextState));

    // Validate if the next state can accept the current state before transitioning
    CHK_STATUS(getStateMachineState(pStateMachine, nextState, &pState));

    DLOGD("PIC %s State Machine - Current state: 0x%x, Next state: 0x%x",
          KINESIS_VIDEO_OBJECT_IDENTIFIER_FROM_CUSTOM_DATA(customData) == KINESIS_VIDEO_OBJECT_IDENTIFIER_CLIENT ? "Client" : "Stream",
          pStateMachine->context.pCurrentState->state, nextState);

    CHK((pState->acceptStates & pStateMachine->context.pCurrentState->state) == pStateMachine->context.pCurrentState->state, STATUS_INVALID_STREAM_STATE);

    // Clear the iteration info if a different state and transition the state
    time = pStateMachine->getCurrentTimeFunc(pStateMachine->getCurrentTimeFuncCustomData);

    // Check if we are changing the state
    if (pState->state != pStateMachine->context.pCurrentState->state) {
        // Clear the iteration data
        pStateMachine->context.retryCount = 0;
        pStateMachine->context.time = time;
    } else {
        // Increment the state retry count
        pStateMachine->context.retryCount++;
        pStateMachine->context.time = time + NEXT_SERVICE_CALL_RETRY_DELAY(pStateMachine->context.retryCount);

        // Check if we have tried enough times
        if (pState->retry != INFINITE_RETRY_COUNT_SENTINEL) {
            CHK(pStateMachine->context.retryCount <= pState->retry, pState->status);
        }
    }

    pStateMachine->context.pCurrentState = pState;

    // Execute the state function if specified
    if (pStateMachine->context.pCurrentState->executeStateFn != NULL) {
        CHK_STATUS(pStateMachine->context.pCurrentState->executeStateFn(pStateMachine->customData,
                                                                        pStateMachine->context.time));
    }

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Checks whether the state machine state is accepted state
 */
STATUS acceptStateMachineState(PStateMachine pStateMachine, UINT64 requiredStates)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pStateMachine != NULL, STATUS_NULL_ARG);

    // Check the current state
    CHK((requiredStates & pStateMachine->context.pCurrentState->state) == pStateMachine->context.pCurrentState->state, STATUS_INVALID_STREAM_STATE);

CleanUp:

    LEAVES();
    return retStatus;
}
