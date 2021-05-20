#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerApiTest : public ProducerClientTestBase {
};

TEST_F(ProducerApiTest, fileLoggerCallbackApiTest)
{
    PClientCallbacks pClientCallbacks = NULL;
    PCHAR testLogDir = ((PCHAR) "/tmp");
    CHAR longPath[MAX_PATH_LEN + 2];

    MEMSET(longPath, 0x01, MAX_PATH_LEN + 2);
    longPath[MAX_PATH_LEN + 1] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     TEST_DEFAULT_REGION,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     EMPTY_STRING,
                                                                     NULL,
                                                                     TEST_USER_AGENT,
                                                                     &pClientCallbacks));

    EXPECT_EQ(STATUS_NULL_ARG, addFileLoggerPlatformCallbacksProvider(NULL, 10 * 1024, 10, testLogDir, FALSE));
    EXPECT_EQ(STATUS_INVALID_ARG, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE - 1, 10, testLogDir, FALSE));
    EXPECT_EQ(STATUS_INVALID_ARG, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MAX_FILE_LOGGER_STRING_BUFFER_SIZE + 1, 10, testLogDir, FALSE));
    EXPECT_EQ(STATUS_INVALID_ARG, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, MAX_FILE_LOGGER_LOG_FILE_COUNT + 1, testLogDir, FALSE));
    EXPECT_EQ(STATUS_INVALID_ARG, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 0, testLogDir, FALSE));
    EXPECT_EQ(STATUS_PATH_TOO_LONG, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 10, longPath, FALSE));

    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks)); // idempotent
}

TEST_F(ProducerApiTest, fileLoggerApiTest)
{
    PCHAR testLogDir = ((PCHAR) "/tmp");
    CHAR longPath[MAX_PATH_LEN + 2];

    MEMSET(longPath, 0x01, MAX_PATH_LEN + 2);
    longPath[MAX_PATH_LEN + 1] = '\0';

    EXPECT_EQ(STATUS_INVALID_ARG, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE - 1, 10, testLogDir, FALSE, FALSE, NULL));
    EXPECT_EQ(STATUS_INVALID_ARG, createFileLogger(MAX_FILE_LOGGER_STRING_BUFFER_SIZE + 1, 10, testLogDir, FALSE, FALSE, NULL));
    EXPECT_EQ(STATUS_INVALID_ARG, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, MAX_FILE_LOGGER_LOG_FILE_COUNT + 1, testLogDir, FALSE, FALSE, NULL));
    EXPECT_EQ(STATUS_INVALID_ARG, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 0, testLogDir, FALSE, FALSE, NULL));
    EXPECT_EQ(STATUS_PATH_TOO_LONG, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 10, longPath, FALSE, FALSE, NULL));
}

}
}
}
}