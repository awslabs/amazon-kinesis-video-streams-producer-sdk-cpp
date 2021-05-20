/**
 * Implementation of a stream events
 */

#define LOG_CLASS "ClientEvent"
#include "Include_i.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Client event functionality
/////////////////////////////////////////////////////////////////////////////////////////
/**
 * Describe stream result event func
 */
STATUS createDeviceResult(PKinesisVideoClient pKinesisVideoClient, SERVICE_CALL_RESULT callResult, PCHAR deviceArn)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    BOOL locked = FALSE;

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoClient->base.pStateMachine, CLIENT_STATE_CREATE, &pState));

    // Check if we are in the correct state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoClient->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR || retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
            retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR,
        retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoClient->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        // Should have the arn
        CHK(deviceArn != NULL, STATUS_INVALID_CREATE_DEVICE_RESPONSE);

        // Validate and store the data
        CHK(deviceArn != NULL && STRNLEN(deviceArn, MAX_ARN_LEN + 1) <= MAX_ARN_LEN, STATUS_INVALID_CREATE_DEVICE_RESPONSE);
        STRNCPY(pKinesisVideoClient->base.arn, deviceArn, MAX_ARN_LEN);
        pKinesisVideoClient->base.arn[MAX_ARN_LEN] = '\0';
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Tag client result event func
 */
STATUS tagClientResult(PKinesisVideoClient pKinesisVideoClient, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    BOOL locked = FALSE;

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoClient->base.pStateMachine, CLIENT_STATE_TAG_CLIENT, &pState));

    // Check if we are in the right state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoClient->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR || retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoClient->base.result = callResult;

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    LEAVES();
    return retStatus;
}

STATUS deviceCertToTokenResult(PKinesisVideoClient pKinesisVideoClient, SERVICE_CALL_RESULT callResult, PBYTE pToken, UINT32 tokenSize,
                               UINT64 expiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    BOOL locked = FALSE;

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoClient->base.pStateMachine, CLIENT_STATE_GET_TOKEN, &pState));

    // Check if we are in the right state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoClient->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR || retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoClient->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        CHK(tokenSize <= MAX_AUTH_LEN, STATUS_INVALID_AUTH_LEN);
        pKinesisVideoClient->tokenAuthInfo.version = AUTH_INFO_CURRENT_VERSION;
        pKinesisVideoClient->tokenAuthInfo.expiration = expiration;
        pKinesisVideoClient->tokenAuthInfo.size = tokenSize;
        if (pToken == NULL || tokenSize == 0) {
            pKinesisVideoClient->tokenAuthInfo.type = AUTH_INFO_NONE;
        } else {
            pKinesisVideoClient->tokenAuthInfo.type = AUTH_INFO_TYPE_STS;
            MEMCPY(pKinesisVideoClient->tokenAuthInfo.data, pToken, tokenSize);
        }
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    LEAVES();
    return retStatus;
}
