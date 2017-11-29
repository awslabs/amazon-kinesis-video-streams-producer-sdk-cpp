#include "ClientTestFixture.h"

class ClientApiTest : public ClientTestBase {
};

TEST_F(ClientApiTest, createKinesisVideoClient_NullInput)
{
    CLIENT_HANDLE clientHandle;

    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(NULL, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, NULL, &clientHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, NULL)));
}

TEST_F(ClientApiTest, createKinesisVideoClient_ValiateCallbacks)
{
    CLIENT_HANDLE clientHandle;

    // Set various callbacks to null and check the behavior
    mClientCallbacks.version = CALLBACKS_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.version = CALLBACKS_CURRENT_VERSION;

    mClientCallbacks.getCurrentTimeFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getCurrentTimeFn = getCurrentTimeFunc;

    mClientCallbacks.getRandomNumberFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getRandomNumberFn = getRandomNumberFunc;

    mClientCallbacks.getDeviceCertificateFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;

    mClientCallbacks.getSecurityTokenFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;

    mClientCallbacks.getDeviceFingerprintFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;

    // All three can be null together
    mClientCallbacks.getDeviceCertificateFn = NULL;
    mClientCallbacks.getSecurityTokenFn = NULL;
    mClientCallbacks.getDeviceFingerprintFn = NULL;
    // We don't support the provisioning yet so it fails
    EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
              createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;

    mClientCallbacks.streamUnderflowReportFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.streamUnderflowReportFn = streamUnderflowReportFunc;

    mClientCallbacks.storageOverflowPressureFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.storageOverflowPressureFn = storageOverflowPressureFunc;

    mClientCallbacks.streamLatencyPressureFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.streamLatencyPressureFn = streamLatencyPressureFunc;

    mClientCallbacks.droppedFrameReportFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.droppedFrameReportFn = droppedFrameReportFunc;

    mClientCallbacks.droppedFragmentReportFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.droppedFragmentReportFn = droppedFragmentReportFunc;

    mClientCallbacks.streamReadyFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.streamReadyFn = streamReadyFunc;

    mClientCallbacks.createMutexFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.createMutexFn = createMutexFunc;

    mClientCallbacks.lockMutexFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.lockMutexFn = lockMutexFunc;

    mClientCallbacks.unlockMutexFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.unlockMutexFn = unlockMutexFunc;

    mClientCallbacks.tryLockMutexFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.tryLockMutexFn = tryLockMutexFunc;

    mClientCallbacks.freeMutexFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.freeMutexFn = freeMutexFunc;

    mClientCallbacks.createStreamFn = NULL;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.createStreamFn = createStreamFunc;

    mClientCallbacks.describeStreamFn = NULL;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.describeStreamFn = describeStreamFunc;

    mClientCallbacks.getStreamingEndpointFn = NULL;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.getStreamingEndpointFn = getStreamingEndpointFunc;

    mClientCallbacks.getStreamingTokenFn = NULL;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.getStreamingTokenFn = getStreamingTokenFunc;

    mClientCallbacks.putStreamFn = NULL;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mClientCallbacks.putStreamFn = putStreamFunc;
}

TEST_F(ClientApiTest, createKinesisVideoClient_ValiateDeviceInfo)
{
    CLIENT_HANDLE clientHandle;

    // Set various device info members to invalid and check the behavior
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;

    mDeviceInfo.streamCount = 0;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.streamCount = MAX_STREAM_COUNT + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.streamCount = MAX_TEST_STREAM_COUNT;

    mDeviceInfo.tagCount = MAX_TAG_COUNT + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.tagCount = 0;

    mDeviceInfo.tagCount = 1;
    // Should fail as tags is NULL
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.tagCount = 0;

    mDeviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;

    mDeviceInfo.storageInfo.storageSize = MIN_STORAGE_ALLOCATION_SIZE - 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = MAX_STORAGE_ALLOCATION_SIZE + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = TEST_DEVICE_STORAGE_SIZE;

    mDeviceInfo.storageInfo.storageSize = MIN_STORAGE_ALLOCATION_SIZE - 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = MAX_STORAGE_ALLOCATION_SIZE + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = TEST_DEVICE_STORAGE_SIZE;

    mDeviceInfo.storageInfo.spillRatio = 101;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.spillRatio = 0;

    MEMSET(mDeviceInfo.storageInfo.rootDirectory, 'a', MAX_PATH_LEN * SIZEOF(CHAR));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.rootDirectory[0] = '\0';

    MEMSET(mDeviceInfo.name, 'a', MAX_DEVICE_NAME_LEN * SIZEOF(CHAR));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    STRCPY(mDeviceInfo.name, TEST_DEVICE_NAME);

    mDeviceInfo.name[0] = '\0';
    // Should still succeed by generating random name
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    STRCPY(mDeviceInfo.name, TEST_DEVICE_NAME);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(ClientApiTest, freeKinesisVideoClient_NullInput)
{
    EXPECT_TRUE(STATUS_FAILED(freeKinesisVideoClient(NULL)));
}

TEST_F(ClientApiTest, stopKinesisVideoStreams_InvalidHandle)
{
    EXPECT_TRUE(STATUS_FAILED(stopKinesisVideoStreams(INVALID_CLIENT_HANDLE_VALUE)));
}

TEST_F(ClientApiTest, getKinesisVideoMetrics_Invalid)
{
    ClientMetrics kinesisVideoClientMetrics;

    kinesisVideoClientMetrics.version = CLIENT_METRICS_CURRENT_VERSION;

    EXPECT_NE(STATUS_SUCCESS, getKinesisVideoMetrics(INVALID_CLIENT_HANDLE_VALUE, &kinesisVideoClientMetrics));
    EXPECT_NE(STATUS_SUCCESS, getKinesisVideoMetrics(mClientHandle, NULL));
    EXPECT_NE(STATUS_SUCCESS, getKinesisVideoMetrics(INVALID_CLIENT_HANDLE_VALUE, NULL));
    EXPECT_NE(STATUS_SUCCESS, getKinesisVideoMetrics(INVALID_CLIENT_HANDLE_VALUE, &kinesisVideoClientMetrics));
    kinesisVideoClientMetrics.version = CLIENT_METRICS_CURRENT_VERSION + 1;
    EXPECT_NE(STATUS_SUCCESS, getKinesisVideoMetrics(mClientHandle, &kinesisVideoClientMetrics));

    kinesisVideoClientMetrics.version = CLIENT_METRICS_CURRENT_VERSION;
    EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoMetrics(mClientHandle, &kinesisVideoClientMetrics));
}
