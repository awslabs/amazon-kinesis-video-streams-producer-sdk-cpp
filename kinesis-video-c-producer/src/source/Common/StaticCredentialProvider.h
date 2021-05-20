
#ifndef __KINESIS_VIDEO_STATIC_CREDENTIAL_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_STATIC_CREDENTIAL_PROVIDER_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */

typedef struct __StaticCredentialProvider StaticCredentialProvider;
struct __StaticCredentialProvider {
    // First member should be the abstract credential provider
    AwsCredentialProvider credentialProvider;

    // Storing the AWS credentials
    PAwsCredentials pAwsCredentials;
};
typedef struct __StaticCredentialProvider* PStaticCredentialProvider;

////////////////////////////////////////////////////////////////////////
// Callback function implementations
////////////////////////////////////////////////////////////////////////
STATUS getStaticCredentials(PAwsCredentialProvider, PAwsCredentials*);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_STATIC_CREDENTIAL_PROVIDER_INCLUDE_I__ */
