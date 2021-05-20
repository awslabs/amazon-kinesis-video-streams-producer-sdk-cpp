/**
 * Kinesis Video Producer Connection Staleness State Machine
 */
#define LOG_CLASS "ConnectionStaleStateMachine"
#include "Include_i.h"

STATUS setConnectionStaleStateMachine(PCallbackStateMachine pCallbackStateMachine, STREAM_CALLBACK_HANDLING_STATE currState, UINT64 currTime,
                                      UINT64 quietTime, UINT64 backToNormalTime)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pCallbackStateMachine != NULL, STATUS_NULL_ARG);

    // Set the version, self
    pCallbackStateMachine->connectionStaleStateMachine.currentState = currState;
    pCallbackStateMachine->connectionStaleStateMachine.currTime = currTime;
    pCallbackStateMachine->connectionStaleStateMachine.backToNormalTime = backToNormalTime;
    pCallbackStateMachine->connectionStaleStateMachine.quietTime = quietTime;
    pCallbackStateMachine->connectionStaleStateMachine.pCallbackStateMachine = pCallbackStateMachine;

CleanUp:
    LEAVES();
    return retStatus;
}

STATUS connectionStaleStateMachineSetResetConnectionState(STREAM_HANDLE streamHandle, PConnectionStaleStateMachine pConnectionStaleStateMachine)
{
    STATUS retStatus = STATUS_SUCCESS;

    DLOGD("Connection Stale State Machine move to RESET_CONNECTION_STATE");

    pConnectionStaleStateMachine->currentState = STREAM_CALLBACK_HANDLING_STATE_RESET_CONNECTION_STATE;
    CONNECTION_STALE_STATE_MACHINE_UPDATE_TIMESTAMP(pConnectionStaleStateMachine);

    if (pConnectionStaleStateMachine->pCallbackStateMachine->streamReady) {
        CHK_STATUS(kinesisVideoStreamResetConnection(streamHandle));
    } else {
        DLOGW("Stream not ready.");
    }

CleanUp:
    return retStatus;
}

STATUS connectionStaleStateMachineHandleConnectionStale(STREAM_HANDLE streamHandle, PConnectionStaleStateMachine pStaleStateMachine)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider;

    CHK(pStaleStateMachine != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pStaleStateMachine->pCallbackStateMachine->pContinuousRetryStreamCallbacks->pCallbacksProvider;
    pStaleStateMachine->currTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    DLOGS("currTime: %" PRIu64 ", quietTime: %" PRIu64 ", backToNormalTime: %" PRIu64 "", pStaleStateMachine->currTime, pStaleStateMachine->quietTime,
          pStaleStateMachine->backToNormalTime);
    if (pStaleStateMachine->quietTime < pStaleStateMachine->currTime) {
        switch (pStaleStateMachine->currentState) {
            case STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE:
                DLOGD("Connection Stale State Machine starting from NORMAL_STATE");
                CHK_STATUS(connectionStaleStateMachineSetResetConnectionState(streamHandle, pStaleStateMachine));
                break;
            case STREAM_CALLBACK_HANDLING_STATE_RESET_CONNECTION_STATE:
                DLOGD("Connection Stale State Machine starting from RESET_CONNECTION_STATE");
                CONNECTION_STALE_STATE_MACHINE_UPDATE_TIMESTAMP(pStaleStateMachine);
                break;
            default:
                break;
        }
    }

CleanUp:

    return retStatus;
}