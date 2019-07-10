
#ifndef __KINESISVIDEO_FILE_AUTH_CALLBACKS_H__
#define __KINESISVIDEO_FILE_AUTH_CALLBACKS_H__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
* Grace period which is added to the current time to determine whether the extracted credentials are still valid
*/
#define CREDENTIAL_FILE_READ_GRACE_PERIOD               (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)

typedef struct __FileAuthCallbacks FileAuthCallbacks;

struct __FileAuthCallbacks {
    // First member should be the Auth callbacks
    AuthCallbacks authCallbacks;

    // Static Aws Credentials structure with the pointer following the main allocation
    PAwsCredentials pAwsCredentials;

    //Pointer to credential file path
    PCHAR credentialsFilepath[MAX_PATH_LEN + 1];

    // Back pointer to the callback provider object
    PCallbacksProvider pCallbacksProvider;
};

typedef struct __FileAuthCallbacks *PFileAuthCallbacks;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////

// The callback functions
STATUS getStreamingTokenFileFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
STATUS getSecurityTokenFileFunc(UINT64, PBYTE *, PUINT32, PUINT64);
STATUS freeFileAuthCallbacksFunc(PUINT64);
STATUS readFileCredentials(PFileAuthCallbacks, UINT64);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif //__KINESISVIDEO_FILE_AUTH_CALLBACKS_H__