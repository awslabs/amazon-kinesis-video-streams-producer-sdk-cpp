#ifndef __KINESIS_VIDEO_IOT_AUTH_CALLBACKS_INCLUDE_I__
#define __KINESIS_VIDEO_IOT_AUTH_CALLBACKS_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
* Forward declarations
*/
struct __CallbacksProvider;

typedef struct __IotAuthCallbacks IotAuthCallbacks;

struct __IotAuthCallbacks {

    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Pointer to Iot Credential Provider
    PAwsCredentialProvider pCredentialProvider;

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __IotAuthCallbacks* PIotAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenIotFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenIotFunc(UINT64, PBYTE *, PUINT32, PUINT64);
STATUS freeIotAuthCallbacksFunc(PUINT64);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_IOT_AUTH_CALLBACKS_INCLUDE_I__ */
