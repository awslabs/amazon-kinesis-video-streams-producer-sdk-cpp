#ifndef __KINESISVIDEO_IOT_CREDENTIALS_AUTH_CALLBACKS_H__
#define __KINESISVIDEO_IOT_CREDENTIALS_AUTH_CALLBACKS_H__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

#define REQUEST_COMPLETION_TIMEOUT_MS       5000
#define SERVICE_CALL_PREFIX                 ((PCHAR) "https://")
#define ROLE_ALIASES_PATH                   ((PCHAR) "/role-aliases")
#define CREDENTIAL_SERVICE                  ((PCHAR) "/credentials" )
#define IOT_CERT_TYPE                       ((PCHAR) "PEM")
#define IOT_THING_NAME_HEADER               "x-amzn-iot-thingname:"

/**
* Grace period which is added to the current time to determine whether the extracted credentials are still valid
*/
#define IOT_CREDENTIAL_FETCH_GRACE_PERIOD               (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
* Forward declarations
*/
struct __CallbacksProvider;

typedef struct __IotAuthCallbacks IotAuthCallbacks;

struct __IotAuthCallbacks {

    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Pointer to Aws Credentials
    PAwsCredentials pAwsCredentials;

    // IoT credential endpoint
    CHAR iotGetCredentialEndpoint[MAX_URI_CHAR_LEN + 1];

    // IoT certificate file path
    CHAR certPath[MAX_PATH_LEN + 1];

    // IoT private key file path
    CHAR privateKeyPath[MAX_PATH_LEN + 1];

    // CA certificate file path
    CHAR caCertPath[MAX_PATH_LEN + 1];

    // IoT role alias
    CHAR roleAlias[MAX_ROLE_ALIAS_LEN + 1];

    // String name is used as IoT thing-name
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];

    // Buffer to write the data to - will be allocated
    PCHAR responseData;

    // Response data size
    UINT32 responseDataLen;

    // Used for cleanning up curl when free iot provider
    CURL* curl;

    // Back pointer to the callback provider object
    struct __CallbacksProvider* pCallbacksProvider;
};

typedef struct __IotAuthCallbacks* PIotAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenIotFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenIotFunc(UINT64, PBYTE *, PUINT32, PUINT64);
STATUS freeIotAuthCallbacksFunc(PUINT64);

// internal functions
STATUS iotCurlHandler(PIotAuthCallbacks);
STATUS parseIotResponse(PIotAuthCallbacks);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESISVIDEO_IOT_CREDENTIALS_AUTH_CALLBACKS_H__ */
