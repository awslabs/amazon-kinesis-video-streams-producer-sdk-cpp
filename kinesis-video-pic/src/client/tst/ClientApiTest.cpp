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

TEST_F(ClientApiTest, createKinesisVideoClientSync_NullInput)
{
    CLIENT_HANDLE clientHandle;

    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClientSync(NULL, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClientSync(&mDeviceInfo, NULL, &clientHandle)));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, NULL)));
    mDeviceInfo.clientInfo.createClientTimeout = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
}

TEST_F(ClientApiTest, createKinesisVideoClient_ValidateCallbacks)
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

    mClientCallbacks.clientShutdownFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.clientShutdownFn = clientShutdownFunc;

    mClientCallbacks.streamShutdownFn = NULL;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
    mClientCallbacks.streamShutdownFn = streamShutdownFunc;

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

TEST_F(ClientApiTest, createKinesisVideoClient_ValidateDeviceInfo)
{
    CLIENT_HANDLE clientHandle;

    // Set various device info members to invalid and check the behavior
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;

    // Set various device info members to invalid and check the behavior
    mDeviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION;

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
    mDeviceInfo.storageInfo.storageSize = MIN_STORAGE_ALLOCATION_SIZE - 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = MAX_STORAGE_ALLOCATION_SIZE + 1;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.storageSize = TEST_DEVICE_STORAGE_SIZE;

    mDeviceInfo.storageInfo.spillRatio = 101;
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.spillRatio = 0;

    MEMSET(mDeviceInfo.storageInfo.rootDirectory, 'a', (MAX_PATH_LEN + 1) * SIZEOF(CHAR));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    mDeviceInfo.storageInfo.rootDirectory[0] = '\0';

    MEMSET(mDeviceInfo.name, 'a', (MAX_DEVICE_NAME_LEN + 1) * SIZEOF(CHAR));
    EXPECT_TRUE(STATUS_FAILED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    STRCPY(mDeviceInfo.name, TEST_DEVICE_NAME);

    MEMSET(mDeviceInfo.clientId, 'a', (MAX_CLIENT_ID_STRING_LENGTH + 1) * SIZEOF(CHAR));
    EXPECT_EQ(STATUS_INVALID_CLIENT_ID_STRING_LENGTH, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    STRCPY(mDeviceInfo.clientId, TEST_CLIENT_ID);

    mDeviceInfo.name[0] = '\0';
    // Should still succeed by generating random name
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    STRCPY(mDeviceInfo.name, TEST_DEVICE_NAME);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Validate the default client timeouts with 0
    mDeviceInfo.clientInfo.createClientTimeout = mDeviceInfo.clientInfo.createStreamTimeout = mDeviceInfo.clientInfo.stopStreamTimeout = 0;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    PKinesisVideoClient pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createClientTimeout);
    EXPECT_EQ(STREAM_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createStreamTimeout);
    EXPECT_EQ(STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.stopStreamTimeout);
    mDeviceInfo.clientInfo.createClientTimeout = TEST_DEFAULT_CREATE_CLIENT_TIMEOUT;
    mDeviceInfo.clientInfo.createStreamTimeout = TEST_DEFAULT_CREATE_STREAM_TIMEOUT;
    mDeviceInfo.clientInfo.stopStreamTimeout = TEST_DEFAULT_STOP_STREAM_TIMEOUT;
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Validate the default client timeouts with INVALID_TIMESTAMP_VALUE
    mDeviceInfo.clientInfo.createClientTimeout = mDeviceInfo.clientInfo.createStreamTimeout = mDeviceInfo.clientInfo.stopStreamTimeout = INVALID_TIMESTAMP_VALUE;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createClientTimeout);
    EXPECT_EQ(STREAM_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createStreamTimeout);
    EXPECT_EQ(STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.stopStreamTimeout);
    mDeviceInfo.clientInfo.createClientTimeout = TEST_DEFAULT_CREATE_CLIENT_TIMEOUT;
    mDeviceInfo.clientInfo.createStreamTimeout = TEST_DEFAULT_CREATE_STREAM_TIMEOUT;
    mDeviceInfo.clientInfo.stopStreamTimeout = TEST_DEFAULT_STOP_STREAM_TIMEOUT;
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Validate the client info logging
    mDeviceInfo.clientInfo.logMetric = TRUE;
    mDeviceInfo.clientInfo.metricLoggingPeriod = 2 * HUNDREDS_OF_NANOS_IN_AN_HOUR;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(TRUE, pKinesisVideoClient->deviceInfo.clientInfo.logMetric);
    EXPECT_EQ(2 * HUNDREDS_OF_NANOS_IN_AN_HOUR, pKinesisVideoClient->deviceInfo.clientInfo.metricLoggingPeriod);
    mDeviceInfo.clientInfo.logMetric = FALSE;
    mDeviceInfo.clientInfo.metricLoggingPeriod = 1 * HUNDREDS_OF_NANOS_IN_A_MINUTE;
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Validate the default client timeouts for earlier version
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION - 1;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle)));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createClientTimeout);
    EXPECT_EQ(STREAM_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createStreamTimeout);
    EXPECT_EQ(STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.stopStreamTimeout);
    EXPECT_EQ(FALSE, pKinesisVideoClient->deviceInfo.clientInfo.logMetric);
    EXPECT_EQ(0, pKinesisVideoClient->deviceInfo.clientInfo.metricLoggingPeriod);
    mDeviceInfo.version = DEVICE_INFO_CURRENT_VERSION;
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Try the older version directly
    DeviceInfo_V0 deviceInfo_v0;
    // Copy the existing stuff
    MEMCPY(&deviceInfo_v0, &mDeviceInfo, SIZEOF(DeviceInfo_V0));
    // Set the version
    deviceInfo_v0.version = DEVICE_INFO_CURRENT_VERSION - 1;
    EXPECT_TRUE(STATUS_SUCCEEDED(createKinesisVideoClient((PDeviceInfo) &deviceInfo_v0, &mClientCallbacks, &clientHandle)));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createClientTimeout);
    EXPECT_EQ(STREAM_READY_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.createStreamTimeout);
    EXPECT_EQ(STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS * HUNDREDS_OF_NANOS_IN_A_SECOND, pKinesisVideoClient->deviceInfo.clientInfo.stopStreamTimeout);
    EXPECT_EQ('\0', pKinesisVideoClient->deviceInfo.clientId[0]);
    EXPECT_EQ(FALSE, pKinesisVideoClient->deviceInfo.clientInfo.logMetric);
    EXPECT_EQ(0, pKinesisVideoClient->deviceInfo.clientInfo.metricLoggingPeriod);
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(ClientApiTest, kinesisVideoClientCreateSync_Valid_Timeout)
{
    CLIENT_HANDLE clientHandle;

    mClientSyncMode = TRUE;
    mDeviceInfo.clientInfo.createClientTimeout = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_TRUE(IS_VALID_CLIENT_HANDLE(clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    // Create synchronously to timeout
    mClientSyncMode = FALSE;
    EXPECT_EQ(STATUS_OPERATION_TIMED_OUT, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_FALSE(IS_VALID_CLIENT_HANDLE(clientHandle));
    // Freeing is invariant and should succeed on freed handle
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(ClientApiTest, kinesisVideoClientCreateSync_Store_Alloc)
{
    CLIENT_HANDLE clientHandle, failedClientHandle;

    mClientSyncMode = TRUE;
    mDeviceInfo.clientInfo.createClientTimeout = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    mDeviceInfo.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM_CONTENT_STORE_ALLOC;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_TRUE(IS_VALID_CLIENT_HANDLE(clientHandle));

    // Allocating another should fail
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &failedClientHandle));
    EXPECT_FALSE(IS_VALID_CLIENT_HANDLE(failedClientHandle));

    // Free the client and re-try
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_TRUE(IS_VALID_CLIENT_HANDLE(clientHandle));

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

TEST_F(ClientApiTest, kinesisVideoClientCreateInvalidSecurityTokenExpiration)
{
    CLIENT_HANDLE clientHandle;

    mTokenExpiration = GETTIME() - HUNDREDS_OF_NANOS_IN_A_SECOND; // expiration < current time
    EXPECT_EQ(STATUS_CLIENT_AUTH_CALL_FAILED, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    mTokenExpiration = GETTIME(); // expiration == current time
    EXPECT_EQ(STATUS_CLIENT_AUTH_CALL_FAILED, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    mTokenExpiration = GETTIME() + 20 * HUNDREDS_OF_NANOS_IN_A_SECOND; // expiration - current time < min token expiration duration
    EXPECT_EQ(STATUS_CLIENT_AUTH_CALL_FAILED, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));

    mTokenExpiration = GETTIME() + 40 * HUNDREDS_OF_NANOS_IN_A_SECOND; // expiration - current time >= min token expiration duration
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}
