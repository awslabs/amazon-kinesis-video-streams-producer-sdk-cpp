#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/cproducer/Include.h>
#include <src/source/Include_i.h>
#include "RotatingStaticAuthCallbacks.h"

#define TEST_AUTH_FILE_PATH                                     (PCHAR) "TEST_KVS_AUTH_FILE_PATH"
#define TEST_STREAM_NAME                                        (PCHAR) "ScaryTestStream_0"
#define TEST_DEVICE_INFO_NAME                                   (PCHAR) "TestDeviceName"
#define TEST_USER_AGENT                                         (PCHAR) "Test User Agent"
#define TEST_CLIENT_ID                                          (PCHAR) "Test Client"
#define TEST_MAX_STREAM_COUNT                                   1000
#define TEST_DEFAULT_STORAGE_SIZE                               (256 * 1024 * 1024)
#define TEST_DEFAULT_REGION                                     (PCHAR) "us-west-2"
#define TEST_CONTROL_PLANE_URI                                  EMPTY_STRING
#define TEST_CERTIFICATE_PATH                                   EMPTY_STRING
#define TEST_DEFAULT_CHAIN_COUNT                                DEFAULT_CALLBACK_CHAIN_COUNT

#define TEST_ACCESS_KEY                                         (PCHAR) "Test access key"
#define TEST_SECRET_KEY                                         (PCHAR) "Test secret key"
#define TEST_SESSION_TOKEN                                      (PCHAR) "Test session token"

#define TEST_FRAME_DURATION                                     (50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_EXECUTION_DURATION                                 (10 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_KEY_FRAME_INTERVAL                                 50
#define TEST_STREAM_COUNT                                       1
#define MAX_TEST_STREAM_COUNT                                   10
#define TEST_FRAME_SIZE                                         1000
#define TEST_STREAMING_TOKEN_DURATION                           (40 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_STORAGE_SIZE_IN_BYTES                              (1024 * 1024 * 1024ull)
#define TEST_MAX_STREAM_LATENCY                                 (30 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_STREAM_BUFFER_DURATION                             (120 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_START_STOP_ITERATION_COUNT                         200
#define TEST_CACHING_ENDPOINT_PERIOD                            (5 * HUNDREDS_OF_NANOS_IN_A_MINUTE)
#define TEST_TAG_COUNT                                          5
#define TEST_DEFAULT_PRESSURE_HANDLER_RETRY_COUNT               10
#define TEST_DEFAULT_PRESSURE_HANDLER_GRACE_PERIOD_SECONDS      5

#define TEST_FPS                                                20
#define TEST_MEDIA_DURATION_SECONDS                             60

// 1 minutes of frames
#define TEST_TOTAL_FRAME_COUNT                                  (TEST_FPS * TEST_MEDIA_DURATION_SECONDS)

#define TEST_TIMESTAMP_SENTINEL                                 -1
#define TEST_RETENTION_PERIOD                                   (2 * HUNDREDS_OF_NANOS_IN_AN_HOUR)

#define TEST_CREATE_PRODUCER_TIMEOUT                            (300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_CREATE_STREAM_TIMEOUT                              (1000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TEST_MAGIC_NUMBER                                       0x1234abcd
//
// Set the allocators to the instrumented equivalents
//
extern memAlloc globalMemAlloc;
extern memAlignAlloc globalMemAlignAlloc;
extern memCalloc globalMemCalloc;
extern memFree globalMemFree;

namespace com { namespace amazonaws { namespace kinesis { namespace video {
//
// Default allocator functions
//
extern UINT64 gTotalProducerClientMemoryUsage;
extern MUTEX gProducerClientMemMutex;
INLINE PVOID instrumentedClientMemAlloc(SIZE_T size)
{
    DLOGS("Test malloc %llu bytes", (UINT64)size);
    MUTEX_LOCK(gProducerClientMemMutex);
    gTotalProducerClientMemoryUsage += size;
    MUTEX_UNLOCK(gProducerClientMemMutex);
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
    MUTEX_LOCK(gProducerClientMemMutex);
    gTotalProducerClientMemoryUsage += overallSize;
    MUTEX_UNLOCK(gProducerClientMemMutex);

    PBYTE pAlloc = (PBYTE) calloc(1, overallSize + SIZEOF(SIZE_T));
    *(PSIZE_T)pAlloc = overallSize;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE VOID instrumentedClientMemFree(PVOID ptr)
{
    PBYTE pAlloc = (PBYTE) ptr - SIZEOF(SIZE_T);
    SIZE_T size = *(PSIZE_T) pAlloc;
    DLOGS("Test free %llu bytes", (UINT64)size);

    MUTEX_LOCK(gProducerClientMemMutex);
    gTotalProducerClientMemoryUsage -= size;
    MUTEX_UNLOCK(gProducerClientMemMutex);

    free(pAlloc);
}

typedef enum {
    BufferPressureOK,
    BufferInPressure
} BufferPressureState;

// Forward declaration
class ProducerClientTestBase;

extern ProducerClientTestBase* gProducerClientTestBase;

class ProducerClientTestBase : public ::testing::Test {
public:

    ProducerClientTestBase();
    VOID updateFrame();

    PVOID basicProducerRoutine(STREAM_HANDLE streamHandle, STREAMING_TYPE streaming_type = STREAMING_TYPE_REALTIME);

protected:

    virtual void SetUp()
    {
        DLOGI("\nSetting up test: %s\n", GetTestName());

        if (NULL != (mAccessKey = getenv(ACCESS_KEY_ENV_VAR))) {
            mAccessKeyIdSet = TRUE;
        }

        mSecretKey = getenv(SECRET_KEY_ENV_VAR);
        mSessionToken = getenv(SESSION_TOKEN_ENV_VAR);

        ASSERT_TRUE(mSecretKey != NULL && mAccessKey != NULL) << "Missing accessKey and secretKey.";

        if (NULL == (mRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
            mRegion = TEST_DEFAULT_REGION;
        }

        if (NULL == (mCaCertPath = getenv(CACERT_PATH_ENV_VAR))) {
            mCaCertPath = EMPTY_STRING;
        }
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        if (IS_VALID_CLIENT_HANDLE(mClientHandle)) {
            freeKinesisVideoClient(&mClientHandle);
        }

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&mCallbacksProvider));
        EXPECT_EQ(STATUS_SUCCESS, freeStreamCallbacks(&mStreamCallbacks));

        if (mProducerCallbacks != NULL) {
            MEMFREE(mProducerCallbacks);
        }

        MEMFREE(mFrameBuffer);
        MUTEX_FREE(mTestCallbackLock);

        // Validate the allocations cleanup
        DLOGI("Final remaining allocation size is %llu", gTotalProducerClientMemoryUsage);
        EXPECT_EQ(0, gTotalProducerClientMemoryUsage);
        globalMemAlloc = mStoredMemAlloc;
        globalMemAlignAlloc = mStoredMemAlignAlloc;
        globalMemCalloc = mStoredMemCalloc;
        globalMemFree = mStoredMemFree;
        MUTEX_FREE(gProducerClientMemMutex);
    };

    VOID handlePressure(volatile BOOL* pressureFlag, UINT32 gracePeriodSeconds);

    PCHAR GetTestName()
    {
        return (PCHAR) ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

    VOID createDefaultProducerClient(BOOL cachingEndpoint = FALSE,
                                     UINT64 createStreamTimeout = TEST_CREATE_STREAM_TIMEOUT,
                                     BOOL continuousRetry = FALSE);
    STATUS createTestStream(UINT32 index,
                            STREAMING_TYPE streamingType = STREAMING_TYPE_REALTIME,
                            UINT32 maxLatency = TEST_MAX_STREAM_LATENCY,
                            UINT32 bufferDuration = TEST_STREAM_BUFFER_DURATION);
    VOID freeStreams(BOOL sync = FALSE);
    VOID printFrameInfo(PFrame pFrame);

    // Test API callback funcs
    static STATUS testFreeApiCallbackFunc(PUINT64);
    static STATUS testCreateDeviceFunc(UINT64, PCHAR, PServiceCallContext);
    static STATUS testTagResourceFunc(UINT64, PCHAR, UINT32, PTag, PServiceCallContext);
    static STATUS testPutStreamFunc(UINT64, PCHAR, PCHAR, UINT64, BOOL, BOOL, PCHAR, PServiceCallContext);
    static STATUS testGetStreamingEndpointFunc(UINT64, PCHAR, PCHAR, PServiceCallContext);
    static STATUS testDescribeStreamFunc(UINT64, PCHAR, PServiceCallContext);
    static STATUS testCreateStreamFunc(UINT64, PCHAR, PCHAR, PCHAR, PCHAR, UINT64, PServiceCallContext);

    static STATUS testDescribeStreamSecondFunc(UINT64, PCHAR, PServiceCallContext);
    static STATUS testDescribeStreamThirdFunc(UINT64, PCHAR, PServiceCallContext);
    //Api callback chain stops if any function in the chain terminates callback continuation through stop chain status
    static STATUS testDescribeStreamStopChainFunc(UINT64, PCHAR, PServiceCallContext);

    static STATUS testBufferDurationOverflowFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS testStreamLatencyPressureFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS testStreamConnectionStaleFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS testDroppedFrameReportFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS testStreamErrorReportFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, STATUS);
    static STATUS testFragmentAckReceivedFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, PFragmentAck);
    static STATUS testStreamReadyFunc(UINT64, STREAM_HANDLE);
    static STATUS testStreamClosedFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE);

    static STATUS testStorageOverflowFunc(UINT64 customData, UINT64 remainingBytes);

    // Test hook function for easy perform
    static STATUS curlEasyPerformHookFunc(PCurlResponse);
    static STATUS curlWriteCallbackHookFunc(PCurlResponse, PCHAR, UINT32, PCHAR*, PUINT32);
    static STATUS curlReadCallbackHookFunc(PCurlResponse,
                                           UPLOAD_HANDLE,
                                           PBYTE,
                                           UINT32,
                                           PUINT32,
                                           STATUS);

    CLIENT_HANDLE mClientHandle;
    PClientCallbacks mCallbacksProvider;
    DeviceInfo mDeviceInfo;
    StreamInfo mStreamInfo;
    TrackInfo mTrackInfo;

    PCHAR mAccessKey;
    PCHAR mSecretKey;
    PCHAR mSessionToken;
    PCHAR mRegion;
    PCHAR mCaCertPath;
    UINT64 mStreamingRotationPeriod;

    CHAR mDefaultRegion[MAX_REGION_NAME_LEN + 1];
    BOOL mAccessKeyIdSet;

    TID mProducerThread;

    STREAM_HANDLE mStreams[MAX_TEST_STREAM_COUNT];

    volatile bool mStartProducer;
    volatile bool mStopProducer;
    volatile bool mProducerStopped;

    // Test callbacks
    ApiCallbacks mApiCallbacks;
    PStreamCallbacks mStreamCallbacks;
    PProducerCallbacks mProducerCallbacks;

    // Callback counters
    UINT32 mFreeApiCallbacksFnCount;
    UINT32 mPutStreamFnCount;
    UINT32 mTagResourceFnCount;
    UINT32 mGetStreamingEndpointFnCount;
    UINT32 mDescribeStreamFnCount;
    UINT32 mDescribeStreamSecondFnCount;
    UINT32 mDescribeStreamThirdFnCount;
    UINT32 mDescribeStreamStopChainFnCount;
    UINT32 mCreateStreamFnCount;
    UINT32 mCreateDeviceFnCount;

    // Test hook counters
    UINT32 mEasyPerformFnCount;
    UINT32 mWriteCallbackFnCount;
    UINT32 mReadCallbackFnCount;

    // Easy perform hook function call tracking variables
    UINT32 mCurlGetDataEndpointCount;
    UINT32 mCurlDescribeStreamCount;
    UINT32 mCurlPutMediaCount;
    UINT32 mCurlTagResourceCount;
    UINT32 mCurlCreateStreamCount;

    // Easy perform hook function controlling variables
    UINT32 mCurlEasyPerformInjectionCount;
    STATUS mCreateStreamStatus;
    SERVICE_CALL_RESULT mCreateStreamCallResult;

    STATUS mDescribeStreamStatus;
    SERVICE_CALL_RESULT mDescribeStreamCallResult;

    STATUS mGetEndpointStatus;
    SERVICE_CALL_RESULT mGetEndpointCallResult;

    STATUS mTagResourceStatus;
    SERVICE_CALL_RESULT mTagResourceCallResult;

    STATUS mPutMediaStatus;
    SERVICE_CALL_RESULT mPutMediaCallResult;

    STATUS mWriteStatus;
    PCHAR mWriteBuffer;
    UINT32 mWriteDataSize;
    volatile BOOL mCurlWriteCallbackPassThrough;

    STATUS mReadStatus;
    UINT32 mReadSize;
    UPLOAD_HANDLE mAbortUploadhandle;

    // reset st connection
    UINT32 mResetStreamCounter;

    // Frame
    Frame mFrame;
    PBYTE mFrameBuffer;
    UINT32 mFrameSize;
    UINT64 mFps;
    UINT64 mKeyFrameInterval;

    MUTEX mTestCallbackLock;
    volatile UINT32 mBufferDurationOverflowFnCount;
    volatile UINT32 mStreamLatencyPressureFnCount;
    volatile UINT32 mConnectionStaleFnCount;
    volatile UINT32 mDroppedFrameFnCount;
    volatile UINT32 mStreamErrorFnCount;
    volatile UINT32 mFragmentAckReceivedFnCount;
    volatile UINT32 mStreamReadyFnCount;
    volatile UINT32 mStreamClosedFnCount;
    volatile UINT32 mPersistedFragmentCount;
    volatile UINT32 mStorageOverflowCount;

    // Buffer pressure state machine variables
    volatile BOOL mBufferDurationInPressure;
    volatile BOOL mBufferStorageInPressure;
    BufferPressureState mCurrentPressureState;
    UINT32 mPressureHandlerRetryCount;

private:

    // Stored function pointers to reset on exit
    memAlloc mStoredMemAlloc;
    memAlignAlloc mStoredMemAlignAlloc;
    memCalloc mStoredMemCalloc;
    memFree mStoredMemFree;

};

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
