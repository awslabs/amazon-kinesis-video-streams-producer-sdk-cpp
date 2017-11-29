/**
 * Implementation of a stream states callbacks
 */

#define LOG_CLASS "StreamState"
#include "Include_i.h"

/**
 * Static definitions of the states
 */
StateMachineState STREAM_STATE_MACHINE_STATES[] = {
        {STREAM_STATE_NEW, STREAM_STATE_NONE | STREAM_STATE_NEW | STREAM_STATE_STOPPED, fromNewStreamState, executeNewStreamState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_INVALID_STREAM_READY_STATE},
        {STREAM_STATE_DESCRIBE, STREAM_STATE_NEW | STREAM_STATE_STOPPED | STREAM_STATE_DESCRIBE, fromDescribeStreamState, executeDescribeStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_DESCRIBE_STREAM_CALL_FAILED},
        {STREAM_STATE_CREATE, STREAM_STATE_STOPPED | STREAM_STATE_DESCRIBE | STREAM_STATE_CREATE, fromCreateStreamState, executeCreateStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_CREATE_STREAM_CALL_FAILED},
        {STREAM_STATE_TAG_STREAM, STREAM_STATE_STOPPED | STREAM_STATE_DESCRIBE | STREAM_STATE_CREATE | STREAM_STATE_TAG_STREAM, fromTagStreamState, executeTagStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_TAG_STREAM_CALL_FAILED},
        {STREAM_STATE_GET_ENDPOINT, STREAM_STATE_STOPPED | STREAM_STATE_DESCRIBE | STREAM_STATE_CREATE | STREAM_STATE_GET_ENDPOINT | STREAM_STATE_TAG_STREAM, fromGetEndpointStreamState, executeGetEndpointStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_GET_STREAMING_ENDPOINT_CALL_FAILED},
        {STREAM_STATE_GET_TOKEN, STREAM_STATE_STOPPED | STREAM_STATE_GET_ENDPOINT | STREAM_STATE_GET_TOKEN, fromGetTokenStreamState, executeGetTokenStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_GET_STREAMING_TOKEN_CALL_FAILED},
        {STREAM_STATE_READY, STREAM_STATE_STOPPED | STREAM_STATE_GET_TOKEN | STREAM_STATE_READY | STREAM_STATE_PUT_STREAM | STREAM_STATE_STREAMING, fromReadyStreamState, executeReadyStreamState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_STREAM_READY_CALLBACK_FAILED},
        {STREAM_STATE_PUT_STREAM, STREAM_STATE_STOPPED | STREAM_STATE_READY | STREAM_STATE_PUT_STREAM, fromPutStreamState, executePutStreamState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_PUT_STREAM_CALL_FAILED},
        {STREAM_STATE_STREAMING, STREAM_STATE_STOPPED | STREAM_STATE_PUT_STREAM | STREAM_STATE_STREAMING, fromStreamingStreamState, executeStreamingStreamState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_PUT_STREAM_CALL_FAILED},
        {STREAM_STATE_STOPPED, STREAM_STATE_STOPPED | STREAM_STATE_CREATE | STREAM_STATE_DESCRIBE | STREAM_STATE_GET_ENDPOINT | STREAM_STATE_GET_TOKEN | STREAM_STATE_READY | STREAM_STATE_PUT_STREAM | STREAM_STATE_STREAMING, fromStoppedStreamState, executeStoppedStreamState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_PUT_STREAM_CALL_FAILED},
};

UINT32 STREAM_STATE_MACHINE_STATE_COUNT = SIZEOF(STREAM_STATE_MACHINE_STATES) / SIZEOF(StateMachineState);

///////////////////////////////////////////////////////////////////////////
// State machine callback functions
///////////////////////////////////////////////////////////////////////////
STATUS fromNewStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to describe state if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else {
        state = STREAM_STATE_DESCRIBE;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeNewStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // Step the state machine to automatically invoke the Describe API
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeDescribeStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call the describe
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.describeStreamFn(pKinesisVideoClient->clientCallbacks.customData,
                                                               pKinesisVideoStream->streamInfo.name,
                                                               &pKinesisVideoStream->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromDescribeStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_DESCRIBE;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else {
        // Check the previous results
        switch (pKinesisVideoStream->base.result) {
            case SERVICE_CALL_RESULT_OK:
                // Check that the stream status is not Deleting as it's an error
                CHK(pKinesisVideoStream->streamStatus != STREAM_STATUS_DELETING, STATUS_STREAM_IS_BEING_DELETED_ERROR);

                // If the call is OK and the stream status is active then move to the next state
                if (pKinesisVideoStream->streamStatus == STREAM_STATUS_ACTIVE) {
                    // If we have tags then we should attempt tagging the stream first
                    if (pKinesisVideoStream->streamInfo.tagCount != 0) {
                        state = STREAM_STATE_TAG_STREAM;
                    } else {
                        state = STREAM_STATE_GET_ENDPOINT;
                    }
                }

                break;

            case SERVICE_CALL_RESOURCE_NOT_FOUND:
                // Move to the create state as no streams have been found
                state = STREAM_STATE_CREATE;
                break;

            default:
                // Re-run the state
                break;
        }
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromCreateStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_CREATE;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else if (pKinesisVideoStream->base.result == SERVICE_CALL_RESULT_OK) {
        if (pKinesisVideoStream->streamInfo.tagCount != 0) {
            state = STREAM_STATE_TAG_STREAM;
        } else {
            state = STREAM_STATE_GET_ENDPOINT;
        }
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromTagStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_TAG_STREAM;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else if (pKinesisVideoStream->base.result == SERVICE_CALL_RESULT_OK) {
        state = STREAM_STATE_GET_ENDPOINT;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromGetEndpointStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_GET_ENDPOINT;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else if (pKinesisVideoStream->base.result == SERVICE_CALL_RESULT_OK) {
        state = STREAM_STATE_GET_TOKEN;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromGetTokenStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_GET_TOKEN;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else if (pKinesisVideoStream->base.result == SERVICE_CALL_RESULT_OK) {
        state = STREAM_STATE_READY;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromPutStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_PUT_STREAM;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to states if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else if (pKinesisVideoStream->base.result == SERVICE_CALL_RESULT_OK && IS_VALID_UPLOAD_HANDLE(pKinesisVideoStream->newStreamHandle)) {
        state = STREAM_STATE_STREAMING;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromStreamingStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_STREAMING;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Check the previous results
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    }

    // Assign the nest state
    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS fromStoppedStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state = STREAM_STATE_NEW;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Indicate that we are not stopped and we are not coming from a new state
    pKinesisVideoStream->streamState = STREAM_STATE_READY;

    retStatus = serviceCallResultCheck(pKinesisVideoStream->base.result);

    // Check the result
    switch (retStatus) {
        case STATUS_SUCCESS:
            // Stream has been terminated. Return to get endpoint
            state = STREAM_STATE_GET_ENDPOINT;
            break;

        case STATUS_SERVICE_CALL_DEVICE_LIMIT_ERROR:
        case STATUS_SERVICE_CALL_STREAM_LIMIT_ERROR:
            // Stream/device limit is reached. Return to get endpoint
            state = STREAM_STATE_GET_ENDPOINT;
            retStatus = STATUS_SUCCESS;
            break;

        case STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR:
            // Not authorized - return to get token
            state = STREAM_STATE_GET_TOKEN;
            retStatus = STATUS_SUCCESS;
            break;

        case STATUS_SERVICE_CALL_RESOURCE_IN_USE_ERROR:
        case STATUS_SERVICE_CALL_UNKOWN_ERROR:
            // resource in use, unknown - return to describe
            state = STREAM_STATE_DESCRIBE;
            retStatus = STATUS_SUCCESS;
            break;

        case STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR:
        case STATUS_SERVICE_CALL_RESOURCE_DELETED_ERROR:
            // resource not found, deleted - return to describe
            state = STREAM_STATE_DESCRIBE;
            retStatus = STATUS_SUCCESS;
            break;

        default:
            // Default state in any other case
            if (pKinesisVideoStream->streamInfo.streamCaps.recoverOnError) {
                retStatus = STATUS_SUCCESS;
            }

            break;
    }

CleanUp:

    if (pState != NULL) {
        *pState = state;
    }

    LEAVES();
    return retStatus;
}

STATUS fromReadyStreamState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    UINT64 state;

    CHK(pKinesisVideoStream != NULL && pState != NULL, STATUS_NULL_ARG);

    // Transition to put stream state if not stopped
    if (pKinesisVideoStream->streamState == STREAM_STATE_STOPPED) {
        state = STREAM_STATE_STOPPED;
    } else {
        state = STREAM_STATE_PUT_STREAM;
    }

    *pState = state;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeGetEndpointStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.getStreamingEndpointFn(
        pKinesisVideoClient->clientCallbacks.customData,
        pKinesisVideoStream->streamInfo.name,
        (PCHAR) GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME,
        &pKinesisVideoStream->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeGetTokenStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.getStreamingTokenFn(
        pKinesisVideoClient->clientCallbacks.customData,
        pKinesisVideoStream->streamInfo.name,
        STREAM_ACCESS_MODE_READ,
        &pKinesisVideoStream->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeCreateStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.createStreamFn(
        pKinesisVideoClient->clientCallbacks.customData,
        pKinesisVideoClient->deviceInfo.name,
        pKinesisVideoStream->streamInfo.name,
        pKinesisVideoStream->streamInfo.streamCaps.contentType,
        pKinesisVideoStream->streamInfo.kmsKeyId,
        pKinesisVideoStream->streamInfo.retention,
        &pKinesisVideoStream->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeTagStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);
    CHK(pKinesisVideoStream->streamInfo.tagCount != 0 && pKinesisVideoStream->streamInfo.tags != NULL, STATUS_INVALID_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoClient->tokenAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // Reset the call result
    pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

    // Call API
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.tagResourceFn(
        pKinesisVideoClient->clientCallbacks.customData,
        pKinesisVideoStream->base.arn,
        pKinesisVideoStream->streamInfo.tagCount,
        pKinesisVideoStream->streamInfo.tags,
        &pKinesisVideoStream->base.serviceCallContext));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeReadyStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Call the ready callback
    CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamReadyFn(
        pKinesisVideoClient->clientCallbacks.customData,
        TO_STREAM_HANDLE(pKinesisVideoStream)));

    // Check if we need to also call put stream API
    if (pKinesisVideoStream->streamState == STREAM_STATE_READY) {
        // Step the state machine to automatically invoke the PutStream API
        CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executePutStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);
    PKinesisVideoClient pKinesisVideoClient = NULL;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Step the client state machine first
    CHK_STATUS(stepStateMachine(pKinesisVideoClient->base.pStateMachine));

    pKinesisVideoStream->base.serviceCallContext.pAuthInfo = &pKinesisVideoStream->streamingAuthInfo;
    pKinesisVideoStream->base.serviceCallContext.version = SERVICE_CALL_CONTEXT_CURRENT_VERSION;
    pKinesisVideoStream->base.serviceCallContext.customData = TO_CUSTOM_DATA(pKinesisVideoStream);
    pKinesisVideoStream->base.serviceCallContext.timeout = SERVICE_CALL_DEFAULT_TIMEOUT;
    pKinesisVideoStream->base.serviceCallContext.callAfter = time;

    // We need to call the put stream API the first time
    if (pKinesisVideoStream->streamState != STREAM_STATE_PUT_STREAM) {
        // Reset the call result
        pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_NOT_SET;

        // Call API
        CHK_STATUS(pKinesisVideoClient->clientCallbacks.putStreamFn(
            pKinesisVideoClient->clientCallbacks.customData,
            pKinesisVideoStream->streamInfo.name,
            (PCHAR) MKV_CONTAINER_TYPE_STRING,
            pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData),
            pKinesisVideoStream->streamInfo.streamCaps.absoluteFragmentTimes,
            pKinesisVideoStream->streamInfo.streamCaps.fragmentAcks,
            pKinesisVideoStream->streamingEndpoint,
            &pKinesisVideoStream->base.serviceCallContext));

        pKinesisVideoStream->streamState = STREAM_STATE_PUT_STREAM;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeStreamingStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // Set the indicator state to streaming
    pKinesisVideoStream->streamState = STREAM_STATE_STREAMING;

    // Nothing to do

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeStoppedStreamState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoStream pKinesisVideoStream = STREAM_FROM_CUSTOM_DATA(customData);

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // Indicate that the stream has been terminated
    pKinesisVideoStream->connectionDropped = TRUE;

    // Also store the connection stopping result
    pKinesisVideoStream->connectionDroppedResult = pKinesisVideoStream->base.result;

CleanUp:

    LEAVES();
    return retStatus;
}