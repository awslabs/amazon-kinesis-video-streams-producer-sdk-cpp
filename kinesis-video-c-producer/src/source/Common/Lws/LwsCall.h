
#ifndef __KINESIS_VIDEO_LWS_CALL_INCLUDE_I__
#define __KINESIS_VIDEO_LWS_CALL_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

// Max send buffer size for LWS
#define IOT_LWS_SEND_BUFFER_SIZE                        (LWS_PRE + MAX_URI_CHAR_LEN)

STATUS blockingLwsCall(PRequestInfo, PCallInfo);
INT32 lwsIotCallbackRoutine(struct lws*, enum lws_callback_reasons, PVOID, PVOID, size_t);

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_LWS_CALL_INCLUDE_I__ */
