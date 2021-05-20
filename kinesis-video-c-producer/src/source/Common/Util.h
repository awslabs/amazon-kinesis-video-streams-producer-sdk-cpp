/*******************************************
Util internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_UTIL_INCLUDE_I__
#define __KINESIS_VIDEO_UTIL_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define EARLY_EXPIRATION_FACTOR              1.0
#define IOT_EXPIRATION_PARSE_CONVERSION_BASE 10

#define SSL_CERTIFICATE_TYPE_UNKNOWN_STR ((PCHAR) "Unknown")
#define SSL_CERTIFICATE_TYPE_DER_STR     ((PCHAR) "DER")
#define SSL_CERTIFICATE_TYPE_ENG_STR     ((PCHAR) "ENG")
#define SSL_CERTIFICATE_TYPE_PEM_STR     ((PCHAR) "PEM")

PCHAR getSslCertNameFromType(SSL_CERTIFICATE_TYPE);
UINT64 commonDefaultGetCurrentTimeFunc(UINT64);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_UTIL_INCLUDE_I__ */