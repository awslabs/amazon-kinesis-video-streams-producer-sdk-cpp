#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

//
// Global memory allocation counter
//
UINT64 gTotalProducerClientMemoryUsage = 0;

//
// Global memory counter lock
//
MUTEX gProducerClientMemMutex;

//
// Global test object pointer
//
ProducerClientTestBase* gProducerClientTestBase;

STATUS ProducerClientTestBase::testBufferDurationOverflowFunc(UINT64 customData,
                                          STREAM_HANDLE streamHandle,
                                          UINT64 remainDuration)
{
    DLOGD("Reported bufferDurationOverflow callback for stream handle %" PRIu64 ". Remaining duration in 100ns: %" PRIu64,
          streamHandle, remainDuration);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mBufferDurationOverflowFnCount++;
    pTest->mBufferDurationInPressure = TRUE;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);
    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStreamLatencyPressureFunc(UINT64 customData,
                                         STREAM_HANDLE streamHandle,
                                         UINT64 currentBufferDuration)
{
    DLOGD("Reported streamLatencyPressure callback for stream handle %" PRIu64 ". Current buffer duration in 100ns: %" PRIu64,
          streamHandle, currentBufferDuration);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mStreamLatencyPressureFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);
    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStreamConnectionStaleFunc(UINT64 customData,
                                         STREAM_HANDLE streamHandle,
                                         UINT64 timeSinceLastBufferingAck)
{
    DLOGD("Reported streamConnectionStale callback for stream handle %" PRIu64 ". Time since last buffering ack in 100ns: %" PRIu64,
          streamHandle, timeSinceLastBufferingAck);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mConnectionStaleFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);
    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testDroppedFrameReportFunc(UINT64 customData,
                                      STREAM_HANDLE streamHandle,
                                      UINT64 frameTimecode)
{
    DLOGD("Reported droppedFrame callback for stream handle %" PRIu64 ". Dropped frame timecode in 100ns: %" PRIu64,
          streamHandle, frameTimecode);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mDroppedFrameFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);
    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStreamErrorReportFunc(UINT64 customData,
                                     STREAM_HANDLE streamHandle,
                                     UPLOAD_HANDLE uploadHandle,
                                     UINT64 fragmentTimecode,
                                     STATUS errorStatus)
{
    DLOGD("Reported streamError callback for stream handle %" PRIu64 ". Upload handle %" PRIu64 ". Fragment timecode in"
            " 100ns: %" PRIu64 ". Error status: 0x%08x\n",
          streamHandle, uploadHandle, fragmentTimecode, errorStatus);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    BOOL resetStream = FALSE;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mStreamErrorFnCount++;
    pTest->mLastError = errorStatus;
    if (IS_RETRIABLE_ERROR(errorStatus) && pTest->mResetStreamCounter > 0) {
        pTest->mResetStreamCounter--;
        resetStream = TRUE;
    }
    MUTEX_UNLOCK(pTest->mTestCallbackLock);

    if (resetStream) {
        EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamResetStream(streamHandle));
    }

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testFragmentAckReceivedFunc(UINT64 customData,
                                       STREAM_HANDLE streamHandle,
                                       UPLOAD_HANDLE uploadHandle,
                                       PFragmentAck pFragmentAck)
{
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(uploadHandle);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;

    MUTEX_LOCK(pTest->mTestCallbackLock);
    switch (pFragmentAck->ackType) {
        case FRAGMENT_ACK_TYPE_PERSISTED:
            pTest->mLastPersistedAckTimestamp = pFragmentAck->timestamp;
            pTest->mPersistedFragmentCount++;
            break;

        case FRAGMENT_ACK_TYPE_BUFFERING:
            pTest->mLastBufferingAckTimestamp = pFragmentAck->timestamp;
            break;

        case FRAGMENT_ACK_TYPE_RECEIVED:
            pTest->mLastReceivedAckTimestamp = pFragmentAck->timestamp;
            break;

        case FRAGMENT_ACK_TYPE_ERROR:
            pTest->mLastErrorAckTimestamp = pFragmentAck->timestamp;
            break;

        default:
            break;
    }

    pTest->mFragmentAckReceivedFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStreamReadyFunc(UINT64 customData,
                               STREAM_HANDLE streamHandle)
{
    DLOGD("Reported streamReady callback for stream handle %" PRIu64,
          streamHandle);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mStreamReadyFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);
    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStreamClosedFunc(UINT64 customData,
                                STREAM_HANDLE streamHandle,
                                UPLOAD_HANDLE uploadHandle)
{
    DLOGD("Reported streamClosed callback for stream handle %" PRIu64 ". Upload handle %" PRIu64,
          streamHandle, uploadHandle);
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mStreamClosedFnCount++;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testStorageOverflowFunc(UINT64 customData, UINT64 remainingBytes)
{
    DLOGD("Reported storageOverflow callback. Remaining bytes %" PRIu64,
          remainingBytes);

    ProducerClientTestBase* pTest = (ProducerClientTestBase*) customData;
    MUTEX_LOCK(pTest->mTestCallbackLock);
    pTest->mStorageOverflowCount++;
    pTest->mBufferStorageInPressure = TRUE;
    MUTEX_UNLOCK(pTest->mTestCallbackLock);

    return STATUS_SUCCESS;
}

ProducerClientTestBase::ProducerClientTestBase() :
        mClientHandle(INVALID_CLIENT_HANDLE_VALUE),
        mCallbacksProvider(NULL),
        mAccessKeyIdSet(FALSE),
        mCaCertPath(NULL),
        mProducerThread(INVALID_TID_VALUE),
        mProducerStopped(FALSE),
        mStartProducer(FALSE),
        mStopProducer(FALSE),
        mAccessKey(NULL),
        mSecretKey(NULL),
        mSessionToken(NULL),
        mRegion(NULL),
        mFreeApiCallbacksFnCount(0),
        mPutStreamFnCount(0),
        mTagResourceFnCount(0),
        mGetStreamingEndpointFnCount(0),
        mDescribeStreamFnCount(0),
        mDescribeStreamSecondFnCount(0),
        mDescribeStreamThirdFnCount(0),
        mDescribeStreamStopChainFnCount(0),
        mCreateStreamFnCount(0),
        mCreateDeviceFnCount(0),
        mEasyPerformFnCount(0),
        mWriteCallbackFnCount(0),
        mReadCallbackFnCount(0),
        mCurlGetDataEndpointCount(0),
        mCurlPutMediaCount(0),
        mCurlDescribeStreamCount(0),
        mCurlCreateStreamCount(0),
        mCurlTagResourceCount(0),
        mCreateStreamStatus(STATUS_SUCCESS),
        mCreateStreamCallResult(SERVICE_CALL_RESULT_OK),
        mDescribeStreamStatus(STATUS_SUCCESS),
        mDescribeStreamCallResult(SERVICE_CALL_RESULT_OK),
        mGetEndpointStatus(STATUS_SUCCESS),
        mGetEndpointCallResult(SERVICE_CALL_RESULT_OK),
        mTagResourceStatus(STATUS_SUCCESS),
        mTagResourceCallResult(SERVICE_CALL_RESULT_OK),
        mPutMediaStatus(STATUS_SUCCESS),
        mPutMediaCallResult(SERVICE_CALL_RESULT_OK),
        mCurlEasyPerformInjectionCount(UINT32_MAX),
        mWriteStatus(STATUS_SUCCESS),
        mWriteBuffer(NULL),
        mWriteDataSize(0),
        mCurlWriteCallbackPassThrough(TRUE),
        mReadStatus(STATUS_SUCCESS),
        mReadSize(0),
        mStreamCallbacks(NULL),
        mProducerCallbacks(NULL),
        mResetStreamCounter(0),
        mAuthCallbacks(NULL),
        mConnectionStaleFnCount(0),
        mLastError(STATUS_SUCCESS),
        mDescribeRetStatus(STATUS_SUCCESS),
        mDescribeFailCount(0),
        mDescribeRecoverCount(0)
{
    auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
    if (logLevelStr != NULL) {
        assert(STRTOUI32(logLevelStr, NULL, 10, &this->loggerLogLevel) == STATUS_SUCCESS);
        SET_LOGGER_LOG_LEVEL(this->loggerLogLevel);
    }

    // Store the function pointers
    gTotalProducerClientMemoryUsage = 0;
    mStoredMemAlloc = globalMemAlloc;
    mStoredMemAlignAlloc = globalMemAlignAlloc;
    mStoredMemCalloc = globalMemCalloc;
    mStoredMemFree = globalMemFree;

    // Create the mutex for the synchronization for the instrumentation
    gProducerClientMemMutex = MUTEX_CREATE(FALSE);

    globalMemAlloc = instrumentedClientMemAlloc;
    globalMemAlignAlloc = instrumentedClientMemAlignAlloc;
    globalMemCalloc = instrumentedClientMemCalloc;
    globalMemFree = instrumentedClientMemFree;

    gProducerClientTestBase = this;

    SRAND(12345);

    MEMSET(&mDeviceInfo, 0x00, SIZEOF(DeviceInfo));
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;
    STRCPY(mDeviceInfo.name, TEST_DEVICE_INFO_NAME);
    STRCPY(mDeviceInfo.clientId, TEST_CLIENT_ID);
    mDeviceInfo.streamCount = TEST_MAX_STREAM_COUNT;
    mDeviceInfo.tagCount = 0;
    mDeviceInfo.tags = NULL;
    mDeviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;
    mDeviceInfo.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM;
    mDeviceInfo.storageInfo.spillRatio = 0;
    mDeviceInfo.storageInfo.storageSize = TEST_STORAGE_SIZE_IN_BYTES;
    mDeviceInfo.storageInfo.rootDirectory[0] = '\0';
    mDeviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION;
    mDeviceInfo.clientInfo.stopStreamTimeout = 0;
    mDeviceInfo.clientInfo.createStreamTimeout = 0;
    mDeviceInfo.clientInfo.createClientTimeout = 0;
    mDeviceInfo.clientInfo.offlineBufferAvailabilityTimeout = 0;
    mDeviceInfo.clientInfo.loggerLogLevel = this->loggerLogLevel;
    mDeviceInfo.clientInfo.logMetric = TRUE;

    mDefaultRegion[0] = '\0';
    mStreamingRotationPeriod = TEST_STREAMING_TOKEN_DURATION;

    MEMSET(mStreams, 0x00, SIZEOF(mStreams));

    mTrackInfo.version = TRACK_INFO_CURRENT_VERSION;
    mTrackInfo.trackId = DEFAULT_VIDEO_TRACK_ID;
    mTrackInfo.codecPrivateData = NULL;
    mTrackInfo.codecPrivateDataSize = 0;
    STRCPY(mTrackInfo.codecId, MKV_H264_AVC_CODEC_ID);
    STRCPY(mTrackInfo.trackName, (PCHAR) "kinesis_video");
    mTrackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
    mTrackInfo.trackCustomData.trackVideoConfig.videoHeight = 1080;
    mTrackInfo.trackCustomData.trackVideoConfig.videoWidth = 1920;

    mStreamInfo.version = STREAM_INFO_CURRENT_VERSION;
    mStreamInfo.kmsKeyId[0] = '\0';
    mStreamInfo.retention = 2 * HUNDREDS_OF_NANOS_IN_AN_HOUR;
    mStreamInfo.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
    mStreamInfo.tagCount = 0;
    mStreamInfo.tags = NULL;
    STRCPY(mStreamInfo.streamCaps.contentType, MKV_H264_CONTENT_TYPE);
    mStreamInfo.streamCaps.fragmentAcks = TRUE;
    mStreamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    mStreamInfo.streamCaps.segmentUuid = NULL;
    mStreamInfo.streamCaps.absoluteFragmentTimes = TRUE;
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;
    mStreamInfo.streamCaps.adaptive = TRUE;
    mStreamInfo.streamCaps.frameTimecodes = TRUE;
    mStreamInfo.streamCaps.recalculateMetrics = TRUE;
    mStreamInfo.streamCaps.recoverOnError = TRUE;
    mStreamInfo.streamCaps.avgBandwidthBps = 4 * 1024 * 1024;
    mStreamInfo.streamCaps.bufferDuration = TEST_STREAM_BUFFER_DURATION;
    mStreamInfo.streamCaps.replayDuration = 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mStreamInfo.streamCaps.fragmentDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    mStreamInfo.streamCaps.connectionStalenessDuration = TEST_STREAM_CONNECTION_STALENESS_DURATION;
    mStreamInfo.streamCaps.maxLatency = TEST_MAX_STREAM_LATENCY;
    mStreamInfo.streamCaps.timecodeScale = 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    mStreamInfo.streamCaps.frameRate = 100;
    mStreamInfo.streamCaps.trackInfoCount = 1;
    mStreamInfo.streamCaps.trackInfoList = &mTrackInfo;
    mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;
    mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_OOM;
    mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;

    mFps = TEST_FPS;
    mKeyFrameInterval = TEST_FPS;
    mFrameSize = TEST_FRAME_SIZE;
    mFrameBuffer = (PBYTE) MEMALLOC(mFrameSize);

    mFrame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND / mFps;
    mFrame.frameData = mFrameBuffer;
    mFrame.version = FRAME_CURRENT_VERSION;
    mFrame.size = mFrameSize;
    mFrame.trackId = DEFAULT_VIDEO_TRACK_ID;
    mFrame.flags = FRAME_FLAG_KEY_FRAME;
    mFrame.presentationTs = 0;
    mFrame.decodingTs = 0;
    mFrame.index = 0;

    mTestCallbackLock = MUTEX_CREATE(FALSE);
    mBufferDurationOverflowFnCount = 0;
    mStreamLatencyPressureFnCount = 0;
    mDroppedFrameFnCount = 0;
    mStreamErrorFnCount = 0;
    mFragmentAckReceivedFnCount = 0;
    mStreamReadyFnCount = 0;
    mStreamClosedFnCount = 0;
    mPersistedFragmentCount = 0;
    mLastBufferingAckTimestamp = 0;
    mLastErrorAckTimestamp = 0;
    mLastReceivedAckTimestamp = 0;
    mLastPersistedAckTimestamp = 0;
    mStorageOverflowCount = 0;

    mAbortUploadhandle = INVALID_UPLOAD_HANDLE_VALUE;

    mBufferDurationInPressure = FALSE;
    mBufferStorageInPressure = FALSE;
    mCurrentPressureState = BufferPressureOK;
    mPressureHandlerRetryCount = TEST_DEFAULT_PRESSURE_HANDLER_RETRY_COUNT;
}

VOID ProducerClientTestBase::handlePressure(volatile BOOL* pressureFlag, UINT32 gracePeriodSeconds)
{
    // first time pressure is detected
    if (*pressureFlag && mCurrentPressureState == BufferPressureOK) {
        mCurrentPressureState = BufferInPressure; // update state
        *pressureFlag = FALSE;  // turn off pressure flag as it is handled

        // whether to give some extra time for the pressure to relieve. For example, storageOverflow takes longer
        // to recover as it needs to wait for persisted acks.
        if (gracePeriodSeconds != 0) {
            DLOGD("Pressure handler in grace period. Sleep for %u seconds.", gracePeriodSeconds);
            THREAD_SLEEP(gracePeriodSeconds * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }
    } else if (mCurrentPressureState == BufferInPressure) { // if we are already in pressured state
        if (*pressureFlag) { // still getting the pressure signal, remain in pressured state.
            DLOGD("Pressure handler sleep for 1 second.");
            THREAD_SLEEP(1 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            *pressureFlag = FALSE;
            mPressureHandlerRetryCount--;
            if (mPressureHandlerRetryCount == 0) {
                GTEST_FAIL() << "Pressure handler tried " << TEST_DEFAULT_PRESSURE_HANDLER_RETRY_COUNT << " times without relieving pressure.";
            }
        } else { // pressure is relieved, back to OK state.
            DLOGD("Pressure handler recovered.");
            mCurrentPressureState = BufferPressureOK;
            mPressureHandlerRetryCount = TEST_DEFAULT_PRESSURE_HANDLER_RETRY_COUNT;
        }
    }
}

VOID ProducerClientTestBase::createDefaultProducerClient(BOOL cachingEndpoint, UINT64 createStreamTimeout, UINT64 stopStreamTimeout, BOOL continuousRetry, UINT64 rotationPeriod)
{
    createDefaultProducerClient(cachingEndpoint ? API_CALL_CACHE_TYPE_ENDPOINT_ONLY : API_CALL_CACHE_TYPE_NONE,
            createStreamTimeout,
            stopStreamTimeout,
            continuousRetry,
            rotationPeriod);
}

VOID ProducerClientTestBase::createDefaultProducerClient(API_CALL_CACHE_TYPE cacheType, UINT64 createStreamTimeout, UINT64 stopStreamTimeout, BOOL continuousRetry, UINT64 rotationPeriod)
{
    PAuthCallbacks pAuthCallbacks;
    PStreamCallbacks pStreamCallbacks;
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     cacheType,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     TEST_USER_AGENT,
                                                                     &mCallbacksProvider));

    EXPECT_EQ(STATUS_SUCCESS, createRotatingStaticAuthCallbacks(mCallbacksProvider,
                                                                mAccessKey,
                                                                mSecretKey,
                                                                mSessionToken,
                                                                rotationPeriod,
                                                                mStreamingRotationPeriod,
                                                                &pAuthCallbacks));

    if (continuousRetry) {
        EXPECT_EQ(STATUS_SUCCESS, createContinuousRetryStreamCallbacks(mCallbacksProvider, &pStreamCallbacks));
    }

    mApiCallbacks.version = API_CALLBACKS_CURRENT_VERSION;
    mApiCallbacks.customData = (UINT64) this;
    mApiCallbacks.freeApiCallbacksFn = testFreeApiCallbackFunc;
    mApiCallbacks.putStreamFn = testPutStreamFunc;
    mApiCallbacks.tagResourceFn = testTagResourceFunc;
    mApiCallbacks.getStreamingEndpointFn = testGetStreamingEndpointFunc;
    mApiCallbacks.describeStreamFn = testDescribeStreamFunc;
    mApiCallbacks.createStreamFn = testCreateStreamFunc;
    mApiCallbacks.createDeviceFn = testCreateDeviceFunc;

    // Add test API callbacks
    EXPECT_EQ(STATUS_SUCCESS, addApiCallbacks(mCallbacksProvider, &mApiCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createStreamCallbacks(&mStreamCallbacks));
    mStreamCallbacks->customData = (UINT64) this;
    mStreamCallbacks->bufferDurationOverflowPressureFn = testBufferDurationOverflowFunc;
    mStreamCallbacks->streamLatencyPressureFn = testStreamLatencyPressureFunc;
    mStreamCallbacks->streamConnectionStaleFn = testStreamConnectionStaleFunc;
    mStreamCallbacks->droppedFrameReportFn = testDroppedFrameReportFunc;
    mStreamCallbacks->streamErrorReportFn = testStreamErrorReportFunc;
    mStreamCallbacks->fragmentAckReceivedFn = testFragmentAckReceivedFunc;
    mStreamCallbacks->streamReadyFn = testStreamReadyFunc;
    mStreamCallbacks->streamClosedFn = testStreamClosedFunc;
    mStreamCallbacks->freeStreamCallbacksFn = NULL;

    EXPECT_EQ(STATUS_SUCCESS, addStreamCallbacks(mCallbacksProvider, mStreamCallbacks));

    mProducerCallbacks = (PProducerCallbacks) MEMCALLOC(1, SIZEOF(ProducerCallbacks));
    mProducerCallbacks->customData = (UINT64) this;
    mProducerCallbacks->storageOverflowPressureFn = testStorageOverflowFunc;

    EXPECT_EQ(STATUS_SUCCESS, addProducerCallbacks(mCallbacksProvider, mProducerCallbacks));

    // Set quick timeouts
    mDeviceInfo.clientInfo.createClientTimeout = TEST_CREATE_PRODUCER_TIMEOUT;
    mDeviceInfo.clientInfo.createStreamTimeout = createStreamTimeout;
    mDeviceInfo.clientInfo.stopStreamTimeout = stopStreamTimeout;
    mDeviceInfo.clientInfo.loggerLogLevel = this->loggerLogLevel;

    // Store the auth callbacks which is used for fault injection
    mAuthCallbacks = pAuthCallbacks;

    // Create the producer client
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, mCallbacksProvider, &mClientHandle));

    // Install a custom test hook
    PCallbacksProvider pCallbacksProvider = (PCallbacksProvider) mCallbacksProvider;
    PCurlApiCallbacks pCurlApiCallbacks = (PCurlApiCallbacks) pCallbacksProvider->pApiCallbacks[0].customData;
    pCurlApiCallbacks->hookCustomData = (UINT64) this;
    pCurlApiCallbacks->curlEasyPerformHookFn = curlEasyPerformHookFunc;
    pCurlApiCallbacks->curlReadCallbackHookFn = curlReadCallbackHookFunc;
    pCurlApiCallbacks->curlWriteCallbackHookFn = curlWriteCallbackHookFunc;
}

VOID ProducerClientTestBase::updateFrame()
{
    mFrame.index++;
    mFrame.flags = mFrame.index % mKeyFrameInterval == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
    mFrame.presentationTs += mFrame.duration;
    mFrame.decodingTs = mFrame.presentationTs;
}

STATUS ProducerClientTestBase::createTestStream(UINT32 index, STREAMING_TYPE streamingType, UINT64 maxLatency, UINT64 bufferDuration, BOOL sync)
{
    if (index >= TEST_MAX_STREAM_COUNT) {
        return STATUS_INVALID_ARG;
    }

    Tag tags[TEST_TAG_COUNT];
    UINT32 tagCount = TEST_TAG_COUNT;
    STATUS retStatus;

    for (UINT32 i = 0; i < tagCount; i++) {
        tags[i].name = (PCHAR) MEMALLOC(SIZEOF(CHAR) * (MAX_TAG_NAME_LEN + 1));
        tags[i].value = (PCHAR) MEMALLOC(SIZEOF(CHAR) * (MAX_TAG_VALUE_LEN + 1));
        SPRINTF(tags[i].name, "testTag_%d_%d", index, i);
        SPRINTF(tags[i].value, "testTag_%d_%d_Value", index, i);
        tags[i].version = TAG_CURRENT_VERSION;
    }

    SPRINTF(mStreamInfo.name, "ScaryTestStream_%d", index);
    mStreamInfo.tagCount = tagCount;
    mStreamInfo.tags = tags;
    mStreamInfo.streamCaps.streamingType = streamingType;
    mStreamInfo.streamCaps.bufferDuration = bufferDuration;
    mStreamInfo.streamCaps.maxLatency = maxLatency;

    if (sync) {
        retStatus = createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &mStreams[index]);
    } else {
        retStatus = createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreams[index]);
    }

    for (UINT32 i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    return retStatus;
}

VOID ProducerClientTestBase::freeStreams(BOOL sync)
{
    mProducerStopped = TRUE;
    for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
        DLOGD("Freeing stream index %u with handle value %" PRIu64 " %s", i, mStreams[i], sync ? "synchronously" : "asynchronously");

        // Freeing the stream synchronously
        if (sync) {
            stopKinesisVideoStreamSync(mStreams[i]);
        } else {
            stopKinesisVideoStream(mStreams[i]);
        }

        mStreams[i] = INVALID_STREAM_HANDLE_VALUE;
    }
}

STATUS ProducerClientTestBase::curlEasyPerformHookFunc(PCurlResponse pCurlResponse)
{
    STATUS retStatus = STATUS_SUCCESS;
    if (pCurlResponse == NULL ||
        pCurlResponse->pCurlRequest == NULL ||
        pCurlResponse->pCurlRequest->pCurlApiCallbacks == NULL) {
        return STATUS_NULL_ARG;
    }

    // Get the test object
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) pCurlResponse->pCurlRequest->pCurlApiCallbacks->hookCustomData;

    DLOGV("Curl perform hook for %s", pCurlResponse->pCurlRequest->requestInfo.url);

    pTest->mEasyPerformFnCount++;

    // Check which API function this is
    if (NULL != STRSTR(pCurlResponse->pCurlRequest->requestInfo.url, CREATE_API_POSTFIX)) {
        pTest->mCurlCreateStreamCount++;
        if (pTest->mCurlEasyPerformInjectionCount > 0 &&
            (STATUS_FAILED(pTest->mCreateStreamStatus) || pTest->mCreateStreamCallResult != SERVICE_CALL_RESULT_OK)) {
            retStatus = pTest->mCreateStreamStatus;
            pCurlResponse->callInfo.callResult = pTest->mCreateStreamCallResult;
            pTest->mCurlEasyPerformInjectionCount--;
        } else {
            pTest->mCreateStreamStatus = STATUS_SUCCESS;
            pTest->mCreateStreamCallResult = SERVICE_CALL_RESULT_OK;
        }
    } else if (NULL != STRSTR(pCurlResponse->pCurlRequest->requestInfo.url, DESCRIBE_API_POSTFIX)) {
        pTest->mCurlDescribeStreamCount++;
        if (pTest->mCurlEasyPerformInjectionCount > 0 &&
            (STATUS_FAILED(pTest->mDescribeStreamStatus) || pTest->mDescribeStreamCallResult != SERVICE_CALL_RESULT_OK)) {
            retStatus = pTest->mDescribeStreamStatus;
            pCurlResponse->callInfo.callResult = pTest->mDescribeStreamCallResult;
            pTest->mCurlEasyPerformInjectionCount--;
        } else {
            pTest->mDescribeStreamStatus = STATUS_SUCCESS;
            pTest->mDescribeStreamCallResult = SERVICE_CALL_RESULT_OK;
        }
    } else if (NULL != STRSTR(pCurlResponse->pCurlRequest->requestInfo.url, GET_DATA_ENDPOINT_API_POSTFIX)) {
        pTest->mCurlGetDataEndpointCount++;
        if (pTest->mCurlEasyPerformInjectionCount > 0 &&
            (STATUS_FAILED(pTest->mGetEndpointStatus) || pTest->mGetEndpointCallResult != SERVICE_CALL_RESULT_OK)) {
            retStatus = pTest->mGetEndpointStatus;
            pCurlResponse->callInfo.callResult = pTest->mGetEndpointCallResult;
            pTest->mCurlEasyPerformInjectionCount--;
        } else {
            pTest->mGetEndpointStatus = STATUS_SUCCESS;
            pTest->mGetEndpointCallResult = SERVICE_CALL_RESULT_OK;
        }
    } else if (NULL != STRSTR(pCurlResponse->pCurlRequest->requestInfo.url, TAG_RESOURCE_API_POSTFIX)) {
        pTest->mCurlTagResourceCount++;
        if (pTest->mCurlEasyPerformInjectionCount > 0 &&
            (STATUS_FAILED(pTest->mTagResourceStatus) || pTest->mTagResourceCallResult != SERVICE_CALL_RESULT_OK)) {
            retStatus = pTest->mTagResourceStatus;
            pCurlResponse->callInfo.callResult = pTest->mTagResourceCallResult;
            pTest->mCurlEasyPerformInjectionCount--;
        } else {
            pTest->mTagResourceStatus = STATUS_SUCCESS;
            pTest->mTagResourceCallResult = SERVICE_CALL_RESULT_OK;
        }
    } else if (NULL != STRSTR(pCurlResponse->pCurlRequest->requestInfo.url, PUT_MEDIA_API_POSTFIX)) {
        pTest->mCurlPutMediaCount++;
        if (pTest->mCurlEasyPerformInjectionCount > 0 &&
            (STATUS_FAILED(pTest->mPutMediaStatus) || pTest->mPutMediaCallResult != SERVICE_CALL_RESULT_OK)) {
            retStatus = pTest->mPutMediaStatus;
            pCurlResponse->callInfo.callResult = pTest->mPutMediaCallResult;
            pTest->mCurlEasyPerformInjectionCount--;
        } else {
            pTest->mPutMediaStatus = STATUS_SUCCESS;
            pTest->mPutMediaCallResult = SERVICE_CALL_RESULT_OK;
        }
    }

    return retStatus;
}

STATUS ProducerClientTestBase::curlWriteCallbackHookFunc(PCurlResponse pCurlResponse, PCHAR pBuffer, UINT32 dataSize, PCHAR* ppRetBuffer, PUINT32 pRetDataSize)
{
    UNUSED_PARAM(pBuffer);
    UNUSED_PARAM(dataSize);

    if (pCurlResponse == NULL ||
        pCurlResponse->pCurlRequest == NULL ||
        pCurlResponse->pCurlRequest->pCurlApiCallbacks == NULL) {
        return STATUS_NULL_ARG;
    }

    // Get the test object
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) pCurlResponse->pCurlRequest->pCurlApiCallbacks->hookCustomData;

    pTest->mWriteCallbackFnCount++;

    if (!pTest->mCurlWriteCallbackPassThrough) {
        *ppRetBuffer = pTest->mWriteBuffer;
        *pRetDataSize = pTest->mWriteDataSize;
    }

    return pTest->mWriteStatus;
}

STATUS ProducerClientTestBase::curlReadCallbackHookFunc(PCurlResponse pCurlResponse,
                                                        UPLOAD_HANDLE uploadHandle,
                                                        PBYTE pBuffer,
                                                        UINT32 bufferSize,
                                                        PUINT32 pRetrievedSize,
                                                        STATUS status)
{
    UNUSED_PARAM(pBuffer);
    UNUSED_PARAM(bufferSize);
    UNUSED_PARAM(uploadHandle);
    UNUSED_PARAM(pRetrievedSize);

    if (pCurlResponse == NULL ||
        pCurlResponse->pCurlRequest == NULL ||
        pCurlResponse->pCurlRequest->pCurlApiCallbacks == NULL) {
        return STATUS_NULL_ARG;
    }

    // Get the test object
    ProducerClientTestBase* pTest = (ProducerClientTestBase*) pCurlResponse->pCurlRequest->pCurlApiCallbacks->hookCustomData;

    pTest->mReadCallbackFnCount++;

    if (IS_VALID_UPLOAD_HANDLE(pTest->mAbortUploadhandle) && uploadHandle == pTest->mAbortUploadhandle) {
        pTest->mReadStatus = CURL_READFUNC_ABORT;
    } else {
        pTest->mReadStatus = status;
    }

    return pTest->mReadStatus;
}

STATUS ProducerClientTestBase::testFreeApiCallbackFunc(PUINT64 customData)
{
    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) *customData;

    pTestBase->mFreeApiCallbacksFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testPutStreamFunc(UINT64 customData, PCHAR streamName, PCHAR containerType,
                                                 UINT64 startTimestamp, BOOL absoluteFragmentTimestamp,
                                                 BOOL acksEnabled, PCHAR streamingEndpoint,
                                                 PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(containerType);
    UNUSED_PARAM(startTimestamp);
    UNUSED_PARAM(absoluteFragmentTimestamp);
    UNUSED_PARAM(acksEnabled);
    UNUSED_PARAM(streamingEndpoint);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mPutStreamFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testTagResourceFunc(UINT64 customData, PCHAR streamArn,
                                                   UINT32 tagCount, PTag tags,
                                                   PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamArn);
    UNUSED_PARAM(tagCount);
    UNUSED_PARAM(tags);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mTagResourceFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testGetStreamingEndpointFunc(UINT64 customData, PCHAR streamName, PCHAR apiName,
                                                            PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(apiName);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mGetStreamingEndpointFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testDescribeStreamFunc(UINT64 customData, PCHAR streamName,
                                                      PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(pServiceCallContext);
    STATUS retStatus = STATUS_SUCCESS;

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    // Fault injection
    if (pTestBase->mDescribeStreamFnCount >= pTestBase->mDescribeFailCount &&
            pTestBase->mDescribeStreamFnCount < pTestBase->mDescribeRecoverCount) {
        retStatus = pTestBase->mDescribeRetStatus;
    }

    pTestBase->mDescribeStreamFnCount++;

    return retStatus;
}

STATUS ProducerClientTestBase::testDescribeStreamSecondFunc(UINT64 customData, PCHAR streamName,
                                                      PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mDescribeStreamSecondFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testDescribeStreamThirdFunc(UINT64 customData, PCHAR streamName,
                                                            PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mDescribeStreamThirdFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testDescribeStreamStopChainFunc(UINT64 customData, PCHAR streamName,
                                                      PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mDescribeStreamStopChainFnCount++;

    return STATUS_STOP_CALLBACK_CHAIN;
}

STATUS ProducerClientTestBase::testCreateStreamFunc(UINT64 customData, PCHAR deviceName, PCHAR streamName,
                                                    PCHAR contentType, PCHAR kmsKeyId, UINT64 retentionPeriod,
                                                    PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(deviceName);
    UNUSED_PARAM(contentType);
    UNUSED_PARAM(kmsKeyId);
    UNUSED_PARAM(retentionPeriod);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mCreateStreamFnCount++;

    return STATUS_SUCCESS;
}

STATUS ProducerClientTestBase::testCreateDeviceFunc(UINT64 customData, PCHAR deviceName,
                                                    PServiceCallContext pServiceCallContext)
{
    UNUSED_PARAM(deviceName);
    UNUSED_PARAM(pServiceCallContext);

    ProducerClientTestBase* pTestBase = (ProducerClientTestBase*) customData;

    pTestBase->mCreateDeviceFnCount++;

    return STATUS_SUCCESS;
}

VOID ProducerClientTestBase::printFrameInfo(PFrame pFrame)
{
    DLOGD("Putting frame. TID: 0x%016x, Id: %llu, Key Frame: %s, Size: %u, Dts: %llu, Pts: %llu",
          GETTID(),
          pFrame->index,
          (((pFrame->flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) ? "true" : "false"),
          pFrame->size,
          pFrame->decodingTs,
          pFrame->presentationTs);
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
