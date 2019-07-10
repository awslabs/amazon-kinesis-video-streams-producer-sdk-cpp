/*******************************************
AWS V4 Signer internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_AWS_V4_SIGNE_INCLUDE_I__
#define __KINESIS_VIDEO_AWS_V4_SIGNE_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

#define AWS_SIG_V4_HEADER_AMZ_DATE              (PCHAR) "X-Amz-Date"
#define AWS_SIG_V4_HEADER_AMZ_SECURITY_TOKEN    (PCHAR) "x-amz-security-token"
#define AWS_SIG_V4_HEADER_AUTH                  (PCHAR) "Authorization"
#define AWS_SIG_V4_HEADER_HOST                  (PCHAR) "host"
#define AWS_SIG_V4_ALGORITHM                    (PCHAR) "AWS4-HMAC-SHA256"
#define AWS_SIG_V4_SIGNATURE_END                (PCHAR) "aws4_request"
#define AWS_SIG_V4_SIGNATURE_START              (PCHAR) "AWS4"
#define AWS_SIG_V4_CONTENT_TYPE_NAME            (PCHAR) "content-type"
#define AWS_SIG_V4_CONTENT_TYPE_VALUE           (PCHAR) "application/json"

// Datetime string length
#define SIGNATURE_DATE_TIME_STRING_LEN          17

// Date only string length
#define SIGNATURE_DATE_STRING_LEN               8

// Scratch buffer extra allocation
#define SCRATCH_BUFFER_EXTRA                    10000

// Datetime string format
#define DATE_TIME_STRING_FORMAT                 (PCHAR) "%Y%m%dT%H%M%SZ"

// Template for the credential scope
#define CREDENTIAL_SCOPE_TEMPLATE               (PCHAR) "%.*s/%s/%s/%s"

// Template for the signed string
#define SIGNED_STRING_TEMPLATE                  (PCHAR) "%s\n%s\n%s\n%s"

// Authentication header template
#define AUTH_HEADER_TEMPLATE                    (PCHAR) "%s Credential=%.*s/%s, SignedHeaders=%.*s, Signature=%s"

// Max string length for the request verb - no NULL terminator
#define MAX_REQUEST_VERB_STRING_LEN             4

// Checks whether a canonical header
#define IS_CANONICAL_HEADER_NAME(h)             ((0 != STRCMP((h), AWS_SIG_V4_HEADER_AMZ_SECURITY_TOKEN)) && \
                                                (0 != STRCMP((h), AWS_SIG_V4_HEADER_AUTH)))

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////
/**
 * Signs a request by appending SigV4 headers
 *
 * @param - PCurlRequest - IN/OUT Curl request to sign
 *
 * @return - STATUS code of the execution
 */
STATUS signCurlRequest(PCurlRequest);

/**
 * Generates a canonical request string
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - OUT/OPT - Canonical request string if specified
 * @param - PUINT32 - IN/OUT - String length in / required out
 *
 * @return - STATUS code of the execution
 */
STATUS generateCanonicalRequestString(PCurlRequest, PCHAR, PUINT32);

/**
 * Generates canonical headers
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - OUT/OPT - Canonical headers string if specified
 * @param - PUINT32 - IN/OUT - String length in / required out
 *
 * @return - STATUS code of the execution
 */
STATUS generateCanonicalHeaders(PCurlRequest, PCHAR, PUINT32);

/**
 * Generates signed headers string
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - OUT/OPT - Canonical headers string if specified
 * @param - PUINT32 - IN/OUT - String length in / required out
 *
 * @return - STATUS code of the execution
 */
STATUS generateSignedHeaders(PCurlRequest, PCHAR, PUINT32);

/**
 * Generates signature date time
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - OUT - Signature date/time with length of SIGNATURE_DATE_TIME_STRING_LEN characters including NULL terminator
 *
 * @return - STATUS code of the execution
 */
STATUS generateSignatureDateTime(PCurlRequest, PCHAR);

/**
 * Generates credential scope
 *
 * @param - PCurlRequest - IN - Request object
 * @param - PCHAR - IN - Datetime when signed
 * @param - PCHAR - OUT/OPT - Scope string if specified
 * @param - PUINT32 - IN/OUT - String length in / required out
 *
 * @return - STATUS code of the execution
 */
STATUS generateCredentialScope(PCurlRequest, PCHAR, PCHAR, PUINT32);

/**
 * Calculates a Hex encoded SHA256 of a message
 *
 * @param - PBYTE - IN - Message to calculate SHA256 for
 * @param - UINT32 - IN - Size of the message in bytes
 * @param - PCHAR - OUT - Hex encoded SHA256 with length of 2 * 32 characters
 *
 * @return - STATUS code of the execution
 */
STATUS hexEncodedSha256(PBYTE, UINT32, PCHAR);

/**
 *
 * Generates HMAC of a message
 *
 * @param - PBYTE - IN - key for HMAC
 * @param - UINT32 - IN - key length
 * @param - PBYTE - IN - message to compute HMAC
 * @param - UINT32 - IN - message length
 * @param - PBYTE - IN - buffer to place the result md
 * @param - PUINT32 - IN/OUT - length of resulting md placed in buffer
 *
 * @return - STATUS code of the execution
 */
STATUS generateRequestHmac(PBYTE, UINT32, PBYTE, UINT32, PBYTE, PUINT32);


#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_AWS_V4_SIGNE_INCLUDE_I__ */
