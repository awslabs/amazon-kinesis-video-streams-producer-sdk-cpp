/*******************************************
Auth internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_AUTH_INCLUDE_I__
#define __KINESIS_VIDEO_AUTH_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////

/**
 * Deserialize an AWS credentials object, adapt the accessKey/secretKey/sessionToken pointer
 * to offset following the AwsCredential structure
 *
 * @param - PBYTE - IN - Token to be deserialized.
 *
 * @return - STATUS code of the execution
 */
STATUS deserializeAwsCredentials(PBYTE);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_AUTH_INCLUDE_I__ */
