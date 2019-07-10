/*******************************************
Response internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_RESPONSE_INCLUDE_I__
#define __KINESIS_VIDEO_RESPONSE_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

// Setting this timeout to terminate CURL connection
#define TIMEOUT_AFTER_STREAM_STOPPED                            (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// HTTP status OK
#define HTTP_STATUS_CODE_OK                                     200

// HTTP status Request timed out
#define HTTP_STATUS_REQUEST_TIMEOUT                             408

// HTTP status code not set
#define HTTP_STATUS_CODE_NOT_SET                                0

// Slow speed time
#define CURLOPT_LOW_SPEED_TIME_VALUE                            30

// Slow speed limit in bytes per time
#define CURLOPT_LOW_SPEED_LIMIT_VALUE                           30

// Request id header name
#define KVS_REQUEST_ID_HEADER_NAME                              ((PCHAR) "x-amzn-RequestId")

// Header delimiter for curl requests and it's size
#define CURL_REQUEST_HEADER_DELIMITER                           ((PCHAR) ": ")
#define CURL_REQUEST_HEADER_DELIMITER_SIZE                      (2 * SIZEOF(CHAR))

// Pause/unpause interval for curl
#define CURL_PAUSE_UNPAUSE_INTERVAL                             (10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// Debug dump data file environment variable
#define KVS_DEBUG_DUMP_DATA_FILE_DIR_ENV_VAR       "KVS_DEBUG_DUMP_DATA_FILE_DIR"

/**
 * Curl Response structure
 */
typedef struct __CurlResponse CurlResponse;
struct __CurlResponse {
    // Related request object
    PCurlRequest pCurlRequest;

    // Curl object to use for the calls
    CURL* pCurl;

    // Whether the call was force-terminated
    volatile BOOL terminated;

    // HTTP status code of the execution
    UINT32 httpStatus;

    // Execution result
    SERVICE_CALL_RESULT callResult;

    // Error buffer for curl calls
    CHAR errorBuffer[CURL_ERROR_SIZE + 1];

    // Request Curl headers list
    struct curl_slist* pRequestHeaders;

    // Response Headers list
    PStackQueue pResponseHeaders;

    // Request ID if specified
    PCurlRequestHeader pRequestId;

    // Buffer to write the data to - will be allocated
    PCHAR responseData;

    // Response data size
    UINT32 responseDataLen;

    ///////////////////////////////////////////////
    // Variables needed for putMedia session

    // track whether end-of-stream has been received from getKinesisVideoStreamData
    BOOL endOfStream;

    // Whether curl is paused
    volatile BOOL paused;

    // Whether to dump streaming session into mkv file
    BOOL debugDumpFile;

    // Path of debug dump file
    CHAR debugDumpFilePath[MAX_PATH_LEN + 1];

    // Lock for exclusive access
    MUTEX lock;

    ///////////////////////////////////////////////
};
typedef struct __CurlResponse* PCurlResponse;

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////
/**
 * Creates a Response object
 *
 * @param - PCurlRequest - IN - Curl Request object to use for the response
 * @param - PCurlResponse* - IN/OUT - The newly created object
 *
 * @return - STATUS code of the execution
 */
STATUS createCurlResponse(PCurlRequest, PCurlResponse*);

/**
 * Frees a Response object
 *
 * @param - PCurlResponse* - IN/OUT - The object to release
 *
 * @return - STATUS code of the execution
 */
STATUS freeCurlResponse(PCurlResponse*);

/**
 * Closes curl handles
 *
 * @param - PCurlResponse - IN - Response object
 *
 * @return - STATUS code of the execution
 */
STATUS closeCurlHandles(PCurlResponse);

/**
 * Debug prints statistics of the request/response
 *
 * @param - PCurlResponse - IN - Response object
 *
 */
VOID dumpResponseCurlEasyInfo(PCurlResponse);

/**
 * Terminates the ongoing curl session
 *
 * @param - PCurlResponse - IN - Response object
 * @param - UINT64 - IN - Timeout value to use in 100ns
 *
 */
VOID terminateCurlSession(PCurlResponse, UINT64);

/**
 * Performs the curl request/response session.
 *
 * NOTE: This is a blocking API
 *
 * @param - PCurlResponse - IN - Response object
 *
 * @return - STATUS code of the execution
 */
STATUS curlCompleteSync(PCurlResponse);

/**
 * Notifies when data is available to read
 *
 * @param - PCurlResponse - IN - Response object
 * @param - UINT64 - IN - Duration of the buffer available in 100ns
 * @param - UINT64 - IN - Size of the buffer available in bytes
 *
 * @return - STATUS code of the execution
 */
STATUS notifyDataAvailable(PCurlResponse, UINT64, UINT64);

/**
 * Convenience method to convert HTTP statuses to SERVICE_CALL_RESULT status.
 *
 * @param - UINT32 - http_status the HTTP status code of the call
 *
 * @return The HTTP status translated into a SERVICE_CALL_RESULT value.
 */
SERVICE_CALL_RESULT getServiceCallResultFromHttpStatus(UINT32);

/**
 * Convenience method to convert CURL return status to SERVICE_CALL_RESULT status.
 *
 * @param - CURLcode - curl_status the CURL status code of the call
 *
 * @return The CURL code translated into a SERVICE_CALL_RESULT value.
 */
SERVICE_CALL_RESULT getServiceCallResultFromCurlStatus(CURLcode);

////////////////////////////////////////////////////
// Curl callbacks
////////////////////////////////////////////////////
SIZE_T writeHeaderCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postWriteCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postReadCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postResponseWriteCallback(PCHAR, SIZE_T, SIZE_T, PVOID);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_RESPONSE_INCLUDE_I__ */
