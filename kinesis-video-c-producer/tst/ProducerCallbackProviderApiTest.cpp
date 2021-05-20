
#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class ProducerCallbackProviderApiTest : public ProducerClientTestBase {
    };

    TEST_F(ProducerCallbackProviderApiTest, AddProducerCallbackProvider_Returns_Valid)
    {
        PClientCallbacks pClientCallbacks = NULL;

        ProducerCallbacks producerCallbacks = {
                PRODUCER_CALLBACKS_CURRENT_VERSION,
                0,
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

        addProducerCallbacks(pClientCallbacks, &producerCallbacks);

        EXPECT_TRUE(clientReadyAggregate != pClientCallbacks->clientReadyFn);
        EXPECT_TRUE(clientShutdownAggregate != producerCallbacks.clientShutdownFn);
        EXPECT_TRUE(storageOverflowPressureAggregate != producerCallbacks.storageOverflowPressureFn);

        //non-null callback definition should return aggregated platform callback
        producerCallbacks.clientReadyFn = (UINT32 (*)(UINT64, UINT64)) 2;

        addProducerCallbacks(pClientCallbacks, &producerCallbacks);

        addProducerCallbacks(pClientCallbacks, &producerCallbacks);
        EXPECT_TRUE(4 == ((PCallbacksProvider) pClientCallbacks)->producerCallbacksCount);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        EXPECT_EQ(NULL, pClientCallbacks);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        EXPECT_EQ(STATUS_NULL_ARG, freeCallbacksProvider(NULL));

    }

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;


