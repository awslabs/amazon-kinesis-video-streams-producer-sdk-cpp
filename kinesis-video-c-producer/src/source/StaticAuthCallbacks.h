
#ifndef __KINESISVIDEO_STATIC_AUTH_CALLBACKS_H__
#define __KINESISVIDEO_STATIC_AUTH_CALLBACKS_H__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
* Forward declarations
*/

typedef struct __StaticAuthCallbacks StaticAuthCallbacks;

struct __StaticAuthCallbacks {
    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Pointer to Static Aws Credentials
    PAwsCredentials  pAwsCredentials;

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __StaticAuthCallbacks* PStaticAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenStaticFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenStaticFunc(UINT64, PBYTE *, PUINT32, PUINT64);
STATUS freeStaticAuthCallbacksFunc(PUINT64);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESISVIDEO_STATIC_AUTH_CALLBACKS_H__ */
