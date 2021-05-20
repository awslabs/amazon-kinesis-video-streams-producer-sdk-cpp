/**
 * Implementation of a Fragment ACK parser
 */

#define LOG_CLASS "AckParser"
#include "Include_i.h"

STATUS resetAckParserState(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // This will zero out fields which will be the initial state for most of the fields
    MEMSET(&pKinesisVideoStream->fragmentAckParser, 0x00, SIZEOF(FragmentAckParser));
    pKinesisVideoStream->fragmentAckParser.fragmentAck.ackType = FRAGMENT_ACK_TYPE_UNDEFINED;
    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_START;
    pKinesisVideoStream->fragmentAckParser.curKeyName = FRAGMENT_ACK_KEY_NAME_UNKNOWN;
    pKinesisVideoStream->fragmentAckParser.fragmentAck.result = SERVICE_CALL_RESULT_OK;
    pKinesisVideoStream->fragmentAckParser.fragmentAck.version = FRAGMENT_ACK_CURRENT_VERSION;
    pKinesisVideoStream->fragmentAckParser.uploadHandle = INVALID_UPLOAD_HANDLE_VALUE;
    pKinesisVideoStream->fragmentAckParser.fragmentAck.timestamp = INVALID_TIMESTAMP_VALUE;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS parseFragmentAck(PKinesisVideoStream pKinesisVideoStream, UPLOAD_HANDLE uploadHandle, PCHAR ackSegment, UINT32 ackSegmentSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 index = 0;
    UINT32 level = 0;
    CHAR curChar;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    BOOL streamLocked = FALSE;

    CHK(pKinesisVideoStream != NULL && ackSegment != NULL, STATUS_NULL_ARG);

    // If ack segment is specified and ack segment size is 0 then we should get the C-string size
    if (0 == ackSegmentSize) {
        ackSegmentSize = (UINT32) STRNLEN(ackSegment, MAX_ACK_FRAGMENT_LEN);
    } else {
        CHK(ackSegmentSize <= MAX_ACK_FRAGMENT_LEN, STATUS_INVALID_ACK_SEGMENT_LEN);
    }

    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    streamLocked = TRUE;

    // Set the upload handle
    if (!IS_VALID_UPLOAD_HANDLE(pKinesisVideoStream->fragmentAckParser.uploadHandle)) {
        pKinesisVideoStream->fragmentAckParser.uploadHandle = uploadHandle;
    }

    for (index = 0; index < ackSegmentSize; index++) {
        curChar = ackSegment[index];

        switch (pKinesisVideoStream->fragmentAckParser.state) {
            case FRAGMENT_ACK_PARSER_STATE_START:
                // Skip until the start of the segment
                if (ACK_PARSER_OPEN_BRACE == curChar) {
                    // Move to segment start state
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_ACK_START;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_ACK_START:
                // Skip until non-whitespace
                if (!IS_WHITE_SPACE(curChar)) {
                    // We should have a quote
                    CHK(curChar == ACK_PARSER_QUOTE, STATUS_INVALID_ACK_KEY_START);

                    // Start the key state
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_KEY_START;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_KEY_START:
                // Accumulate until end of string
                if (curChar == ACK_PARSER_QUOTE) {
                    // End of key start - parse the key name
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos] = '\0';
                    pKinesisVideoStream->fragmentAckParser.curKeyName = getFragmentAckKeyName(pKinesisVideoStream->fragmentAckParser.accumulator);

                    // Move to delimiter state
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_DELIMITER;

                    // Reset the cur position of the accumulator
                    pKinesisVideoStream->fragmentAckParser.curPos = 0;

                } else {
                    // Accumulate it
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos++] = curChar;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_DELIMITER:
                // Skip whitespaces until the delimiter
                if (!IS_WHITE_SPACE(curChar)) {
                    if (curChar == ACK_PARSER_DELIMITER) {
                        // Move to the body start state
                        pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_BODY_START;
                    }
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_BODY_START:
                // Skip whitespaces until the body start indicator
                if (!IS_WHITE_SPACE(curChar)) {
                    if (curChar == ACK_PARSER_OPEN_BRACE) {
                        // Set the nestedness level
                        level = 1;
                        // Move to the skip body end state
                        pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACE_END;
                    } else if (curChar == ACK_PARSER_OPEN_BRACKET) {
                        // Set the nestedness level
                        level = 1;
                        // Move to the skip body end state
                        pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACKET_END;
                    } else if (curChar == ACK_PARSER_QUOTE) {
                        // Move to text value type
                        pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_TEXT_VALUE;
                    } else if (IS_ACK_START_OF_NUMERIC_VALUE(curChar)) {
                        // Push the cur char to the accumulator
                        pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos++] = curChar;
                        pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_NUMERIC_VALUE;
                    } else {
                        CHK(FALSE, STATUS_INVALID_ACK_INVALID_VALUE_START);
                    }
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_TEXT_VALUE:
                // Accumulate until next quote
                if (curChar == ACK_PARSER_QUOTE) {
                    // Null terminate the string
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos] = '\0';

                    // Process the value
                    CHK_STATUS(processAckValue(&pKinesisVideoStream->fragmentAckParser));

                    // Skip until the end of the value
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_VALUE_END;
                } else {
                    // Accumulate it
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos++] = curChar;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_NUMERIC_VALUE:
                // Accumulate until delimitation
                if (IS_WHITE_SPACE(curChar)) {
                    // Null terminate the string
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos] = '\0';

                    // Process the value
                    CHK_STATUS(processAckValue(&pKinesisVideoStream->fragmentAckParser));

                    // Skip until the end of the value
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_VALUE_END;
                } else if (curChar == ACK_PARSER_COMMA) {
                    // Null terminate the string
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos] = '\0';

                    // Process the value
                    CHK_STATUS(processAckValue(&pKinesisVideoStream->fragmentAckParser));

                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_VALUE_END;
                } else if (curChar == ACK_PARSER_CLOSE_BRACE) {
                    // Null terminate the string
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos] = '\0';

                    // Process the value
                    CHK_STATUS(processAckValue(&pKinesisVideoStream->fragmentAckParser));

                    // Process the parsed ACK
                    CHK_STATUS(processParsedAck(pKinesisVideoStream));
                } else if (curChar == ACK_PARSER_QUOTE || curChar == ACK_PARSER_OPEN_BRACE || curChar == ACK_PARSER_OPEN_BRACKET ||
                           curChar == ACK_PARSER_CLOSE_BRACKET || curChar == ACK_PARSER_DELIMITER) {
                    // Error condition
                    CHK(FALSE, STATUS_INVALID_ACK_INVALID_VALUE_END);
                } else {
                    // Accumulate it
                    pKinesisVideoStream->fragmentAckParser.accumulator[pKinesisVideoStream->fragmentAckParser.curPos++] = curChar;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACE_END:
                // Skipping until the end of the body
                if (curChar == ACK_PARSER_OPEN_BRACE) {
                    level++;
                } else if (curChar == ACK_PARSER_CLOSE_BRACE) {
                    level--;
                }

                if (0 == level) {
                    // Body skipped. Move to next state
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_VALUE_END;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACKET_END:
                // Skipping until the end of the body
                if (curChar == ACK_PARSER_OPEN_BRACKET) {
                    level++;
                } else if (curChar == ACK_PARSER_CLOSE_BRACKET) {
                    level--;
                }

                if (0 == level) {
                    // Body skipped. Move to next state
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_VALUE_END;
                }

                break;

            case FRAGMENT_ACK_PARSER_STATE_VALUE_END:
                // End of the value
                // Skip the whitespaces or comma
                if (IS_WHITE_SPACE(curChar) || ACK_PARSER_COMMA == curChar) {
                    break;
                }

                // If it's a closing curly then we are done
                if (ACK_PARSER_CLOSE_BRACE == curChar) {
                    // Process the parsed ACK
                    CHK_STATUS(processParsedAck(pKinesisVideoStream));
                } else if (ACK_PARSER_QUOTE == curChar) {
                    // Start of the new key
                    pKinesisVideoStream->fragmentAckParser.state = FRAGMENT_ACK_PARSER_STATE_KEY_START;
                } else {
                    // An error is detected
                    CHK(FALSE, STATUS_INVALID_ACK_KEY_START);
                }

                break;
        }
    }

CleanUp:

    // Reset the parser on error
    if (STATUS_FAILED(retStatus)) {
        resetAckParserState(pKinesisVideoStream);
    }

    if (streamLocked) {
        pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    }

    LEAVES();
    return retStatus;
}

STATUS processParsedAck(PKinesisVideoStream pKinesisVideoStream)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pKinesisVideoStream != NULL, STATUS_NULL_ARG);

    // We are done with the current ACK - validate it first
    CHK_STATUS(validateParsedAck(&pKinesisVideoStream->fragmentAckParser));

    // Push the ACK
    CHK_STATUS(streamFragmentAckEvent(pKinesisVideoStream, pKinesisVideoStream->fragmentAckParser.uploadHandle,
                                      &pKinesisVideoStream->fragmentAckParser.fragmentAck));

    // Reset the parser state
    CHK_STATUS(resetAckParserState(pKinesisVideoStream));

CleanUp:

    LEAVES();
    return retStatus;
}

FRAGMENT_ACK_TYPE getFragmentAckType(PCHAR eventType)
{
    if (0 == STRNCMP(ACK_EVENT_TYPE_BUFFERING, eventType, SIZEOF(ACK_EVENT_TYPE_BUFFERING) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_TYPE_BUFFERING;
    } else if (0 == STRNCMP(ACK_EVENT_TYPE_RECEIVED, eventType, SIZEOF(ACK_EVENT_TYPE_RECEIVED) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_TYPE_RECEIVED;
    } else if (0 == STRNCMP(ACK_EVENT_TYPE_PERSISTED, eventType, SIZEOF(ACK_EVENT_TYPE_PERSISTED) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_TYPE_PERSISTED;
    } else if (0 == STRNCMP(ACK_EVENT_TYPE_ERROR, eventType, SIZEOF(ACK_EVENT_TYPE_ERROR) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_TYPE_ERROR;
    } else if (0 == STRNCMP(ACK_EVENT_TYPE_IDLE, eventType, SIZEOF(ACK_EVENT_TYPE_IDLE) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_TYPE_IDLE;
    } else {
        return FRAGMENT_ACK_TYPE_UNDEFINED;
    }
}

FRAGMENT_ACK_KEY_NAME getFragmentAckKeyName(PCHAR keyName)
{
    if (0 == STRNCMP(ACK_KEY_NAME_EVENT_TYPE, keyName, SIZEOF(ACK_KEY_NAME_EVENT_TYPE) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_KEY_NAME_EVENT_TYPE;
    } else if (0 == STRNCMP(ACK_KEY_NAME_FRAGMENT_NUMBER, keyName, SIZEOF(ACK_KEY_NAME_FRAGMENT_NUMBER) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_KEY_NAME_FRAGMENT_NUMBER;
    } else if (0 == STRNCMP(ACK_KEY_NAME_FRAGMENT_TIMECODE, keyName, SIZEOF(ACK_KEY_NAME_FRAGMENT_TIMECODE) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_KEY_NAME_FRAGMENT_TIMECODE;
    } else if (0 == STRNCMP(ACK_KEY_NAME_ERROR_ID, keyName, SIZEOF(ACK_KEY_NAME_ERROR_ID) / SIZEOF(CHAR))) {
        return FRAGMENT_ACK_KEY_NAME_ERROR_ID;
    } else {
        return FRAGMENT_ACK_KEY_NAME_UNKNOWN;
    }
}

STATUS processAckValue(PFragmentAckParser pFragmentAckParser)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 value;

    CHK(pFragmentAckParser != NULL, STATUS_NULL_ARG);

    switch (pFragmentAckParser->curKeyName) {
        case FRAGMENT_ACK_KEY_NAME_EVENT_TYPE:
            // Check if we have seen it already
            CHK(!pFragmentAckParser->keys[pFragmentAckParser->curKeyName], STATUS_INVALID_ACK_DUPLICATE_KEY_NAME);

            // Get the event type
            pFragmentAckParser->fragmentAck.ackType = getFragmentAckType(pFragmentAckParser->accumulator);

            break;

        case FRAGMENT_ACK_KEY_NAME_FRAGMENT_NUMBER:
            // Check if we have seen it already
            CHK(!pFragmentAckParser->keys[pFragmentAckParser->curKeyName], STATUS_INVALID_ACK_DUPLICATE_KEY_NAME);

            // Store the value
            STRNCPY(pFragmentAckParser->fragmentAck.sequenceNumber, pFragmentAckParser->accumulator, MAX_FRAGMENT_SEQUENCE_NUMBER);

            // Null terminate
            pFragmentAckParser->fragmentAck.sequenceNumber[MAX_FRAGMENT_SEQUENCE_NUMBER] = '\0';
            break;

        case FRAGMENT_ACK_KEY_NAME_FRAGMENT_TIMECODE:
            // Check if we have seen it already
            CHK(!pFragmentAckParser->keys[pFragmentAckParser->curKeyName], STATUS_INVALID_ACK_DUPLICATE_KEY_NAME);

            // Parse the value into a temporary variable. This is odd as the compiler for Arm v7 has an issue
            // in arranging the stack (FASTCALL) and the pointer passed in is being interpreted as the
            // return address which causes a crash. This is a quick workaround to use a temp variable instead.
            CHK_STATUS(STRTOUI64(pFragmentAckParser->accumulator, NULL, 10, &value));
            pFragmentAckParser->fragmentAck.timestamp = value;

            break;

        case FRAGMENT_ACK_KEY_NAME_ERROR_ID:
            // Check if we have seen it already
            CHK(!pFragmentAckParser->keys[pFragmentAckParser->curKeyName], STATUS_INVALID_ACK_DUPLICATE_KEY_NAME);

            // Parse the value
            CHK_STATUS(STRTOUI64(pFragmentAckParser->accumulator, NULL, 10, &value));

            // This will overwrite the value
            pFragmentAckParser->fragmentAck.result = getAckErrorTypeFromErrorId(value);

            break;

        default:
            // do nothing
            break;
    }

    // Set the 'seen' indicator
    pFragmentAckParser->keys[pFragmentAckParser->curKeyName] = TRUE;

    // Reset the accumulator
    pFragmentAckParser->curPos = 0;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS validateParsedAck(PFragmentAckParser pFragmentAckParser)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pFragmentAckParser != NULL, STATUS_NULL_ARG);

    CHK(pFragmentAckParser->fragmentAck.ackType != FRAGMENT_ACK_TYPE_UNDEFINED, STATUS_INVALID_PARSED_ACK_TYPE);

    CHK(pFragmentAckParser->fragmentAck.ackType != FRAGMENT_ACK_TYPE_ERROR || pFragmentAckParser->fragmentAck.result != SERVICE_CALL_RESULT_OK,
        STATUS_MISSING_ERR_ACK_ID);

    // The rest will be validated during the ACK processing.

CleanUp:

    LEAVES();
    return retStatus;
}

SERVICE_CALL_RESULT getAckErrorTypeFromErrorId(UINT64 errorId)
{
    // Cast to the service call result first
    SERVICE_CALL_RESULT callResult = (SERVICE_CALL_RESULT) errorId;

    switch (callResult) {
        case SERVICE_CALL_RESULT_STREAM_READ_ERROR:
        case SERVICE_CALL_RESULT_FRAGMENT_SIZE_REACHED:
        case SERVICE_CALL_RESULT_FRAGMENT_DURATION_REACHED:
        case SERVICE_CALL_RESULT_CONNECTION_DURATION_REACHED:
        case SERVICE_CALL_RESULT_FRAGMENT_TIMECODE_NOT_MONOTONIC:
        case SERVICE_CALL_RESULT_MULTI_TRACK_MKV:
        case SERVICE_CALL_RESULT_INVALID_MKV_DATA:
        case SERVICE_CALL_RESULT_INVALID_PRODUCER_TIMESTAMP:
        case SERVICE_CALL_RESULT_STREAM_NOT_ACTIVE:
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
        case SERVICE_CALL_RESULT_KMS_KEY_NOT_FOUND:
        case SERVICE_CALL_RESULT_STREAM_DELETED:
        case SERVICE_CALL_RESULT_ACK_INTERNAL_ERROR:
        case SERVICE_CALL_RESULT_FRAGMENT_ARCHIVAL_ERROR:
            return callResult;

        default:
            return SERVICE_CALL_RESULT_UNKNOWN_ACK_ERROR;
    }
}
