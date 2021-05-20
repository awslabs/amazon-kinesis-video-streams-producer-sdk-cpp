
#ifndef __KINESIS_VIDEO_FILE_AUTH_CALLBACK_INCLUDE_I__
#define __KINESIS_VIDEO_FILE_AUTH_CALLBACK_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __FileAuthCallbacks FileAuthCallbacks;

struct __FileAuthCallbacks {
    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // File credential provider
    PAwsCredentialProvider pCredentialProvider;

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __FileAuthCallbacks* PFileAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenFileFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenFileFunc(UINT64, PBYTE*, PUINT32, PUINT64);
STATUS freeFileAuthCallbacksFunc(PUINT64);

#ifdef __cplusplus
}
#endif
#endif //__KINESIS_VIDEO_FILE_AUTH_CALLBACK_INCLUDE_I__