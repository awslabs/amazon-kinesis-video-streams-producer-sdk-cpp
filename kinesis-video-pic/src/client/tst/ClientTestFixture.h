#define TEST_DEVICE_NAME         ((PCHAR) "Test device name")
#define MAX_TEST_STREAM_COUNT    10
#define TEST_DEVICE_STORAGE_SIZE ((UINT64) 10 * 1024 * 1024)

#define TEST_CLIENT_MAGIC_NUMBER 0x12345678ULL

#define TEST_CERTIFICATE_BITS   "Test certificate bits"
#define TEST_TOKEN_BITS         "Test token bits"
#define TEST_DEVICE_FINGERPRINT ((PCHAR) "Test device fingerprint")

#define TEST_STREAM_NAME      ((PCHAR) "Test stream name")
#define TEST_CONTENT_TYPE     ((PCHAR) "TestContentType")
#define TEST_CODEC_ID         ((PCHAR) "TestCodec")
#define TEST_TRACK_NAME       ((PCHAR) "TestTrack")
#define TEST_TRACKID          1
#define TEST_TRACK_INDEX      0
#define TEST_INVALID_TRACK_ID 100

#define TEST_VIDEO_TRACK_ID 1
#define TEST_AUDIO_TRACK_ID 2

#define TEST_DEVICE_ARN ((PCHAR) "TestDeviceARN")

#define TEST_STREAM_ARN ((PCHAR) "TestStreamARN")

#define TEST_STREAMING_ENDPOINT ((PCHAR) "http://test.com/test_endpoint")

#define TEST_UPDATE_VERSION ((PCHAR) "TestUpdateVer")

#define TEST_CLIENT_ID ((PCHAR) "Test Client Id")

#define TEST_STREAMING_TOKEN "TestStreamingToken"

#define TEST_UPLOAD_HANDLE 12345

#define TEST_BUFFER_DURATION (40 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_REPLAY_DURATION (20 * HUNDREDS_OF_NANOS_IN_A_SECOND)

#define TEST_FRAME_DURATION      (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_LONG_FRAME_DURATION (400 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TEST_SLEEP_TIME          (1000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_PRODUCER_SLEEP_TIME (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_CONSUMER_SLEEP_TIME (50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TEST_AUTH_EXPIRATION (UINT64)(-1LL)
#define TEST_TRACK_COUNT     1

#define TEST_SEGMENT_UUID ((PBYTE) "0123456789abcdef")

#define TEST_DEFAULT_CREATE_CLIENT_TIMEOUT       CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS* HUNDREDS_OF_NANOS_IN_A_SECOND;
#define TEST_DEFAULT_CREATE_STREAM_TIMEOUT       STREAM_READY_TIMEOUT_DURATION_IN_SECONDS* HUNDREDS_OF_NANOS_IN_A_SECOND;
#define TEST_DEFAULT_STOP_STREAM_TIMEOUT         STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS* HUNDREDS_OF_NANOS_IN_A_SECOND;
#define TEST_DEFAULT_BUFFER_AVAILABILITY_TIMEOUT 15 * HUNDREDS_OF_NANOS_IN_A_SECOND;
#define TEST_STOP_STREAM_TIMEOUT_SHORT           3 * HUNDREDS_OF_NANOS_IN_A_SECOND;

#define TEST_TIME_INCREMENT (100 * HUNDREDS_OF_NANOS_IN_A_MICROSECOND)

#define TEST_TIME_BEGIN (1549835353 * HUNDREDS_OF_NANOS_IN_A_SECOND)

#define TEST_AUDIO_CODEC_ID           "A_AAC"
#define TEST_AUDIO_TRACK_NAME         "Audio"
#define TEST_AUDIO_VIDEO_CONTENT_TYPE "video/h264,audio/aac"

// Random const value to be returned
#define TEST_CONST_RAND_FUNC_BYTE 42

// Offset of the MKV Segment Info UUID from the beginning of the header
#define MKV_SEGMENT_UUID_OFFSET 54

// Default producer configuration frame size
#define TEST_DEFAULT_PRODUCER_CONFIG_FRAME_SIZE 50000
#define TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE 20

#define PASS_TEST_FOR_ZERO_RETENTION_AND_OFFLINE()                                                                                                   \
    if ((mStreamInfo.retention == 0 || !mStreamInfo.streamCaps.fragmentAcks) && mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE) {    \
        EXPECT_EQ(STATUS_OFFLINE_MODE_WITH_ZERO_RETENTION, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &mStreamHandle));               \
        return;                                                                                                                                      \
    }

#define PASS_TEST_FOR_OFFLINE_OR_ZERO_RETENTION()                                                                                                    \
    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE || mStreamInfo.retention == 0 || !mStreamInfo.streamCaps.fragmentAcks) {      \
        return;                                                                                                                                      \
    }

#define PASS_TEST_FOR_OFFLINE_ZERO_REPLAY_DURATION()                                                                                                 \
    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE || mStreamInfo.streamCaps.replayDuration == 0) {                              \
        return;                                                                                                                                      \
    }

#define PASS_TEST_FOR_OFFLINE()                                                                                                                      \
    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_OFFLINE) {                                                                            \
        return;                                                                                                                                      \
    }

#define PASS_TEST_FOR_REALTIME()                                                                                                                     \
    if (mStreamInfo.streamCaps.streamingType == STREAMING_TYPE_REALTIME) {                                                                           \
        return;                                                                                                                                      \
    }

#include <string>
#include <ostream>
#include <iomanip>
#include <functional>
#include <queue>
#include <vector>
#include <map>
#include <iostream>

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/client/Include.h>
#include "src/client/src/Include_i.h"
#include "src/client/src/Stream.h"
#include "src/mkvgen/src/Include_i.h"

#include "MockConsumer.h"
#include "MockProducer.h"
#include "StreamingSession.h"

//
// Default allocator functions
//
extern UINT64 gTotalClientMemoryUsage;
extern MUTEX gClientMemMutex;
INLINE PVOID instrumentedClientMemAlloc(SIZE_T size)
{
    DLOGS("Test malloc %llu bytes", (UINT64) size);
    MUTEX_LOCK(gClientMemMutex);
    gTotalClientMemoryUsage += size;
    MUTEX_UNLOCK(gClientMemMutex);
    PBYTE pAlloc = (PBYTE) malloc(size + SIZEOF(SIZE_T));
    *(PSIZE_T) pAlloc = size;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE PVOID instrumentedClientMemAlignAlloc(SIZE_T size, SIZE_T alignment)
{
    DLOGS("Test align malloc %llu bytes", (UINT64) size);
    // Just do malloc
    UNUSED_PARAM(alignment);
    return instrumentedClientMemAlloc(size);
}

INLINE PVOID instrumentedClientMemCalloc(SIZE_T num, SIZE_T size)
{
    SIZE_T overallSize = num * size;
    DLOGS("Test calloc %llu bytes", (UINT64) overallSize);
    MUTEX_LOCK(gClientMemMutex);
    gTotalClientMemoryUsage += overallSize;
    MUTEX_UNLOCK(gClientMemMutex);

    PBYTE pAlloc = (PBYTE) calloc(1, overallSize + SIZEOF(SIZE_T));
    *(PSIZE_T) pAlloc = overallSize;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE VOID instrumentedClientMemFree(PVOID ptr)
{
    PBYTE pAlloc = (PBYTE) ptr - SIZEOF(SIZE_T);
    SIZE_T size = *(PSIZE_T) pAlloc;
    DLOGS("Test free %llu bytes", (UINT64) size);

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

typedef enum {
    DISABLE_AUTO_SUBMIT = 0,
    STOP_AT_DESCRIBE_STREAM = 1,
    STOP_AT_CREATE_STREAM = 2,
    STOP_AT_TAG_RESOURCE = 3,
    STOP_AT_GET_STREAMING_ENDPOINT = 4,
    STOP_AT_GET_STREAMING_TOKEN = 5,
    STOP_AT_PUT_STREAM = 6,
} AutoSubmitServiceCallResultMode;

class ClientTestBase : public ::testing::Test {
  protected:
    // Lay out the atomics first
    volatile ATOMIC_BOOL mStreamReady;
    volatile ATOMIC_BOOL mStreamClosed;
    volatile ATOMIC_BOOL mClientReady;
    volatile ATOMIC_BOOL mClientShutdown;
    volatile ATOMIC_BOOL mStreamShutdown;
    volatile ATOMIC_BOOL mResetStream;
    volatile ATOMIC_BOOL mAckRequired;
    volatile ATOMIC_BOOL mTerminate;
    volatile ATOMIC_BOOL mStartThreads;

    // Callback function count
    volatile SIZE_T mGetCurrentTimeFuncCount;
    volatile SIZE_T mGetRandomNumberFuncCount;
    volatile SIZE_T mGetDeviceCertificateFuncCount;
    volatile SIZE_T mGetSecurityTokenFuncCount;
    volatile SIZE_T mGetDeviceFingerprintFuncCount;
    volatile SIZE_T mStreamUnderflowReportFuncCount;
    volatile SIZE_T mStorageOverflowPressureFuncCount;
    volatile SIZE_T mStreamLatencyPressureFuncCount;
    volatile SIZE_T mDroppedFrameReportFuncCount;
    volatile SIZE_T mBufferDurationOverflowPrssureFuncCount;
    volatile SIZE_T mDroppedFragmentReportFuncCount;
    volatile SIZE_T mStreamReadyFuncCount;
    volatile SIZE_T mStreamClosedFuncCount;
    volatile SIZE_T mCreateMutexFuncCount;
    volatile SIZE_T mLockMutexFuncCount;
    volatile SIZE_T mUnlockMutexFuncCount;
    volatile SIZE_T mTryLockMutexFuncCount;
    volatile SIZE_T mFreeMutexFuncCount;
    volatile SIZE_T mCreateConditionVariableFuncCount;
    volatile SIZE_T mSignalConditionVariableFuncCount;
    volatile SIZE_T mBroadcastConditionVariableFuncCount;
    volatile SIZE_T mWaitConditionVariableFuncCount;
    volatile SIZE_T mFreeConditionVariableFuncCount;
    volatile SIZE_T mCreateStreamFuncCount;
    volatile SIZE_T mDescribeStreamFuncCount;
    volatile SIZE_T mGetStreamingEndpointFuncCount;
    volatile SIZE_T mGetStreamingTokenFuncCount;
    volatile SIZE_T mPutStreamFuncCount;
    volatile SIZE_T mTagResourceFuncCount;
    volatile SIZE_T mCreateDeviceFuncCount;
    volatile SIZE_T mDeviceCertToTokenFuncCount;
    volatile SIZE_T mClientReadyFuncCount;
    volatile SIZE_T mStreamDataAvailableFuncCount;
    volatile SIZE_T mStreamErrorReportFuncCount;
    volatile SIZE_T mStreamConnectionStaleFuncCount;
    volatile SIZE_T mFragmentAckReceivedFuncCount;
    volatile SIZE_T mClientShutdownFuncCount;
    volatile SIZE_T mStreamShutdownFuncCount;

    volatile SIZE_T mDescribeStreamDoneFuncCount;
    volatile SIZE_T mCreateDeviceDoneFuncCount;

    volatile SIZE_T mMagic;

  public:
    ClientTestBase()
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

        // Set the magic number for verification later
        ATOMIC_STORE(&mMagic, TEST_CLIENT_MAGIC_NUMBER);

        // Initialize the callbacks
        mClientCallbacks.version = CALLBACKS_CURRENT_VERSION;
        mClientCallbacks.customData = (UINT64) this;
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
        mClientCallbacks.clientShutdownFn = clientShutdownFunc;
        mClientCallbacks.streamShutdownFn = streamShutdownFunc;
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
    }

    PVOID basicProducerRoutine(UINT64);
    PVOID basicConsumerRoutine(UINT64);
    volatile UINT32 mStreamCount;
    volatile SIZE_T mTimerCallbackFuncCount;

  protected:
    CLIENT_HANDLE mClientHandle;
    CLIENT_HANDLE mReturnedClientHandle;
    STREAM_HANDLE mStreamHandle;
    DeviceInfo mDeviceInfo;
    ClientCallbacks mClientCallbacks;
    volatile UINT64 mTime;
    UINT64 mStreamStartTime;
    UINT64 mDuration;
    UINT64 mFrameTime;
    UINT64 mFragmentTime;
    STATUS mStatus;
    FragmentAck mFragmentAck;
    STREAM_ACCESS_MODE mAccessMode;
    CHAR mApiName[256];
    CHAR mDeviceName[MAX_DEVICE_NAME_LEN];
    CHAR mStreamName[MAX_STREAM_NAME_LEN];
    CHAR mStreamingEndpoint[MAX_URI_CHAR_LEN];
    CHAR mContentType[MAX_CONTENT_TYPE_LEN];
    CHAR mKmsKeyId[MAX_ARN_LEN];
    StreamInfo mStreamInfo;
    ServiceCallContext mCallContext;
    UINT64 mRemainingSize;
    UINT64 mRemainingDuration;
    UINT64 mDataReadyDuration;
    UINT64 mDataReadySize;
    StreamDescription mStreamDescription;
    TID mProducerThreads[MAX_TEST_STREAM_COUNT];
    TID mConsumerThreads[MAX_TEST_STREAM_COUNT];
    STREAM_HANDLE mStreamHandles[MAX_TEST_STREAM_COUNT];
    UINT64 mCustomDatas[MAX_TEST_STREAM_COUNT];
    UINT32 mTagCount;
    CHAR mResourceArn[MAX_ARN_LEN];
    UINT64 mStreamUploadHandle;
    TrackInfo mTrackInfo;
    BOOL mClientSyncMode;
    PBYTE mToken;
    UINT32 mTokenSize;
    UINT64 mTokenExpiration;
    UINT32 mRepeatTime;

    BOOL mChainControlPlaneServiceCall;
    StreamingSession mStreamingSession;
    MockConsumerConfig mMockConsumerConfig;
    MockProducerConfig mMockProducerConfig;

    MUTEX mTestClientMutex;
    AutoSubmitServiceCallResultMode mSubmitServiceCallResultMode;

    STATUS mThreadReturnStatus;

    MUTEX mAtomicLock;

    void initTestMembers()
    {
        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
        }

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

        // Initialize the device info, etc..
        mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;
        STRNCPY(mDeviceInfo.name, TEST_DEVICE_NAME, MAX_DEVICE_NAME_LEN);
        mDeviceInfo.streamCount = MAX_TEST_STREAM_COUNT;
        mDeviceInfo.tagCount = 0;
        mDeviceInfo.tags = NULL;
        mDeviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;
        mDeviceInfo.storageInfo.rootDirectory[0] = '\0';
        STRNCPY(mDeviceInfo.clientId, TEST_CLIENT_ID, MAX_CLIENT_ID_STRING_LENGTH);
        mDeviceInfo.storageInfo.spillRatio = 0;
        mDeviceInfo.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM;
        mDeviceInfo.storageInfo.storageSize = TEST_DEVICE_STORAGE_SIZE;
        mDeviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION;
        mDeviceInfo.clientInfo.createClientTimeout = TEST_DEFAULT_CREATE_CLIENT_TIMEOUT;
        mDeviceInfo.clientInfo.createStreamTimeout = TEST_DEFAULT_CREATE_STREAM_TIMEOUT;
        mDeviceInfo.clientInfo.stopStreamTimeout = TEST_DEFAULT_STOP_STREAM_TIMEOUT;
        mDeviceInfo.clientInfo.offlineBufferAvailabilityTimeout = TEST_DEFAULT_BUFFER_AVAILABILITY_TIMEOUT;
        mDeviceInfo.clientInfo.loggerLogLevel = logLevel;
        mDeviceInfo.clientInfo.logMetric = FALSE;
        mDeviceInfo.clientInfo.metricLoggingPeriod = 1 * HUNDREDS_OF_NANOS_IN_A_MINUTE;
        mDeviceInfo.clientInfo.automaticStreamingFlags = AUTOMATIC_STREAMING_INTERMITTENT_PRODUCER;
        mDeviceInfo.clientInfo.reservedCallbackPeriod = INTERMITTENT_PRODUCER_PERIOD_DEFAULT;

        // Initialize stream info
        mStreamInfo.version = STREAM_INFO_CURRENT_VERSION;
        mStreamInfo.tagCount = 0;
        mStreamInfo.retention = 0;
        mStreamInfo.tags = NULL;
        STRNCPY(mStreamInfo.name, TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
        mStreamInfo.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
        STRNCPY(mStreamInfo.streamCaps.contentType, TEST_CONTENT_TYPE, MAX_CONTENT_TYPE_LEN);
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
        mStreamInfo.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;
        mStreamInfo.streamCaps.storePressurePolicy = CONTENT_STORE_PRESSURE_POLICY_OOM;
        mStreamInfo.streamCaps.viewOverflowPolicy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM;
        mTrackInfo.trackId = TEST_TRACKID;
        mTrackInfo.codecPrivateDataSize = 0;
        mTrackInfo.codecPrivateData = NULL;
        STRNCPY(mTrackInfo.codecId, TEST_CODEC_ID, MKV_MAX_CODEC_ID_LEN);
        STRNCPY(mTrackInfo.trackName, TEST_TRACK_NAME, MKV_MAX_TRACK_NAME_LEN);
        mTrackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        mTrackInfo.trackCustomData.trackVideoConfig.videoWidth = 1280;
        mTrackInfo.trackCustomData.trackVideoConfig.videoHeight = 720;
        mStreamInfo.streamCaps.trackInfoList = &mTrackInfo;

        setTestTimeVal(0);
        mDuration = 0;
        mFrameTime = 0;
        mFragmentTime = 0;
        mStatus = STATUS_SUCCESS;
        mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
        ATOMIC_STORE_BOOL(&mStreamReady, FALSE);
        ATOMIC_STORE_BOOL(&mStreamClosed, FALSE);
        mClientHandle = INVALID_CLIENT_HANDLE_VALUE;
        mReturnedClientHandle = INVALID_CLIENT_HANDLE_VALUE;
        ATOMIC_STORE_BOOL(&mClientReady, FALSE);
        mAccessMode = STREAM_ACCESS_MODE_READ;
        mClientSyncMode = FALSE;
        mStreamStartTime = 0;
        ATOMIC_STORE_BOOL(&mAckRequired, FALSE);
        mRemainingSize = 0;
        mRemainingDuration = 0;
        ATOMIC_STORE_BOOL(&mTerminate, FALSE);
        mTagCount = 0;
        mStreamCount = 0;
        ATOMIC_STORE_BOOL(&mStartThreads, FALSE);
        mDataReadyDuration = 0;
        mDataReadySize = 0;
        mStreamUploadHandle = INVALID_UPLOAD_HANDLE_VALUE;

        ATOMIC_STORE(&mGetCurrentTimeFuncCount, 0);
        ATOMIC_STORE(&mGetRandomNumberFuncCount, 0);
        ATOMIC_STORE(&mGetDeviceCertificateFuncCount, 0);
        ATOMIC_STORE(&mGetSecurityTokenFuncCount, 0);
        ATOMIC_STORE(&mGetDeviceFingerprintFuncCount, 0);
        ATOMIC_STORE(&mStreamUnderflowReportFuncCount, 0);
        ATOMIC_STORE(&mStorageOverflowPressureFuncCount, 0);
        ATOMIC_STORE(&mStreamLatencyPressureFuncCount, 0);
        ATOMIC_STORE(&mDroppedFrameReportFuncCount, 0);
        ATOMIC_STORE(&mBufferDurationOverflowPrssureFuncCount, 0);
        ATOMIC_STORE(&mDroppedFragmentReportFuncCount, 0);
        ATOMIC_STORE(&mStreamReadyFuncCount, 0);
        ATOMIC_STORE(&mStreamClosedFuncCount, 0);
        ATOMIC_STORE(&mCreateMutexFuncCount, 0);
        ATOMIC_STORE(&mLockMutexFuncCount, 0);
        ATOMIC_STORE(&mUnlockMutexFuncCount, 0);
        ATOMIC_STORE(&mTryLockMutexFuncCount, 0);
        ATOMIC_STORE(&mFreeMutexFuncCount, 0);
        ATOMIC_STORE(&mCreateConditionVariableFuncCount, 0);
        ATOMIC_STORE(&mSignalConditionVariableFuncCount, 0);
        ATOMIC_STORE(&mBroadcastConditionVariableFuncCount, 0);
        ATOMIC_STORE(&mWaitConditionVariableFuncCount, 0);
        ATOMIC_STORE(&mFreeConditionVariableFuncCount, 0);
        ATOMIC_STORE(&mCreateStreamFuncCount, 0);
        ATOMIC_STORE(&mDescribeStreamFuncCount, 0);
        ATOMIC_STORE(&mDescribeStreamDoneFuncCount, 0);
        ATOMIC_STORE(&mGetStreamingEndpointFuncCount, 0);
        ATOMIC_STORE(&mGetStreamingTokenFuncCount, 0);
        ATOMIC_STORE(&mPutStreamFuncCount, 0);
        ATOMIC_STORE(&mTagResourceFuncCount, 0);
        ATOMIC_STORE(&mCreateDeviceFuncCount, 0);
        ATOMIC_STORE(&mCreateDeviceDoneFuncCount, 0);
        ATOMIC_STORE(&mDeviceCertToTokenFuncCount, 0);
        ATOMIC_STORE(&mClientReadyFuncCount, 0);
        ATOMIC_STORE(&mClientShutdownFuncCount, 0);
        ATOMIC_STORE(&mStreamShutdownFuncCount, 0);
        ATOMIC_STORE(&mTimerCallbackFuncCount, 0);
        ATOMIC_STORE(&mStreamDataAvailableFuncCount, 0);
        ATOMIC_STORE(&mStreamErrorReportFuncCount, 0);
        ATOMIC_STORE(&mStreamConnectionStaleFuncCount, 0);
        ATOMIC_STORE(&mFragmentAckReceivedFuncCount, 0);

        mChainControlPlaneServiceCall = FALSE;
        mRepeatTime = 10;
        mSubmitServiceCallResultMode = DISABLE_AUTO_SUBMIT;
        mTokenExpiration = TEST_AUTH_EXPIRATION;
        mThreadReturnStatus = STATUS_SUCCESS;
    }

    /*
     * Default Consumer:
     * - 70kb data buffer.
     * - 1mbps upload speed.
     * - 50 ms buffering ack, received ack and persisted ack delay.
     */
    void initDefaultConsumer()
    {
        mMockConsumerConfig.mDataBufferSizeByte = 70000;
        mMockConsumerConfig.mUploadSpeedBytesPerSecond = 1000000;
        mMockConsumerConfig.mBufferingAckDelayMs = 50;
        mMockConsumerConfig.mReceiveAckDelayMs = 50;
        mMockConsumerConfig.mPersistAckDelayMs = 50;
        mMockConsumerConfig.mEnableAck = mStreamInfo.streamCaps.fragmentAcks;
        mMockConsumerConfig.mRetention = mStreamInfo.retention;
    }

    /*
     * Default Producer
     * - 20 fps
     * - generate key frame every 20 frame
     * - each frame is 50kb
     * - is live
     * - no eofr
     * - sinlge track with track id 1
     */
    void initDefaultProducer()
    {
        mMockProducerConfig.mFps = TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE;
        mMockProducerConfig.mKeyFrameInterval = TEST_DEFAULT_PRODUCER_CONFIG_FRAME_RATE;
        mMockProducerConfig.mFrameSizeByte = TEST_DEFAULT_PRODUCER_CONFIG_FRAME_SIZE;
        mMockProducerConfig.mIsLive = TRUE;
        mMockProducerConfig.mSetEOFR = FALSE;
        mMockProducerConfig.mFrameTrackId = TEST_TRACKID;
    }

    void setupStreamDescription()
    {
        mStreamDescription.version = STREAM_DESCRIPTION_CURRENT_VERSION;
        STRNCPY(mStreamDescription.deviceName, TEST_DEVICE_NAME, MAX_DEVICE_NAME_LEN);
        STRNCPY(mStreamDescription.streamName, TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
        STRNCPY(mStreamDescription.contentType, TEST_CONTENT_TYPE, MAX_CONTENT_TYPE_LEN);
        STRNCPY(mStreamDescription.streamArn, TEST_STREAM_ARN, MAX_ARN_LEN);
        STRNCPY(mStreamDescription.updateVersion, TEST_UPDATE_VERSION, MAX_UPDATE_VERSION_LEN);
        STRNCPY(mStreamDescription.kmsKeyId, mStreamInfo.kmsKeyId, MAX_ARN_LEN);
        mStreamDescription.retention = mStreamInfo.retention;
        mStreamDescription.streamStatus = STREAM_STATUS_ACTIVE;
        // creation time should be sometime in the past.
        mStreamDescription.creationTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) - 6 * HUNDREDS_OF_NANOS_IN_AN_HOUR;
    }

    STATUS CreateClient()
    {
        ATOMIC_STORE(&mGetDeviceFingerprintFuncCount, 0);
        ATOMIC_STORE(&mGetDeviceCertificateFuncCount, 0);
        ATOMIC_STORE(&mDeviceCertToTokenFuncCount, 0);
        ATOMIC_STORE(&mGetSecurityTokenFuncCount, 0);
        ATOMIC_STORE(&mCreateDeviceFuncCount, 0);

        // Set the random number generator seed for reproducibility
        SRAND(12345);

        // Create the client
        STATUS status = createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &mClientHandle);

        DLOGI("Create client returned status code is %08x\n", status);
        EXPECT_EQ(STATUS_SUCCESS, status);

        // Move the client through the state machine.
        // We should have the the security token called and not the cert or device fingerprint
        EXPECT_EQ(0, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount));
        EXPECT_EQ(0, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
        EXPECT_EQ(0, ATOMIC_LOAD(&mDeviceCertToTokenFuncCount));
        EXPECT_EQ(1, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
        EXPECT_EQ(1, ATOMIC_LOAD(&mCreateDeviceFuncCount));
        EXPECT_EQ(0, STRNCMP(TEST_DEVICE_NAME, mDeviceName, SIZEOF(mDeviceName)));

        // Satisfy the create device callback in assync mode
        if (!mClientSyncMode) {
            EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
        }

        // Ensure the OK is called
        EXPECT_TRUE(ATOMIC_LOAD_BOOL(&mClientReady));
        EXPECT_EQ(mClientHandle, mReturnedClientHandle);

        return STATUS_SUCCESS;
    };

    void consumeStream(UINT64 streamStopTimeout)
    {
        UINT64 stopStreamTime, currentTime;
        std::vector<UPLOAD_HANDLE> currentUploadHandles;
        MockConsumer* mockConsumer;
        BOOL gotStreamData, didSubmitAck;

        // The stream should stream out remaining buffer within streamStopTimeout.
        stopStreamTime = mClientCallbacks.getCurrentTimeFn((UINT64) this) + streamStopTimeout;

        do {
            currentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);

            // Get a list of currently active upload handles.
            mStreamingSession.getActiveUploadHandles(currentUploadHandles);

            // Simulate consumer behavior by looping through each active consumer and attempt to prime getKinesisVideoStreamData
            // and kinesisVideoStreamFragmentAck actions.
            for (int i = 0; i < currentUploadHandles.size(); i++) {
                UPLOAD_HANDLE uploadHandle = currentUploadHandles[i];
                mockConsumer = mStreamingSession.getConsumer(uploadHandle);

                STATUS retStatus = mockConsumer->timedGetStreamData(currentTime, &gotStreamData);
                EXPECT_EQ(STATUS_SUCCESS, mockConsumer->timedSubmitNormalAck(currentTime, &didSubmitAck));
                VerifyGetStreamDataResult(retStatus, gotStreamData, uploadHandle, &currentTime, &mockConsumer);
            }
        } while (currentTime < stopStreamTime && !currentUploadHandles.empty());
    }

    void VerifyStopStreamSyncAndFree(BOOL timeout = FALSE)
    {
        TID thread;
        // this thread goes to block.
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&thread, stopStreamSyncRoutine, (PVOID) this));
        THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

        // consumer should continue to consumer data.
        consumeStream(mDeviceInfo.clientInfo.stopStreamTimeout);
        THREAD_JOIN(thread, NULL);
        if (timeout) {
            EXPECT_TRUE(STATUS_OPERATION_TIMED_OUT == mThreadReturnStatus);
            // if expecting timeout, there could be consumers left that are still consuming.
        } else {
            EXPECT_TRUE(STATUS_SUCCESS == mThreadReturnStatus);
            EXPECT_TRUE(mStreamingSession.mConsumerList.empty());
            EXPECT_EQ(TRUE, ATOMIC_LOAD_BOOL(&mStreamClosed));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
    }
    void VerifyGetStreamDataResult(STATUS getStreamDataStatus, BOOL gotStreamData, UPLOAD_HANDLE uploadHandle, PUINT64 pCurrentTime,
                                   MockConsumer** ppMockConsumer)
    {
        EXPECT_TRUE(getStreamDataStatus == STATUS_SUCCESS || getStreamDataStatus == STATUS_END_OF_STREAM ||
                    getStreamDataStatus == STATUS_AWAITING_PERSISTED_ACK || getStreamDataStatus == STATUS_NO_MORE_DATA_AVAILABLE ||
                    getStreamDataStatus == STATUS_UPLOAD_HANDLE_ABORTED);
        // This is the signal for the consumer to terminate, thus taking it out of the active sessions.
        if (getStreamDataStatus == STATUS_END_OF_STREAM || getStreamDataStatus == STATUS_UPLOAD_HANDLE_ABORTED) {
            DLOGD("got end-of-stream for upload handle %llu", uploadHandle);
            mStreamingSession.removeConsumerSession(uploadHandle);
            *ppMockConsumer = NULL;
        }
        // Need to advance time such that each getStreamData happen at different times so that we can
        // check getStreamData time is monotonically increasing
        if (gotStreamData) {
            *pCurrentTime = mClientCallbacks.getCurrentTimeFn((UINT64) this);
        }
    }

    static PVOID stopStreamSyncRoutine(PVOID arg)
    {
        ClientTestBase* pClient = (ClientTestBase*) arg;
        MUTEX_LOCK(pClient->mTestClientMutex);
        // need to release lock before calling stopKinesisVideoStreamSync or will deadlock
        STREAM_HANDLE streamHandle = pClient->mStreamHandle;
        MUTEX_UNLOCK(pClient->mTestClientMutex);
        pClient->mThreadReturnStatus = stopKinesisVideoStreamSync(pClient->mStreamHandle);
        return NULL;
    }

    static PVOID defaultProducerRoutine(PVOID arg)
    {
        ClientTestBase* pClient = (ClientTestBase*) arg;
        MockProducer producer = MockProducer(pClient->mMockProducerConfig, pClient->mStreamHandle);
        STATUS retStatus = STATUS_SUCCESS;
        UINT64 currentTime;
        BOOL didPutFrame;

        do {
            currentTime = pClient->mClientCallbacks.getCurrentTimeFn((UINT64) pClient);
            retStatus = producer.timedPutFrame(currentTime, &didPutFrame);
        } while (!ATOMIC_LOAD_BOOL(&pClient->mTerminate) && STATUS_SUCCEEDED(retStatus));

        MUTEX_LOCK(pClient->mTestClientMutex);
        pClient->mStatus = retStatus;
        MUTEX_UNLOCK(pClient->mTestClientMutex);
        return NULL;
    }

    static PVOID CreateClientRoutineSync(PVOID arg)
    {
        ClientTestBase* pClient = (ClientTestBase*) arg;
        // Free the existing client
        if (IS_VALID_CLIENT_HANDLE(pClient->mClientHandle)) {
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&pClient->mClientHandle));
        }
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&pClient->mDeviceInfo, &pClient->mClientCallbacks, &pClient->mClientHandle));

        return NULL;
    }

    static PVOID CreateStreamSyncRoutine(PVOID arg)
    {
        ClientTestBase* pClient = (ClientTestBase*) arg;
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(pClient->mClientHandle, &pClient->mStreamInfo, &pClient->mStreamHandle));

        return NULL;
    }

    void initScenarioTestMembers()
    {
        // Assumes initTestMembers() has been called first
        // setup attributes

        mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

        // Override get time function
        mClientCallbacks.getCurrentTimeFn = getCurrentIncrementalTimeFunc;
        // set the starting point for getCurrentTimeFunc. Using non zero value for rollback purpose.
        setTestTimeVal(TEST_TIME_BEGIN);
        // Use absoluteFragmentTimes for easier mock ack timestamp calculation.
        mStreamInfo.streamCaps.absoluteFragmentTimes = TRUE;
        mStreamInfo.streamCaps.recoverOnError = TRUE;
        // Increase memory size
        mDeviceInfo.storageInfo.storageSize = ((UINT64) 100 * 1024 * 1024);
    }

    STATUS CreateScenarioTestClient()
    {
        // Set the random number generator seed for reproducibility
        SRAND(12345);
        STATUS status = STATUS_SUCCESS;

        // Free the existing client
        if (IS_VALID_CLIENT_HANDLE(mClientHandle)) {
            status = freeKinesisVideoClient(&mClientHandle);
            ATOMIC_STORE(&mClientShutdownFuncCount, 0);
            ATOMIC_STORE(&mStreamShutdownFuncCount, 0);
            ATOMIC_STORE_BOOL(&mClientShutdown, FALSE);
            ATOMIC_STORE_BOOL(&mStreamShutdown, FALSE);
            ATOMIC_STORE_BOOL(&mResetStream, FALSE);
            EXPECT_EQ(STATUS_SUCCESS, status);
        }

        initScenarioTestMembers();

        // Create the client
        status = createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &mClientHandle);
        DLOGI("Create client returned status code is %08x\n", status);
        EXPECT_EQ(STATUS_SUCCESS, status) << "Create client failed with " << std::hex << status;

        // Satisfy the create device callback
        EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));

        // Ensure the OK is called
        EXPECT_TRUE(ATOMIC_LOAD_BOOL(&mClientReady));
        EXPECT_EQ(mClientHandle, mReturnedClientHandle);

        initDefaultProducer();
        initDefaultConsumer();

        return STATUS_SUCCESS;
    };

    STATUS CreateStream(CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE)
    {
        clientHandle = IS_VALID_CLIENT_HANDLE(clientHandle) ? clientHandle : mClientHandle;

        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &mStreamHandle));

        // Ensure describe has been called
        EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));

        return STATUS_SUCCESS;
    }

    STATUS CreateStreamSync(CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE)
    {
        clientHandle = IS_VALID_CLIENT_HANDLE(clientHandle) ? clientHandle : mClientHandle;

        mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM; // automate all service call result submission
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, &mStreamInfo, &mStreamHandle));

        return STATUS_SUCCESS;
    }

    STATUS ReadyStream()
    {
        // Create a stream
        CreateStream();

        // Ensure the describe called
        EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));

        // Move to ready state
        setupStreamDescription();

        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

        // Ensure the get endpoint is called
        EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));
        EXPECT_EQ(0, STRNCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName, SIZEOF(mApiName)));

        // Move the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

        // Ensure the get token is called
        EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));
        EXPECT_EQ(0, STRNCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName, SIZEOF(mApiName)));

        // Move to the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS,
                  getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN,
                                               SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

        // Ensure the callbacks have been called
        EXPECT_EQ(1, ATOMIC_LOAD(&mDescribeStreamFuncCount));
        EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));
        EXPECT_EQ(1, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));

        return STATUS_SUCCESS;
    }

    STATUS MoveFromTokenToReady()
    {
        // Move to the next state
        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS,
                  getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN,
                                               SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

        return STATUS_SUCCESS;
    }

    STATUS MoveFromEndpointToReady()
    {
        // Validate the callback count
        EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingEndpointFuncCount));

        // Move the next state
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));

        // Ensure the get token is called
        EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));
        EXPECT_EQ(0, STRNCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName, SIZEOF(mApiName)));

        // Validate the callback count
        EXPECT_EQ(2, ATOMIC_LOAD(&mGetStreamingTokenFuncCount));

        return MoveFromTokenToReady();
    }

    STATUS MoveFromDescribeToReady()
    {
        // Validate the callback count
        EXPECT_EQ(2, ATOMIC_LOAD(&mDescribeStreamFuncCount));

        // Move to ready state
        setupStreamDescription();

        // Reset the stream name
        mStreamName[0] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, &mStreamDescription));

        // Ensure the get token is called
        EXPECT_EQ(0, STRNCMP(TEST_STREAM_NAME, mStreamName, MAX_STREAM_NAME_LEN));
        EXPECT_EQ(0, STRNCMP(GET_DATA_ENDPOINT_REAL_TIME_PUT_API_NAME, mApiName, SIZEOF(mApiName)));

        // Validate the callback count
        EXPECT_EQ(2, ATOMIC_LOAD(&mDescribeStreamFuncCount));

        return MoveFromEndpointToReady();
    }

    virtual void SetUp()
    {
        SetUpWithoutClientCreation();
        ASSERT_EQ(STATUS_SUCCESS, CreateClient());
    };

    virtual void SetUpWithoutClientCreation()
    {
        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
            SET_LOGGER_LOG_LEVEL(logLevel);
        }

        DLOGI("\nSetting up test: %s\n", GetTestName());
        mAtomicLock = MUTEX_CREATE(TRUE); // atomicity lock for 64 bit primitives
        mTestClientMutex = MUTEX_CREATE(TRUE);
        initTestMembers();
    }

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        if (IS_VALID_CLIENT_HANDLE(mClientHandle)) {
            freeKinesisVideoClient(&mClientHandle);
        }
        MUTEX_FREE(mTestClientMutex);

        // Clear potential mock consumers from previous run
        mStreamingSession.clearSessions();

        MUTEX_FREE(mAtomicLock);

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
        return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

    UINT64 getTestTimeVal()
    {
        // Meed to atomize the get operation too due to 64 bit on 32 bit platform
        UINT64 timeVal;
        MUTEX_LOCK(mAtomicLock);
        timeVal = mTime;
        MUTEX_UNLOCK(mAtomicLock);

        return timeVal;
    };

    VOID setTestTimeVal(UINT64 timeVal)
    {
        MUTEX_LOCK(mAtomicLock);
        mTime = timeVal;
        MUTEX_UNLOCK(mAtomicLock);
    };

    VOID incrementTestTimeVal(UINT64 timeVal)
    {
        MUTEX_LOCK(mAtomicLock);
        mTime += timeVal;
        MUTEX_UNLOCK(mAtomicLock);
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
    static UINT64 getCurrentPresetTimeFunc(UINT64);
    static UINT64 getCurrentIncrementalTimeFunc(UINT64);
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
    static STATUS signalConditionVariableFunc(UINT64, CVAR);
    static STATUS broadcastConditionVariableFunc(UINT64, CVAR);
    static STATUS waitConditionVariableFunc(UINT64, CVAR, MUTEX, UINT64);
    static VOID freeConditionVariableFunc(UINT64, CVAR);
    static STATUS createStreamFunc(UINT64, PCHAR, PCHAR, PCHAR, PCHAR, UINT64, PServiceCallContext);
    static STATUS describeStreamFunc(UINT64, PCHAR, PServiceCallContext);
    static STATUS describeStreamMultiStreamFunc(UINT64, PCHAR, PServiceCallContext);
    static STATUS getStreamingEndpointFunc(UINT64, PCHAR, PCHAR, PServiceCallContext);
    static STATUS getStreamingEndpointMultiStreamFunc(UINT64, PCHAR, PCHAR, PServiceCallContext);
    static STATUS getStreamingTokenFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
    static STATUS getStreamingTokenMultiStreamFunc(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);

    static STATUS putStreamFunc(UINT64, PCHAR, PCHAR, UINT64, BOOL, BOOL, PCHAR, PServiceCallContext);
    static STATUS putStreamMultiStreamFunc(UINT64, PCHAR, PCHAR, UINT64, BOOL, BOOL, PCHAR, PServiceCallContext);
    static STATUS tagResourceFunc(UINT64, PCHAR, UINT32, PTag, PServiceCallContext);

    static STATUS createDeviceFunc(UINT64, PCHAR, PServiceCallContext);

    static STATUS deviceCertToTokenFunc(UINT64, PCHAR, PServiceCallContext);

    static STATUS clientReadyFunc(UINT64, CLIENT_HANDLE);

    static STATUS streamDataAvailableFunc(UINT64, STREAM_HANDLE, PCHAR, UINT64, UINT64, UINT64);

    static STATUS streamErrorReportFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, STATUS);

    static STATUS streamConnectionStaleFunc(UINT64, STREAM_HANDLE, UINT64);

    static STATUS fragmentAckReceivedFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, PFragmentAck);

    static STATUS clientShutdownFunc(UINT64, CLIENT_HANDLE);
    static STATUS streamShutdownFunc(UINT64, STREAM_HANDLE, BOOL);
};
