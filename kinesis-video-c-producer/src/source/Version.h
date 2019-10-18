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
#define AWS_SDK_KVS_PRODUCER_VERSION_STRING             (PCHAR) "2.0.2"

/**
 * Default user agent string
 */
#define USER_AGENT_NAME                                 (PCHAR) "AWS-SDK-KVS"

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////
/**
 * Returns the computed user agent string
 *
 * @param - PCHAR - IN/OPT - SDK postfix string
 * @param - PCHAR - IN/OPT - Custom user agent string to be appended at the end
 * @param - UINT32 - IN - Size of the output string
 * @param - PCHAR - OUT - User agent output string
 *
 * @return - STATUS code of the execution
 */
STATUS getUserAgentString(PCHAR, PCHAR, UINT32, PCHAR);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_VERSION_INCLUDE_I__ */
