#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class AuthCallbackTest : public ProducerClientTestBase {
};

TEST_F(AuthCallbackTest, RotatingStaticAuthCallback_ReturnsExtendedExpiration)
{

    // check AWSCredentials is extended by rotation period for stream token function.

}

TEST_F(AuthCallbackTest, ioTExpirationParsing_Returns_Success)
{

    UINT64 iotTimeInEpoch = 1548972059;
    CHAR validFormatIotExpirationTimeStamp[] = "2019-01-31T23:00:59Z"; // expiration is current time + 1 hour
    UINT64 expirationTimestampInEpoch = 0;

    convertTimestampToEpoch(validFormatIotExpirationTimeStamp, iotTimeInEpoch, &expirationTimestampInEpoch);

    EXPECT_TRUE(iotTimeInEpoch == expirationTimestampInEpoch / HUNDREDS_OF_NANOS_IN_A_SECOND - 3600);

    iotTimeInEpoch = 1548975659;   // iot expiration same as current time

    convertTimestampToEpoch(validFormatIotExpirationTimeStamp, iotTimeInEpoch, &expirationTimestampInEpoch);

    EXPECT_TRUE(iotTimeInEpoch == expirationTimestampInEpoch / HUNDREDS_OF_NANOS_IN_A_SECOND);

    iotTimeInEpoch = 1548975660;   // iot expiration occurs in the past

    EXPECT_EQ(STATUS_IOT_EXPIRATION_OCCURS_IN_PAST,
              convertTimestampToEpoch(validFormatIotExpirationTimeStamp,
                                      iotTimeInEpoch,
                                      &expirationTimestampInEpoch));

}

TEST_F(AuthCallbackTest, invalidIoTExpirationParsing_Returns_Failure)
{

    UINT64 iotTimeInEpoch = 1548972059;
    UINT64 expirationTimestampInEpoch = 0;
    CHAR invalidIotExpirationTimeStamp[] = "2019-00-31T23:00:59Z";
    CHAR invalidFormatIotExpirationTimeStamp[] = "2019-00-31-23:00:59Z";
    CHAR emptyIotExpirationTimestamp[] = "";

    EXPECT_EQ(STATUS_IOT_EXPIRATION_PARSING_FAILED,
              convertTimestampToEpoch(invalidIotExpirationTimeStamp, iotTimeInEpoch, &expirationTimestampInEpoch));

    EXPECT_EQ(STATUS_IOT_EXPIRATION_PARSING_FAILED,
              convertTimestampToEpoch(invalidFormatIotExpirationTimeStamp,
                                      iotTimeInEpoch,
                                      &expirationTimestampInEpoch));

    EXPECT_EQ(STATUS_EMPTY_STRING,
              convertTimestampToEpoch(emptyIotExpirationTimestamp, iotTimeInEpoch, &expirationTimestampInEpoch));

}

TEST_F(AuthCallbackTest, verify_fileAuthCallback_provider_works)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    PCHAR authFilePath = NULL;
    PAuthCallbacks pAuthCallbacks = NULL;

    authFilePath = getenv(TEST_AUTH_FILE_PATH);
    if (authFilePath == NULL) {
        DLOGI("Auth file not provided, passing test");
        return;
    }

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = this->loggerLogLevel;
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     NULL,
                                                                     &pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createFileAuthCallbacks(pClientCallbacks,
                                                      authFilePath,
                                                      &pAuthCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
