#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class PlatformCallbackProviderApiTest : public ProducerClientTestBase {
    };

    TEST_F(PlatformCallbackProviderApiTest, SetPlatformCallbackProvider_Returns_Valid)
    {
        PClientCallbacks pClientCallbacks = NULL;

        EXPECT_EQ(STATUS_NULL_ARG, setDefaultPlatformCallbacks((PCallbacksProvider) pClientCallbacks));

        PlatformCallbacks platformCallbacks = {
                PLATFORM_CALLBACKS_CURRENT_VERSION,
                0,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
        };

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

        setPlatformCallbacks(pClientCallbacks, &platformCallbacks);

        EXPECT_TRUE(broadcastConditionVariableAggregate != pClientCallbacks->broadcastConditionVariableFn);

        EXPECT_TRUE(createMutexAggregate != pClientCallbacks->createMutexFn);
        EXPECT_TRUE(createConditionVariableAggregate != pClientCallbacks->createConditionVariableFn);
        EXPECT_TRUE(freeMutexAggregate != pClientCallbacks->freeMutexFn);
        EXPECT_TRUE(freeConditionVariableAggregate != pClientCallbacks->freeConditionVariableFn);
        EXPECT_TRUE(getCurrentTimeAggregate != pClientCallbacks->getCurrentTimeFn);
        EXPECT_TRUE(getRandomNumberAggregate != pClientCallbacks->getRandomNumberFn);
        EXPECT_TRUE(lockMutexAggregate != pClientCallbacks->lockMutexFn);
        EXPECT_TRUE(signalConditionVariableAggregate != pClientCallbacks->signalConditionVariableFn);
        EXPECT_TRUE(unlockMutexAggregate != pClientCallbacks->unlockMutexFn);
        EXPECT_TRUE(unlockMutexAggregate != pClientCallbacks->unlockMutexFn);
        EXPECT_TRUE(waitConditionVariableAggregate != pClientCallbacks->waitConditionVariableFn);

        //non-null callback definition should return aggregated platform callback
        platformCallbacks.getCurrentTimeFn = (UINT64 (*)(UINT64)) 2;

        setPlatformCallbacks(pClientCallbacks, &platformCallbacks);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        EXPECT_EQ(NULL, pClientCallbacks);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        EXPECT_EQ(STATUS_NULL_ARG, freeCallbacksProvider(NULL));

    }

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;


