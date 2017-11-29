#include "ClientTestFixture.h"

class ClientApiFunctionalityTest : public ClientTestBase {
};

TEST_F(ClientApiFunctionalityTest, createKinesisVideoClient_AuthIntegration)
{
    CLIENT_HANDLE clientHandle;
    PKinesisVideoClient pKinesisVideoClient;

    // Validate the client-level callbacks - starting from default base class object
    EXPECT_EQ(1, mGetSecurityTokenFuncCount);
    EXPECT_EQ(0, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(0, mGetDeviceFingerprintFuncCount);

    // Try the certificate integration first
    mClientCallbacks.getSecurityTokenFn = NULL;
    mClientCallbacks.getDeviceFingerprintFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, STRCMP((PCHAR) pKinesisVideoClient->certAuthInfo.data, TEST_CERTIFICATE_BITS));
    EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, mGetSecurityTokenFuncCount);
    EXPECT_EQ(1, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(0, mGetDeviceFingerprintFuncCount);
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the provisioning
    mClientCallbacks.getDeviceCertificateFn = NULL;
    mClientCallbacks.getSecurityTokenFn = NULL;
    // Currently provisioning is unsupported so it should fail
    EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
              createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, STRCMP(pKinesisVideoClient->deviceFingerprint, TEST_DEVICE_FINGERPRINT));
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, mGetSecurityTokenFuncCount);
    EXPECT_EQ(1, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mGetDeviceFingerprintFuncCount); // MAX_RETRY_COUNT + 1 times
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try with all being null
    mClientCallbacks.getDeviceCertificateFn = NULL;
    mClientCallbacks.getSecurityTokenFn = NULL;
    mClientCallbacks.getDeviceFingerprintFn = NULL;
    // Currently provisioning is unsupported so it should fail
    EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
              createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    // EXPECT_EQ(0, pKinesisVideoClient->certAuthInfo.data[0]);
    EXPECT_EQ(MAX_DEVICE_FINGERPRINT_LENGTH, STRLEN(pKinesisVideoClient->deviceFingerprint));
    // EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(1, mGetSecurityTokenFuncCount);
    EXPECT_EQ(1, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mGetDeviceFingerprintFuncCount); // MAX_RETRY_COUNT + 1
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the security token integration and empty data
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(2, mGetSecurityTokenFuncCount);
    EXPECT_EQ(2, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mGetDeviceFingerprintFuncCount); // stays the same count
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try the empty token to call the cert
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    EXPECT_NE(0, pKinesisVideoClient->certAuthInfo.data[0]);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    EXPECT_EQ(TEST_AUTH_EXPIRATION, pKinesisVideoClient->certAuthInfo.expiration);
    EXPECT_EQ('\0', pKinesisVideoClient->deviceFingerprint[0]);
    // Extra one as the base test has already called the function once
    EXPECT_EQ(3, mGetSecurityTokenFuncCount);
    EXPECT_EQ(3, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mGetDeviceFingerprintFuncCount);
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));

    // Try with all returning empty
    mClientCallbacks.getDeviceCertificateFn = getEmptyDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getEmptySecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getEmptyDeviceFingerprintFunc;
    // Currently provisioning is unsupported so it should fail
    EXPECT_EQ(STATUS_CLIENT_PROVISION_CALL_FAILED,
              createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    pKinesisVideoClient = FROM_CLIENT_HANDLE(clientHandle);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.data[0]);
    EXPECT_EQ(0, pKinesisVideoClient->tokenAuthInfo.expiration);
    EXPECT_EQ(MAX_DEVICE_FINGERPRINT_LENGTH, STRLEN(pKinesisVideoClient->deviceFingerprint));
    // Extra one as the base test has already called the function once
    EXPECT_EQ(4, mGetSecurityTokenFuncCount);
    EXPECT_EQ(4, mGetDeviceCertificateFuncCount);
    EXPECT_EQ(12, mGetDeviceFingerprintFuncCount);
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    EXPECT_TRUE(STATUS_SUCCEEDED(freeKinesisVideoClient(&clientHandle)));
}
