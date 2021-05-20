
#ifndef __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__
#define __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// CA pem file extension
#define CA_CERT_PEM_FILE_EXTENSION ".pem"

SIZE_T writeCurlResponseCallback(PCHAR, SIZE_T, SIZE_T, PVOID);
STATUS blockingCurlCall(PRequestInfo, PCallInfo);
STATUS createCurlHeaderList(PRequestInfo, struct curl_slist**);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_CURL_CALL_INCLUDE_I__ */
