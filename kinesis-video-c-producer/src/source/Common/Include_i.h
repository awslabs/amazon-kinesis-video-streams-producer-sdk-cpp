/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_COMMON_INCLUDE_I__
#define __KINESIS_VIDEO_COMMON_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#if defined(KVS_BUILD_WITH_LWS)
#include <libwebsockets.h>
#endif
#if defined(KVS_BUILD_WITH_CURL)
#include <curl/curl.h>
#endif

#include <com/amazonaws/kinesis/video/client/Include.h>
#include <com/amazonaws/kinesis/video/common/Include.h>

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

////////////////////////////////////////////////////
// Project internal includes
////////////////////////////////////////////////////
#include "Version.h"
#include "Auth.h"
#include "StaticCredentialProvider.h"
#include "FileCredentialProvider.h"

#if !(defined(KVS_BUILD_WITH_CURL) || defined(KVS_BUILD_WITH_LWS))
#error Need to specify the networking client at build time
#endif
#if defined(KVS_BUILD_WITH_CURL)
#include "Curl/CurlCall.h"
#include "Curl/CurlIotCredentialProvider.h"
#endif
#if defined(KVS_BUILD_WITH_LWS)
#include "Lws/LwsCall.h"
#include "Lws/LwsIotCredentialProvider.h"
#endif

#include "IotCredentialProvider.h"
#include "AwsV4Signer.h"
#include "Util.h"
#include "RequestInfo.h"

////////////////////////////////////////////////////
// Project internal defines
////////////////////////////////////////////////////

////////////////////////////////////////////////////
// Project internal functions
////////////////////////////////////////////////////

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_COMMON_INCLUDE_I__ */
