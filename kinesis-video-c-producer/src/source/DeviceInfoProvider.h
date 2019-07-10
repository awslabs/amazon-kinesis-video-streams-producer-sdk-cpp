/*******************************************
DeviceInfoProvider internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_DEVICE_INFO_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_DEVICE_INFO_PROVIDER_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

#define STORAGE_ALLOCATION_DEFRAGMENTATION_FACTOR                           1.2
#define DEFAULT_STREAM_COUNT                                                100
#define DEFAULT_DEVICE_STORAGE_SIZE                                         (128 * 1024 * 1024)

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif //__KINESIS_VIDEO_DEVICE_INFO_PROVIDER_INCLUDE_I__
