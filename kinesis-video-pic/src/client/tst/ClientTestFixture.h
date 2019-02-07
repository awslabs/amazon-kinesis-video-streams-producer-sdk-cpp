#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/client/Include.h>
#include "src/client/src/Include_i.h"
#include "src/client/src/Stream.h"
#include "src/mkvgen/src/Include_i.h"

#define TEST_DEVICE_NAME                ((PCHAR) "Test device name")
#define MAX_TEST_STREAM_COUNT           10
#define TEST_DEVICE_STORAGE_SIZE        ((UINT64) 10 * 1024 * 1024)

#define TEST_CLIENT_MAGIC_NUMBER        0x1234567890ULL

#define TEST_CERTIFICATE_BITS           "Test certificate bits"
#define TEST_TOKEN_BITS                 "Test token bits"
#define TEST_DEVICE_FINGERPRINT         ((PCHAR) "Test device fingerprint")

#define TEST_STREAM_NAME                ((PCHAR) "Test stream name")
#define TEST_CONTENT_TYPE               ((PCHAR) "TestContentType")
#define TEST_CODEC_ID                   ((PCHAR) "TestCodec")
#define TEST_TRACK_NAME                 ((PCHAR) "TestTrack")
#define TEST_TRACKID                    0

#define TEST_DEVICE_ARN                 ((PCHAR) "TestDeviceARN")

#define TEST_STREAM_ARN                 ((PCHAR) "TestStreamARN")

#define TEST_STREAMING_ENDPOINT         ((PCHAR) "http://test.com/test_endpoint")

#define TEST_UPDATE_VERSION             ((PCHAR) "TestUpdateVer")

#define TEST_STREAMING_TOKEN            "TestStreamingToken"

#define TEST_UPLOAD_HANDLE              12345

#define TEST_BUFFER_DURATION            (40 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_REPLAY_DURATION            (20 * HUNDREDS_OF_NANOS_IN_A_SECOND)

#define TEST_FRAME_DURATION             (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_LONG_FRAME_DURATION        (400 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TEST_SLEEP_TIME                 (1000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_PRODUCER_SLEEP_TIME        (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_CONSUMER_SLEEP_TIME        (50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TEST_AUTH_EXPIRATION            (UINT64)(-1LL)
#define TEST_TRACK_COUNT                1

#define TEST_SEGMENT_UUID               ((PBYTE) "0123456789abcdef")

// Random const value to be returned
#define TEST_CONST_RAND_FUNC_BYTE       42

// Offset of the MKV Segment Info UUID from the beginning of the header
#define MKV_SEGMENT_UUID_OFFSET         53

//
// Default allocator functions
//
extern UINT64 gTotalClientMemoryUsage;
extern MUTEX gClientMemMutex;
INLINE PVOID instrumentedClientMemAlloc(SIZE_T size)
{
    DLOGS("Test malloc %llu bytes", (UINT64)size);
    MUTEX_LOCK(gClientMemMutex);
    gTotalClientMemoryUsage += size;
    MUTEX_UNLOCK(gClientMemMutex);
    PBYTE pAlloc = (PBYTE) malloc(size + SIZEOF(SIZE_T));
    *(PSIZE_T)pAlloc = size;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE PVOID instrumentedClientMemAlignAlloc(SIZE_T size, SIZE_T alignment)
{
    DLOGS("Test align malloc %llu bytes", (UINT64)size);
    // Just do malloc
    UNUSED_PARAM(alignment);
    return instrumentedClientMemAlloc(size);
}

INLINE PVOID instrumentedClientMemCalloc(SIZE_T num, SIZE_T size)
{
    SIZE_T overallSize = num * size;
    DLOGS("Test calloc %llu bytes", (UINT64)overallSize);
    MUTEX_LOCK(gClientMemMutex);
    gTotalClientMemoryUsage += overallSize;
    MUTEX_UNLOCK(gClientMemMutex);

    PBYTE pAlloc = (PBYTE) calloc(1, overallSize + SIZEOF(SIZE_T));
    *(PSIZE_T)pAlloc = overallSize;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE VOID instrumentedClientMemFree(PVOID ptr)
{
    PBYTE pAlloc = (PBYTE) ptr - SIZEOF(SIZE_T);
    SIZE_T size = *(PSIZE_T) pAlloc;
    DLOGS("Test free %llu bytes", (UINT64)size);

    MUTEX_LOCK(gClientMemMutex);
    gTotalClientMemoryUsage -= size;
    MUTEX_UNLOCK(gClientMemMutex);

    free(pAlloc);
}

//
// Set the allocators to the instrumented equivalents
//
extern memAlloc globalMemAlloc;
extern memAlignAlloc globalMemAlignAlloc;
extern memCalloc globalMemCalloc;
extern memFree globalMemFree;

class ClientTestBase : public ::testing::Test {

public:
    ClientTestBase(): mMagic(TEST_CLIENT_MAGIC_NUMBER),
                      mTime(0),
                      mDuration(0),
                      mFrameTime(0),
                      mFragmentTime(0),
                      mStatus(STATUS_SUCCESS),
                      mStreamHandle(INVALID_STREAM_HANDLE_VALUE),
                      mStreamReady(FALSE),
                      mStreamClosed(FALSE),
                      mClientHandle(INVALID_CLIENT_HANDLE_VALUE),
                      mReturnedClientHandle(INVALID_CLIENT_HANDLE_VALUE),
                      mClientReady(FALSE),
                      mAccessMode(STREAM_ACCESS_MODE_READ),
                      mStreamStartTime(0),
                      mAckRequired(FALSE),
                      mRemainingSize(0),
                      mRemainingDuration(0),
                      mRetention(0),
                      mTerminate(FALSE),
                      mTagCount(0),
                      mStreamCount(0),
                      mStartThreads(FALSE),
                      mDataReadyDuration(0),
                      mDataReadySize(0),
                      mStreamUploadHandle(INVALID_UPLOAD_HANDLE_VALUE),
                      mGetCurrentTimeFuncCount(0),
                      mGetRandomNumberFuncCount(0),
                      mGetDeviceCertificateFuncCount(0),
                      mGetSecurityTokenFuncCount(0),
                      mGetDeviceFingerprintFuncCount(0),
                      mStreamUnderflowReportFuncCount(0),
                      mStorageOverflowPressureFuncCount(0),
                      mStreamLatencyPressureFuncCount(0),
                      mDroppedFrameReportFuncCount(0),
                      mBufferDurationOverflowPrssureFuncCount(0),
                      mDroppedFragmentReportFuncCount(0),
                      mStreamReadyFuncCount(0),
                      mStreamClosedFuncCount(0),
                      mCreateMutexFuncCount(0),
                      mLockMutexFuncCount(0),
                      mUnlockMutexFuncCount(0),
                      mTryLockMutexFuncCount(0),
                      mFreeMutexFuncCount(0),
                      mCreateConditionVariableFuncCount(0),
                      mSignalConditionVariableFuncCount(0),
                      mBroadcastConditionVariableFuncCount(0),
                      mWaitConditionVariableFuncCount(0),
                      mFreeConditionVariableFuncCount(0),
                      mCreateStreamFuncCount(0),
                      mDescribeStreamFuncCount(0),
                      mGetStreamingEndpointFuncCount(0),
                      mGetStreamingTokenFuncCount(0),
                      mPutStreamFuncCount(0),
                      mTagResourceFuncCount(0),
                      mCreateDeviceFuncCount(0),
                      mDeviceCertToTokenFuncCount(0),
                      mClientReadyFuncCount(0),
                      mStreamDataAvailableFuncCount(0),
                      mStreamErrorReportFuncCount(0),
                      mStreamConnectionStaleFuncCount(0),
                      mFragmentAckReceivedFuncCount(0)
    {
        // Store the function pointers
        gTotalClientMemoryUsage = 0;
        storedMemAlloc = globalMemAlloc;
        storedMemAlignAlloc = globalMemAlignAlloc;
        storedMemCalloc = globalMemCalloc;
        storedMemFree = globalMemFree;

        // Create the mutex for the synchronization for the instrumentation
        gClientMemMutex = MUTEX_CREATE(FALSE);

        globalMemAlloc = instrumentedClientMemAlloc;
        globalMemAlignAlloc = instrumentedClientMemAlignAlloc;
        globalMemCalloc = instrumentedClientMemCalloc;
        globalMemFree = instrumentedClientMemFree;

        // Zero things out
        mClientHandle = INVALID_CLIENT_HANDLE_VALUE;
        MEMSET(&mDeviceInfo, 0x00, SIZEOF(DeviceInfo));
        MEMSET(&mClientHandle, 0x00, SIZEOF(ClientCallbacks));
        MEMSET(mDeviceName, 0x00, MAX_DEVICE_NAME_LEN);
        MEMSET(mStreamName, 0x00, MAX_STREAM_NAME_LEN);
        MEMSET(mStreamingEndpoint, 0x00, MAX_URI_CHAR_LEN);
        MEMSET(mContentType, 0x00, MAX_CONTENT_TYPE_LEN);
        MEMSET(&mCallContext, 0x00, SIZEOF(ServiceCallContext));
        MEMSET(mCustomDatas, 0x00, SIZEOF(UINT64) * MAX_TEST_STREAM_COUNT);
        MEMSET(&mStreamDescription, 0x00, SIZEOF(StreamDescription));
        MEMSET(mStreamHandles, 0xFF, SIZEOF(UINT64) * MAX_TEST_STREAM_COUNT);
        MEMSET(mKmsKeyId, 0x00, MAX_ARN_LEN);
        MEMSET(mApiName, 0x00, 256);
        MEMSET(mResourceArn, 0x00, MAX_ARN_LEN);
        MEMSET(&mFragmentAck, 0x00, SIZEOF(FragmentAck));

        // Set the magic number for verification later
        mMagic = TEST_CLIENT_MAGIC_NUMBER;

        // Initialize the callbacks
        mClientCallbacks.customData = (UINT64) this;
        mClientCallbacks.version = CALLBACKS_CURRENT_VERSION;
        mClientCallbacks.createDeviceFn = createDeviceFunc;
        mClientCallbacks.deviceCertToTokenFn = deviceCertToTokenFunc;
        mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
        mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
        mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
        mClientCallbacks.streamUnderflowReportFn = streamUnderflowReportFunc;
        mClientCallbacks.storageOverflowPressureFn = storageOverflowPressureFunc;
        mClientCallbacks.streamLatencyPressureFn = streamLatencyPressureFunc;
        mClientCallbacks.droppedFrameReportFn = droppedFrameReportFunc;
        mClientCallbacks.bufferDurationOverflowPressureFn = bufferDurationOverflowPressureFunc;
        mClientCallbacks.droppedFragmentReportFn = droppedFragmentReportFunc;
        mClientCallbacks.streamReadyFn = streamReadyFunc;
        mClientCallbacks.streamClosedFn = streamClosedFunc;
        mClientCallbacks.createMutexFn = createMutexFunc;
        mClientCallbacks.lockMutexFn = lockMutexFunc;
        mClientCallbacks.unlockMutexFn = unlockMutexFunc;
        mClientCallbacks.tryLockMutexFn = tryLockMutexFunc;
        mClientCallbacks.freeMutexFn = freeMutexFunc;
        mClientCallbacks.createConditionVariableFn = createConditionVariableFunc;
        mClientCallbacks.signalConditionVariableFn = signalConditionVariableFunc;
        mClientCallbacks.broadcastConditionVariableFn = broadcastConditionVariableFunc;
        mClientCallbacks.waitConditionVariableFn = waitConditionVariableFunc;
        mClientCallbacks.freeConditionVariableFn = freeConditionVariableFunc;
        mClientCallbacks.createStreamFn = createStreamFunc;
        mClientCallbacks.describeStreamFn = describeStreamFunc;
        mClientCallbacks.getStreamingEndpointFn = getStreamingEndpointFunc;
        mClientCallbacks.getStreamingTokenFn = getStreamingTokenFunc;
        mClientCallbacks.putStreamFn = putStreamFunc;
        mClientCallbacks.tagResourceFn = tagResourceFunc;
        mClientCallbacks.getCurrentTimeFn = getCurrentTimeFunc;
        mClientCallbacks.getRandomNumberFn = getRandomNumberFunc;
        mClientCallbacks.logPrintFn = logPrintFunc;
        mClientCallbacks.clientReadyFn = clientReadyFunc;
        mClientCallbacks.streamDataAvailableFn = streamDataAvailableFunc;
        mClientCallbacks.streamErrorReportFn = streamErrorReportFunc;
        mClientCallbacks.streamConnectionStaleFn = streamConnectionStaleFunc;
        mClientCallbacks.fragmentAckReceivedFn = fragmentAckReceivedFunc;

        // Initialize the device info, etc..
        mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;
        STRCPY(mDeviceInfo.name, TEST_DEVICE_NAME);
        mDeviceInfo.streamCount = MAX_TEST_STREAM_COUNT;
        mDeviceInfo.tagCount = 0;
        mDeviceInfo.tags = NULL;
        mDeviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;
        mDeviceInfo.storageInfo.rootDirectory[0] = '\0';
        mDeviceInfo.certPath[0] = '\0';
        mDeviceInfo.storageInfo.spillRatio = 0;
        mDeviceInfo.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM;
        mDeviceInfo.storageInfo.storageSize = TEST_DEVICE_STORAGE_SIZE;

        // Initialize stream info
        mStreamInfo.version = STREAM_INFO_CURRENT_VERSION;
        mStreamInfo.tagCount = 0;
        mStreamInfo.retention = mRetention;
        mStreamInfo.tags = NULL;
        STRCPY(mStreamInfo.name, TEST_STREAM_NAME);
        mStreamInfo.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
        STRCPY(mStreamInfo.streamCaps.contentType, TEST_CONTENT_TYPE);
        mStreamInfo.streamCaps.adaptive = FALSE;
        mStreamInfo.streamCaps.maxLatency = STREAM_LATENCY_PRESSURE_CHECK_SENTINEL;
        mStreamInfo.streamCaps.fragmentDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
        mStreamInfo.streamCaps.keyFrameFragmentation = FALSE;
        mStreamInfo.streamCaps.frameTimecodes = TRUE;
        mStreamInfo.streamCaps.absoluteFragmentTimes = FALSE;
        mStreamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
        mStreamInfo.streamCaps.fragmentAcks = TRUE;
        mStreamInfo.streamCaps.recoverOnError = FALSE;
        mStreamInfo.streamCaps.recalculateMetrics = TRUE;
        mStreamInfo.streamCaps.avgBandwidthBps = 15 * 1000000;
        mStreamInfo.streamCaps.frameRate = 24;
        mStreamInfo.streamCaps.bufferDuration = TEST_BUFFER_DURATION;
        mStreamInfo.streamCaps.replayDuration = TEST_REPLAY_DURATION;
        mStreamInfo.streamCaps.timecodeScale = 0;
        mStreamInfo.streamCaps.trackInfoCount = 1;
        mStreamInfo.streamCaps.segmentUuid = TEST_SEGMENT_UUID;
        mTrackInfo.trackId = TEST_TRACKID;
        mTrackInfo.codecPrivateDataSize = 0;
        mTrackInfo.codecPrivateData = NULL;
        STRCPY(mTrackInfo.codecId, TEST_CODEC_ID);
        STRCPY(mTrackInfo.trackName, TEST_TRACK_NAME);
        mTrackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        mTrackInfo.trackCustomData.trackVideoConfig.videoWidth = 1280;
        mTrackInfo.trackCustomData.trackVideoConfig.videoHeight = 720;
        mStreamInfo.streamCaps.trackInfoList = &mTrackInfo;
    }

    PVOID basicProducerRoutine(UINT64);
    PVOID basicConsumerRoutine(UINT64);
    volatile UINT32 mStreamCount;

protected:
    CLIENT_HANDLE mClientHandle;
    CLIENT_HANDLE mReturnedClientHandle;
    STREAM_HANDLE mStreamHandle;
    DeviceInfo mDeviceInfo;
    ClientCallbacks mClientCallbacks;
    UINT64 mMagic;
    UINT64 mTime;
    UINT64 mStreamStartTime;
    UINT64 mDuration;
    UINT64 mFrameTime;
    UINT64 mFragmentTime;
    STATUS mStatus;
    FragmentAck mFragmentAck;
    BOOL mStreamReady;
    BOOL mStreamClosed;
    BOOL mClientReady;
    BOOL mAckRequired;
    STREAM_ACCESS_MODE mAccessMode;
    CHAR mApiName[256];
    CHAR mDeviceName[MAX_DEVICE_NAME_LEN];
    CHAR mStreamName[MAX_STREAM_NAME_LEN];
    CHAR mStreamingEndpoint[MAX_URI_CHAR_LEN];
    CHAR mContentType[MAX_CONTENT_TYPE_LEN];
    CHAR mKmsKeyId[MAX_ARN_LEN];
    UINT64 mRetention;
    StreamInfo mStreamInfo;
    ServiceCallContext mCallContext;
    UINT64 mRemainingSize;
    UINT64 mRemainingDuration;
    UINT64 mDataReadyDuration;
    UINT64 mDataReadySize;
    StreamDescription mStreamDescription;
    volatile BOOL mTerminate;
    volatile BOOL mStartThreads;
    TID mProducerThreads[MAX_TEST_STREAM_COUNT];
    TID mConsumerThreads[MAX_TEST_STREAM_COUNT];
    STREAM_HANDLE mStreamHandles[MAX_TEST_STREAM_COUNT];
    UINT64 mCustomDatas[MAX_TEST_STREAM_COUNT];
    UINT32 mTagCount;
    CHAR mResourceArn[MAX_ARN_LEN];
    UINT64 mStreamUploadHandle;
    TrackInfo mTrackInfo;

    // Callback function count
    volatile UINT32 mGetCurrentTimeFuncCount;
    volatile UINT32 mGetRandomNumberFuncCount;
    volatile UINT32 mGetDeviceCertificateFuncCount;
    volatile UINT32 mGetSecurityTokenFuncCount;
    volatile UINT32 mGetDeviceFingerprintFuncCount;
    volatile UINT32 mStreamUnderflowReportFuncCount;
    volatile UINT32 mStorageOverflowPressureFuncCount;
    volatile UINT32 mStreamLatencyPressureFuncCount;
    volatile UINT32 mDroppedFrameReportFuncCount;
    volatile UINT32 mBufferDurationOverflowPrssureFuncCount;
    volatile UINT32 mDroppedFragmentReportFuncCount;
    volatile UINT32 mStreamReadyFuncCount;
    volatile UINT32 mStreamClosedFuncCount;
    volatile UINT32 mCreateMutexFuncCount;
    volatile UINT32 mLockMutexFuncCount;
    volatile UINT32 mUnlockMutexFuncCount;
    volatile UINT32 mTryLockMutexFuncCount;
    volatile UINT32 mFreeMutexFuncCount;
    volatile UINT32 mCreateConditionVariableFuncCount;
    volatile UINT32 mSignalConditionVariableFuncCount;
    volatile UINT32 mBroadcastConditionVariableFuncCount;
    volatile UINT32 mWaitConditionVariableFuncCount;
    volatile UINT32 mFreeConditionVariableFuncCount;
    volatile UINT32 mCreateStreamFuncCount;
    volatile UINT32 mDescribeStreamFuncCount;
    volatile UINT32 mGetStreamingEndpointFuncCount;
    volatile UINT32 mGetStreamingTokenFuncCount;
    volatile UINT32 mPutStreamFuncCount;
    volatile UINT32 mTagResourceFuncCount;
    volatile UINT32 mCreateDeviceFuncCount;
    volatile UINT32 mDeviceCertToTokenFuncCount;
    volatile UINT32 mClientReadyFuncCount;
    volatile UINT32 mStreamDataAvailableFuncCount;
    volatile UINT32 mStreamErrorReportFuncCount;
    volatile UINT32 mStreamConnectionStaleFuncCount;
    volatile UINT32 mFragmentAckReceivedFuncCount;

    STATUS CreateClient()
    {
        // Set the random number generator seed for reproducibility
        SRAND(12345);

        // Create the client
        STATUS status = createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &mClientHandle);
        DLOGI("Create client returned status code is %08x\n", status);
        EXPECT_EQ(STATUS_SUCCESS, status);

        // Move the client through the state machine.
        // We should have the the security token called and not the cert or device fingerprint
        EXPECT_EQ(0, mGetDeviceFingerprintFuncCount);
        EXPECT_EQ(0, mGetDeviceCertificateFuncCount);
        EXPECT_EQ(0, mDeviceCertToTokenFuncCount);
        EXPECT_EQ(1, mGetSecurityTokenFuncCount);
        EXPECT_EQ(1, mCreateDeviceFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_DEVICE_NAME, mDeviceName));

        // Satisfy the create device callback
        EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));

        // Ensure the OK is called
        EXPECT_TRUE(mClientReady);
        EXPECT_EQ(mClientHandle, mReturnedClientHandle);

        return STATUS_SUCCESS;
    };

    STATUS CreateStream()
    {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(mClientHandle, &mStreamInfo, &mStreamHandle));

        // Ensure describe has been called
        EXPECT_EQ(1, mDescribeStreamFuncCount);

        return STATUS_SUCCESS;
    }

    STATUS ReadyStream()
    {
        // Create a stream
        CreateStream();

        // Ensure the describe called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));

        // Move to ready state
        mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
        STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
        STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
        STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
        STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
        STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
        mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
        mStreamDescription.creationTime = GETTIME();
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

        // Ensure the get endpoint is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName));

        // Move the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

        // Ensure the get token is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName));

        // Move to the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN, SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

        // Ensure the callbacks have been called
        EXPECT_EQ(1, mDescribeStreamFuncCount);
        EXPECT_EQ(1, mGetStreamingTokenFuncCount);
        EXPECT_EQ(1, mGetStreamingEndpointFuncCount);

        return STATUS_SUCCESS;
    }

    STATUS MoveFromTokenToReady()
    {
        // Move to the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN, SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

        return STATUS_SUCCESS;
    }

    STATUS MoveFromEndpointToReady()
    {
        // Validate the callback count
        EXPECT_EQ(2, mGetStreamingEndpointFuncCount);

        // Move the next state
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

        // Ensure the get token is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName));

        // Validate the callback count
        EXPECT_EQ(2, mGetStreamingTokenFuncCount);

        return MoveFromTokenToReady();
    }

    STATUS MoveFromDescribeToReady()
    {
        // Validate the callback count
        EXPECT_EQ(2, mDescribeStreamFuncCount);

        // Move to ready state
        mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
        STRCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME);
        STRCPY(mStreamDescription.streamName, TEST_STREAM_NAME);
        STRCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE);
        STRCPY(mStreamDescription.streamArn, TEST_STREAM_ARN);
        STRCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION);
        mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
        mStreamDescription.creationTime = GETTIME();
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

        // Ensure the get token is called
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName));

        // Validate the callback count
        EXPECT_EQ(2, mDescribeStreamFuncCount);

        return MoveFromEndpointToReady();
    }

    virtual void SetUp()
    {
        DLOGI("\nSetting up test: %s\n", GetTestName());
        ASSERT_EQ(STATUS_SUCCESS, CreateClient());
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        if (IS_VALID_CLIENT_HANDLE(mClientHandle)) {
            freeKinesisVideoClient(&mClientHandle);
        }

        // Validate the allocations cleanup
        DLOGI("Final remaining allocation size is %llu\n", gTotalClientMemoryUsage);
        EXPECT_EQ(0, gTotalClientMemoryUsage);
        globalMemAlloc = storedMemAlloc;
        globalMemAlignAlloc = storedMemAlignAlloc;
        globalMemCalloc = storedMemCalloc;
        globalMemFree = storedMemFree;
        MUTEX_FREE(gClientMemMutex);
    };

    PCHAR GetTestName()
    {
        return (PCHAR) ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

protected:

    // Stored function pointers to reset on exit
    memAlloc storedMemAlloc;
    memAlignAlloc storedMemAlignAlloc;
    memCalloc storedMemCalloc;
    memFree storedMemFree;

    //////////////////////////////////////////////////////////////////////////////////////
    // Static callbacks definitions
    //////////////////////////////////////////////////////////////////////////////////////
    static UINT64 getCurrentTimeFunc(UINT64);
    static UINT32 getRandomNumberFunc(UINT64);
    static UINT32 getRandomNumberConstFunc(UINT64);
    static VOID logPrintFunc(UINT32, PCHAR, PCHAR, ...);
    static STATUS getDeviceCertificateFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getSecurityTokenFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getDeviceFingerprintFunc(UINT64, PCHAR*);
    static STATUS getEmptyDeviceCertificateFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getEmptySecurityTokenFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getEmptyDeviceFingerprintFunc(UINT64, PCHAR*);
    static STATUS streamUnderflowReportFunc(UINT64, STREAM_HANDLE);
    static STATUS storageOverflowPressureFunc(UINT64, UINT64);
    static STATUS streamLatencyPressureFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS droppedFrameReportFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS bufferDurationOverflowPressureFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS droppedFragmentReportFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS streamReadyFunc(UINT64, STREAM_HANDLE);
    static STATUS streamClosedFunc(UINT64, STREAM_HANDLE, UINT64);
    static MUTEX createMutexFunc(UINT64, BOOL);
    static VOID lockMutexFunc(UINT64, MUTEX);
    static VOID unlockMutexFunc(UINT64, MUTEX);
    static BOOL tryLockMutexFunc(UINT64, MUTEX);
    static VOID freeMutexFunc(UINT64, MUTEX);
    static CVAR createConditionVariableFunc(UINT64);
    static STATUS signalConditionVariableFunc(UINT64,
                                              CVAR);
    static STATUS broadcastConditionVariableFunc(UINT64,
                                                 CVAR);
    static STATUS waitConditionVariableFunc(UINT64,
                                            CVAR,
                                            MUTEX,
                                            UINT64);
    static VOID freeConditionVariableFunc(UINT64,
                                          CVAR);
    static STATUS createStreamFunc(UINT64,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   UINT64,
                                   PServiceCallContext);
    static STATUS describeStreamFunc(UINT64,
                                     PCHAR,
                                     PServiceCallContext);
    static STATUS getStreamingEndpointFunc(UINT64,
                                           PCHAR,
                                           PCHAR,
                                           PServiceCallContext);
    static STATUS getStreamingTokenFunc(UINT64,
                                        PCHAR,
                                        STREAM_ACCESS_MODE,
                                        PServiceCallContext);
    static STATUS putStreamFunc(UINT64,
                                PCHAR,
                                PCHAR,
                                UINT64,
                                BOOL,
                                BOOL,
                                PCHAR,
                                PServiceCallContext);

    static STATUS tagResourceFunc(UINT64,
                                  PCHAR,
                                  UINT32,
                                  PTag,
                                  PServiceCallContext);

    static STATUS createDeviceFunc(UINT64,
                                   PCHAR,
                                   PServiceCallContext);

    static STATUS deviceCertToTokenFunc(UINT64,
                                        PCHAR,
                                        PServiceCallContext);

    static STATUS clientReadyFunc(UINT64,
                                  CLIENT_HANDLE);

    static STATUS streamDataAvailableFunc(UINT64,
                                          STREAM_HANDLE,
                                          PCHAR,
                                          UINT64,
                                          UINT64,
                                          UINT64);

    static STATUS streamErrorReportFunc(UINT64,
                                        STREAM_HANDLE,
                                        UPLOAD_HANDLE,
                                        UINT64,
                                        STATUS);

    static STATUS streamConnectionStaleFunc(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);

    static STATUS fragmentAckReceivedFunc(UINT64,
                                          STREAM_HANDLE,
                                          UPLOAD_HANDLE,
                                          PFragmentAck);


};
