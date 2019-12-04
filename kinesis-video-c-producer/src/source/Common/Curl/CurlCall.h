
#ifndef __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__
#define __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

// CA pem file extension
#define CA_CERT_PEM_FILE_EXTENSION                          ".pem"

SIZE_T writeCurlResponseCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
STATUS blockingCurlCall(PRequestInfo, PCallInfo);
STATUS createCurlHeaderList(PRequestInfo, struct curl_slist**);
    
#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__ */
