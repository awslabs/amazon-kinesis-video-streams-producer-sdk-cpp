#include "ProducerTestFixture.h"

// length of time and log level string in log: "2019-11-09 19:11:16 VERBOSE "
#define TIMESTRING_OFFSET               28

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class FileLoggerFunctionalityTest : public ProducerClientTestBase
    {
        public:
            FileLoggerFunctionalityTest() :  ProducerClientTestBase() {
                // Tests will fail if log level is not warn
                SET_LOGGER_LOG_LEVEL(LOG_LEVEL_WARN);
            }
    };

    TEST_F(FileLoggerFunctionalityTest, basicFileLoggerUsage)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        // low log level logs have no effect
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(FALSE, fileFound); // index file should not be there

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed because the new message overflows string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        // second log is still in buffer. Thus this log will cause the second log to be flushed.
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // index file should be there

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, NULL, &fileIndexBufferSize));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
        fileIndexBuffer[fileIndexBufferSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
        EXPECT_EQ(2, currentFileIndex); // index should be 2 since we flushed twice.

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2"), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, checkFileRotation)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2, i = 0, logIterationCount = 12, logFileCount = 0, maxLogFileCount = 5;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;
        CHAR filePath[MAX_PATH_LEN];

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        for(; i < logIterationCount; ++i) {
            SPRINTF(filePath, TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".%u", i);
            FREMOVE(filePath);
        }

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, maxLogFileCount, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        // should create 12 files if limit allows
        for(i = 0; i < logIterationCount; ++i) {
            logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        }

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        for(i = 0; i < logIterationCount; ++i) {
            SPRINTF(filePath, TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".%u", i);
            EXPECT_EQ(STATUS_SUCCESS, fileExists(filePath, &fileFound));
            // only log file with index from 6 to 11 should remain. The rest are rotated out.
            if (i < logIterationCount && i >= (logIterationCount - maxLogFileCount)) {
                EXPECT_EQ(TRUE, fileFound);
                logFileCount++;
            } else {
                EXPECT_EQ(FALSE, fileFound);
            }
        }

        EXPECT_EQ(maxLogFileCount, logFileCount);

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, fileLoggerPicksUpFromLastFileIndex)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2, fileIndexStrSize = 256;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);

        ULLTOSTR(128, fileIndexBuffer, ARRAY_SIZE(fileIndexBuffer), 10, &fileIndexStrSize);
        EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, FALSE, (PBYTE) fileIndexBuffer, STRLEN(fileIndexBuffer) * SIZEOF(CHAR)));

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        // Cause a log flush.
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

        // check that log file name follows last index
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".128"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // index file should be there

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        // remaining log get flushed
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".129"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, NULL, &fileIndexBufferSize));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
        fileIndexBuffer[fileIndexBufferSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
        EXPECT_EQ(130, currentFileIndex); // index should be incremented 3 times

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, logMessageLongerThanStringBuffer)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE + 100, i;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        PCHAR fileBuffer = (PCHAR) MEMALLOC(logMessageSize + 1);
        UINT64 fileBufferLen = logMessageSize + 1;
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;

        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        // everthing starting from i should be truncated. So there should be no b in the log file.
        // minus - 1 because vsnprintf prints at most MIN_FILE_LOGGER_STRING_BUFFER_SIZE - 1 chars.
        for (i = MIN_FILE_LOGGER_STRING_BUFFER_SIZE - TIMESTRING_OFFSET - 1; i < logMessageSize; ++i) {
            logMessage[i] = 'b';
        }

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        // no log should be produced as the only log message was dropped because it was too long.
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), TRUE, NULL, &fileBufferLen));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
        fileBuffer[fileBufferLen] = '\0';

        EXPECT_EQ('a', fileBuffer[STRLEN(fileBuffer) - 1]); // there should be no b in the log.

        MEMFREE(logMessage);
        MEMFREE(fileBuffer);
    }

    TEST_F(FileLoggerFunctionalityTest, oldLogFileOverrittenByFlushTriggerredByNewLog)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        PCHAR fileBuffer = (PCHAR) MEMALLOC(MIN_FILE_LOGGER_STRING_BUFFER_SIZE);
        UINT64 fileBufferLen = MIN_FILE_LOGGER_STRING_BUFFER_SIZE;
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".3");

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 3, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        MEMSET(logMessage, 'b', logMessageSize);
        logMessage[logMessageSize] = '\0';
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

        // kvsProducerLog.0 is deleted
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".3"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".3"), TRUE, NULL, &fileBufferLen));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".3"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
        fileBuffer[fileBufferLen] = '\0';
        // skip over the timestamp string
        // use STRNCMP to not catch the newline at the end of fileBuffer
        EXPECT_EQ(0, STRNCMP(logMessage, fileBuffer + TIMESTRING_OFFSET, STRLEN(logMessage)));

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));

        MEMFREE(logMessage);
        MEMFREE(fileBuffer);
    }

    TEST_F(FileLoggerFunctionalityTest, oldLogFileOverrittenByFlushTriggerredByFreeCallbacks)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        PCHAR fileBuffer = (PCHAR) MEMALLOC(MIN_FILE_LOGGER_STRING_BUFFER_SIZE);
        UINT64 fileBufferLen = MIN_FILE_LOGGER_STRING_BUFFER_SIZE;
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';
        EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                         API_CALL_CACHE_TYPE_NONE,
                                                                         TEST_CACHING_ENDPOINT_PERIOD,
                                                                         TEST_DEFAULT_REGION,
                                                                         TEST_CONTROL_PLANE_URI,
                                                                         EMPTY_STRING,
                                                                         NULL,
                                                                         TEST_USER_AGENT,
                                                                         &pClientCallbacks));

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 1, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE));
        logFunc = pClientCallbacks->logPrintFn;

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

        MEMSET(logMessage, 'b', logMessageSize);
        logMessage[logMessageSize] = '\0';

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1"), TRUE, NULL, &fileBufferLen));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
        fileBuffer[fileBufferLen] = '\0';

        // skip over the timestamp string
        // use STRNCMP to not catch the newline at the end of fileBuffer
        EXPECT_EQ(0, STRNCMP(logMessage, fileBuffer + TIMESTRING_OFFSET, STRLEN(logMessage)));

        MEMFREE(logMessage);
        MEMFREE(fileBuffer);
    }

    TEST_F(FileLoggerFunctionalityTest, basicCommonFileLoggerUsageViaCallbackNoGlobalLog)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        EXPECT_EQ(STATUS_SUCCESS, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE, FALSE, &logFunc));

        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        // low log level logs have no effect
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(FALSE, fileFound); // index file should not be there

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed because the new message overflows string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        // second log is still in buffer. Thus this log will cause the second log to be flushed.
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // index file should be there

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, NULL, &fileIndexBufferSize));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
        fileIndexBuffer[fileIndexBufferSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
        EXPECT_EQ(2, currentFileIndex); // index should be 2 since we flushed twice.

        EXPECT_EQ(STATUS_SUCCESS, freeFileLogger());

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2"), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, basicCommonFileLoggerUsageViaCallbackWithGlobalLog)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        LogPrintFunc logFunc;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        EXPECT_EQ(STATUS_SUCCESS, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE, TRUE, &logFunc));

        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
        // low log level logs have no effect
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(FALSE, fileFound); // index file should not be there

        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed because the new message overflows string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        // second log is still in buffer. Thus this log will cause the second log to be flushed.
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
        // log should have been flushed
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // index file should be there

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, NULL, &fileIndexBufferSize));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
        fileIndexBuffer[fileIndexBufferSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
        EXPECT_EQ(2, currentFileIndex); // index should be 2 since we flushed twice.

        EXPECT_EQ(STATUS_SUCCESS, freeFileLogger());

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2"), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, basicCommonFileLoggerUsageViaGlobalLog)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        // make sure the files dont exist
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        EXPECT_EQ(STATUS_SUCCESS, createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE, 5, TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR, FALSE, TRUE, NULL));

        DLOGV("%s", logMessage);
        DLOGV("%s", logMessage);
        // low log level logs have no effect
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);

        DLOGE("%s", logMessage);
        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(FALSE, fileFound); // index file should not be there

        DLOGE("%s", logMessage);
        // log should have been flushed because the new message overflows string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        // second log is still in buffer. Thus this log will cause the second log to be flushed.
        DLOGE("%s", logMessage);
        // log should have been flushed
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME".1"), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // index file should be there

        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, NULL, &fileIndexBufferSize));
        EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
        fileIndexBuffer[fileIndexBufferSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
        EXPECT_EQ(2, currentFileIndex); // index should be 2 since we flushed twice.

        EXPECT_EQ(STATUS_SUCCESS, freeFileLogger());

        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH FILE_LOGGER_LOG_FILE_NAME ".2"), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.

        MEMFREE(logMessage);
    }

    TEST_F(FileLoggerFunctionalityTest, basicCommonFileLoggerUsageViaGlobalLogSetResetMacros)
    {
        PClientCallbacks pClientCallbacks = NULL;
        UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
        PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
        BOOL fileFound = FALSE;
        UINT64 currentFileIndex = 0;
        CHAR fileIndexBuffer[256];
        UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

        MEMSET(logMessage, 'a', logMessageSize);
        logMessage[logMessageSize] = '\0';

        // make sure the files dont exist
        FREMOVE(FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME);
        FREMOVE(FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LOG_FILE_NAME ".0");
        FREMOVE(FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LOG_FILE_NAME ".1");
        FREMOVE(FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LOG_FILE_NAME ".2");

        EXPECT_EQ(STATUS_SUCCESS, SET_FILE_LOGGER());

        DLOGE("%s", logMessage);
        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(FALSE, fileFound);
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(FALSE, fileFound); // index file should not be there

        EXPECT_EQ(STATUS_SUCCESS, RESET_FILE_LOGGER());

        // log should not have been flushed because we havent fill out string buffer
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LOG_FILE_NAME ".0"), &fileFound));
        EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.
        EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (FILE_LOGGER_LOG_FILE_DIRECTORY_PATH FILE_LOGGER_LAST_INDEX_FILE_NAME), &fileFound));
        EXPECT_EQ(TRUE, fileFound);

        MEMFREE(logMessage);

        // Ensure we can still log err
        DLOGE("Printout after reset");
    }
}
}
}
}
