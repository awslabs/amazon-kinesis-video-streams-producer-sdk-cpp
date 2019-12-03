
#ifndef __KINESIS_VIDEO_LWS_IOT_CREDENTIAL_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_LWS_IOT_CREDENTIAL_PROVIDER_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

STATUS blockingLwsCall(PRequestInfo, PCallInfo);
    
#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_LWS_IOT_CREDENTIAL_PROVIDER_INCLUDE_I__ */
