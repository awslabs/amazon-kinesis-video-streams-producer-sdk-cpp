/*******************************************
Kinesis Video ACK parser include file
*******************************************/
#ifndef __KINESIS_VIDEO_ACK_PARSER_H__
#define __KINESIS_VIDEO_ACK_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

// Various significant chars
#define ACK_PARSER_OPEN_BRACE    '{'
#define ACK_PARSER_CLOSE_BRACE   '}'
#define ACK_PARSER_OPEN_BRACKET  '['
#define ACK_PARSER_CLOSE_BRACKET ']'
#define ACK_PARSER_QUOTE         '\"'
#define ACK_PARSER_DELIMITER     ':'
#define ACK_PARSER_COMMA         ','

// ACK event names
#define ACK_EVENT_TYPE_BUFFERING "BUFFERING"
#define ACK_EVENT_TYPE_RECEIVED  "RECEIVED"
#define ACK_EVENT_TYPE_PERSISTED "PERSISTED"
#define ACK_EVENT_TYPE_IDLE      "IDLE"
#define ACK_EVENT_TYPE_ERROR     "ERROR"

// ACK JSON key names
#define ACK_KEY_NAME_EVENT_TYPE        "EventType"
#define ACK_KEY_NAME_FRAGMENT_NUMBER   "FragmentNumber"
#define ACK_KEY_NAME_FRAGMENT_TIMECODE "FragmentTimecode"
#define ACK_KEY_NAME_ERROR_ID          "ErrorId"

// If the character is start of numeric value
#define IS_ACK_START_OF_NUMERIC_VALUE(ch)                                                                                                            \
    ((ch) == '-' || (ch) == '+' || (ch) == '.' || ((ch) >= '0' && (ch) <= '9') || ((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))

/**
 * Kinesis Video ACK parser states
 */
typedef enum {
    // Initial state
    FRAGMENT_ACK_PARSER_STATE_START,

    // Found the opening curly brace
    FRAGMENT_ACK_PARSER_STATE_ACK_START,

    // Found the key start
    FRAGMENT_ACK_PARSER_STATE_KEY_START,

    // JSON delimiter state
    FRAGMENT_ACK_PARSER_STATE_DELIMITER,

    // Body start
    FRAGMENT_ACK_PARSER_STATE_BODY_START,

    // Skip body till end brace state
    FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACE_END,

    // Skip body till end bracket state
    FRAGMENT_ACK_PARSER_STATE_SKIP_BODY_BRACKET_END,

    // Text body parse state
    FRAGMENT_ACK_PARSER_STATE_TEXT_VALUE,

    // Numeric body parse state
    FRAGMENT_ACK_PARSER_STATE_NUMERIC_VALUE,

    // Value end state
    FRAGMENT_ACK_PARSER_STATE_VALUE_END,

} FRAGMENT_ACK_PARSER_STATE;

/**
 * Kinesis Video ACK key names
 */
typedef enum {
    // Event type
    FRAGMENT_ACK_KEY_NAME_EVENT_TYPE,

    // Fragment number
    FRAGMENT_ACK_KEY_NAME_FRAGMENT_NUMBER,

    // Fragment timecode
    FRAGMENT_ACK_KEY_NAME_FRAGMENT_TIMECODE,

    // Error id
    FRAGMENT_ACK_KEY_NAME_ERROR_ID,

    // Unknown type
    FRAGMENT_ACK_KEY_NAME_UNKNOWN,

    // Max enum item as a sentinel
    FRAGMENT_ACK_KEY_NAME_MAX,

} FRAGMENT_ACK_KEY_NAME;

/**
 * Kinesis Video ACK parser
 */
typedef struct __FragmentAckParser FragmentAckParser;
struct __FragmentAckParser {
    FRAGMENT_ACK_PARSER_STATE state;
    UPLOAD_HANDLE uploadHandle;
    FragmentAck fragmentAck;
    FRAGMENT_ACK_KEY_NAME curKeyName;
    UINT32 curPos;
    BOOL keys[FRAGMENT_ACK_KEY_NAME_MAX];
    CHAR accumulator[MAX_ACK_FRAGMENT_LEN + 1];
};
typedef struct __FragmentAckParser* PFragmentAckParser;

///////////////////////////////////////////////////////////////////////////////////////
// Functionality
///////////////////////////////////////////////////////////////////////////////////////

/**
 * Resets the state of the ack parser
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream to clear the parser
 *
 * @return STATUS of the operation
 */
STATUS resetAckParserState(PKinesisVideoStream);

/**
 * Parses the consecutive ACK fragment, assembles the ACK and calls the ACK consumption on success.
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream to use
 * @param 2 UPLOAD_HANDLE - Stream upload handle.
 * @param 3 PCHAR - current chunk to parse
 * @param 4 UINT32 - chunk size. If 0 is specified then the size will be till the NULL terminator.
 *
 * @return STATUS of the operation
 */
STATUS parseFragmentAck(PKinesisVideoStream, UPLOAD_HANDLE, PCHAR, UINT32);

/**
 * Tries to match a string to an ACK type. Returns Unknown if can't extract the type.
 *
 * @param 1 PCHAR - Event type to parse and match to an ACK type
 *
 * @return Fragment ACK type if successful
 */
FRAGMENT_ACK_TYPE getFragmentAckType(PCHAR);

/**
 * Tries to match a string to a key name. Returns Unknown if can't extract the key name.
 *
 * @param 1 PCHAR - Key name string to match.
 *
 * @return Key name if successful
 */
FRAGMENT_ACK_KEY_NAME getFragmentAckKeyName(PCHAR);

/**
 * Processes/consumes the parsed ACK value.
 *
 * @param 1 PFragmentAckParser - Ack parser
 *
 * @return STATUS of the operation
 */
STATUS processAckValue(PFragmentAckParser);

/**
 * Validates the newly parsed ACK.
 *
 * @param 1 PFragmentAckParser - Ack parser
 *
 * @return STATUS of the operation
 */
STATUS validateParsedAck(PFragmentAckParser);

/**
 * Gets a service call result from an ACK error id integer
 *
 * @param 1 UINT64 - Ack error id
 *
 * @return SERVICE_CALL_RESULT from the parsed string
 */
SERVICE_CALL_RESULT getAckErrorTypeFromErrorId(UINT64);

/**
 * Processed the parsed ACK
 *
 * @param 1 PKinesisVideoStream - Kinesis Video stream object
 *
 * @return STATUS of the operation
 */
STATUS processParsedAck(PKinesisVideoStream);

#ifdef __cplusplus
}
#endif

#endif // __KINESIS_VIDEO_ACK_PARSER_H__
