#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class CallbacksProviderApiTest : public ProducerClientTestBase {
};

TEST_F(CallbacksProviderApiTest, createDefaultCallbacksProvider_variations)
{
    PClientCallbacks pClientCallbacks = NULL;
    EXPECT_NE(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             NULL));
    EXPECT_NE(STATUS_SUCCESS, freeCallbacksProvider(NULL));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_NE(STATUS_SUCCESS, createDefaultCallbacksProvider(MAX_CALLBACK_CHAIN_COUNT + 1,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE(STATUS_SUCCESS, freeCallbacksProvider(NULL));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_NE(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             MAX_ENDPOINT_CACHE_UPDATE_PERIOD + 1,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE(STATUS_SUCCESS, freeCallbacksProvider(NULL));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_NE(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_ENDPOINT_ONLY,
                                                             MAX_ENDPOINT_CACHE_UPDATE_PERIOD + 1,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE(STATUS_SUCCESS, freeCallbacksProvider(NULL));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_ENDPOINT_ONLY,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             FALSE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             NULL,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             NULL,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             TEST_USER_AGENT,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             TEST_DEFAULT_REGION,
                                                             TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath,
                                                             NULL,
                                                             NULL,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             API_CALL_CACHE_TYPE_NONE,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                             TEST_ACCESS_KEY,
                                                             TEST_SECRET_KEY,
                                                             TEST_SESSION_TOKEN,
                                                             TEST_STREAMING_TOKEN_DURATION,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             EMPTY_STRING,
                                                             API_CALL_CACHE_TYPE_ENDPOINT_ONLY,
                                                             TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE,
                                                             &pClientCallbacks));
    EXPECT_NE((PClientCallbacks) NULL, pClientCallbacks);
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(NULL, pClientCallbacks);
}

TEST_F(CallbacksProviderApiTest, createStreamVerifyCallbackChainIntegration)
{
    PDeviceInfo pDeviceInfo;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    PClientCallbacks pClientCallbacks;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    UINT64 currentTime = GETTIME();

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = this->loggerLogLevel;
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;

    EXPECT_EQ(STATUS_INVALID_ARG, createAbstractDefaultCallbacksProvider(MAX_CALLBACK_CHAIN_COUNT + 1,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         mRegion,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         mCaCertPath,
                                                                         NULL,
                                                                         NULL,
                                                                         &pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     NULL,
                                                                     &pClientCallbacks));

    mApiCallbacks.version = API_CALLBACKS_CURRENT_VERSION;
    mApiCallbacks.customData = (UINT64) this;
    mApiCallbacks.freeApiCallbacksFn = testFreeApiCallbackFunc;
    mApiCallbacks.putStreamFn = testPutStreamFunc;
    mApiCallbacks.tagResourceFn = testTagResourceFunc;
    mApiCallbacks.getStreamingEndpointFn = testGetStreamingEndpointFunc;
    mApiCallbacks.describeStreamFn = testDescribeStreamFunc;
    mApiCallbacks.createStreamFn = testCreateStreamFunc;
    mApiCallbacks.createDeviceFn = testCreateDeviceFunc;
    addApiCallbacks(pClientCallbacks, &mApiCallbacks);

    // Add second callback on API callback chain
    mApiCallbacks.describeStreamFn = testDescribeStreamSecondFunc;
    mApiCallbacks.freeApiCallbacksFn = nullptr;
    EXPECT_EQ(STATUS_SUCCESS, addApiCallbacks(pClientCallbacks, &mApiCallbacks));

    // Add third callback on API callback chain
    mApiCallbacks.describeStreamFn = testDescribeStreamThirdFunc;
    mApiCallbacks.freeApiCallbacksFn = nullptr;
    EXPECT_EQ(STATUS_SUCCESS, addApiCallbacks(pClientCallbacks, &mApiCallbacks));

    UINT64 expiration = currentTime + TEST_STREAMING_TOKEN_DURATION;
    PAuthCallbacks pAuthCallbacks;
    EXPECT_EQ(STATUS_SUCCESS, createRotatingStaticAuthCallbacks(pClientCallbacks,
                                                                mAccessKey,
                                                                mSecretKey,
                                                                mSessionToken,
                                                                expiration,
                                                                TEST_STREAMING_TOKEN_DURATION,
                                                                &pAuthCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));
    EXPECT_EQ(4, ((PCallbacksProvider) pClientCallbacks)->apiCallbacksCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_EQ(1, mDescribeStreamSecondFnCount);
    EXPECT_EQ(1, mDescribeStreamThirdFnCount);

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(CallbacksProviderApiTest, createStreamVerifyCallbackChainStopsOnStopStatus)
{
    PDeviceInfo pDeviceInfo;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    PClientCallbacks pClientCallbacks;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    UINT64 currentTime = GETTIME();

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = LOG_LEVEL_DEBUG;
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;

    EXPECT_EQ(STATUS_INVALID_ARG, createAbstractDefaultCallbacksProvider(MAX_CALLBACK_CHAIN_COUNT + 1,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         mRegion,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         mCaCertPath,
                                                                         NULL,
                                                                         NULL,
                                                                         &pClientCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     NULL,
                                                                     &pClientCallbacks));

    mApiCallbacks.version = API_CALLBACKS_CURRENT_VERSION;
    mApiCallbacks.customData = (UINT64) this;
    mApiCallbacks.freeApiCallbacksFn = testFreeApiCallbackFunc;
    mApiCallbacks.putStreamFn = testPutStreamFunc;
    mApiCallbacks.tagResourceFn = testTagResourceFunc;
    mApiCallbacks.getStreamingEndpointFn = testGetStreamingEndpointFunc;
    mApiCallbacks.describeStreamFn = testDescribeStreamFunc;
    mApiCallbacks.createStreamFn = testCreateStreamFunc;
    mApiCallbacks.createDeviceFn = testCreateDeviceFunc;
    addApiCallbacks(pClientCallbacks, &mApiCallbacks);

    // Add stop callback on API callback chain
    mApiCallbacks.describeStreamFn = testDescribeStreamStopChainFunc;
    mApiCallbacks.freeApiCallbacksFn = nullptr;
    EXPECT_EQ(STATUS_SUCCESS, addApiCallbacks(pClientCallbacks, &mApiCallbacks));

    // Add another callback on API callback chain following the stop callback, this should not be called
    mApiCallbacks.describeStreamFn = testDescribeStreamSecondFunc;
    mApiCallbacks.freeApiCallbacksFn = nullptr;
    EXPECT_EQ(STATUS_SUCCESS, addApiCallbacks(pClientCallbacks, &mApiCallbacks));

    UINT64 expiration = currentTime + TEST_STREAMING_TOKEN_DURATION;
    PAuthCallbacks pAuthCallbacks;
    EXPECT_EQ(STATUS_SUCCESS, createRotatingStaticAuthCallbacks(pClientCallbacks,
                                                                mAccessKey,
                                                                mSecretKey,
                                                                mSessionToken,
                                                                expiration,
                                                                TEST_STREAMING_TOKEN_DURATION,
                                                                &pAuthCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));
    EXPECT_EQ(4, ((PCallbacksProvider) pClientCallbacks)->apiCallbacksCount);
    EXPECT_EQ(1, mDescribeStreamFnCount);
    EXPECT_EQ(1, mDescribeStreamStopChainFnCount);
    EXPECT_EQ(0, mDescribeStreamSecondFnCount);

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
