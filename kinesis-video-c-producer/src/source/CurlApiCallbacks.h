/*******************************************
CURL API callbacks internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_CURL_API_CALLBACKS_INCLUDE_I__
#define __KINESIS_VIDEO_CURL_API_CALLBACKS_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Dummy value returned as a device arn
#define DUMMY_DEVICE_ARN "arn:aws:kinesisvideo:us-west-2:11111111111:mediastream/device"

// API postfix definitions
#define CREATE_API_POSTFIX            "/createStream"
#define DESCRIBE_API_POSTFIX          "/describeStream"
#define GET_DATA_ENDPOINT_API_POSTFIX "/getDataEndpoint"
#define TAG_RESOURCE_API_POSTFIX      "/tagStream"
#define PUT_MEDIA_API_POSTFIX         "/putMedia"

// NOTE: THe longest string would be the tag resource where we get the maximal tag count and sizes plus some extra.
#define MAX_TAGS_JSON_PARAMETER_STRING_LEN (MAX_JSON_PARAMETER_STRING_LEN + (MAX_TAG_COUNT * (MAX_TAG_NAME_LEN + MAX_TAG_VALUE_LEN)))

// Max stream status string length in describe API call in chars
#define MAX_DESCRIBE_STREAM_STATUS_LEN 32

// Default curl API callbacks shutdown timeout
#define CURL_API_CALLBACKS_SHUTDOWN_TIMEOUT (50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// Default connection timeout
#define CURL_API_DEFAULT_CONNECTION_TIMEOUT (5000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// Default shutdown polling interval
#define CURL_API_DEFAULT_SHUTDOWN_POLLING_INTERVAL (200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// Max parameter JSON string for KMS key len
#define MAX_JSON_KMS_KEY_ID_STRING_LEN (MAX_ARN_LEN + 100)

// Parameterized string for CreateStream API
#define CREATE_STREAM_PARAM_JSON_TEMPLATE                                                                                                            \
    "{\n\t\"DeviceName\": \"%s\",\n\t"                                                                                                               \
    "\"StreamName\": \"%s\",\n\t\"MediaType\": \"%s\",\n\t"                                                                                          \
    "%s"                                                                                                                                             \
    "\"DataRetentionInHours\": %" PRIu64 "\n}"

// Parameterized string for optional KMS key id
#define KMS_KEY_PARAM_JSON_TEMPLATE "\"KmsKeyId\": \"%s\",\n\t"

// Parameterized string for DescribeStream API
#define DESCRIBE_STREAM_PARAM_JSON_TEMPLATE "{\n\t\"StreamName\": \"%s\"\n}"

// Parameterized string for GetDataEndpoint API
#define GET_DATA_ENDPOINT_PARAM_JSON_TEMPLATE                                                                                                        \
    "{\n\t\"StreamName\": \"%s\",\n\t"                                                                                                               \
    "\"APIName\": \"%s\"\n}"

// Parameterized string for TagStream API - we should have at least one tag
#define TAG_RESOURCE_PARAM_JSON_TEMPLATE                                                                                                             \
    "{\n\t\"StreamARN\": \"%s\",\n\t"                                                                                                                \
    "\"Tags\": {%s\n\t}\n}"

/**
 * Forward declarations
 */
struct __CallbacksProvider;

// Testability hooks functions
typedef STATUS (*CurlEasyPerformHookFunc)(PCurlResponse);
typedef STATUS (*CurlWriteCallbackHookFunc)(PCurlResponse, PCHAR, UINT32, PCHAR*, PUINT32);
typedef STATUS (*CurlReadCallbackHookFunc)(PCurlResponse, UPLOAD_HANDLE, PBYTE, UINT32, PUINT32, STATUS);

/**
 * Endpoint tracker
 */
typedef struct __EndpointTracker EndpointTracker;
struct __EndpointTracker {
    // Endpoint last update time
    UINT64 endpointLastUpdateTime;

    // Cached endpoint
    CHAR streamingEndpoint[MAX_URI_CHAR_LEN + 1];
};
typedef struct __EndpointTracker* PEndpointTracker;

/**
 * The KVS backend specific callbacks
 */
typedef struct __CurlApiCallbacks CurlApiCallbacks;
struct __CurlApiCallbacks {
    // First member should be the API callbacks
    ApiCallbacks apiCallbacks;

    // Representing the stream callbacks
    StreamCallbacks streamCallbacks;

    // Representing the producer callbacks
    ProducerCallbacks producerCallbacks;

    // Whether we have the producer client in shutdown or in shutdown sequence
    // Cant move shutdown to top of struct because it will affect callback mapping
    // Ensure shutdown is SIZE_T aligned
    volatile ATOMIC_BOOL shutdown;

    // Region name
    CHAR region[MAX_REGION_NAME_LEN + 1];

    // User agent
    CHAR userAgent[MAX_USER_AGENT_LEN + 1];

    // Control plane url of the service
    CHAR controlPlaneUrl[MAX_URI_CHAR_LEN + 1];

    // Cert path
    CHAR certPath[MAX_PATH_LEN + 1];

    // Back pointer to the callback provider object
    struct __CallbacksProvider* pCallbacksProvider;

    // Shutdown timeout which is passed through the callbacks when enumerating the hash table
    UINT64 shutdownTimeout;

    // Active uploads list: UPLOAD_HANDLE -> Node where data is a request
    PDoubleList pActiveUploads;

    // Lock guarding the active uploads
    MUTEX activeUploadsLock;

    // Ongoing requests: STREAM_HANDLE -> Request
    PHashTable pActiveRequests;

    // Lock guarding the active requests
    MUTEX activeRequestsLock;

    // Cached endpoints: STREAM_HANDLE -> EndpointTracker
    PHashTable pCachedEndpoints;

    // Lock guarding the endpoints table
    MUTEX cachedEndpointsLock;

    // Number of streaming session created
    UINT64 streamingRequestCount;

    // Cached endpoint update period
    UINT64 cacheUpdatePeriod;

    // Caching mode
    API_CALL_CACHE_TYPE cacheType;

    // Cached endpoints: STREAM_HANDLE -> EndpointTracker
    PHashTable pStreamsShuttingDown;

    // Lock guarding the endpoints table
    MUTEX shutdownLock;

    ///////////////////////////////////////////////
    // Test hooks for CURL calls

    // Custom data to be passed back to the caller
    UINT64 hookCustomData;

    // CURL call main API
    CurlEasyPerformHookFunc curlEasyPerformHookFn;

    // CURL write function called before processing ACKs
    CurlWriteCallbackHookFunc curlWriteCallbackHookFn;

    // CURL read function called after getStreamData
    CurlReadCallbackHookFunc curlReadCallbackHookFn;

    ///////////////////////////////////////////////
};
typedef struct __CurlApiCallbacks* PCurlApiCallbacks;

//////////////////////////////////////////////////////////////////////
// Curl API Callbacks main functionality
//////////////////////////////////////////////////////////////////////
STATUS createCurlApiCallbacks(struct __CallbacksProvider*, PCHAR, API_CALL_CACHE_TYPE, UINT64, PCHAR, PCHAR, PCHAR, PCHAR, PCurlApiCallbacks*);
STATUS freeCurlApiCallbacks(PCurlApiCallbacks*);
STATUS curlApiCallbacksShutdownActiveRequests(PCurlApiCallbacks, STREAM_HANDLE, UINT64, BOOL, BOOL);
STATUS curlApiCallbacksShutdownCachedEndpoints(PCurlApiCallbacks, STREAM_HANDLE, BOOL);
STATUS curlApiCallbacksShutdownActiveUploads(PCurlApiCallbacks, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, BOOL, BOOL);
STATUS curlApiCallbacksShutdown(PCurlApiCallbacks, UINT64);
STATUS freeApiCallbacksCurl(PUINT64);
STATUS findRequestWithUploadHandle(UPLOAD_HANDLE, PCurlApiCallbacks, PCurlRequest*);

//////////////////////////////////////////////////////////////////////
// Auxiliary functionality
//////////////////////////////////////////////////////////////////////
STATUS notifyCallResult(struct __CallbacksProvider*, STATUS, STREAM_HANDLE);
STREAM_STATUS getStreamStatusFromString(PCHAR, UINT32);
STATUS curlApiCallbacksMarkStreamShuttingdownCallback(UINT64, PHashEntry);
STATUS curlApiCallbacksCachedEndpointsTableShutdownCallback(UINT64, PHashEntry);
STATUS curlApiCallbacksFreeRequest(PCurlRequest);
STATUS checkApiCallEmulation(PCurlApiCallbacks, STREAM_HANDLE, PBOOL);

////////////////////////////////////////////////////////////////////////
// API Callback function implementations
////////////////////////////////////////////////////////////////////////
STATUS createDeviceCurl(UINT64, PCHAR, PServiceCallContext);
STATUS createStreamCurl(UINT64, PCHAR, PCHAR, PCHAR, PCHAR, UINT64, PServiceCallContext);
STATUS createStreamCachingCurl(UINT64, PCHAR, PCHAR, PCHAR, PCHAR, UINT64, PServiceCallContext);
STATUS describeStreamCurl(UINT64, PCHAR, PServiceCallContext);
STATUS describeStreamCachingCurl(UINT64, PCHAR, PServiceCallContext);
STATUS getStreamingEndpointCurl(UINT64, PCHAR, PCHAR, PServiceCallContext);
STATUS getStreamingEndpointCachingCurl(UINT64, PCHAR, PCHAR, PServiceCallContext);
STATUS tagResourceCurl(UINT64, PCHAR, UINT32, PTag, PServiceCallContext);
STATUS tagResourceCachingCurl(UINT64, PCHAR, UINT32, PTag, PServiceCallContext);
STATUS putStreamCurl(UINT64, PCHAR, PCHAR, UINT64, BOOL, BOOL, PCHAR, PServiceCallContext);

//////////////////////////////////////////////////////////////////////
// Stream Callbacks function implementations
//////////////////////////////////////////////////////////////////////
STATUS shutdownStreamCurl(UINT64, STREAM_HANDLE, BOOL);
STATUS dataAvailableCurl(UINT64, STREAM_HANDLE, PCHAR, UPLOAD_HANDLE, UINT64, UINT64);
STATUS streamClosedCurl(UINT64, STREAM_HANDLE, UPLOAD_HANDLE);
STATUS fragmentAckReceivedCurl(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, PFragmentAck);

//////////////////////////////////////////////////////////////////////
// Producer Callbacks function implementations
//////////////////////////////////////////////////////////////////////
STATUS clientShutdownCurl(UINT64, CLIENT_HANDLE);

////////////////////////////////////////////////////////////////////////
// API handler routines
////////////////////////////////////////////////////////////////////////
PVOID createStreamCurlHandler(PVOID);
PVOID describeStreamCurlHandler(PVOID);
PVOID getStreamingEndpointCurlHandler(PVOID);
PVOID tagResourceCurlHandler(PVOID);
PVOID putStreamCurlHandler(PVOID);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_CURL_API_CALLBACKS_INCLUDE_I__ */
