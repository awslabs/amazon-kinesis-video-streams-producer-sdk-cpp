/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_COMMON_INCLUDE_I__
#define __KINESIS_VIDEO_COMMON_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#if defined(KVS_USE_OPENSSL)

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>

#define KVS_HMAC(k, klen, m, mlen, ob, plen)                                                                                                         \
    CHK(NULL != HMAC(EVP_sha256(), (k), (INT32)(klen), (m), (mlen), (ob), (plen)), STATUS_HMAC_GENERATION_ERROR);
#define KVS_SHA256(m, mlen, ob) SHA256((m), (mlen), (ob));

#elif defined(KVS_USE_MBEDTLS)

#include <mbedtls/sha256.h>
#include <mbedtls/md.h>
#define KVS_HMAC(k, klen, m, mlen, ob, plen)                                                                                                         \
    CHK(0 == mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (k), (klen), (m), (mlen), (ob)), STATUS_HMAC_GENERATION_ERROR);           \
    *(plen) = mbedtls_md_get_size(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256));
#define KVS_SHA256(m, mlen, ob) mbedtls_sha256((m), (mlen), (ob), 0);

#else
#error Need to specify the ssl library at build time
#endif

#if defined(KVS_BUILD_WITH_LWS)
#include <libwebsockets.h>
#endif
#if defined(KVS_BUILD_WITH_CURL)
#include <curl/curl.h>
#endif

#include <com/amazonaws/kinesis/video/common/Include.h>

/**
 * Opaque struct for OpenSSL locking
 */
typedef struct __CRYPTO_dynlock_value CRYPTO_dynlock_value;
struct __CRYPTO_dynlock_value {
    MUTEX mutex;
};
typedef struct __CRYPTO_dynlock_value* PCRYPTO_dynlock_value;

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

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_COMMON_INCLUDE_I__ */
