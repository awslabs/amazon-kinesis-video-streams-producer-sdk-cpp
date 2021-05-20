#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class CallbacksProviderPublicApiTest : public ProducerClientTestBase {
};

TEST_F(CallbacksProviderPublicApiTest, createDefaultCallbacksProviderWithIotCertificate)
{
    PClientCallbacks pClientCallbacks;

    EXPECT_EQ(STATUS_DIRECTORY_ENTRY_STAT_ERROR, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_DIRECTORY_ENTRY_STAT_ERROR, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            NULL));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            NULL,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            NULL,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            NULL,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_IOT_FAILED, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            NULL,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_CA_CERT_PATH,
            NULL,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            NULL,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_DIRECTORY_ENTRY_STAT_ERROR, createDefaultCallbacksProviderWithIotCertificate(
            EMPTY_STRING,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_DIRECTORY_ENTRY_STAT_ERROR, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            EMPTY_STRING,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_DIRECTORY_ENTRY_STAT_ERROR, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            EMPTY_STRING,
            TEST_CA_CERT_PATH,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_IOT_FAILED, createDefaultCallbacksProviderWithIotCertificate(
            TEST_IOT_ENDPOINT,
            TEST_IOT_CERT_PATH,
            TEST_IOT_CERT_PRIVATE_KEY_PATH,
            EMPTY_STRING,
            TEST_IOT_ROLE_ALIAS,
            TEST_IOT_THING_NAME,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            NULL,
            &pClientCallbacks));
}

TEST_F(CallbacksProviderPublicApiTest, createDefaultCallbacksProviderWithAwsCredentials)
{
    PClientCallbacks pClientCallbacks = NULL;
    CHAR authStr[MAX_AUTH_LEN + 2];
    
    MEMSET(authStr, 'a', ARRAY_SIZE(authStr));
    authStr[MAX_AUTH_LEN + 1] = '\0';

    // Positive case permutations

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    // Idempotent call
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            NULL,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            EMPTY_STRING,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            NULL,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            NULL,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            NULL,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            NULL,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            EMPTY_STRING,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            EMPTY_STRING,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            0,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    // Negative case permutations

    EXPECT_EQ(STATUS_INVALID_ARG, createDefaultCallbacksProviderWithAwsCredentials(
            NULL,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_INVALID_ARG, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            NULL,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createDefaultCallbacksProviderWithAwsCredentials(
            authStr,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            authStr,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            authStr,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithAwsCredentials(
            TEST_ACCESS_KEY,
            TEST_SECRET_KEY,
            TEST_SESSION_TOKEN,
            MAX_UINT64,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            NULL));

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithAwsCredentials(
            NULL,
            NULL,
            NULL,
            MAX_UINT64,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL));
}

TEST_F(CallbacksProviderPublicApiTest, createDefaultCallbacksProviderWithFileAuth)
{
    PClientCallbacks pClientCallbacks = NULL;
    CHAR filePath[MAX_PATH_LEN + 2];
    CHAR fileContent[MAX_CREDENTIAL_FILE_LEN];

    MEMSET(filePath, 'a', ARRAY_SIZE(filePath));
    filePath[MAX_PATH_LEN + 1] = '\0';

    EXPECT_EQ(STATUS_FILE_CREDENTIAL_PROVIDER_OPEN_FILE_FAILED, createDefaultCallbacksProviderWithFileAuth(
            TEST_AUTH_FILE_PATH,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithFileAuth(
            TEST_AUTH_FILE_PATH,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            NULL));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_INVALID_ARG, createDefaultCallbacksProviderWithFileAuth(
            filePath,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    // Try max file length
    STRCPY(filePath, "credsFile");
    MEMSET(fileContent, 'a', ARRAY_SIZE(fileContent));
    ASSERT_EQ(STATUS_SUCCESS, writeFile(filePath, FALSE, FALSE, (PBYTE) fileContent, ARRAY_SIZE(fileContent)));

    EXPECT_EQ(STATUS_FILE_CREDENTIAL_PROVIDER_INVALID_FILE_LENGTH, createDefaultCallbacksProviderWithFileAuth(
            filePath,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    // Try bad content creds file
    STRCPY(filePath, "credsFile");
    STRCPY(fileContent, "Bad creds file");
    ASSERT_EQ(STATUS_SUCCESS, writeFile(filePath, FALSE, FALSE, (PBYTE) fileContent, STRLEN(fileContent)));

    EXPECT_EQ(STATUS_FILE_CREDENTIAL_PROVIDER_INVALID_FILE_FORMAT, createDefaultCallbacksProviderWithFileAuth(
            filePath,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);
}

TEST_F(CallbacksProviderPublicApiTest, createDefaultCallbacksProviderWithAuthCallbacks)
{
    PClientCallbacks pClientCallbacks = NULL;
    AuthCallbacks authCallbacks;
    PAuthCallbacks pAuthCallbacks = &authCallbacks;
    PVOID pStored;

    // Set some of the members of the structure
    MEMSET(pAuthCallbacks, 0x00, SIZEOF(AuthCallbacks));
    authCallbacks.version = AUTH_CALLBACKS_CURRENT_VERSION;
    authCallbacks.customData = 123;
    authCallbacks.getSecurityTokenFn = (GetSecurityTokenFunc) 1;
    authCallbacks.getDeviceCertificateFn = (GetDeviceCertificateFunc) 2;
    authCallbacks.deviceCertToTokenFn = (DeviceCertToTokenFunc) 3;
    authCallbacks.getDeviceFingerprintFn = (GetDeviceFingerprintFunc) 4;
    authCallbacks.getStreamingTokenFn = (GetStreamingTokenFunc) 5;

    // Need to have a proper function for this
    authCallbacks.freeAuthCallbacksFn = [](PUINT64 ptr) {
        UNUSED_PARAM(ptr);
        return STATUS_SUCCESS;
    };

    // Positive cases

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    pStored = (PVOID) pAuthCallbacks->getDeviceCertificateFn;
    pAuthCallbacks->getDeviceCertificateFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->getDeviceCertificateFn = (GetDeviceCertificateFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->getStreamingTokenFn;
    pAuthCallbacks->getStreamingTokenFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->getStreamingTokenFn = (GetStreamingTokenFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->getDeviceFingerprintFn;
    pAuthCallbacks->getDeviceFingerprintFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->getDeviceFingerprintFn = (GetDeviceFingerprintFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->deviceCertToTokenFn;
    pAuthCallbacks->deviceCertToTokenFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->deviceCertToTokenFn = (DeviceCertToTokenFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->getSecurityTokenFn;
    pAuthCallbacks->getSecurityTokenFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->getSecurityTokenFn = (GetSecurityTokenFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->freeAuthCallbacksFn;
    pAuthCallbacks->freeAuthCallbacksFn = NULL;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->freeAuthCallbacksFn = (FreeAuthCallbacksFunc) pStored;

    pStored = (PVOID) pAuthCallbacks->freeAuthCallbacksFn;
    pAuthCallbacks->freeAuthCallbacksFn = [] (PUINT64 ptr) -> STATUS {
        UNUSED_PARAM(ptr);
        return STATUS_INVALID_ARG;
    };
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    pAuthCallbacks->freeAuthCallbacksFn = (FreeAuthCallbacksFunc) pStored;

    // Negative cases

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithAuthCallbacks(
            NULL,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            NULL));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_NULL_ARG, createDefaultCallbacksProviderWithAuthCallbacks(
            NULL,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            NULL));
    EXPECT_EQ(NULL, pClientCallbacks);

    pAuthCallbacks->version = AUTH_CALLBACKS_CURRENT_VERSION + 1;
    EXPECT_EQ(STATUS_INVALID_AUTH_CALLBACKS_VERSION, createDefaultCallbacksProviderWithAuthCallbacks(
            pAuthCallbacks,
            (PCHAR) DEFAULT_AWS_REGION,
            TEST_CA_CERT_PATH,
            TEST_USER_AGENT_POSTFIX,
            TEST_USER_AGENT,
            &pClientCallbacks));
    pAuthCallbacks->version = AUTH_CALLBACKS_CURRENT_VERSION;



}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
