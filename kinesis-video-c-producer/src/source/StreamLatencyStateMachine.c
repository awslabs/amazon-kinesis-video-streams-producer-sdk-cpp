/**
 * Kinesis Video Producer Stream Latency State Machine
 */
#define LOG_CLASS "StreamLatencyStateMachine"
#include "Include_i.h"

STATUS setStreamLatencyStateMachine(PCallbackStateMachine pCallbackStateMachine, STREAM_CALLBACK_HANDLING_STATE currState, UINT64 currTime,
                                    UINT64 quietTime, UINT64 backToNormalTime)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pCallbackStateMachine != NULL, STATUS_NULL_ARG);

    // Set the version, self
    pCallbackStateMachine->streamLatencyStateMachine.currentState = currState;
    pCallbackStateMachine->streamLatencyStateMachine.currTime = currTime;
    pCallbackStateMachine->streamLatencyStateMachine.backToNormalTime = backToNormalTime;
    pCallbackStateMachine->streamLatencyStateMachine.quietTime = quietTime;
    pCallbackStateMachine->streamLatencyStateMachine.pCallbackStateMachine = pCallbackStateMachine;

CleanUp:
    LEAVES();
    return retStatus;
}

STATUS streamLatencyStateMachineToResetConnectionState(STREAM_HANDLE streamHandle, PStreamLatencyStateMachine pStreamLatencyStateMachine)
{
    STATUS retStatus = STATUS_SUCCESS;
    CHK(pStreamLatencyStateMachine != NULL, STATUS_NULL_ARG);

    DLOGD("Stream Latency State Machine move to RESET_CONNECTION_STATE");

    pStreamLatencyStateMachine->currentState = STREAM_CALLBACK_HANDLING_STATE_RESET_CONNECTION_STATE;
    STREAM_LATENCY_STATE_MACHINE_UPDATE_TIMESTAMP(pStreamLatencyStateMachine);
    if (pStreamLatencyStateMachine->pCallbackStateMachine->streamReady) {
        kinesisVideoStreamResetConnection(streamHandle);
    } else {
        DLOGW("Stream not ready.");
    }

CleanUp:

    return retStatus;
}

VOID streamLatencyStateMachineSetThrottlePipelineState(PStreamLatencyStateMachine pStreamLatencyStateMachine)
{
    DLOGD("Stream Latency State Machine move to THROTTLE_PIPELINE_STATE");
    pStreamLatencyStateMachine->currentState = STREAM_CALLBACK_HANDLING_STATE_THROTTLE_PIPELINE_STATE;
    STREAM_LATENCY_STATE_MACHINE_UPDATE_TIMESTAMP(pStreamLatencyStateMachine);

    // no-op for now. Should send qos event to upstream
}

VOID streamLatencyStateMachineSetInfiniteRetryState(PStreamLatencyStateMachine pStreamLatencyStateMachine)
{
    DLOGD("Stream Latency State Machine move to ABORTING_STATE");
    pStreamLatencyStateMachine->currentState = STREAM_CALLBACK_HANDLING_STATE_INFINITE_RETRY_STATE;
}

STATUS streamLatencyStateMachineDoInfiniteRetry(STREAM_HANDLE streamHandle, PStreamLatencyStateMachine pStreamLatencyStateMachine)
{
    STATUS retStatus = STATUS_SUCCESS;

    STREAM_LATENCY_STATE_MACHINE_UPDATE_TIMESTAMP(pStreamLatencyStateMachine);
    if (pStreamLatencyStateMachine->pCallbackStateMachine->streamReady) {
        CHK_STATUS(kinesisVideoStreamResetConnection(streamHandle));
    } else {
        DLOGW("Stream not ready.");
    }

CleanUp:
    return retStatus;
}

STATUS streamLatencyStateMachineHandleStreamLatency(STREAM_HANDLE streamHandle, PStreamLatencyStateMachine pStreamLatencyStateMachine)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider;

    CHK(pStreamLatencyStateMachine != NULL, STATUS_NULL_ARG);

    pCallbacksProvider = pStreamLatencyStateMachine->pCallbackStateMachine->pContinuousRetryStreamCallbacks->pCallbacksProvider;
    pStreamLatencyStateMachine->currTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);
    DLOGS("currTime: %" PRIu64 ", quietTime: %" PRIu64 ", backToNormalTime: %" PRIu64 "", pStreamLatencyStateMachine->currTime,
          pStreamLatencyStateMachine->quietTime, pStreamLatencyStateMachine->backToNormalTime);
    if (pStreamLatencyStateMachine->currTime > pStreamLatencyStateMachine->backToNormalTime) {
        pStreamLatencyStateMachine->currentState = STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE;
    }

    if (pStreamLatencyStateMachine->currTime > pStreamLatencyStateMachine->quietTime) {
        switch (pStreamLatencyStateMachine->currentState) {
            case STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE:
                DLOGD("Stream Latency State Machine starting from NORMAL_STATE");
                CHK_STATUS(streamLatencyStateMachineToResetConnectionState(streamHandle, pStreamLatencyStateMachine));
                break;
            case STREAM_CALLBACK_HANDLING_STATE_RESET_CONNECTION_STATE:
                DLOGD("Stream Latency State Machine starting from RESET_CONNECTION_STATE");
                streamLatencyStateMachineSetThrottlePipelineState(pStreamLatencyStateMachine);
                break;
            case STREAM_CALLBACK_HANDLING_STATE_THROTTLE_PIPELINE_STATE:
                DLOGD("Stream Latency State Machine starting from THROTTLE_PIPELINE_STATE");
                streamLatencyStateMachineSetInfiniteRetryState(pStreamLatencyStateMachine);
                break;
            case STREAM_CALLBACK_HANDLING_STATE_INFINITE_RETRY_STATE:
                DLOGD("Stream Latency State Machine starting from INFINITE_RETRY_STATE");
                CHK_STATUS(streamLatencyStateMachineDoInfiniteRetry(streamHandle, pStreamLatencyStateMachine));
                break;
            default:
                break;
        }
    }

CleanUp:
    return retStatus;
}
