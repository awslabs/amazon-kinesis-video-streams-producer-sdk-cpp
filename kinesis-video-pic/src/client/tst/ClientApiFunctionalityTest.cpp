#include "ClientTestFixture.h"

class ClientApiFunctionalityTest : public ClientTestBase {
public:
    VOID authIntegrationTest(BOOL sync);
};

VOID ClientApiFunctionalityTest::authIntegrationTest(BOOL sync)
{
    CLIENT_HANDLE clientHandle;
    PKinesisVideoClient pKinesisVideoClient;

    // Validate the client-level callbacks - starting from default base class object
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount));

    // Try the certificate integration first
    mClientCallbacks.getSecurityTokenFn = NULL;
    mClientCallbacks.getDeviceFingerprintFn = NULL;
    if (!sync) {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    } else {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    }

    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, STRCMP((PCHAR) pKinesisVideoClient->certAuthInfo.data, TEST_CERTIFICATE_BITS));
    EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(0, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount));
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the provisioning
    mClientCallbacks.getDeviceCertificateFn = NULL;
    mClientCallbacks.getSecurityTokenFn = NULL;
    // Currently provisioning is unsupported so it should fail
    if (!sync) {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    } else {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    }

    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, STRCMP(pKinesisVideoClient->deviceFingerprint, TEST_DEVICE_FINGERPRINT));
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount)); // MAX_RETRY_COUNT + 1 times
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try with all being null
    mClientCallbacks.getDeviceCertificateFn = NULL;
    mClientCallbacks.getSecurityTokenFn = NULL;
    mClientCallbacks.getDeviceFingerprintFn = NULL;
    // Currently provisioning is unsupported so it should fail
    if (!sync) {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    } else {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    }

    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    // EXPECT_EQ(0, pKinesisVideoClient->certAuthInfo.data[0]);
    EXPECT_EQ(MAX_DEVICE_FINGERPRINT_LENGTH, STRLEN(pKinesisVideoClient->deviceFingerprint));
    // EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(1, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount)); // MAX_RETRY_COUNT + 1
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the security token integration and empty data
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    if (!sync) {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    } else {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    }

    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    if (!sync) {
        EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    } else {
        // In case of SYNC we primed the state machine so we got the token from the certificate already
        EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->tokenAuthInfo.expiration);
    }
    EXPECT_EQ(0, STRCMP((PCHAR) pKinesisVideoClient->certAuthInfo.data, TEST_CERTIFICATE_BITS));
    EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(2, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount)); // stays the same count
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the empty token to call the cert
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    if (!sync) {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
        pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
        EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    } else {
        EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
        pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
        EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->tokenAuthInfo.expiration);
    }

    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    EXPECT_NE(0, pKinesisVideoClient->certAuthInfo.data[0]);

    EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    EXPECT_EQ('\0', pKinesisVideoClient->deviceFingerprint[0]);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(3, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount));
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try with all returning empty
    mClientCallbacks.getDeviceCertificateFn = getEmptyDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getEmptyDeviceFingerprintFunc;
    // Currently provisioning is unsupported so it should fail
    if (!sync) {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    } else {
        EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
                  createKinesisVideoClientSync(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    }

    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    EXPECT_EQ(MAX_DEVICE_FINGERPRINT_LENGTH, STRLEN(pKinesisVideoClient->deviceFingerprint));
    // Extra one as the base test has already called the function once
    EXPECT_EQ(4, ATOMIC_LOAD(&mGetSecurityTokenFuncCount));
    EXPECT_EQ(4, ATOMIC_LOAD(&mGetDeviceCertificateFuncCount));
    EXPECT_EQ(12, ATOMIC_LOAD(&mGetDeviceFingerprintFuncCount));
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
}

TEST_F(ClientApiFunctionalityTest, createKinesisVideoClient_AuthIntegration)
{
    authIntegrationTest(FALSE);
}

TEST_F(ClientApiFunctionalityTest, createKinesisVideoClientSync_AuthIntegration)
{
    mClientSyncMode = TRUE;
    authIntegrationTest(TRUE);
}

TEST_F(ClientApiFunctionalityTest, createClientCreateStream_Iterate)
{
    mClientSyncMode = TRUE;
    mSubmitServiceCallResultMode = STOP_AT_PUT_STREAM;
    STREAM_HANDLE streams[10];

    // Free the initial client that was created by the setup of the test
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));

    for (UINT32 iterate = 0; iterate < 100; iterate++) {
        CreateClient();
        // Create a few streams, delete stream
        for (UINT32 i = 0; i < ARRAY_SIZE(streams); i++) {
            EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &mStreamHandle));
            EXPECT_TRUE(IS_VALID_STREAM_HANDLE(mStreamHandle));

            // delete the stream
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&mStreamHandle));
        }

        // Create a few streams and don't delete immediately
        for (UINT32 i = 0; i < ARRAY_SIZE(streams); i++) {
            SPRINTF(mStreamInfo.name, "TestStream_%d", i);
            EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &mStreamHandle));
            EXPECT_TRUE(IS_VALID_STREAM_HANDLE(mStreamHandle));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));

        // Set the stream as invalid
        mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    }

    // Same with free streams
    for (UINT32 iterate = 0; iterate < 100; iterate++) {
        CreateClient();

        // Create a few streams and don't delete immediately
        for (UINT32 i = 0; i < ARRAY_SIZE(streams); i++) {
            SPRINTF(mStreamInfo.name, "TestStream_%d", i);
            EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(mClientHandle, &mStreamInfo, &streams[i]));
            EXPECT_TRUE(IS_VALID_STREAM_HANDLE(streams[i]));
        }

        for (UINT32 i = 0; i < ARRAY_SIZE(streams); i++) {
            EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streams[ARRAY_SIZE(streams) - i - 1]));
        }

        EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&mClientHandle));

        // Set the stream as invalid
        mStreamHandle = INVALID_STREAM_HANDLE_VALUE;
    }
}
