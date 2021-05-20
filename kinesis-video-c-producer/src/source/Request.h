/*******************************************
Request internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_REQUEST_INCLUDE_I__
#define __KINESIS_VIDEO_REQUEST_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */
struct __CurlApiCallbacks;
struct __CurlResponse;

/**
 * Curl Request structure
 */
typedef struct __CurlRequest CurlRequest;
struct __CurlRequest {
    // whether request is blocked in curl call
    volatile ATOMIC_BOOL blockedInCurl;

    // Request information
    RequestInfo requestInfo;

    // Stream name
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];

    // Whether in streaming mode
    BOOL streaming;

    // Stream handle
    STREAM_HANDLE streamHandle;

    // Curl callbacks pointer
    struct __CurlApiCallbacks* pCurlApiCallbacks;

    // Curl response object
    struct __CurlResponse* pCurlResponse;

    // Mutex to be used as a lock for guarding the start routine
    MUTEX startLock;

    // The thread ID we are running the request on
    volatile TID threadId;

    // upload handle if request.streaming is TRUE
    UPLOAD_HANDLE uploadHandle;
    // Body of the request will follow if specified
};
typedef struct __CurlRequest* PCurlRequest;

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////

/**
 * Creates a Request object
 *
 * @param - CURL_VERB - IN - Curl verb to use for the request
 * @param - PCHAR - IN - URL of the request
 * @param - PCHAR - IN/OPT - Body of the request
 * @param - STREAM_HANDLE - IN - Stream handle for which the request is for
 * @param - PCHAR - IN - Region
 * @param - UINT64 - IN - Current time
 * @param - UINT64 - IN - Connection timeout
 * @param - UINT64 - IN - Completion timeout
 * @param - UINT64 - IN - Call after time
 * @param - PCHAR - IN/OPT - Certificate path to use
 * @param - PAwsCredentials - IN/OPT - Credentials to use for the call
 * @param - PCurlApiCallbacks - IN - Curl API callbacks
 * @param - PCurlRequest* - IN/OUT - The newly created object
 *
 * @return - STATUS code of the execution
 */
STATUS createCurlRequest(HTTP_REQUEST_VERB, PCHAR, PCHAR, STREAM_HANDLE, PCHAR, UINT64, UINT64, UINT64, UINT64, PCHAR, PAwsCredentials,
                         struct __CurlApiCallbacks*, PCurlRequest*);

/**
 * Frees a Request object
 *
 * @param - PCurlRequest* - IN/OUT - The object to release
 *
 * @return - STATUS code of the execution
 */
STATUS freeCurlRequest(PCurlRequest*);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_REQUEST_INCLUDE_I__ */
