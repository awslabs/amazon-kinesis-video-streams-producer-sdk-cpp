#include "ClientTestFixture.h"

//////////////////////////////////////////////////////////////////////////////////////
// Static callbacks definitions
//////////////////////////////////////////////////////////////////////////////////////

UINT64 ClientTestBase::getCurrentTimeFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getCurrentTimeFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetCurrentTimeFuncCount);
    pClient->setTestTimeVal(GETTIME());

    return pClient->getTestTimeVal();
}

// Global variable which is used as a preset time
UINT64 gPresetCurrentTime = 0;
UINT64 ClientTestBase::getCurrentPresetTimeFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getCurrentPresetTimeFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetCurrentTimeFuncCount);
    pClient->setTestTimeVal(gPresetCurrentTime);

    // Increment the preset time - wrap in the atomicizing locks
    MUTEX_LOCK(pClient->mAtomicLock);
    gPresetCurrentTime++;
    MUTEX_UNLOCK(pClient->mAtomicLock);

    return pClient->getTestTimeVal();
}

UINT64 ClientTestBase::getCurrentIncrementalTimeFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getCurrentIncrementalTimeFunc called.", GETTID());


    ClientTestBase *pClient = (ClientTestBase*) customData;

    UINT64 time = pClient->getTestTimeVal();
    pClient->incrementTestTimeVal(TEST_TIME_INCREMENT);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetCurrentTimeFuncCount);

    return time;
}

UINT32 ClientTestBase::getRandomNumberFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getRandomNumberFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetRandomNumberFuncCount);

    return RAND();
}

// Global variable which is used for the random function that returns constant value for testing
UINT32 gConstReturnFromRandomFunction = TEST_CONST_RAND_FUNC_BYTE;

UINT32 ClientTestBase::getRandomNumberConstFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx getRandomNumberConstFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetRandomNumberFuncCount);

    return gConstReturnFromRandomFunction;
}

VOID ClientTestBase::logPrintFunc(UINT32 level, PCHAR tag, PCHAR fmt, ...)
{
    if (level >= loggerGetLogLevel()) {
        // Temp scratch buffer = 10KB
        CHAR tempBuf[10 * 1024];
        SNPRINTF(tempBuf, SIZEOF(tempBuf), "\n[0x%016" PRIx64 "] [level %u] %s %s", GETTID(), level, tag, fmt);
        va_list valist;
        va_start(valist, fmt);
        vprintf(tempBuf, valist);
        va_end(valist);
    }
}

STATUS ClientTestBase::getDeviceCertificateFunc(UINT64 customData, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getDeviceCertificateFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetDeviceCertificateFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    *ppCert = (PBYTE) TEST_CERTIFICATE_BITS;
    *pSize = SIZEOF(TEST_CERTIFICATE_BITS);
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getSecurityTokenFunc(UINT64 customData, PBYTE* ppToken, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getSecurityTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetSecurityTokenFuncCount);

    *ppToken = pClient->mToken = (PBYTE) TEST_TOKEN_BITS;
    *pSize = pClient->mTokenSize = SIZEOF(TEST_TOKEN_BITS);
    *pExpiration = pClient->mTokenExpiration;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getDeviceFingerprintFunc(UINT64 customData, PCHAR* ppFingerprint)
{
    DLOGV("TID 0x%016llx getDeviceFingerprintFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetDeviceFingerprintFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    *ppFingerprint = (PCHAR) TEST_DEVICE_FINGERPRINT;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptyDeviceCertificateFunc(UINT64 customData, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getEmptyDeviceCertificateFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetDeviceCertificateFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    *ppCert = NULL;
    *pSize = 0;
    *pExpiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptySecurityTokenFunc(UINT64 customData, PBYTE* ppToken, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGV("TID 0x%016llx getEmptySecurityTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetSecurityTokenFuncCount);

    *ppToken = pClient->mToken = NULL;
    *pSize = pClient->mTokenSize = 0;
    *pExpiration = pClient->mTokenExpiration;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::getEmptyDeviceFingerprintFunc(UINT64 customData, PCHAR* ppFingerprint)
{
    DLOGV("TID 0x%016llx getEmptyDeviceFingerprintFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetDeviceFingerprintFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    *ppFingerprint = NULL;

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamUnderflowReportFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGV("TID 0x%016llx streamUnderflowReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamUnderflowReportFuncCount);

    pClient->mStreamHandle = streamHandle;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::storageOverflowPressureFunc(UINT64 customData, UINT64 remainingSize)
{
    DLOGV("TID 0x%016llx storageOverflowPressureFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStorageOverflowPressureFuncCount);

    pClient->mRemainingSize = remainingSize;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamLatencyPressureFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 duration)
{
    DLOGV("TID 0x%016llx streamLatencyPressureFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamLatencyPressureFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mDuration = duration;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::droppedFrameReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 frameTimecode)
{
    DLOGV("TID 0x%016llx droppedFrameReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mDroppedFrameReportFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mFrameTime = frameTimecode;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::bufferDurationOverflowPressureFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 remainingDuration){
    DLOGV("TID 0x%016llx bufferDurationOverflowPressureFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mBufferDurationOverflowPrssureFuncCount);

    pClient->mRemainingDuration = remainingDuration;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::droppedFragmentReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 fragmentTimecode)
{
    DLOGV("TID 0x%016llx droppedFragmentReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mDroppedFragmentReportFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentTime = fragmentTimecode;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamReadyFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGV("TID 0x%016llx streamReadyFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamReadyFuncCount);

    pClient->mStreamHandle = streamHandle;
    ATOMIC_STORE_BOOL(&pClient->mStreamReady, TRUE);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamClosedFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 streamUploadHandle)
{
    DLOGV("TID 0x%016llx streamClosedFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamClosedFuncCount);

    pClient->mStreamHandle = streamHandle;
    ATOMIC_STORE_BOOL(&pClient->mStreamClosed, TRUE);
    pClient->mStreamUploadHandle = streamUploadHandle;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

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

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamDataAvailableFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mDataReadyDuration = duration;
    pClient->mDataReadySize = size;
    pClient->mStreamUploadHandle = streamUploadHandle;
    STRCPY(pClient->mStreamName, streamName);

    pClient->mStreamingSession.signalDataAvailable(streamUploadHandle);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamErrorReportFunc(UINT64 customData,
                                             STREAM_HANDLE streamHandle,
                                             UPLOAD_HANDLE upload_handle,
                                             UINT64 timecode,
                                             STATUS status)
{
    DLOGV("TID 0x%016llx streamErrorReportFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamErrorReportFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentTime = timecode;
    pClient->mStatus = status;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamConnectionStaleFunc(UINT64 customData,
                                                 STREAM_HANDLE streamHandle,
                                                 UINT64 duration)
{
    DLOGV("TID 0x%016llx streamConnectionStaleFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamConnectionStaleFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mDuration = duration;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::fragmentAckReceivedFunc(UINT64 customData,
                                               STREAM_HANDLE streamHandle,
                                               UPLOAD_HANDLE upload_handle,
                                               PFragmentAck pFragmentAck)
{
    DLOGV("TID 0x%016llx fragmentAckReceivedFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mFragmentAckReceivedFuncCount);

    pClient->mStreamHandle = streamHandle;
    pClient->mFragmentAck = *pFragmentAck;
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::clientReadyFunc(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    DLOGV("TID 0x%016llx clientReadyFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mClientReadyFuncCount);

    pClient->mReturnedClientHandle = clientHandle;
    ATOMIC_STORE_BOOL(&pClient->mClientReady, TRUE);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::clientShutdownFunc(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    DLOGV("TID 0x%016llx clientShutdownFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mClientShutdownFuncCount);

    pClient->mReturnedClientHandle = clientHandle;
    ATOMIC_STORE_BOOL(&pClient->mClientShutdown, TRUE);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::streamShutdownFunc(UINT64 customData, STREAM_HANDLE streamHandle, BOOL resetStream)
{
    DLOGV("TID 0x%016llx streamShutdownFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mStreamShutdownFuncCount);

    pClient->mStreamHandle = streamHandle;
    ATOMIC_STORE_BOOL(&pClient->mStreamShutdown, TRUE);
    ATOMIC_STORE_BOOL(&pClient->mResetStream, resetStream);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

MUTEX ClientTestBase::createMutexFunc(UINT64 customData, BOOL reentant)
{
    DLOGV("TID 0x%016llx createMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mCreateMutexFuncCount);

    return MUTEX_CREATE(reentant);
}

VOID ClientTestBase::lockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx lockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mLockMutexFuncCount);

    return MUTEX_LOCK(mutex);
}

VOID ClientTestBase::unlockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx unlockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mUnlockMutexFuncCount);

    return MUTEX_UNLOCK(mutex);
}

BOOL ClientTestBase::tryLockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx tryLockMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mTryLockMutexFuncCount);

    return MUTEX_TRYLOCK(mutex);
}

VOID ClientTestBase::freeMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGV("TID 0x%016llx freeMutexFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mFreeMutexFuncCount);

    return MUTEX_FREE(mutex);
}

CVAR ClientTestBase::createConditionVariableFunc(UINT64 customData)
{
    DLOGV("TID 0x%016llx createConditionVariableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mCreateConditionVariableFuncCount);

    return CVAR_CREATE();
}

STATUS ClientTestBase::signalConditionVariableFunc(UINT64 customData, CVAR cvar)
{
    DLOGV("TID 0x%016llx signalConditionVariableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mSignalConditionVariableFuncCount);

    return CVAR_SIGNAL(cvar);
}

STATUS ClientTestBase::broadcastConditionVariableFunc(UINT64 customData, CVAR cvar)
{
    DLOGV("TID 0x%016llx broadcastConditionVariableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mBroadcastConditionVariableFuncCount);

    return CVAR_BROADCAST(cvar);
}

STATUS ClientTestBase::waitConditionVariableFunc(UINT64 customData,
                                                 CVAR cvar,
                                                 MUTEX mutex,
                                                 UINT64 timeout)
{
    DLOGV("TID 0x%016llx waitConditionVariableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mWaitConditionVariableFuncCount);

    return CVAR_WAIT(cvar, mutex, timeout);
}

VOID ClientTestBase::freeConditionVariableFunc(UINT64 customData, CVAR cvar)
{
    DLOGV("TID 0x%016llx freeConditionVariableFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mFreeConditionVariableFuncCount);

    return CVAR_FREE(cvar);
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

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mCreateStreamFuncCount);

    STRCPY(pClient->mDeviceName, deviceName);
    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mContentType, contentType);
    STRCPY(pClient->mKmsKeyId, kmsKeyId);

    pClient->mStreamInfo.retention = retention;

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
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::describeStreamFunc(UINT64 customData,
                                          PCHAR streamName,
                                          PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx describeStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mDescribeStreamFuncCount);

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

    if (pClient->mSubmitServiceCallResultMode >= STOP_AT_DESCRIBE_STREAM) {
        // Move to ready state
        pClient->setupStreamDescription();

        // Execute a couple of times
        pClient->mStreamDescription.streamStatus = ATOMIC_LOAD(&pClient->mDescribeStreamFuncCount) < 3 ? STREAM_STATUS_CREATING : STREAM_STATUS_ACTIVE;
        retStatus = describeStreamResultEvent(pCallbackContext->customData, SERVICE_CALL_RESULT_OK, &pClient->mStreamDescription);
    }
    ATOMIC_INCREMENT(&pClient->mDescribeStreamDoneFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
}

STATUS ClientTestBase::describeStreamMultiStreamFunc(UINT64 customData,
                                          PCHAR streamName,
                                          PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx describeStreamMultiStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);
    // Move to ready state
    pClient->setupStreamDescription();

    STRCPY(pClient->mStreamDescription.streamName, streamName);

    // Execute a couple of times
    pClient->mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
    retStatus = describeStreamResultEvent(pCallbackContext->customData, SERVICE_CALL_RESULT_OK, &pClient->mStreamDescription);

    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
}

STATUS ClientTestBase::getStreamingEndpointFunc(UINT64 customData,
                                                PCHAR streamName,
                                                PCHAR apiName,
                                                PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingEndpointFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetStreamingEndpointFuncCount);

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

    if (pClient->mSubmitServiceCallResultMode >= STOP_AT_GET_STREAMING_ENDPOINT) {
        retStatus = getStreamingEndpointResultEvent(pCallbackContext->customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT);
    }
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
}

STATUS ClientTestBase::getStreamingEndpointMultiStreamFunc(UINT64 customData,
                                                PCHAR streamName,
                                                PCHAR apiName,
                                                PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingEndpointMultiStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);

    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mApiName, apiName);

    retStatus = getStreamingEndpointResultEvent(pCallbackContext->customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT);

    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
}

STATUS ClientTestBase::getStreamingTokenFunc(UINT64 customData,
                                             PCHAR streamName,
                                             STREAM_ACCESS_MODE accessMode,
                                             PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mGetStreamingTokenFuncCount);

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

    if (pClient->mSubmitServiceCallResultMode >= STOP_AT_GET_STREAMING_TOKEN) {
        UINT64 currentTime;
        PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(pClient->mStreamHandle);
        // initial create Stream
        if (pKinesisVideoStream == NULL) {
            currentTime = pClient->mClientCallbacks.getCurrentTimeFn(customData);
        } else {
            // requires lock because pic may also be calling getCurrentTime
            PKinesisVideoClient pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
            EXPECT_TRUE(pKinesisVideoClient != NULL);
            pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
            currentTime = pKinesisVideoClient->clientCallbacks.getCurrentTimeFn(pKinesisVideoClient->clientCallbacks.customData);
            pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
        }

        retStatus = getStreamingTokenResultEvent(pCallbackContext->customData,
                                                 SERVICE_CALL_RESULT_OK,
                                                 (PBYTE) TEST_STREAMING_TOKEN,
                                                 SIZEOF(TEST_STREAMING_TOKEN),
                                                 currentTime + HUNDREDS_OF_NANOS_IN_A_SECOND + MIN_STREAMING_TOKEN_EXPIRATION_DURATION);
    }
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
}

STATUS ClientTestBase::getStreamingTokenMultiStreamFunc(UINT64 customData,
                                             PCHAR streamName,
                                             STREAM_ACCESS_MODE accessMode,
                                             PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx getStreamingTokenMultiStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;
    STATUS retStatus = STATUS_SUCCESS;

    MUTEX_LOCK(pClient->mTestClientMutex);

    STRCPY(pClient->mStreamName, streamName);


    retStatus = getStreamingTokenResultEvent(pCallbackContext->customData,
                                                 SERVICE_CALL_RESULT_OK,
                                                 (PBYTE) TEST_STREAMING_TOKEN,
                                                 SIZEOF(TEST_STREAMING_TOKEN),
                                                 MAX_UINT64);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return retStatus;
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

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);
    EXPECT_TRUE(0 == STRCMP(containerType, MKV_CONTAINER_TYPE_STRING));

    ATOMIC_INCREMENT(&pClient->mPutStreamFuncCount);

    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mStreamingEndpoint, streamingEndpoint);
    pClient->mStreamStartTime = streamStartTime;
    ATOMIC_STORE_BOOL(&pClient->mAckRequired, ackRequired);

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

    if (pClient->mSubmitServiceCallResultMode >= STOP_AT_PUT_STREAM) {
        UPLOAD_HANDLE uploadHandle = pClient->mStreamingSession.addNewConsumerSession(pClient->mMockConsumerConfig,
                                                                                            pClient->mStreamHandle);
        EXPECT_EQ(STATUS_SUCCESS,
                  putStreamResultEvent(pClient->mCallContext.customData, SERVICE_CALL_RESULT_OK, uploadHandle));
    }
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::putStreamMultiStreamFunc(UINT64 customData,
                                     PCHAR streamName,
                                     PCHAR containerType,
                                     UINT64 streamStartTime,
                                     BOOL absoluteFragmentTimestamp,
                                     BOOL ackRequired,
                                     PCHAR streamingEndpoint,
                                     PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx putStreamMultiStreamFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);

    STRCPY(pClient->mStreamName, streamName);
    STRCPY(pClient->mStreamingEndpoint, streamingEndpoint);
    pClient->mStreamStartTime = streamStartTime;
    pClient->mAckRequired = ackRequired;
    EXPECT_EQ(STATUS_SUCCESS,
                  putStreamResultEvent((STREAM_HANDLE) pCallbackContext->customData, SERVICE_CALL_RESULT_OK, (UPLOAD_HANDLE) 0));
    MUTEX_UNLOCK(pClient->mTestClientMutex);

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

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mTagResourceFuncCount);

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
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::createDeviceFunc(UINT64 customData,
                                        PCHAR deviceName,
                                        PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx createDeviceFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mCreateDeviceFuncCount);

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

    if (pClient->mClientSyncMode) {
        EXPECT_EQ(STATUS_SUCCESS,
                  createDeviceResultEvent(pCallbackContext->customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    }
    ATOMIC_INCREMENT(&pClient->mCreateDeviceDoneFuncCount);
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

STATUS ClientTestBase::deviceCertToTokenFunc(UINT64 customData,
                                             PCHAR deviceName,
                                             PServiceCallContext pCallbackContext)
{
    DLOGV("TID 0x%016llx deviceCertToTokenFunc called.", GETTID());

    ClientTestBase *pClient = (ClientTestBase*) customData;

    MUTEX_LOCK(pClient->mTestClientMutex);
    EXPECT_TRUE(pClient != NULL && ATOMIC_LOAD(&pClient->mMagic) == TEST_CLIENT_MAGIC_NUMBER);

    ATOMIC_INCREMENT(&pClient->mDeviceCertToTokenFuncCount);

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

    if (pClient->mClientSyncMode) {
        EXPECT_EQ(STATUS_SUCCESS, deviceCertToTokenResultEvent(pCallbackContext->customData,
                                                               SERVICE_CALL_RESULT_OK,
                                                               pClient->mToken,
                                                               pClient->mTokenSize,
                                                               pClient->mTokenExpiration));
    }
    MUTEX_UNLOCK(pClient->mTestClientMutex);

    return STATUS_SUCCESS;
}

//
// Global memory allocation counter
//
UINT64 gTotalClientMemoryUsage = 0;

// Global memory counter lock
MUTEX gClientMemMutex;
