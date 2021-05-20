/**
 * Implementation of a client states callbacks
 */

#define LOG_CLASS "ClientState"
#include "Include_i.h"

/**
 * Static definitions of the states
 */
StateMachineState CLIENT_STATE_MACHINE_STATES[] = {
    {CLIENT_STATE_NEW, CLIENT_STATE_NONE | CLIENT_STATE_NEW, fromNewClientState, executeNewClientState, INFINITE_RETRY_COUNT_SENTINEL,
     STATUS_INVALID_CLIENT_READY_STATE},
    {CLIENT_STATE_AUTH, CLIENT_STATE_READY | CLIENT_STATE_NEW | CLIENT_STATE_AUTH, fromAuthClientState, executeAuthClientState,
     SERVICE_CALL_MAX_RETRY_COUNT, STATUS_CLIENT_AUTH_CALL_FAILED},
    {CLIENT_STATE_GET_TOKEN, CLIENT_STATE_AUTH | CLIENT_STATE_PROVISION | CLIENT_STATE_GET_TOKEN, fromGetTokenClientState, executeGetTokenClientState,
     SERVICE_CALL_MAX_RETRY_COUNT, STATUS_GET_CLIENT_TOKEN_CALL_FAILED},
    {CLIENT_STATE_PROVISION, CLIENT_STATE_AUTH | CLIENT_STATE_PROVISION, fromProvisionClientState, executeProvisionClientState,
     SERVICE_CALL_MAX_RETRY_COUNT, STATUS_CLIENT_PROVISION_CALL_FAILED},
    {CLIENT_STATE_CREATE, CLIENT_STATE_PROVISION | CLIENT_STATE_GET_TOKEN | CLIENT_STATE_AUTH | CLIENT_STATE_CREATE, fromCreateClientState,
     executeCreateClientState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_CREATE_CLIENT_CALL_FAILED},
    {CLIENT_STATE_TAG_CLIENT, CLIENT_STATE_CREATE | CLIENT_STATE_TAG_CLIENT | CLIENT_STATE_READY, fromTagClientState, executeTagClientState,
     SERVICE_CALL_MAX_RETRY_COUNT, STATUS_TAG_CLIENT_CALL_FAILED},
    {CLIENT_STATE_READY, CLIENT_STATE_GET_TOKEN | CLIENT_STATE_AUTH | CLIENT_STATE_TAG_CLIENT | CLIENT_STATE_CREATE | CLIENT_STATE_READY,
     fromReadyClientState, executeReadyClientState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_CLIENT_READY_CALLBACK_FAILED},
};

UINT32 CLIENT_STATE_MACHINE_STATE_COUNT = SIZEOF(CLIENT_STATE_MACHINE_STATES) / SIZEOF(StateMachineState);

// Helper method for stepping the client state machine
STATUS stepClientStateMachine(PKinesisVideoClient pKinesisVideoClient)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL clientLocked = FALSE;

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Interlock the state stepping
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = TRUE;

    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    clientLocked = FALSE;

CleanUp:

    if (clientLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->base.lock);
    }

    LEAVES();
    return retStatus;
}

///////////////////////////////////////////////////////////////////////////
// State machine callback functions
///////////////////////////////////////////////////////////////////////////
STATUS fromNewClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to auth state
    state = CLIENT_STATE_AUTH;
    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeNewClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Nothing to do

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromAuthClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_AUTH, currentTime;
    AUTH_INFO_TYPE authType;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // Auth integration with the client application
    authType = getCurrentAuthType(pKinesisVideoClient);

    switch (authType) {
        case AUTH_INFO_UNDEFINED:
            // If the authentication is not provided then we need to run provisioning
            // NOTE: Currently, this is a serial operation with callbacks which
            // might run long in case of a need to do provisioning.
            // The client should not have been in a ready state
            CHK(!pKinesisVideoClient->clientReady, STATUS_INVALID_CLIENT_READY_STATE);
            state = CLIENT_STATE_PROVISION;
            break;

        case AUTH_INFO_TYPE_STS:
            currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
            if (currentTime >= pKinesisVideoClient->tokenAuthInfo.expiration ||
                (pKinesisVideoClient->tokenAuthInfo.expiration - currentTime) < MIN_STREAMING_TOKEN_EXPIRATION_DURATION) {
                DLOGW("Invalid auth token as it is expiring in less than %u seconds",
                      MIN_STREAMING_TOKEN_EXPIRATION_DURATION / HUNDREDS_OF_NANOS_IN_A_SECOND);
                state = CLIENT_STATE_AUTH;
                break;
            }

            // Deliberate fall-through
        case AUTH_INFO_NONE:
            // Token integration - proceed with create state if we are creating a new client, otherwise, move to ready
            if (pKinesisVideoClient->clientReady) {
                state = CLIENT_STATE_READY;
            } else {
                state = CLIENT_STATE_CREATE;
            }

            break;

        case AUTH_INFO_TYPE_CERT:
            // Certificate integration - proceed with getting token if we are creating a new client,
            state = CLIENT_STATE_GET_TOKEN;
            break;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeAuthClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Auth integration with the client application
    CHK_STATUS(getAuthInfo(pKinesisVideoClient));

    // Step the state machine as we need the execution quanta.
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromProvisionClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_PROVISION;
    AUTH_INFO_TYPE authType = AUTH_INFO_UNDEFINED;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // Auth integration with the client application
    authType = getCurrentAuthType(pKinesisVideoClient);

    // If the authentication is not provided then we need to run provisioning
    switch (authType) {
        case AUTH_INFO_UNDEFINED:
            // NOTE: Currently, this is a serial operation with callbacks which
            // might run long in case of a need to do provisioning.
            state = CLIENT_STATE_PROVISION;
            break;

        case AUTH_INFO_NONE:
            // Deliberate fall-through
        case AUTH_INFO_TYPE_STS:
            // Token integration - proceed with create state
            state = CLIENT_STATE_CREATE;
            break;

        case AUTH_INFO_TYPE_CERT:
            // Certificate integration - proceed with getting token
            state = CLIENT_STATE_GET_TOKEN;
            break;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeProvisionClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // Try to provision the device
    CHK_STATUS(provisionKinesisVideoProducer(pKinesisVideoClient));

    // Step the state machine as we need the execution quanta.
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromGetTokenClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_GET_TOKEN;
    AUTH_INFO_TYPE authType;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // If the call succeeds and we have the token or auth none then we are ready to move on
    authType = getCurrentAuthType(pKinesisVideoClient);
    if (pKinesisVideoClient->base.result == SERVICE_CALL_RESULT_OK && (authType == AUTH_INFO_TYPE_STS || authType == AUTH_INFO_NONE)) {
        // Move the create client state if we are creating a new client, otherwise, move to ready state
        if (pKinesisVideoClient->clientReady) {
            state = CLIENT_STATE_READY;
        } else {
            state = CLIENT_STATE_CREATE;
        }
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeGetTokenClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->certAuthInfo;
    pKinesisVideoClient->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoClient->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoClient);
    pKinesisVideoClient->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoClient->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API if specified. Raise and error at this stage if not.
    CHK(pKinesisVideoClient->clientCallbacks.deviceCertToTokenFn != NULL, STATUS_SERVICE_CALL_CALLBACKS_MISSING);

    // NOTE: The following callback is a non-prompt operation.
    // The client will be awaiting for the resulting event and
    // the state machine will not be primed to continue.
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.deviceCertToTokenFn(
        pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->deviceInfo.name, &pKinesisVideoClient->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromCreateClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_CREATE;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // If the call succeeds then proceed forward
    if (pKinesisVideoClient->base.result == SERVICE_CALL_RESULT_OK) {
        if (pKinesisVideoClient->deviceInfo.tagCount != 0 && pKinesisVideoClient->deviceInfo.tags != NULL) {
            state = CLIENT_STATE_TAG_CLIENT;
        } else {
            state = CLIENT_STATE_READY;
        }
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeCreateClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoClient->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoClient->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoClient);
    pKinesisVideoClient->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoClient->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.createDeviceFn(
        pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoClient->deviceInfo.name, &pKinesisVideoClient->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromTagClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_TAG_CLIENT;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // If the call succeeds then proceed forward
    if (pKinesisVideoClient->base.result == SERVICE_CALL_RESULT_OK) {
        state = CLIENT_STATE_READY;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeTagClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoClient->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoClient->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoClient);
    pKinesisVideoClient->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoClient->base.serviceCallContext.callAfter = time;

    // Reset the call result
    // FIXME: Enable tag resource when we have device tagging
    // pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    // FIXME: Enable tag resource when we have device tagging
    //    CHK_STATUS(pKinesisVideoClient->clientCallbacks.tagResourceFn(
    //        pKinesisVideoClient->clientCallbacks.customData,
    //        pKinesisVideoClient->base.arn,
    //        pKinesisVideoClient->deviceInfo.tagCount,
    //        pKinesisVideoClient->deviceInfo.tags,
    //        &pKinesisVideoClient->base.serviceCallContext));

    pKinesisVideoClient->base.result = SERVICE_CALL_RESULT_OK;

    // Step the state machine as we need the execution quanta.
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));
CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromReadyClientState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);
    UINT64 state = CLIENT_STATE_READY;
    UINT64 currentTime, expiration;

    CHK(pKinesisVideoClient != NULL && pState != NULL, STATUS_NULL_ARG);

    // We need to move to Auth state if the auth has expired
    currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
    expiration = getCurrentAuthExpiration(pKinesisVideoClient);

    // If the auth info expired then we need to move to the auth state
    if (currentTime > expiration) {
        // Get to the auth state
        state = CLIENT_STATE_AUTH;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeReadyClientState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = CLIENT_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoClient != NULL, STATUS_NULL_ARG);

    // If we are creating a new client then we need to call the ready callback
    if (!pKinesisVideoClient->clientReady) {
        // Set the client to ready
        pKinesisVideoClient->clientReady = TRUE;

        // Pulse the ready condition variable
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.broadcastConditionVariableFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                                     pKinesisVideoClient->base.ready));

        // Call the ready callback
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.clientReadyFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                      TO_CLIENT_HANDLE(pKinesisVideoClient)));
    }

CleanUp:

    LEAVES();
    return retStatus;
}
