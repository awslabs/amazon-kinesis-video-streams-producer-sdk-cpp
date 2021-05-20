/**
 * Kinesis Video Producer File based logger
 */
#define LOG_CLASS "FileLogger"
#include "Include_i.h"

PFileLogger gFileLogger = NULL;

STATUS flushLogToFile()
{
    STATUS retStatus = STATUS_SUCCESS;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 filePathLen = 0, fileIndexStrSize = 0;
    UINT64 fileIndexToRemove = 0;
    CHAR fileIndexBuffer[KVS_COMMON_FILE_INDEX_BUFFER_SIZE];
    UINT64 charLenToWrite = 0;

    CHK(gFileLogger != NULL, STATUS_NULL_ARG);
    CHK(gFileLogger->currentOffset != 0, retStatus);

    if (gFileLogger->currentFileIndex >= gFileLogger->maxFileCount) {
        fileIndexToRemove = gFileLogger->currentFileIndex - gFileLogger->maxFileCount;
        filePathLen = SNPRINTF(filePath, ARRAY_SIZE(filePath), "%s%s%s.%" PRIu64, gFileLogger->logFileDir, FPATHSEPARATOR_STR,
                               FILE_LOGGER_LOG_FILE_NAME, fileIndexToRemove);
        CHK(filePathLen <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
        if (0 != FREMOVE(filePath)) {
            PRINTF("failed to remove file %s\n", filePath);
        }
    }

    filePathLen = SNPRINTF(filePath, ARRAY_SIZE(filePath), "%s%s%s.%" PRIu64, gFileLogger->logFileDir, FPATHSEPARATOR_STR, FILE_LOGGER_LOG_FILE_NAME,
                           gFileLogger->currentFileIndex);
    CHK(filePathLen <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);

    // we need to set null terminator properly because flush is triggered after a vsnprintf.
    // currentOffset should never be equal to stringBufferLen since vsnprintf always leave space for null terminator.
    // just in case currentOffset is greater than stringBufferLen, then use stringBufferLen.
    charLenToWrite = MIN(gFileLogger->currentOffset, gFileLogger->stringBufferLen - 1);
    gFileLogger->stringBuffer[charLenToWrite] = '\0';
    CHK_STATUS(writeFile(filePath, TRUE, FALSE, (PBYTE) gFileLogger->stringBuffer, charLenToWrite * SIZEOF(CHAR)));
    gFileLogger->currentFileIndex++;

    ULLTOSTR(gFileLogger->currentFileIndex, fileIndexBuffer, ARRAY_SIZE(fileIndexBuffer), 10, &fileIndexStrSize);
    retStatus = writeFile(gFileLogger->indexFilePath, TRUE, FALSE, (PBYTE) fileIndexBuffer, (STRLEN(fileIndexBuffer)) * SIZEOF(CHAR));
    if (STATUS_FAILED(retStatus)) {
        PRINTF("Failed to write to index file due to error 0x%08x\n", retStatus);
        retStatus = STATUS_SUCCESS;
    }

CleanUp:

    if (gFileLogger != NULL) {
        gFileLogger->currentOffset = 0;
    }

    return retStatus;
}

VOID fileLoggerLogPrintFn(UINT32 level, PCHAR tag, PCHAR fmt, ...)
{
    CHAR logFmtString[MAX_LOG_FORMAT_LENGTH + 1];
    INT32 offset = 0;
    STATUS status = STATUS_SUCCESS;
    va_list valist;

    UNUSED_PARAM(tag);

    if (level >= GET_LOGGER_LOG_LEVEL() && gFileLogger != NULL) {
        MUTEX_LOCK(gFileLogger->lock);

        addLogMetadata(logFmtString, (UINT32) ARRAY_SIZE(logFmtString), fmt, level);

        if (gFileLogger->printLog) {
            va_start(valist, fmt);
            vprintf(logFmtString, valist);
            va_end(valist);
        }

#if defined _WIN32 || defined _WIN64

        // On mingw, vsnprintf has a bug where if the string length is greater than the buffer
        // size it would just return -1.

        va_start(valist, fmt);
        // _vscprintf give the resulting string length
        offset = _vscprintf(logFmtString, valist);
        va_end(valist);

        if (gFileLogger->currentOffset + offset >= gFileLogger->stringBufferLen) {
            status = flushLogToFile();
            if (STATUS_FAILED(status)) {
                PRINTF("flush log to file failed with 0x%08x\n", status);
            }
        }

        // even if flushLogToFile failed, currentOffset will still be reset to 0
        // _vsnprintf truncates the string if it is larger than buffer
        va_start(valist, fmt);
        offset = _vsnprintf(gFileLogger->stringBuffer + gFileLogger->currentOffset, gFileLogger->stringBufferLen - gFileLogger->currentOffset,
                            logFmtString, valist);
        va_end(valist);

        // truncation happened
        if (offset == -1) {
            PRINTF("truncating log message as it can't fit into string buffer\n");
            offset = (INT32) gFileLogger->stringBufferLen - 1;
        } else if (offset < 0) {
            // something went wrong
            PRINTF("_vsnprintf failed\n");
            offset = 0; // shouldnt cause any change to gFileLogger->currentOffset
        }
#else
        va_start(valist, fmt);
        offset = vsnprintf(gFileLogger->stringBuffer + gFileLogger->currentOffset, gFileLogger->stringBufferLen - gFileLogger->currentOffset,
                           logFmtString, valist);
        va_end(valist);

        // If vsnprintf fills the stringBuffer then flush first and then vsnprintf again into the stringBuffer.
        // This is because we dont know how long the log message is
        if (offset > 0 && gFileLogger->currentOffset + offset >= gFileLogger->stringBufferLen) {
            status = flushLogToFile();
            if (STATUS_FAILED(status)) {
                PRINTF("flush log to file failed with 0x%08x\n", status);
            }

            // even if flushLogToFile failed, currentOffset will still be reset to 0
            va_start(valist, fmt);
            offset = vsnprintf(gFileLogger->stringBuffer + gFileLogger->currentOffset, gFileLogger->stringBufferLen - gFileLogger->currentOffset,
                               logFmtString, valist);
            va_end(valist);

            // if buffer is not big enough, vsnprintf returns number of characters (excluding the terminating null byte)
            // which would have been written to the final string if enough space had been available, after writing
            // gFileLogger->stringBufferLen - 1 bytes. Here we are truncating the log if its length is longer than stringBufferLen.
            if (offset > gFileLogger->stringBufferLen) {
                PRINTF("truncating log message as it can't fit into string buffer\n");
                offset = (INT32) gFileLogger->stringBufferLen - 1;
            }
        }

        if (offset < 0) {
            // something went wrong
            PRINTF("vsnprintf failed\n");
            offset = 0; // shouldn't cause any change to gFileLogger->currentOffset
        }
#endif

        gFileLogger->currentOffset += offset;

        MUTEX_UNLOCK(gFileLogger->lock);
    }
}

STATUS createFileLogger(UINT64 maxStringBufferLen, UINT64 maxLogFileCount, PCHAR logFileDir, BOOL printLog, BOOL setGlobalLogFn,
                        logPrintFunc* pFilePrintFn)
{
    STATUS retStatus = STATUS_SUCCESS;
    CHK(gFileLogger == NULL, retStatus); // dont allocate again if already allocated
    CHK(maxStringBufferLen <= MAX_FILE_LOGGER_STRING_BUFFER_SIZE && maxStringBufferLen >= MIN_FILE_LOGGER_STRING_BUFFER_SIZE &&
            maxLogFileCount <= MAX_FILE_LOGGER_LOG_FILE_COUNT && maxLogFileCount > 0,
        STATUS_INVALID_ARG);
    CHK(STRNLEN(logFileDir, MAX_PATH_LEN + 1) <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    BOOL fileFound = FALSE;
    CHAR fileIndexBuffer[KVS_COMMON_FILE_INDEX_BUFFER_SIZE];
    UINT64 charWritten = 0, indexFileSize = KVS_COMMON_FILE_INDEX_BUFFER_SIZE;

    // allocate the struct and string buffer together
    gFileLogger = (PFileLogger) MEMALLOC(SIZEOF(FileLogger) + maxStringBufferLen * SIZEOF(CHAR));
    MEMSET(gFileLogger, 0x00, SIZEOF(FileLogger));
    // point stringBuffer to the right place
    gFileLogger->stringBuffer = (PCHAR)(gFileLogger + 1);
    gFileLogger->stringBufferLen = maxStringBufferLen;
    gFileLogger->lock = MUTEX_CREATE(FALSE);
    gFileLogger->currentOffset = 0;
    gFileLogger->maxFileCount = maxLogFileCount;
    gFileLogger->currentFileIndex = 0;
    gFileLogger->printLog = printLog;
    gFileLogger->fileLoggerLogPrintFn = fileLoggerLogPrintFn;
    STRNCPY(gFileLogger->logFileDir, logFileDir, MAX_PATH_LEN);
    gFileLogger->logFileDir[MAX_PATH_LEN] = '\0';

    charWritten = SNPRINTF(gFileLogger->indexFilePath, MAX_PATH_LEN + 1, "%s%s%s", gFileLogger->logFileDir, FPATHSEPARATOR_STR,
                           FILE_LOGGER_LAST_INDEX_FILE_NAME);
    CHK(charWritten <= MAX_PATH_LEN, STATUS_PATH_TOO_LONG);
    gFileLogger->indexFilePath[charWritten] = '\0';

    CHK_STATUS(fileExists(gFileLogger->indexFilePath, &fileFound));
    if (fileFound) {
        CHK_STATUS(readFile(gFileLogger->indexFilePath, FALSE, NULL, &indexFileSize));
        CHK(indexFileSize < KVS_COMMON_FILE_INDEX_BUFFER_SIZE, STATUS_FILE_LOGGER_INDEX_FILE_INVALID_SIZE);
        CHK_STATUS(readFile(gFileLogger->indexFilePath, FALSE, (PBYTE) fileIndexBuffer, &indexFileSize));
        fileIndexBuffer[indexFileSize] = '\0';
        STRTOUI64(fileIndexBuffer, NULL, 10, &gFileLogger->currentFileIndex);
    }

    // See if we are required to set the global log function pointer as well
    if (setGlobalLogFn) {
        // Store the original one to be reset later
        gFileLogger->storedLoggerLogPrintFn = globalCustomLogPrintFn;
        // Overwrite with the file logger
        globalCustomLogPrintFn = fileLoggerLogPrintFn;
    }

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeFileLogger();
        gFileLogger = NULL;
    } else if (pFilePrintFn != NULL) {
        *pFilePrintFn = fileLoggerLogPrintFn;
    }

    return retStatus;
}

STATUS freeFileLogger()
{
    STATUS retStatus = STATUS_SUCCESS;
    CHK(gFileLogger != NULL, retStatus);

    if (IS_VALID_MUTEX_VALUE(gFileLogger->lock)) {
        // flush out remaining log
        MUTEX_LOCK(gFileLogger->lock);
        retStatus = flushLogToFile();
        if (STATUS_FAILED(retStatus)) {
            PRINTF("flush log to file failed with 0x%08x\n", retStatus);
        }

        retStatus = STATUS_SUCCESS;
        MUTEX_UNLOCK(gFileLogger->lock);

        MUTEX_FREE(gFileLogger->lock);
    }

    // Reset the original logger functionality
    if (gFileLogger->storedLoggerLogPrintFn != NULL) {
        globalCustomLogPrintFn = gFileLogger->storedLoggerLogPrintFn;
    }

    MEMFREE(gFileLogger);
    gFileLogger = NULL;

CleanUp:

    return retStatus;
}
