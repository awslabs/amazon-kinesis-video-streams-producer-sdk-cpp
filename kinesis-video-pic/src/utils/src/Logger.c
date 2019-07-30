#include "Include_i.h"

UINT32 gLoggerLogLevel = LOG_LEVEL_WARN;

VOID updateLogFormat(PCHAR buffer, UINT32 bufferLen, PCHAR fmt)
{
    UINT32 timeStrLen = 0;
    /* space for "yyyy-mm-dd HH:MM:SS\0" + space + null */
    CHAR timeString[MAX_TIMESTAMP_FORMAT_STR_LEN + 1 + 1];
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 offset = 0;

#ifdef ENABLE_LOG_THREAD_ID
    // MAX_THREAD_ID_STR_LEN + null
    CHAR tidString[MAX_THREAD_ID_STR_LEN + 1];
    TID threadId = GETTID();
    SNPRINTF(tidString, ARRAY_SIZE(tidString), "(thread-0x%" PRIx64 ")", threadId);
#endif

    /* if something fails in getting time, still print the log, just without timestamp */
    retStatus = generateTimestampStr(globalGetTime(), "%F %H:%M:%S ", timeString, (UINT32) ARRAY_SIZE(timeString), &timeStrLen);
    if (STATUS_FAILED(retStatus)) {
        PRINTF("Fail to get time with status code is %08x\n", retStatus);
        timeString[0] = '\0';
    }

    offset = SNPRINTF(buffer, bufferLen, "\n%s", timeString);
#ifdef ENABLE_LOG_THREAD_ID
    offset += SNPRINTF(buffer + offset, bufferLen - offset, "%s ", tidString);
#endif
    SNPRINTF(buffer + offset, bufferLen - offset, "%s", fmt);
}


//
// Default logger function
//
VOID defaultLogPrint(UINT32 level, PCHAR tag, PCHAR fmt, ...)
{
    CHAR logFmtString[MAX_LOG_FORMAT_LENGTH + 1];

    UNUSED_PARAM(tag);

    if (level >= gLoggerLogLevel) {
        updateLogFormat(logFmtString, (UINT32) ARRAY_SIZE(logFmtString), fmt);

        va_list valist;
        va_start(valist, fmt);
        vprintf(logFmtString, valist);
        va_end(valist);
    }
}

logPrintFunc globalCustomLogPrintFn = defaultLogPrint;