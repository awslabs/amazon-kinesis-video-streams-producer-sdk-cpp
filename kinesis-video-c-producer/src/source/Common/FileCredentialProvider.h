
#ifndef __KINESIS_VIDEO_FILE_CREDENTIAL_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_FILE_CREDENTIAL_PROVIDER_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Grace period which is added to the current time to determine whether the extracted credentials are still valid
 */
#define CREDENTIAL_FILE_READ_GRACE_PERIOD                                                                                                            \
    (5 * HUNDREDS_OF_NANOS_IN_A_SECOND + MIN_STREAMING_TOKEN_EXPIRATION_DURATION + STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD)

typedef struct __FileCredentialProvider FileCredentialProvider;
struct __FileCredentialProvider {
    // First member should be the abstract credential provider
    AwsCredentialProvider credentialProvider;

    // Current time functionality - optional
    GetCurrentTimeFunc getCurrentTimeFn;

    // Custom data supplied to time function
    UINT64 customData;

    // Static Aws Credentials structure with the pointer following the main allocation
    PAwsCredentials pAwsCredentials;

    // Pointer to credential file path
    PCHAR credentialsFilepath[MAX_PATH_LEN + 1];
};
typedef struct __FileCredentialProvider* PFileCredentialProvider;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////
STATUS getFileCredentials(PAwsCredentialProvider, PAwsCredentials*);

// Internal functionality
STATUS readFileCredentials(PFileCredentialProvider);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_FILE_CREDENTIAL_PROVIDER_INCLUDE_I__ */
