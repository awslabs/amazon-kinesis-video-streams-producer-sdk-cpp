/*******************************************
Util internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_UTIL_INCLUDE_I__
#define __KINESIS_VIDEO_UTIL_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
 * Convert a time string to epoch
 *
 * @param - PCHAR - IN - The timesting to be passed
 * @param - PCHAR - IN - Current local time in seconds
 * @param - PUINT64 - OUT - returned value in epoch
 *
 * @return - STATUS code of the execution
 */
STATUS convertTimestampToEpoch(PCHAR, UINT64, PUINT64);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_UTIL_INCLUDE_I__ */