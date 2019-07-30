/**
 * Kinesis Video Producer AWS V4 Signer functionality
 */
#define LOG_CLASS "AwsV4Signer"
#include "Include_i.h"

STATUS signCurlRequest(PCurlRequest pCurlRequest)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 requestLen, hostLen, scopeLen, signedStrLen, signedHeadersLen = 0, scratchLen, curSize, hmacSize, hexHmacLen;
    PCHAR pHostStart, pHostEnd;
    CHAR dateTimeStr[SIGNATURE_DATE_TIME_STRING_LEN];
    PCHAR pScratchBuf = NULL, pCredentialScope = NULL, pSignedStr = NULL, pSignedHeaders = NULL;
    CHAR requestHexSha256[2 * SHA256_DIGEST_LENGTH + 1];
    BYTE hmac[EVP_MAX_MD_SIZE];
    CHAR hexHmac[EVP_MAX_MD_SIZE * 2 + 1];

    CHK(pCurlRequest != NULL && pCurlRequest->pCurlApiCallbacks != NULL && pCurlRequest->pAwsCredentials != NULL, STATUS_NULL_ARG);

    // Generate the time
    CHK_STATUS(generateSignatureDateTime(pCurlRequest, dateTimeStr));

    // Get the host header
    CHK_STATUS(getRequestHost(pCurlRequest, &pHostStart, &pHostEnd));
    hostLen = (UINT32) (pHostEnd - pHostStart);

    CHK_STATUS(setCurlRequestHeader(pCurlRequest, AWS_SIG_V4_HEADER_HOST, 0, pHostStart, hostLen));
    CHK_STATUS(setCurlRequestHeader(pCurlRequest, AWS_SIG_V4_HEADER_AMZ_DATE, 0, dateTimeStr, 0));
    CHK_STATUS(setCurlRequestHeader(pCurlRequest, AWS_SIG_V4_CONTENT_TYPE_NAME, 0, AWS_SIG_V4_CONTENT_TYPE_VALUE, 0));

    // Get the required sizes and select the largest for the scratch buffer
    CHK_STATUS(generateCanonicalRequestString(pCurlRequest, NULL, &requestLen));
    CHK_STATUS(generateCredentialScope(pCurlRequest, dateTimeStr, NULL, &scopeLen));
    CHK_STATUS(generateSignedHeaders(pCurlRequest, NULL, &signedHeadersLen));

    scratchLen = MAX(requestLen, scopeLen) + MAX_AUTH_LEN + SCRATCH_BUFFER_EXTRA;

    // Get the request length and allocate enough space and package the request
    CHK(NULL != (pScratchBuf = (PCHAR) MEMALLOC(scratchLen * SIZEOF(CHAR))), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(generateCanonicalRequestString(pCurlRequest, pScratchBuf, &requestLen));

    // Calculate the hex encoded SHA256 of the canonical request
    CHK_STATUS(hexEncodedSha256((PBYTE) pScratchBuf, requestLen * SIZEOF(CHAR), requestHexSha256));

    // Get the credential scope
    CHK(NULL != (pCredentialScope = (PCHAR) MEMALLOC(scopeLen * SIZEOF(CHAR))), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(generateCredentialScope(pCurlRequest, dateTimeStr, pCredentialScope, &scopeLen));

    // Get the signed headers
    CHK(NULL != (pSignedHeaders = (PCHAR) MEMALLOC(signedHeadersLen * SIZEOF(CHAR))), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(generateSignedHeaders(pCurlRequest, pSignedHeaders, &signedHeadersLen));

    // https://docs.aws.amazon.com/general/latest/gr/sigv4-create-string-to-sign.html
    // StringToSign =
    //    Algorithm + \n +
    //    RequestDateTime + \n +
    //    CredentialScope + \n +
    //    HashedCanonicalRequest
    signedStrLen = (UINT32) STRLEN(AWS_SIG_V4_ALGORITHM) + 1 +
            SIGNATURE_DATE_TIME_STRING_LEN + 1 +
            scopeLen + 1 +
            SIZEOF(requestHexSha256) + 1;
    CHK(NULL != (pSignedStr = (PCHAR) MEMALLOC(signedStrLen * SIZEOF(CHAR))), STATUS_NOT_ENOUGH_MEMORY);
    curSize = SNPRINTF(pSignedStr, signedStrLen, SIGNED_STRING_TEMPLATE, AWS_SIG_V4_ALGORITHM, dateTimeStr,
                       pCredentialScope, requestHexSha256);
    CHK(curSize > 0 && curSize < signedStrLen, STATUS_BUFFER_TOO_SMALL);

    // Set the actual size
    signedStrLen = curSize;

    // Create V4 signature
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-calculate-signature.html
    curSize = (UINT32) STRLEN(AWS_SIG_V4_SIGNATURE_START);
    MEMCPY(pScratchBuf, AWS_SIG_V4_SIGNATURE_START, curSize);
    MEMCPY((PBYTE) pScratchBuf + curSize, pCurlRequest->pAwsCredentials->secretKey, pCurlRequest->pAwsCredentials->secretKeyLen);
    curSize += pCurlRequest->pAwsCredentials->secretKeyLen;

    hmacSize = SIZEOF(hmac);
    CHK_STATUS(generateRequestHmac((PBYTE) pScratchBuf, curSize, (PBYTE) dateTimeStr,
                                   SIGNATURE_DATE_STRING_LEN * SIZEOF(CHAR),
                                   hmac, &hmacSize));
    CHK_STATUS(generateRequestHmac(hmac, hmacSize, (PBYTE) pCurlRequest->pCurlApiCallbacks->region,
                                   (UINT32) STRLEN(pCurlRequest->pCurlApiCallbacks->region),
                                   hmac, &hmacSize));
    CHK_STATUS(generateRequestHmac(hmac, hmacSize, (PBYTE) KINESIS_VIDEO_SERVICE_NAME,
                                   (UINT32) STRLEN(KINESIS_VIDEO_SERVICE_NAME),
                                   hmac, &hmacSize));
    CHK_STATUS(generateRequestHmac(hmac, hmacSize, (PBYTE) AWS_SIG_V4_SIGNATURE_END,
                                   (UINT32) STRLEN(AWS_SIG_V4_SIGNATURE_END),
                                   hmac, &hmacSize));
    CHK_STATUS(generateRequestHmac(hmac, hmacSize, (PBYTE) pSignedStr,
                                   signedStrLen * SIZEOF(CHAR),
                                   hmac, &hmacSize));

    // Increment the curSize to account for the NULL terminator that's required by the hex encoder
    hexHmacLen = ARRAY_SIZE(hexHmac);
    CHK_STATUS(hexEncodeCase(hmac, hmacSize, hexHmac, &hexHmacLen, FALSE));

    // set the auth header
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-add-signature-to-request.html
    curSize = SNPRINTF(pScratchBuf, scratchLen, AUTH_HEADER_TEMPLATE,
                       AWS_SIG_V4_ALGORITHM, pCurlRequest->pAwsCredentials->accessKeyIdLen,
                       pCurlRequest->pAwsCredentials->accessKeyId,
                       pCredentialScope, signedHeadersLen, pSignedHeaders, hexHmac);
    CHK(curSize > 0 && curSize < scratchLen, STATUS_BUFFER_TOO_SMALL);

    // Set the header
    CHK_STATUS(setCurlRequestHeader(pCurlRequest, AWS_SIG_V4_HEADER_AUTH, 0, pScratchBuf, curSize));

    // Set the security token header if provided
    if (pCurlRequest->pAwsCredentials->sessionTokenLen != 0) {
        CHK_STATUS(setCurlRequestHeader(pCurlRequest,
                                        AWS_SIG_V4_HEADER_AMZ_SECURITY_TOKEN,
                                        0,
                                        pCurlRequest->pAwsCredentials->sessionToken,
                                        pCurlRequest->pAwsCredentials->sessionTokenLen));
    }

CleanUp:

    if (pScratchBuf != NULL) {
        MEMFREE(pScratchBuf);
    }

    if (pCredentialScope != NULL) {
        MEMFREE(pCredentialScope);
    }

    if (pSignedStr != NULL) {
        MEMFREE(pSignedStr);
    }

    if (pSignedHeaders != NULL) {
        MEMFREE(pSignedHeaders);
    }

    LEAVES();
    return retStatus;
}

/**
 * Create a canonical request string for signing
 *
 * Info: http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html
 *
 * @param pCurlRequest
 *
 * @return Status code of the operation
 */
STATUS generateCanonicalRequestString(PCurlRequest pCurlRequest, PCHAR pRequestStr, PUINT32 pRequestLen)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pCurPtr, pVerbString, pUriStart, pUriEnd, pQueryStart, pQueryEnd;
    UINT32 requestLen = 0, curLen, len, itemCount;

    CHK(pCurlRequest != NULL && pRequestLen != NULL, STATUS_NULL_ARG);

    CHK_STATUS(singleListGetNodeCount(pCurlRequest->pRequestHeaders, &itemCount));

    // Calculate the rough max size first including the new lines and hex of the 256 bit hash (2 * 32)
    //    CanonicalRequest =
    //            HTTPRequestMethod + '\n' +
    //            CanonicalURI + '\n' +
    //            CanonicalQueryString + '\n' +
    //            CanonicalHeaders + '\n' +
    //            SignedHeaders + '\n' +
    //            HexEncode(Hash(RequestPayload))
    requestLen = MAX_REQUEST_VERB_STRING_LEN + 1 +
                 MAX_URI_CHAR_LEN + 1 +
                 MAX_URI_CHAR_LEN + 1 +
                 itemCount * (MAX_REQUEST_HEADER_NAME_LEN + 1 + MAX_REQUEST_HEADER_VALUE_LEN + 1) +
                 itemCount * (MAX_REQUEST_HEADER_NAME_LEN + 1) +
                 SHA256_DIGEST_LENGTH * 2 + 1;

    // See if we only are interested in the size
    CHK(pRequestStr != NULL, retStatus);

    pCurPtr = pRequestStr;
    requestLen = *pRequestLen;
    curLen = 0;

    // Get the request verb string
    pVerbString = getRequestVerbString(pCurlRequest->verb);
    CHK(pVerbString != NULL, STATUS_INVALID_ARG);
    len = (UINT32) STRLEN(pVerbString);
    CHK(curLen + len + 1 <= requestLen, STATUS_BUFFER_TOO_SMALL);
    MEMCPY(pCurPtr, pVerbString, SIZEOF(CHAR) * len);
    pCurPtr += len;
    *pCurPtr++ = '\n';
    curLen += len + 1;

    // Get the canonical URI
    CHK_STATUS(getCanonicalUri(pCurlRequest, &pUriStart, &pUriEnd));
    len = (UINT32) (pUriEnd - pUriStart);
    CHK(curLen + len + 1 <= requestLen, STATUS_BUFFER_TOO_SMALL);
    MEMCPY(pCurPtr, pUriStart, len * SIZEOF(CHAR));
    pCurPtr += len;
    *pCurPtr++ = '\n';
    curLen += len + 1;

    // Get the canonical query.
    // TODO: Implement this properly. In KVS we don't use params currently
    pQueryStart = EMPTY_STRING;
    pQueryEnd = pQueryStart;
    len = (UINT32) (pQueryEnd - pQueryStart);
    CHK(curLen + len + 1 <= requestLen, STATUS_BUFFER_TOO_SMALL);
    MEMCPY(pCurPtr, pQueryStart, len * SIZEOF(CHAR));
    pCurPtr += len;
    *pCurPtr++ = '\n';
    curLen += len + 1;

    len = requestLen - curLen;
    CHK_STATUS(generateCanonicalHeaders(pCurlRequest, pCurPtr, &len));
    CHK(curLen + len + 1 <= requestLen, STATUS_BUFFER_TOO_SMALL);
    pCurPtr += len;

    *pCurPtr++ = '\n';
    curLen += len + 1;

    len = requestLen - curLen;
    CHK_STATUS(generateSignedHeaders(pCurlRequest, pCurPtr, &len));
    CHK(curLen + len + 1 <= requestLen, STATUS_BUFFER_TOO_SMALL);
    pCurPtr += len;
    *pCurPtr++ = '\n';
    curLen += len + 1;

    // Generate the hex encoded hash
    len = SHA256_DIGEST_LENGTH * 2;
    CHK(curLen + len <= requestLen, STATUS_BUFFER_TOO_SMALL);
    if (pCurlRequest->streaming) {
        // Streaming treats this portion as if the body were empty
        CHK_STATUS(hexEncodedSha256((PBYTE) EMPTY_STRING, 0, pCurPtr));
    } else {
        // standard signing
        CHK_STATUS(hexEncodedSha256((PBYTE) pCurlRequest->body, pCurlRequest->bodySize, pCurPtr));
    }

    pCurPtr += len;
    curLen += len;

    CHK(curLen <= requestLen, STATUS_BUFFER_TOO_SMALL);
    requestLen = curLen;

CleanUp:

    if (pRequestLen != NULL) {
        *pRequestLen = requestLen;
    }

    LEAVES();
    return retStatus;
}

STATUS generateCanonicalHeaders(PCurlRequest pCurlRequest, PCHAR pCanonicalHeaders, PUINT32 pCanonicalHeadersLen)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 overallLen = 0, valueLen, specifiedLen;
    PSingleListNode pCurNode;
    PCurlRequestHeader pCurlRequestHeader;
    UINT64 item;
    PCHAR pStart, pEnd;
    PCHAR pCurPtr = pCanonicalHeaders;

    CHK(pCurlRequest != NULL && pCanonicalHeadersLen != NULL, STATUS_NULL_ARG);

    specifiedLen = *pCanonicalHeadersLen;

    CHK_STATUS(singleListGetHeadNode(pCurlRequest->pRequestHeaders, &pCurNode));

    // Iterate through the headers
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pCurlRequestHeader = (PCurlRequestHeader) item;

        // Process only if we have a canonical header name
        if (IS_CANONICAL_HEADER_NAME(pCurlRequestHeader->pName)) {
            CHK_STATUS(TRIMSTRALL(pCurlRequestHeader->pValue, pCurlRequestHeader->valueLen, &pStart, &pEnd));
            valueLen = (UINT32) (pEnd - pStart);

            // Increase the overall length as we use the lower-case header, colon, trimmed lower-case value and new line
            overallLen += pCurlRequestHeader->nameLen + 1 + valueLen + 1;

            // Pack if we have the destination specified
            if (pCanonicalHeaders != NULL) {
                CHK(overallLen <= specifiedLen, STATUS_BUFFER_TOO_SMALL);

                // Copy over and convert to lower case
                TOLOWERSTR(pCurlRequestHeader->pName, pCurlRequestHeader->nameLen, pCurPtr);
                pCurPtr += pCurlRequestHeader->nameLen;

                // Append the colon
                *pCurPtr++ = ':';

                // Append the trimmed lower-case string
                STRNCPY(pCurPtr, pStart, valueLen);
                pCurPtr += valueLen;

                // Append the new line
                *pCurPtr++ = '\n';
            }
        }

        // Iterate
        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

CleanUp:

    if (pCanonicalHeadersLen != NULL) {
        *pCanonicalHeadersLen = overallLen;
    }

    LEAVES();
    return retStatus;
}

STATUS generateSignedHeaders(PCurlRequest pCurlRequest, PCHAR pSignedHeaders, PUINT32 pSignedHeadersLen)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 overallLen = 0, specifiedLen;
    PSingleListNode pCurNode;
    PCurlRequestHeader pCurlRequestHeader;
    UINT64 item;
    PCHAR pCurPtr = pSignedHeaders;
    BOOL appended = FALSE;

    CHK(pCurlRequest != NULL && pSignedHeadersLen != NULL, STATUS_NULL_ARG);

    specifiedLen = *pSignedHeadersLen;

    CHK_STATUS(singleListGetHeadNode(pCurlRequest->pRequestHeaders, &pCurNode));

    // Iterate through the headers
    while (pCurNode != NULL) {
        CHK_STATUS(singleListGetNodeData(pCurNode, &item));
        pCurlRequestHeader = (PCurlRequestHeader) item;

        // Process only if we have a canonical header name
        if (IS_CANONICAL_HEADER_NAME(pCurlRequestHeader->pName)) {
            // Increase the overall length with the length of the header and a semicolon
            overallLen += pCurlRequestHeader->nameLen;

            // Check if we need to append the semicolon
            if (appended) {
                overallLen++;
            }

            // Pack if we have the destination specified
            if (pSignedHeaders != NULL) {
                CHK(overallLen <= specifiedLen, STATUS_BUFFER_TOO_SMALL);

                // Append the colon if needed
                if (appended) {
                    *pCurPtr++ = ';';
                }

                // Copy over and convert to lower case
                TOLOWERSTR(pCurlRequestHeader->pName, pCurlRequestHeader->nameLen, pCurPtr);
                pCurPtr += pCurlRequestHeader->nameLen;
            }

            appended = TRUE;
        }

        // Iterate
        CHK_STATUS(singleListGetNextNode(pCurNode, &pCurNode));
    }

CleanUp:

    if (pSignedHeadersLen != NULL) {
        *pSignedHeadersLen = overallLen;
    }

    LEAVES();
    return retStatus;
}

STATUS hexEncodedSha256(PBYTE pMessage, UINT32 size, PCHAR pEncodedHash)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BYTE hash[SHA256_DIGEST_LENGTH];
    UINT32 encodedSize = SHA256_DIGEST_LENGTH * 2 + 1;

    CHK(pMessage != NULL && pEncodedHash != NULL, STATUS_NULL_ARG);

    // Generate the SHA256 of the message first
    SHA256(pMessage, size, hash);

    // Hex encode lower case
    CHK_STATUS(hexEncodeCase(hash, SHA256_DIGEST_LENGTH, pEncodedHash, &encodedSize, FALSE));

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS generateSignatureDateTime(PCurlRequest pCurlRequest, PCHAR pDateTimeStr)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbacksProvider;
    UINT64 curTime;
    time_t timeT;
    SIZE_T retSize;

    CHK(pCurlRequest != NULL && pDateTimeStr != NULL, STATUS_NULL_ARG);
    CHK(pCurlRequest->pCurlApiCallbacks != NULL && pCurlRequest->pCurlApiCallbacks->pCallbacksProvider != NULL, STATUS_INVALID_ARG);
    pCallbacksProvider = pCurlRequest->pCurlApiCallbacks->pCallbacksProvider;
    curTime = pCallbacksProvider->clientCallbacks.getCurrentTimeFn(pCallbacksProvider->clientCallbacks.customData);

    // Convert to time_t
    timeT = (time_t) (curTime / HUNDREDS_OF_NANOS_IN_A_SECOND);
    retSize = STRFTIME(pDateTimeStr, SIGNATURE_DATE_TIME_STRING_LEN, DATE_TIME_STRING_FORMAT, GMTIME(&timeT));
    CHK(retSize > 0, STATUS_BUFFER_TOO_SMALL);
    pDateTimeStr[retSize] = '\0';

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS generateRequestHmac(PBYTE key, UINT32 keyLen, PBYTE message, UINT32 messageLen,
                           PBYTE outBuffer, PUINT32 pHmacLen)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 hmacLen;
    EVP_MD* pEvp;

    CHK(pHmacLen != NULL && pHmacLen != NULL, STATUS_NULL_ARG);

    *pHmacLen = 0;

    pEvp = (EVP_MD*) EVP_sha256();

    // Potentially in-place HMAC
    CHK(NULL != HMAC(pEvp, key, (INT32) keyLen, message, messageLen, outBuffer, &hmacLen), STATUS_HMAC_GENERATION_ERROR);

    *pHmacLen = hmacLen;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS generateCredentialScope(PCurlRequest pCurlRequest, PCHAR dateTimeStr, PCHAR pScope, PUINT32 pScopeLen)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 scopeLen = 0;

    CHK(pCurlRequest != NULL && pCurlRequest->pCurlApiCallbacks != NULL && pScopeLen != NULL, STATUS_NULL_ARG);

    // Calculate the max string length with a null terminator at the end
    scopeLen = SIGNATURE_DATE_TIME_STRING_LEN + 1 +
               MAX_REGION_NAME_LEN + 1 +
               (UINT32) STRLEN(KINESIS_VIDEO_SERVICE_NAME) + 1 +
               (UINT32) STRLEN(AWS_SIG_V4_SIGNATURE_END) + 1;

    // Early exit on buffer calculation
    CHK (pScope != NULL, retStatus);

    scopeLen = SNPRINTF(pScope, *pScopeLen, CREDENTIAL_SCOPE_TEMPLATE,
                        SIGNATURE_DATE_STRING_LEN, dateTimeStr,
                        pCurlRequest->pCurlApiCallbacks->region,
                        KINESIS_VIDEO_SERVICE_NAME, AWS_SIG_V4_SIGNATURE_END);
    CHK(scopeLen > 0 && scopeLen <= *pScopeLen, STATUS_BUFFER_TOO_SMALL);

CleanUp:

    if (pScopeLen != NULL) {
        *pScopeLen = scopeLen;
    }

    LEAVES();
    return retStatus;
}