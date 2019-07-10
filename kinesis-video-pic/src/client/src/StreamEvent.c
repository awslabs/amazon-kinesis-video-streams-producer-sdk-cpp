/**
 * Implementation of a stream events
 */

#define LOG_CLASS "StreamEvent"

#include "Include_i.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Stream event functionality
/////////////////////////////////////////////////////////////////////////////////////////
STATUS serviceCallResultCheck(SERVICE_CALL_RESULT callResult)
{
    switch (callResult) {
        case SERVICE_CALL_INVALID_ARG:
            // Invalid args
            return STATUS_SERVICE_CALL_INVALID_ARG_ERROR;

        case SERVICE_CALL_DEVICE_NOT_FOUND:
            // Device not found error
            return STATUS_SERVICE_CALL_DEVICE_NOT_FOND_ERROR;

        case SERVICE_CALL_DEVICE_NOT_PROVISIONED:
            // Device not provisioned
            return STATUS_SERVICE_CALL_DEVICE_NOT_PROVISIONED_ERROR;

        case SERVICE_CALL_NOT_AUTHORIZED:
        case SERVICE_CALL_FORBIDDEN:
            // might be provisioning issue
            return STATUS_SERVICE_CALL_NOT_AUTHORIZED_ERROR;

        case SERVICE_CALL_RESOURCE_NOT_FOUND:
            // No streams found
            return STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR;

        case SERVICE_CALL_RESOURCE_IN_USE:
            // No streams found
            return STATUS_SERVICE_CALL_RESOURCE_IN_USE_ERROR;

        case SERVICE_CALL_REQUEST_TIMEOUT:
        case SERVICE_CALL_GATEWAY_TIMEOUT:
        case SERVICE_CALL_NETWORK_READ_TIMEOUT:
        case SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT:
            // Timeout
            return STATUS_SERVICE_CALL_TIMEOUT_ERROR;

        case SERVICE_CALL_RESOURCE_DELETED:
            // Resource deleted
            return STATUS_SERVICE_CALL_RESOURCE_DELETED_ERROR;

        case SERVICE_CALL_CLIENT_LIMIT:
            // Client limit reached
            return STATUS_SERVICE_CALL_CLIENT_LIMIT_ERROR;

        case SERVICE_CALL_DEVICE_LIMIT:
            // Device limit reached
            return STATUS_SERVICE_CALL_DEVICE_LIMIT_ERROR;

        case SERVICE_CALL_STREAM_LIMIT:
            // Device limit reached
            return STATUS_SERVICE_CALL_STREAM_LIMIT_ERROR;

        case SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD:
            // We are still in a grace period, should be OK
            // Explicit fall-through
        case SERVICE_CALL_RESULT_OK:
            // All OK result
            return STATUS_SUCCESS;

            // ACK errors
        case SERVICE_CALL_RESULT_STREAM_READ_ERROR:
            return STATUS_ACK_ERR_STREAM_READ_ERROR;
        case SERVICE_CALL_RESULT_FRAGMENT_SIZE_REACHED:
            return STATUS_ACK_ERR_FRAGMENT_SIZE_REACHED;
        case SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED:
            return STATUS_ACK_ERR_FRAGMENT_DURATION_REACHED;
        case SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED:
            return STATUS_ACK_ERR_CONNECTION_DURATION_REACHED;
        case SERVICE_CALL_RESULT_FRAGMENT_TIMECODE_NOT_MONOTONIC:
            return STATUS_ACK_ERR_FRAGMENT_TIMECODE_NOT_MONOTONIC;
        case SERVICE_CALL_RESULT_MULTI_TRACK_MKV:
            return STATUS_ACK_ERR_MULTI_TRACK_MKV;
        case SERVICE_CALL_RESULT_INVALID_MKV_DATA:
            return STATUS_ACK_ERR_INVALID_MKV_DATA;
        case SERVICE_CALL_RESULT_INVALID_PRODUCER_TIMESTAMP:
            return STATUS_ACK_ERR_INVALID_PRODUCER_TIMESTAMP;
        case SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE:
            return STATUS_ACK_ERR_STREAM_NOT_ACTIVE;
        case SERVICE_CALL_RESULT_FRAGMENT_METADATA_LIMIT_REACHED:
            return STATUS_ACK_ERR_FRAGMENT_METADATA_LIMIT_REACHED;
        case SERVICE_CALL_RESULT_TRACK_NUMBER_MISMATCH:
            return STATUS_ACK_ERR_TRACK_NUMBER_MISMATCH;
        case SERVICE_CALL_RESULT_FRAMES_MISSING_FOR_TRACK:
            return STATUS_ACK_ERR_FRAMES_MISSING_FOR_TRACK;
        case SERVICE_CALL_RESULT_MORE_THAN_ALLOWED_TRACKS_FOUND:
            return STATUS_ACK_ERR_MORE_THAN_ALLOWED_TRACKS_FOUND;
        case SERVICE_CALL_RESULT_KMS_KEY_ACCESS_DENIED:
            return STATUS_ACK_ERR_KMS_KEY_ACCESS_DENIED;
        case SERVICE_CALL_RESULT_KMS_KEY_DISABLED:
            return STATUS_ACK_ERR_KMS_KEY_DISABLED;
        case SERVICE_CALL_RESULT_KMS_KEY_VALIDATION_ERROR:
            return STATUS_ACK_ERR_KMS_KEY_VALIDATION_ERROR;
        case SERVICE_CALL_RESULT_KMS_KEY_UNAVAILABLE:
            return STATUS_ACK_ERR_KMS_KEY_UNAVAILABLE;
        case SERVICE_CALL_RESULT_KMS_KEY_INVALID_USAGE:
            return STATUS_ACK_ERR_KMS_KEY_INVALID_USAGE;
        case SERVICE_CALL_RESULT_KMS_KEY_INVALID_STATE:
            return STATUS_ACK_ERR_KMS_KEY_INVALID_STATE;
        case SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND:
            return STATUS_ACK_ERR_KMS_KEY_NOT_FOUND;
        case SERVICE_CALL_RESULT_STREAM_DELETED:
            return STATUS_ACK_ERR_STREAM_DELETED;
        case SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR:
            return STATUS_ACK_ERR_ACK_INTERNAL_ERROR;
        case SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR:
            return STATUS_ACK_ERR_FRAGMENT_ARCHIVAL_ERROR;
        case SERVICE_CALL_RESULT_UNKNOWN_ACK_ERROR:
            return STATUS_ACK_ERR_UNKNOWN_ACK_ERROR;

        case SERVICE_CALL_UNKNOWN:
            // Explicit fall-through
        default:
            // Unknown error
            return STATUS_SERVICE_CALL_UNKOWN_ERROR;
    }
}

BOOL serviceCallResultRetry(SERVICE_CALL_RESULT callResult)
{
    switch (callResult) {
        case SERVICE_CALL_INVALID_ARG:
        case SERVICE_CALL_DEVICE_NOT_FOUND:
        case SERVICE_CALL_DEVICE_NOT_PROVISIONED:
        case SERVICE_CALL_NOT_AUTHORIZED:
        case SERVICE_CALL_FORBIDDEN:
        case SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND:
        case SERVICE_CALL_RESOURCE_DELETED:
        case SERVICE_CALL_CLIENT_LIMIT:
        case SERVICE_CALL_DEVICE_LIMIT:
        case SERVICE_CALL_STREAM_LIMIT:
        case SERVICE_CALL_RESULT_FRAGMENT_SIZE_REACHED:
        case SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED:
        case SERVICE_CALL_RESULT_FRAGMENT_TIMECODE_NOT_MONOTONIC:
        case SERVICE_CALL_RESULT_INVALID_MKV_DATA:
        case SERVICE_CALL_RESULT_MULTI_TRACK_MKV:
        case SERVICE_CALL_RESULT_INVALID_PRODUCER_TIMESTAMP:
        case SERVICE_CALL_RESULT_FRAGMENT_METADATA_LIMIT_REACHED:
        case SERVICE_CALL_RESULT_TRACK_NUMBER_MISMATCH:
        case SERVICE_CALL_RESULT_FRAMES_MISSING_FOR_TRACK:
        case SERVICE_CALL_RESULT_MORE_THAN_ALLOWED_TRACKS_FOUND:
        case SERVICE_CALL_RESULT_KMS_KEY_ACCESS_DENIED:
        case SERVICE_CALL_RESULT_KMS_KEY_DISABLED:
        case SERVICE_CALL_RESULT_KMS_KEY_VALIDATION_ERROR:
        case SERVICE_CALL_RESULT_KMS_KEY_UNAVAILABLE:
        case SERVICE_CALL_RESULT_KMS_KEY_INVALID_USAGE:
        case SERVICE_CALL_RESULT_KMS_KEY_INVALID_STATE:
        case SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE:
        case SERVICE_CALL_RESULT_STREAM_DELETED:
            return FALSE;

        case SERVICE_CALL_RESOURCE_NOT_FOUND:
        case SERVICE_CALL_RESOURCE_IN_USE:
        case SERVICE_CALL_REQUEST_TIMEOUT:
        case SERVICE_CALL_GATEWAY_TIMEOUT:
        case SERVICE_CALL_NETWORK_READ_TIMEOUT:
        case SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT:
        case SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD:
        case SERVICE_CALL_RESULT_OK:
        case SERVICE_CALL_RESULT_STREAM_READ_ERROR:
        case SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED:
        case SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR:
        case SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR:
        case SERVICE_CALL_RESULT_UNKNOWN_ACK_ERROR:
        case SERVICE_CALL_UNKNOWN:
            // Explicit fall-through
        default:
            // Unknown error
            return TRUE;
    }
}

/**
 * Describe stream result event func
 */
STATUS describeStreamResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult, PStreamDescription pStreamDescription)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;
    StreamDescription streamDescription;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_DESCRIBE, &pState));

    // Check if we are in describe state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_RESOURCE_NOT_FOUND_ERROR ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR,
        retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        // Should have the description
        CHK(pStreamDescription != NULL, STATUS_INVALID_DESCRIBE_STREAM_RESPONSE);

        // Copy and fix-up the versions
        MEMCPY(&streamDescription, pStreamDescription, sizeOfStreamDescription(pStreamDescription));
        fixupStreamDescription(&streamDescription);

        // We will error out if the stream is being deleted.
        // Up-to the client to what to do as deleting a stream is
        // a very lengthy operation.
        CHK(streamDescription.streamStatus != STREAM_STATUS_DELETING, STATUS_STREAM_IS_BEING_DELETED_ERROR);

        // Check the version
        CHK(streamDescription.version <= STREAM_DESCRIPTION_CURRENT_VERSION, STATUS_INVALID_STREAM_DESCRIPTION_VERSION);

        // Check and store the ARN
        CHK(streamDescription.streamArn != NULL &&
            STRNLEN(streamDescription.streamArn, MAX_ARN_LEN + 1) <= MAX_ARN_LEN, STATUS_INVALID_DESCRIBE_STREAM_RESPONSE);
        STRNCPY(pKinesisVideoStream->base.arn, streamDescription.streamArn, MAX_ARN_LEN);
        pKinesisVideoStream->base.arn[MAX_ARN_LEN] = '\0';

        // Store the values we need
        pKinesisVideoStream->streamStatus = streamDescription.streamStatus;
        pKinesisVideoStream->retention = streamDescription.retention;
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Create stream result event func
 */
STATUS createStreamResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult, PCHAR streamArn)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_CREATE, &pState));

    // Check if we are in create state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        // Should have the stream arn
        CHK(streamArn != NULL &&
            STRNLEN(streamArn, MAX_ARN_LEN + 1) <= MAX_ARN_LEN, STATUS_INVALID_CREATE_STREAM_RESPONSE);
        STRNCPY(pKinesisVideoStream->base.arn, streamArn, MAX_ARN_LEN);
        pKinesisVideoStream->base.arn[MAX_ARN_LEN] = '\0';
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Get streaming token result event func
 */
STATUS getStreamingTokenResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult, PBYTE pToken, UINT32 tokenSize, UINT64 expiration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;
    UINT64 currentTime;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_GET_TOKEN, &pState));

    // Check if we are in get token state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        // We are good now to proceed - store the data and set the state
        CHK(tokenSize <= MAX_AUTH_LEN, STATUS_INVALID_AUTH_LEN);
        pKinesisVideoStream->streamingAuthInfo.version = AUTH_INFO_CURRENT_VERSION;
        pKinesisVideoStream->streamingAuthInfo.size = tokenSize;

        currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);

        // Validate the minimum duration for the token expiration
        CHK(expiration > currentTime && (expiration - currentTime) >= MIN_STREAMING_TOKEN_EXPIRATION_DURATION, STATUS_INVALID_TOKEN_EXPIRATION);

        // Ensure that we rotate before the max period is enforced
        pKinesisVideoStream->streamingAuthInfo.expiration = MIN(expiration, currentTime + MAX_ENFORCED_TOKEN_EXPIRATION_DURATION);

        // Introduce jitter to the expiration time
        pKinesisVideoStream->streamingAuthInfo.expiration = randomizeAuthInfoExpiration(pKinesisVideoClient,
                                                                                        pKinesisVideoStream->streamingAuthInfo.expiration,
                                                                                        currentTime);

        // If we don't have a token we assume there is no auth.
        if (pToken == NULL || tokenSize == 0) {
            pKinesisVideoStream->streamingAuthInfo.type = AUTH_INFO_NONE;
        } else {
            pKinesisVideoStream->streamingAuthInfo.type = AUTH_INFO_TYPE_STS;
            MEMCPY(pKinesisVideoStream->streamingAuthInfo.data, pToken, tokenSize);
        }

        // Reset the grace period
        pKinesisVideoStream->gracePeriod = FALSE;
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Get streaming endpoint result event func
 */
STATUS getStreamingEndpointResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult, PCHAR pEndpoint)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_GET_ENDPOINT, &pState));

    // Check if we are in get endpoint state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // store the info
    if (callResult == SERVICE_CALL_RESULT_OK) {
        // We are good now to proceed - store the data and set the state
        CHK(STRNLEN(pEndpoint, MAX_URI_CHAR_LEN + 1) <= MAX_URI_CHAR_LEN, STATUS_INVALID_URI_LEN);
        STRNCPY(pKinesisVideoStream->streamingEndpoint, pEndpoint, MAX_URI_CHAR_LEN);
        pKinesisVideoStream->streamingEndpoint[MAX_URI_CHAR_LEN] = '\0';
    }

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * PutStream result event func
 */
STATUS putStreamResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult, UPLOAD_HANDLE streamHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PUploadHandleInfo pUploadHandleInfo = NULL;
    BOOL locked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the stream
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_PUT_STREAM, &pState));

    // Check if we are in put stream state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // Store the client stream handle to call back with.
    CHK(NULL != (pUploadHandleInfo = (PUploadHandleInfo) MEMALLOC(SIZEOF(UploadHandleInfo))), STATUS_NOT_ENOUGH_MEMORY);
    pUploadHandleInfo->handle = streamHandle;
    pUploadHandleInfo->lastFragmentTs = INVALID_TIMESTAMP_VALUE;
    pUploadHandleInfo->timestamp = INVALID_TIMESTAMP_VALUE;
    pUploadHandleInfo->lastPersistedAckTs = INVALID_TIMESTAMP_VALUE;
    pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_NEW;

    // Enqueue the stream upload info object
    CHK_STATUS(stackQueueEnqueue(pKinesisVideoStream->pUploadInfoQueue, (UINT64) pUploadHandleInfo));

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    if (STATUS_FAILED(retStatus) && (NULL != pUploadHandleInfo)) {
        MEMFREE(pUploadHandleInfo);
    }

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Tag stream result event func
 */
STATUS tagStreamResult(PKinesisVideoStream pKinesisVideoStream, SERVICE_CALL_RESULT callResult)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL locked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_TAG_STREAM, &pState));

    // Check if we are in get endpoint state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // Basic checks
    retStatus = serviceCallResultCheck(callResult);
    CHK(retStatus == STATUS_SUCCESS ||
                retStatus == STATUS_SERVICE_CALL_TIMEOUT_ERROR ||
                retStatus == STATUS_SERVICE_CALL_UNKOWN_ERROR, retStatus);

    // Reset the status
    retStatus = STATUS_SUCCESS;

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // Step the machine
    retStatus = stepStateMachine(pKinesisVideoStream->base.pStateMachine);
    CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_TAG_STREAM_CALL_FAILED, retStatus);

    // Override tagStream failure because it is not critical to streaming.
    if (retStatus == STATUS_TAG_STREAM_CALL_FAILED) {
        DLOGW("tagResourceResultEvent failed with status 0x%08x. Overriding service call result to move to next state", retStatus);
        pKinesisVideoClient->clientCallbacks.streamErrorReportFn(
                pKinesisVideoClient->clientCallbacks.customData,
                TO_STREAM_HANDLE(pKinesisVideoStream),
                INVALID_UPLOAD_HANDLE_VALUE,
                INVALID_TIMESTAMP_VALUE,
                STATUS_TAG_STREAM_CALL_FAILED);

        pKinesisVideoStream->base.result = SERVICE_CALL_RESULT_OK;
        retStatus = stepStateMachine(pKinesisVideoStream->base.pStateMachine);
    }

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Stream terminated notification
 */
STATUS streamTerminatedEvent(PKinesisVideoStream pKinesisVideoStream, UPLOAD_HANDLE uploadHandle,
                             SERVICE_CALL_RESULT callResult, BOOL connectionStillAlive)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStateMachineState pState;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PUploadHandleInfo pUploadHandleInfo;
    BOOL locked = FALSE;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL, STATUS_NULL_ARG);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // We should handle the in-grace termination differently by not setting the terminated state
    if (SERVICE_CALL_STREAM_AUTH_IN_GRACE_PERIOD != callResult) {
        // Set the indicator of the retrying handle
        pKinesisVideoStream->connectionState = UPLOAD_CONNECTION_STATE_IN_USE;

        // Get the first upload handle in case of invalid handle specified
        if (!IS_VALID_UPLOAD_HANDLE(uploadHandle)) {
            pUploadHandleInfo = getStreamUploadInfoWithState(pKinesisVideoStream, UPLOAD_HANDLE_STATE_ACTIVE);
        } else {
            pUploadHandleInfo = getStreamUploadInfo(pKinesisVideoStream, uploadHandle);
        }

        // If the upload handle has streamed some data, set flag to trigger rollback in the next getStreamData call.
        // If the upload handle has not stream any data, we can safely ignore this event.
        if (NULL != pUploadHandleInfo) {
            if ((pUploadHandleInfo->state & UPLOAD_HANDLE_STATE_NOT_IN_USE) != UPLOAD_HANDLE_STATE_NONE) {
                // Need to indicate to the getStreamData to rollback.
                pKinesisVideoStream->connectionState = UPLOAD_CONNECTION_STATE_NOT_IN_USE;
            }

            // Set the state to terminated
            pUploadHandleInfo->state = UPLOAD_HANDLE_STATE_TERMINATED;

            // In case of reset connection and error acks, need to ping the terminated upload handle so that it
            // can unpause if paused and then call getStreamData and receive the end-of-stream status
            if (connectionStillAlive) {
                CHK_STATUS(pKinesisVideoClient->clientCallbacks.streamDataAvailableFn(
                        pKinesisVideoClient->clientCallbacks.customData,
                        TO_STREAM_HANDLE(pKinesisVideoStream),
                        pKinesisVideoStream->streamInfo.name,
                        pUploadHandleInfo->handle,
                        0,
                        0));
            }
        }

        // If the connection that upload handle represents is already dead. Then it will not make anymore
        // getStreamData call so it should be removed.
        if (!connectionStillAlive) {
            // Remove the handle info from the queue
            deleteStreamUploadInfo(pKinesisVideoStream, pUploadHandleInfo);
            pUploadHandleInfo = NULL;
        }
    }

    // return early if pic is already in process of traversing control plane states.
    CHK(!STATUS_SUCCEEDED(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine,
                                                 STREAM_STATE_DESCRIBE |
                                                 STREAM_STATE_CREATE |
                                                 STREAM_STATE_TAG_STREAM |
                                                 STREAM_STATE_GET_TOKEN |
                                                 STREAM_STATE_GET_ENDPOINT |
                                                 STREAM_STATE_READY)), retStatus);

    // Stop the stream
    pKinesisVideoStream->streamState = STREAM_STATE_STOPPED;

    // Get the accepted state
    CHK_STATUS(getStateMachineState(pKinesisVideoStream->base.pStateMachine, STREAM_STATE_STOPPED, &pState));

    // Check for the right state
    CHK_STATUS(acceptStateMachineState(pKinesisVideoStream->base.pStateMachine, pState->acceptStates));

    // store the result
    pKinesisVideoStream->base.result = callResult;

    // Step the machine
    CHK_STATUS(stepStateMachine(pKinesisVideoStream->base.pStateMachine));

CleanUp:

    // Unlock the stream
    if (locked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

/**
 * Stream fragment ACK received notification
 */
STATUS streamFragmentAckEvent(PKinesisVideoStream pKinesisVideoStream, UPLOAD_HANDLE uploadHandle, PFragmentAck pFragmentAck)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PUploadHandleInfo pUploadHandleInfo;
    BOOL locked = FALSE, inView = FALSE;
    UINT64 timestamp = 0, curIndex;
    PViewItem pViewItem;

    CHK(pKinesisVideoStream != NULL && pKinesisVideoStream->pKinesisVideoClient != NULL && pFragmentAck != NULL, STATUS_NULL_ARG);

    // Check the version
    CHK(pFragmentAck->version <= FRAGMENT_ACK_CURRENT_VERSION, STATUS_INVALID_FRAGMENT_ACK_VERSION);

    // Early return if we have an IDLE ack
    if (pFragmentAck->ackType == FRAGMENT_ACK_TYPE_IDLE) {
        // Nothing to do if we have an IDLE ack
        CHK(FALSE, retStatus);
    }

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    // Lock the state
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    locked = TRUE;

    // First of all, check if the ACK is for a session that's expired/closed already and ignore if it is
    pUploadHandleInfo = getStreamUploadInfo(pKinesisVideoStream, uploadHandle);
    if (NULL == pUploadHandleInfo ||
        (pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_ERROR || pUploadHandleInfo->state == UPLOAD_HANDLE_STATE_TERMINATED) ||
        !IS_VALID_UPLOAD_HANDLE(pUploadHandleInfo->handle)) {
        // No session is present - early return
        DLOGW("An ACK is received for an already expired upload handle %" PRIu64, uploadHandle);
        CHK(FALSE, retStatus);
    }

    // Fix-up the invalid timestamp
    if (!IS_VALID_TIMESTAMP(pFragmentAck->timestamp)) {
        // Use the current from the view.
        // If the current is greater than head which is the case when the networking pulls fast enough
        // then we will use the head instead.
        CHK_STATUS(contentViewGetCurrentIndex(pKinesisVideoStream->pView, &curIndex));
        CHK_STATUS(contentViewGetHead(pKinesisVideoStream->pView, &pViewItem));

        if (curIndex < pViewItem->index) {
            CHK_STATUS(contentViewGetItemAt(pKinesisVideoStream->pView, curIndex, &pViewItem));
        }

        timestamp = pViewItem->timestamp;
    } else {
        // Calculate the timestamp based on the ACK.
        // Convert the timestamp
        CHK_STATUS(mkvgenTimecodeToTimestamp(pKinesisVideoStream->pMkvGenerator, pFragmentAck->timestamp, &timestamp));

        // In case we have a relative cluster timestamp stream we need to adjust for the stream start.
        // The stream start timestamp is extracted from the stream session map.
        if (!pKinesisVideoStream->streamInfo.streamCaps.absoluteFragmentTimes &&
            IS_VALID_TIMESTAMP(pUploadHandleInfo->timestamp)) {
            // Adjust the relative timestamp to make an absolute timestamp
            timestamp += pUploadHandleInfo->timestamp;
        }
    }

    // Quick check if we have the timestamp in the view window and if not then bail out early
    CHK_STATUS(contentViewTimestampInRange(pKinesisVideoStream->pView, timestamp, TRUE, &inView));

    // NOTE: IMPORTANT: For the Error Ack case we will still need to process the ACK. The side-effect of
    // processing the Error Ack is the connection termination which is needed as the higher-level clients like
    // CURL might not trigger the termination after the LB sends 'FIN' TCP packet.
    switch (pFragmentAck->ackType) {
        case FRAGMENT_ACK_TYPE_BUFFERING:
            if (inView) {
                CHK_STATUS(streamFragmentBufferingAck(pKinesisVideoStream, timestamp));
            }

            break;
        case FRAGMENT_ACK_TYPE_RECEIVED:
            if (inView) {
                CHK_STATUS(streamFragmentReceivedAck(pKinesisVideoStream, timestamp));
            }

            break;
        case FRAGMENT_ACK_TYPE_PERSISTED:
            if (inView) {
                CHK_STATUS(streamFragmentPersistedAck(pKinesisVideoStream, timestamp, pUploadHandleInfo));
            }

            break;
        case FRAGMENT_ACK_TYPE_ERROR:
            if (!inView) {
                // Apply to the earliest.
                CHK_STATUS(contentViewGetTail(pKinesisVideoStream->pView, &pViewItem));
                timestamp = pViewItem->timestamp;
            }

            CHK_STATUS(streamFragmentErrorAck(pKinesisVideoStream, timestamp, pFragmentAck->result));

            break;
        default:
            // shouldn't ever be the case.
            CHK(FALSE, STATUS_INVALID_FRAGMENT_ACK_TYPE);
    }

    // Still return an error on an out-of-bounds timestamp
    CHK(inView, STATUS_ACK_TIMESTAMP_NOT_IN_VIEW_WINDOW);

CleanUp:

    if (pKinesisVideoClient != NULL)
    {
        // We will notify the fragment ACK received callback even if the processing failed
        if (pKinesisVideoClient->clientCallbacks.fragmentAckReceivedFn != NULL) {
            pKinesisVideoClient->clientCallbacks.fragmentAckReceivedFn(pKinesisVideoClient->clientCallbacks.customData,
                                                                       TO_STREAM_HANDLE(pKinesisVideoStream),
                                                                       uploadHandle,
                                                                       pFragmentAck);
        }

        // Unlock the stream
        if (locked) {
            pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
        }
    }

    LEAVES();
    return retStatus;
}
