#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class AwsCredentialsTest : public ProducerClientTestBase {
};

TEST_F(AwsCredentialsTest, createAwsCredentials)
{
    PAwsCredentials pAwsCredentials = NULL;
    CHAR authStr[MAX_AUTH_LEN + 2];
    
    MEMSET(authStr, 'a', ARRAY_SIZE(authStr));
    authStr[MAX_AUTH_LEN + 1] = '\0';

    // Positive case permutations

    EXPECT_EQ(STATUS_SUCCESS, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_GE(MAX_AUTH_LEN, pAwsCredentials->size);
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));

    EXPECT_EQ(STATUS_SUCCESS, createAwsCredentials(
            TEST_ACCESS_KEY,
            STRLEN(TEST_ACCESS_KEY),
            TEST_SECRET_KEY,
            STRLEN(TEST_SECRET_KEY),
            TEST_SESSION_TOKEN,
            STRLEN(TEST_SESSION_TOKEN),
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));

    EXPECT_EQ(STATUS_SUCCESS, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            0,
            &pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));

    EXPECT_EQ(STATUS_SUCCESS, createAwsCredentials(
            EMPTY_STRING,
            0,
            EMPTY_STRING,
            0,
            EMPTY_STRING,
            0,
            0,
            &pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));

    // Negative cases

    EXPECT_EQ(STATUS_INVALID_ARG, createAwsCredentials(
            NULL,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_INVALID_ARG, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            NULL,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createAwsCredentials(
            authStr,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            authStr,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            TEST_SECRET_KEY,
            0,
            authStr,
            0,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_INVALID_AUTH_LEN, createAwsCredentials(
            TEST_ACCESS_KEY,
            MAX_AUTH_LEN / 3,
            TEST_SECRET_KEY,
            MAX_AUTH_LEN / 3,
            TEST_SESSION_TOKEN,
            MAX_AUTH_LEN / 3,
            MAX_UINT64,
            &pAwsCredentials));
    EXPECT_EQ(NULL, pAwsCredentials);

    EXPECT_EQ(STATUS_NULL_ARG, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            NULL));
}

TEST_F(AwsCredentialsTest, freeAwsCredentials)
{
    EXPECT_EQ(STATUS_NULL_ARG, freeAwsCredentials(NULL));
}

TEST_F(AwsCredentialsTest, deserializeAwsCredentials)
{
    BYTE tempBuff[MAX_AUTH_LEN + 1000];
    PAwsCredentials pAwsCredentials = NULL, pDeserialized;
    PCHAR pStored;

    EXPECT_EQ(STATUS_NULL_ARG, deserializeAwsCredentials(NULL));

    // Create valid credentials, roundtrip.
    EXPECT_EQ(STATUS_SUCCESS, createAwsCredentials(
            TEST_ACCESS_KEY,
            0,
            TEST_SECRET_KEY,
            0,
            TEST_SESSION_TOKEN,
            0,
            MAX_UINT64,
            &pAwsCredentials));

    // Copy forward the bits
    MEMCPY(tempBuff, pAwsCredentials, pAwsCredentials->size);

    // Free the actual allocation
    EXPECT_EQ(STATUS_SUCCESS, freeAwsCredentials(&pAwsCredentials));

    // De-serialize the copy
    EXPECT_EQ(STATUS_SUCCESS, deserializeAwsCredentials(tempBuff));

    // Validate we have good data
    pDeserialized = (PAwsCredentials) tempBuff;
    EXPECT_EQ(0, STRCMP(TEST_ACCESS_KEY, pDeserialized->accessKeyId));
    EXPECT_EQ(0, STRCMP(TEST_SECRET_KEY, pDeserialized->secretKey));
    EXPECT_EQ(0, STRCMP(TEST_SESSION_TOKEN, pDeserialized->sessionToken));
    EXPECT_EQ(MAX_UINT64, pDeserialized->expiration);

    // Validate the negative cases with de-serialization
    pStored = pDeserialized->accessKeyId;
    pDeserialized->accessKeyId = NULL;
    EXPECT_EQ(STATUS_INVALID_ARG, deserializeAwsCredentials(tempBuff));
    pDeserialized->accessKeyId = pStored;

    pStored = pDeserialized->secretKey;
    pDeserialized->secretKey = NULL;
    EXPECT_EQ(STATUS_INVALID_ARG, deserializeAwsCredentials(tempBuff));
    pDeserialized->secretKey = pStored;

    pStored = pDeserialized->sessionToken;
    pDeserialized->sessionToken = NULL;
    pDeserialized->sessionTokenLen = 1;
    EXPECT_EQ(STATUS_INVALID_ARG, deserializeAwsCredentials(tempBuff));
    pDeserialized->sessionToken = pStored;
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
