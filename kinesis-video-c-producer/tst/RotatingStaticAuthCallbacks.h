
#ifndef __KINESISVIDEO_ROTATING_AUTH_CALLBACKS_H__
#define __KINESISVIDEO_ROTATING_AUTH_CALLBACKS_H__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
 * Forward declarations
 */
typedef struct __CallbacksProvider CallbacksProvider;
typedef struct __CallbacksProvider* PCallbacksProvider;

typedef struct __RotatingStaticAuthCallbacks RotatingStaticAuthCallbacks;

struct __RotatingStaticAuthCallbacks {
    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Pointer to Static Aws Credentials
    PAwsCredentials  pAwsCredentials;

    // Rotation Period
    UINT64 rotationPeriod;

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __RotatingStaticAuthCallbacks* PRotatingStaticAuthCallbacks;

STATUS createRotatingStaticAuthCallbacks(PClientCallbacks, PCHAR, PCHAR, PCHAR, UINT64, UINT64, PAuthCallbacks*);
STATUS freeRotatingStaticAuthCallbacks(PAuthCallbacks*);

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenEnvVarFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenEnvVarFunc(UINT64, PBYTE *, PUINT32, PUINT64);
STATUS freeRotatingStaticAuthCallbacksFunc(PUINT64);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESISVIDEO_ROTATING_AUTH_CALLBACKS_H__ */
