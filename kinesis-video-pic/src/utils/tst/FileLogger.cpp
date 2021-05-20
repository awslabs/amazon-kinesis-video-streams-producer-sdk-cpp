#include "UtilTestFixture.h"

// length of time and log level string in log: "2019-11-09 19:11:16 VERBOSE "
#define TIMESTRING_OFFSET               28

#ifdef _WIN32
#define TEST_TEMP_DIR_PATH                                      (PCHAR) "C:\\Windows\\Temp\\"
#define TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR                   (PCHAR) "C:\\Windows\\Temp"
#else
#define TEST_TEMP_DIR_PATH                                      (PCHAR) "/tmp/"
#define TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR                   (PCHAR) "/tmp"
#endif

class FileLoggerTest : public UtilTestBase
{
public:
    FileLoggerTest() :  UtilTestBase() {}

    void SetUp() {
        UtilTestBase::SetUp();

        // Tests will fail if log level is not warn
        SET_LOGGER_LOG_LEVEL(LOG_LEVEL_WARN);
    }
};

TEST_F(FileLoggerTest, basicFileLoggerUsage)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;
    UINT64 currentFileIndex = 0;
    CHAR fileIndexBuffer[256];
    UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.0");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.1");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.2");

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     5,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
    logFunc(LOG_LEVEL_VERBOSE, NULL, (PCHAR) "%s", logMessage);
    // low log level logs have no effect
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(FALSE, fileFound);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    // log should not have been flushed because we havent fill out string buffer
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(FALSE, fileFound);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), &fileFound));
    EXPECT_EQ(FALSE, fileFound); // index file should not be there

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    // log should have been flushed because the new message overflows string buffer
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    // second log is still in buffer. Thus this log will cause the second log to be flushed.
    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    // log should have been flushed
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.1"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), &fileFound));
    EXPECT_EQ(TRUE, fileFound); // index file should be there

    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), TRUE, NULL, &fileIndexBufferSize));
    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
    fileIndexBuffer[fileIndexBufferSize] = '\0';
    STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
    EXPECT_EQ(2, currentFileIndex); // index should be 2 since we flushed twice.

    RELEASE_FILE_LOGGER();

    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.2"), &fileFound));
    EXPECT_EQ(TRUE, fileFound); // remaining log get flushed when callbacks are freed.

    MEMFREE(logMessage);
}

TEST_F(FileLoggerTest, checkFileRotation)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2, i = 0, logIterationCount = 12, logFileCount = 0, maxLogFileCount = 5;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;
    CHAR filePath[MAX_PATH_LEN];

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");
    for(; i < logIterationCount; ++i) {
        SPRINTF(filePath, TEST_TEMP_DIR_PATH "kvsFileLog.%u", i);
        FREMOVE(filePath);
    }

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     maxLogFileCount,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    // should create 12 files if limit allows
    for(i = 0; i < logIterationCount; ++i) {
        logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    }

    RELEASE_FILE_LOGGER();

    for(i = 0; i < logIterationCount; ++i) {
        SPRINTF(filePath, TEST_TEMP_DIR_PATH "kvsFileLog.%u", i);
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

TEST_F(FileLoggerTest, fileLoggerPicksUpFromLastFileIndex)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2, fileIndexStrSize = 256;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;
    UINT64 currentFileIndex = 0;
    CHAR fileIndexBuffer[256];
    UINT64 fileIndexBufferSize = ARRAY_SIZE(fileIndexBuffer);

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");

    ULLTOSTR(128, fileIndexBuffer, ARRAY_SIZE(fileIndexBuffer), 10, &fileIndexStrSize);
    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), TRUE, FALSE, (PBYTE) fileIndexBuffer, STRLEN(fileIndexBuffer) * SIZEOF(CHAR)));

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     5,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    // Cause a log flush.
    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

    // check that log file name follows last index
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.128"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), &fileFound));
    EXPECT_EQ(TRUE, fileFound); // index file should be there

    RELEASE_FILE_LOGGER();

    // remaining log get flushed
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.129"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), TRUE, NULL, &fileIndexBufferSize));
    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLogIndex"), TRUE, (PBYTE) fileIndexBuffer, &fileIndexBufferSize));
    fileIndexBuffer[fileIndexBufferSize] = '\0';
    STRTOUI64(fileIndexBuffer, NULL, 10, &currentFileIndex);
    EXPECT_EQ(130, currentFileIndex); // index should be incremented 3 times

    MEMFREE(logMessage);
}

TEST_F(FileLoggerTest, logMessageLongerThanStringBuffer)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE + 100, i;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    PCHAR fileBuffer = (PCHAR) MEMALLOC(logMessageSize + 1);
    UINT64 fileBufferLen = logMessageSize + 1;
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.0");

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';
    // everthing starting from i should be truncated. So there should be no b in the log file.
    // minus - 1 because vsnprintf prints at most MIN_FILE_LOGGER_STRING_BUFFER_SIZE - 1 chars.
    for (i = MIN_FILE_LOGGER_STRING_BUFFER_SIZE - TIMESTRING_OFFSET - 1; i < logMessageSize; ++i) {
        logMessage[i] = 'b';
    }

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     5,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

    RELEASE_FILE_LOGGER();

    // no log should be produced as the only log message was dropped because it was too long.
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), TRUE, NULL, &fileBufferLen));
    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
    fileBuffer[fileBufferLen] = '\0';

    EXPECT_EQ('a', fileBuffer[STRLEN(fileBuffer) - 1]); // there should be no b in the log.

    MEMFREE(logMessage);
    MEMFREE(fileBuffer);
}

TEST_F(FileLoggerTest, oldLogFileOverrittenByFlushTriggerredByNewLog)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    PCHAR fileBuffer = (PCHAR) MEMALLOC(MIN_FILE_LOGGER_STRING_BUFFER_SIZE);
    UINT64 fileBufferLen = MIN_FILE_LOGGER_STRING_BUFFER_SIZE;
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.0");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.1");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.2");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.3");

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     3,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.1"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    MEMSET(logMessage, 'b', logMessageSize);
    logMessage[logMessageSize] = '\0';
    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.2"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

    // kvsFileLog.0 is deleted
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(FALSE, fileFound);
    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.3"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.3"), TRUE, NULL, &fileBufferLen));
    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.3"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
    fileBuffer[fileBufferLen] = '\0';
    // skip over the timestamp string
    // use STRNCMP to not catch the newline at the end of fileBuffer
    EXPECT_EQ(0, STRNCMP(logMessage, fileBuffer + TIMESTRING_OFFSET, STRLEN(logMessage)));

    RELEASE_FILE_LOGGER();

    MEMFREE(logMessage);
    MEMFREE(fileBuffer);
}

TEST_F(FileLoggerTest, oldLogFileOverrittenByFlushTriggerredByFreeCallbacks)
{
    UINT32 logMessageSize = MIN_FILE_LOGGER_STRING_BUFFER_SIZE / 2;
    PCHAR logMessage = (PCHAR) MEMALLOC(logMessageSize + 1);
    PCHAR fileBuffer = (PCHAR) MEMALLOC(MIN_FILE_LOGGER_STRING_BUFFER_SIZE);
    UINT64 fileBufferLen = MIN_FILE_LOGGER_STRING_BUFFER_SIZE;
    BOOL fileFound = FALSE;
    logPrintFunc logFunc;

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    // make sure the files dont exist
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLogIndex");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.0");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.1");
    FREMOVE(TEST_TEMP_DIR_PATH "kvsFileLog.2");

    MEMSET(logMessage, 'a', logMessageSize);
    logMessage[logMessageSize] = '\0';

    createFileLogger(MIN_FILE_LOGGER_STRING_BUFFER_SIZE,
                     1,
                     (PCHAR) TEST_TEMP_DIR_PATH_NO_ENDING_SEPARTOR,
                     FALSE, TRUE, &logFunc);

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

    MEMSET(logMessage, 'b', logMessageSize);
    logMessage[logMessageSize] = '\0';

    logFunc(LOG_LEVEL_ERROR, NULL, (PCHAR) "%s", logMessage);

    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    RELEASE_FILE_LOGGER();

    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.0"), &fileFound));
    EXPECT_EQ(FALSE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, fileExists((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.1"), &fileFound));
    EXPECT_EQ(TRUE, fileFound);

    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.1"), TRUE, NULL, &fileBufferLen));
    EXPECT_EQ(STATUS_SUCCESS, readFile((PCHAR) (TEST_TEMP_DIR_PATH "kvsFileLog.1"), TRUE, (PBYTE) fileBuffer, &fileBufferLen));
    fileBuffer[fileBufferLen] = '\0';

    // skip over the timestamp string
    // use STRNCMP to not catch the newline at the end of fileBuffer
    EXPECT_EQ(0, STRNCMP(logMessage, fileBuffer + TIMESTRING_OFFSET, STRLEN(logMessage)));

    MEMFREE(logMessage);
    MEMFREE(fileBuffer);
}
