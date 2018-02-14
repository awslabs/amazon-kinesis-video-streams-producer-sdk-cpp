#include "ClientTestFixture.h"

//////////////////////////////////////////////////////////////////////////////////////
// Static callbacks definitions
//////////////////////////////////////////////////////////////////////////////////////

UINT64 ClientTestBase::getCurrentTimeFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getCurrentTimeFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetCurrentTimeFuncCount++;
    pClient->mTime = GETTIME();

    return pClient->mTime;
}

UINT32 ClientTestBase::getRandomNumberFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getRandomNumberFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetRandomNumberFuncCount++;

    return RAND();
}

STATUS ClientTestBase::getDeviceCertificateFunc(UINT64 customData, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getDeviceCertificateFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetDeviceCertificateFuncCount++;

    *ppCert = (PBYTE) TEST_CERTIFICATE_BITS;
    *pSize = SIZEOF(TEST_CERTIFICATE_BITS);
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getSecurityTokenFunc(UINT64 customData, PBYTE* ppToken, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getSecurityTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetSecurityTokenFuncCount++;

    *ppToken = (PBYTE) TEST_TOKEN_BITS;
    *pSize = SIZEOF(TEST_TOKEN_BITS);
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getDeviceFingerprintFunc(UINT64 customData, PCHAR* ppFingerprint)
{
    DLOGV("TID 0x%016llx getDeviceFingerprintFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetDeviceFingerprintFuncCount++;

    *ppFingerprint = TEST_DEVICE_FINGERPRINT;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptyDeviceCertificateFunc(UINT64 customData, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getEmptyDeviceCertificateFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetDeviceCertificateFuncCount++;

    *ppCert = NULL;
    *pSize = 0;
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptySecurityTokenFunc(UINT64 customData, PBYTE* ppToken, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getEmptySecurityTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetSecurityTokenFuncCount++;

    *ppToken = NULL;
    *pSize = 0;
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptyDeviceFingerprintFunc(UINT64 customData, PCHAR* ppFingerprint)
{
    DLOGV("TID 0x%016llx getEmptyDeviceFingerprintFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetDeviceFingerprintFuncCount++;

    *ppFingerprint = NULL;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamUnderflowReportFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGV("TID 0x%016llx streamUnderflowReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamUnderflowReportFuncCount++;

    pClient->mStreamHandle = streamHandle;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::storageOverflowPressureFunc(UINT64 customData, UINT64 remainingSize)
{
    DLOGV("TID 0x%016llx storageOverflowPressureFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStorageOverflowPressureFuncCount++;

    pClient->mRemainingSize = remainingSize;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamLatencyPressureFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 duration)
{
    DLOGV("TID 0x%016llx streamLatencyPressureFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamLatencyPressureFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mDuration = duration;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::droppedFrameReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 frameTimecode)
{
    DLOGV("TID 0x%016llx droppedFrameReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mDroppedFrameReportFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mFrameTime = frameTimecode;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::droppedFragmentReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 fragmentTimecode)
{
    DLOGV("TID 0x%016llx droppedFragmentReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mDroppedFragmentReportFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentTime = fragmentTimecode;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamReadyFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGV("TID 0x%016llx streamReadyFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamReadyFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mStreamReady = TRUE;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamClosedFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 streamUploadHandle)
{
    DLOGV("TID 0x%016llx streamClosedFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamClosedFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mStreamClosed = TRUE;
    pClient->mStreamUploadHandle = streamUploadHandle;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamDataAvailableFunc(UINT64 customData,
                                               STREAM_HANDLE streamHandle,
                                               PCHAR streamName,
                                               UINT64 streamUploadHandle,
                                               UINT64 duration,
                                               UINT64 size)
{
    DLOGV("TID 0x%016llx streamDataAvailableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamDataAvailableFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mDataReadyDuration = duration;
    pClient->mDataReadySize = size;
    pClient->mStreamUploadHandle = streamUploadHandle;
    STRCPY(pClient->mStreamName, streamName);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamErrorReportFunc(UINT64 customData,
                                             STREAM_HANDLE streamHandle,
                                             UINT64 timecode,
                                             STATUS status)
{
    DLOGV("TID 0x%016llx streamErrorReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamErrorReportFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentTime = timecode;
    pClient->mStatus = status;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamConnectionStaleFunc(UINT64 customData,
                                                 STREAM_HANDLE streamHandle,
                                                 UINT64 duration)
{
    DLOGV("TID 0x%016llx streamConnectionStaleFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mStreamConnectionStaleFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mDuration = duration;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::fragmentAckReceivedFunc(UINT64 customData,
                                               STREAM_HANDLE streamHandle,
                                               PFragmentAck pFragmentAck)
{
    DLOGV("TID 0x%016llx fragmentAckReceivedFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mFragmentAckReceivedFuncCount++;

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentAck = *pFragmentAck;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::clientReadyFunc(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    DLOGV("TID 0x%016llx clientReadyFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mClientReadyFuncCount++;

    pClient->mReturnedClientHandle = clientHandle;
    pClient->mClientReady = TRUE;

    return STATUS_SUCCESS;
}

MUTEX ClientTestBase::createMutexFunc(UINT64 customData, BOOL reentant)
{
    DLOGV("TID 0x%016llx createMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mCreateMutexFuncCount++;

    return MUTEX_CREATE(reentant);
}

VOID ClientTestBase::lockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx lockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mLockMutexFuncCount++;

    return MUTEX_LOCK(mutex);
}

VOID ClientTestBase::unlockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx unlockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mUnlockMutexFuncCount++;

    return MUTEX_UNLOCK(mutex);
}

VOID ClientTestBase::tryLockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx tryLockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mTryLockMutexFuncCount++;

    return MUTEX_TRYLOCK(mutex);
}

VOID ClientTestBase::freeMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx freeMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mFreeMutexFuncCount++;

    return MUTEX_FREE(mutex);
}

STATUS ClientTestBase::createStreamFunc(UINT64 customData,
                                        PCHAR deviceName,
                                        PCHAR streamName,
                                        PCHAR contentType,
                                        PCHAR kmsKeyId,
                                        UINT64 retention,
                                        PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx createStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mCreateStreamFuncCount++;

    STRCPY(pClient->mDeviceName, deviceName);
    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mContentType, contentType);
    STRCPY(pClient->mKmsKeyId, kmsKeyId);

    pClient->mRetention = retention;

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::describeStreamFunc(UINT64 customData,
                                          PCHAR streamName,
                                          PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx describeStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mDescribeStreamFuncCount++;

    STRCPY(pClient->mStreamName, streamName);

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getStreamingEndpointFunc(UINT64 customData,
                                                PCHAR streamName,
                                                PCHAR apiName,
                                                PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingEndpointFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetStreamingEndpointFuncCount++;

    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mApiName, apiName);

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getStreamingTokenFunc(UINT64 customData,
                                             PCHAR streamName,
                                             STREAM_ACCESS_MODE accessMode,
                                             PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mGetStreamingTokenFuncCount++;

    STRCPY(pClient->mStreamName, streamName);
    pClient->mAccessMode = accessMode;

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::putStreamFunc(UINT64 customData,
                                     PCHAR streamName,
                                     PCHAR containerType,
                                     UINT64 streamStartTime,
                                     BOOL absoluteFragmentTimestamp,
                                     BOOL ackRequired,
                                     PCHAR streamingEndpoint,
                                     PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx putStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);
    EXPECT_TRUE(0 == STRCMP(containerType, MKV_CONTAINER_TYPE_STRING));

    pClient->mPutStreamFuncCount++;

    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mStreamingEndpoint, streamingEndpoint);
    pClient->mStreamStartTime = streamStartTime;
    pClient->mAckRequired = ackRequired;

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::tagResourceFunc(UINT64 customData,
                                     PCHAR resourceArn,
                                     UINT32 tagCount,
                                     PTag tags,
                                     PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx tagResourceFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mTagResourceFuncCount++;

    STRCPY(pClient->mResourceArn, resourceArn);
    pClient->mTagCount = tagCount;

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::createDeviceFunc(UINT64 customData,
                                        PCHAR deviceName,
                                        PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx createDeviceFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mCreateDeviceFuncCount++;

    STRCPY(pClient->mDeviceName, deviceName);

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::deviceCertToTokenFunc(UINT64 customData,
                                             PCHAR deviceName,
                                             PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx deviceCertToTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    EXPECT_TRUE(pClient != NULL && pClient->mMagic == TEST_CLIENT_MAGIC_NUMBER);

    pClient->mDeviceCertToTokenFuncCount++;

    STRCPY(pClient->mDeviceName, deviceName);

    // Store the custom data
    pClient->mCallContext = *pCallbackContext;
    EXPECT_EQ(SERVICE_CALL_CONTEXT_CURRENT_VERSION, pClient->mCallContext.version);

    if (pClient->mCallContext.pAuthInfo != NULL) {
        EXPECT_EQ(AUTH_INFO_CURRENT_VERSION, pClient->mCallContext.pAuthInfo->version);
        if (pClient->mCallContext.pAuthInfo->type == AUTH_INFO_TYPE_STS) {
            EXPECT_EQ(SIZEOF(TEST_TOKEN_BITS), pClient->mCallContext.pAuthInfo->size);
            EXPECT_EQ(0, STRCMP(TEST_TOKEN_BITS, (PCHAR) pClient->mCallContext.pAuthInfo->data));
        }
    }

    return STATUS_SUCCESS;
}

//
// Global memory allocation counter
//
UINT64 gTotalMemoryUsage = 0;
