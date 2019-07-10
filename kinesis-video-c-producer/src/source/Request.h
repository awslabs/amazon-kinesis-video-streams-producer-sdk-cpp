/*******************************************
Request internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_REQUEST_INCLUDE_I__
#define __KINESIS_VIDEO_REQUEST_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

#define CURL_HTTPS_SCHEME "https"

// Max header name length in chars
#define MAX_REQUEST_HEADER_NAME_LEN             128

// Max header value length in chars
#define MAX_REQUEST_HEADER_VALUE_LEN            2048

// Max header count
#define MAX_REQUEST_HEADER_COUNT                200

// Max delimiter characters when packing headers into a string for printout
#define MAX_REQUEST_HEADER_OUTPUT_DELIMITER     5

// Max request header length in chars including the name/value, delimiter and null terminator
#define MAX_REQUEST_HEADER_STRING_LEN           (MAX_REQUEST_HEADER_NAME_LEN + MAX_REQUEST_HEADER_VALUE_LEN + 3)

// Literal definitions of the request verbs
#define CURL_VERB_GET_STRING                    (PCHAR) "GET"
#define CURL_VERB_PUT_STRING                    (PCHAR) "PUT"
#define CURL_VERB_POST_STRING                   (PCHAR) "POST"

// Schema delimiter string
#define SCHEMA_DELIMITER_STRING                 (PCHAR) "://"

// Default canonical URI if we fail to get anything from the parsing
#define DEFAULT_CANONICAL_URI_STRING            (PCHAR) "/"

/**
 * Forward declarations
 */
struct __CurlApiCallbacks;
struct __CurlResponse;

/**
 * Types of verbs
 */
typedef enum {
    CURL_VERB_GET,
    CURL_VERB_POST,
    CURL_VERB_PUT
} CURL_VERB;

/**
 * Curl Request Header structure
 */
typedef struct __CurlRequestHeader CurlRequestHeader;
struct __CurlRequestHeader {
    // Request header name
    PCHAR pName;

    // Header name length
    UINT32 nameLen;

    // Request header value
    PCHAR pValue;

    // Header value length
    UINT32 valueLen;
};
typedef struct __CurlRequestHeader* PCurlRequestHeader;

/**
 * Curl Request structure
 */
typedef struct __CurlRequest CurlRequest;
struct __CurlRequest {
    // Curl verb
    CURL_VERB verb;

    // Body of the request.
    // NOTE: In streaming mode the body will be NULL
    // NOTE: The body will follow the main struct
    PCHAR body;

    // Size of the body in bytes
    UINT32 bodySize;

    // The URL for the request
    CHAR url[MAX_URI_CHAR_LEN + 1];

    // Certificate path to use - optional
    CHAR certPath[MAX_PATH_LEN + 1];

    // StreamInfo pointer
    PStreamInfo pStreamInfo;

    // Call completion timeout
    UINT64 completionTimeout;

    // Connection completion timeout
    UINT64 connectionTimeout;

    // Call after time
    UINT64 callAfter;

    // Whether in streaming mode
    BOOL streaming;

    // Stream handle
    STREAM_HANDLE streamHandle;

    // AWS Credentials
    PAwsCredentials pAwsCredentials;

    // Curl callbacks pointer
    struct __CurlApiCallbacks* pCurlApiCallbacks;

    // Request headers
    PSingleList pRequestHeaders;

    // Curl response object
    struct __CurlResponse* pCurlResponse;

    // Mutex to be used as a lock for guarding the start routine
    MUTEX startLock;

    // The thread ID we are running the request on
    volatile TID threadId;

    // Indicating whether the request is being terminated
    volatile BOOL terminating;

    // Set on shutdown
    volatile BOOL shutdown;

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
STATUS createCurlRequest(CURL_VERB, PCHAR, PCHAR, STREAM_HANDLE, UINT64, UINT64, UINT64, PCHAR, PAwsCredentials, struct __CurlApiCallbacks*, PCurlRequest*);

/**
 * Frees a Request object
 *
 * @param - PCurlRequest* - IN/OUT - The object to release
 *
 * @return - STATUS code of the execution
 */
STATUS freeCurlRequest(PCurlRequest*);

/**
 * Checks whether the request requires a secure connection
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PBOOL - OUT - returned value
 *
 * @return - STATUS code of the execution
 */
STATUS requestRequiresSecureConnection(PCurlRequest, PBOOL);

/**
 * Sets a header in the request
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - IN - Header name
 * @param - UINT32 - IN/OPT - Header name length. Calculated if 0
 * @param - PCHAR - IN - Header value
 * @param - UINT32 - IN/OPT - Header value length. Calculated if 0
 *
 * @return - STATUS code of the execution
 */
STATUS setCurlRequestHeader(PCurlRequest, PCHAR, UINT32, PCHAR, UINT32);

/**
 * Gets a canonical URI for the request
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR* - OUT - The canonical URI start character
 * @param - PCHAR* - OUT - The canonical URI end character
 *
 * @return - STATUS code of the execution
 */
STATUS getCanonicalUri(PCurlRequest, PCHAR*, PCHAR*);

/**
 * Gets a request host string
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR* - OUT - The request host start character
 * @param - PCHAR* - OUT - The request host end character
 *
 * @return - STATUS code of the execution
 */
STATUS getRequestHost(PCurlRequest, PCHAR*, PCHAR*);

/**
 * Returns a string representing the specified Verb
 *
 * @param - CURL_VERB - IN - Specified Verb to convert to string representation
 *
 * @return - PCHAR - Representative string or NULL on error
 */
PCHAR getRequestVerbString(CURL_VERB);

/**
 * Creates a request header
 *
 * @param - PCHAR - IN - Header name
 * @param - UINT32 - IN - Header name length
 * @param - PCHAR - IN - Header value
 * @param - UINT32 - IN - Header value length
 * @param - PCurlRequestHeader* - OUT - Resulting object
 *
 * @return
 */
STATUS createRequestHeader(PCHAR, UINT32, PCHAR, UINT32, PCurlRequestHeader*);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_REQUEST_INCLUDE_I__ */
