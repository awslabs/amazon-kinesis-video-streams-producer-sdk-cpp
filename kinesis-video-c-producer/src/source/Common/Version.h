/*******************************************
Auth internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_VERSION_INCLUDE_I__
#define __KINESIS_VIDEO_VERSION_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
 * IMPORTANT!!! This is the current version of the SDK which needs to be maintained
 */
#define AWS_SDK_KVS_PRODUCER_VERSION_STRING             (PCHAR) "2.0.1"

/**
 * Default user agent string
 */
#define USER_AGENT_NAME                                 (PCHAR) "AWS-SDK-KVS"

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_VERSION_INCLUDE_I__ */
