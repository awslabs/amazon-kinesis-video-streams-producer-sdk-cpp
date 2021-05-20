
#ifndef __KINESIS_VIDEO_STATIC_AUTH_CALLBACKS_INCLUDE_I__
#define __KINESIS_VIDEO_STATIC_AUTH_CALLBACKS_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */

typedef struct __StaticAuthCallbacks StaticAuthCallbacks;

struct __StaticAuthCallbacks {
    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Static credential provider
    PAwsCredentialProvider pCredentialProvider;

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __StaticAuthCallbacks* PStaticAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenStaticFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenStaticFunc(UINT64, PBYTE*, PUINT32, PUINT64);
STATUS freeStaticAuthCallbacksFunc(PUINT64);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_STATIC_AUTH_CALLBACKS_INCLUDE_I__ */
