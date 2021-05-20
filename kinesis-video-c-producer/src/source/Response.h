/*******************************************
Response internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_RESPONSE_INCLUDE_I__
#define __KINESIS_VIDEO_RESPONSE_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Setting this timeout to terminate CURL connection
#define TIMEOUT_AFTER_STREAM_STOPPED (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// HTTP status code not set
#define HTTP_STATUS_CODE_NOT_SET 0

// Pause/unpause interval for curl
#define CURL_PAUSE_UNPAUSE_INTERVAL (10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// CA file extension
#define CA_CERT_FILE_SUFFIX ".pem"

// Debug dump data file environment variable
#define KVS_DEBUG_DUMP_DATA_FILE_DIR_ENV_VAR "KVS_DEBUG_DUMP_DATA_FILE_DIR"

/**
 * CURL callback function definitions
 */
typedef SIZE_T (*CurlCallbackFunc)(PCHAR, SIZE_T, SIZE_T, PVOID);

/**
 * Curl Response structure
 */
typedef struct __CurlResponse CurlResponse;
struct __CurlResponse {
    // Related request object
    PCurlRequest pCurlRequest;

    // Curl object to use for the calls
    CURL* pCurl;

    // Request Curl headers list
    struct curl_slist* pRequestHeaders;

    // Curl call data
    CallInfo callInfo;

    // Whether the call was force-terminated
    volatile ATOMIC_BOOL terminated;

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
 * @param - PCurlResponse - IN - Curl response object to close
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
 * Convenience method to convert CURL return status to SERVICE_CALL_RESULT status.
 *
 * @param - CURLcode - curl_status the CURL status code of the call
 *
 * @return The CURL code translated into a SERVICE_CALL_RESULT value.
 */
SERVICE_CALL_RESULT getServiceCallResultFromCurlStatus(CURLcode);

/**
 * Initializes curl session
 *
 * @param - PRequestInfo - IN - Request info object
 * @param - PCurlCallInfo - IN - Curl call info object to initialize values for
 * @param - Curl** - OUT - Curl object pointer to be set
 * @param - PVOID - IN - Data object to pass to Curl
 * @param - CurlCallbackFunc - IN - Curl write header callback
 * @param - CurlCallbackFunc - IN - Curl read callback
 * @param - CurlCallbackFunc - IN - Curl write callback
 * @param - CurlCallbackFunc - IN - Curl post write callback
 *
 * @return - STATUS code of the execution
 */
STATUS initializeCurlSession(PRequestInfo, PCallInfo, CURL**, PVOID, CurlCallbackFunc, CurlCallbackFunc, CurlCallbackFunc, CurlCallbackFunc);

////////////////////////////////////////////////////
// Curl callbacks
////////////////////////////////////////////////////
SIZE_T writeHeaderCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postWriteCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postReadCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
SIZE_T postResponseWriteCallback(PCHAR, SIZE_T, SIZE_T, PVOID);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_RESPONSE_INCLUDE_I__ */
