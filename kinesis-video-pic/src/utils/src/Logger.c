#include "Include_i.h"

UINT32 gLoggerLogLevel = LOG_LEVEL_WARN;

//
// Default logger function
//
VOID defaultLogPrint(UINT32 level, PCHAR tag, PCHAR fmt, ...)
{
    /* space for "yyyy-mm-dd HH:MM:SS\0" */
    CHAR timeString[MAX_TIMESTAMP_FORMAT_STR_LEN + 1];
    CHAR logString[MAX_LOG_LENGTH + 1];
#ifdef ENABLE_LOG_THREAD_ID
    TID threadId;
#endif
    UINT32 timeStrLen = 0;
    STATUS retStatus = STATUS_SUCCESS;

    UNUSED_PARAM(tag);

    if (level >= gLoggerLogLevel) {
        /* if something fails in getting time, still print the log, just without timestamp */
        retStatus = generateTimestampStr(globalGetTime(), "%F %H:%M:%S", timeString, SIZEOF(timeString), &timeStrLen);
        if (STATUS_FAILED(retStatus)) {
            PRINTF("Fail to get time with status code is %08x\n", retStatus);
            SNPRINTF(logString, SIZEOF(logString), "\n%s", fmt);
        } else {
            SNPRINTF(logString, SIZEOF(logString), "\n%s %s", timeString, fmt);
        }

#ifdef ENABLE_LOG_THREAD_ID
        threadId = globalGetThreadId();
        SNPRINTF(logString, SIZEOF(logString), "\n%s (thread-0x%" PRIx64 ") %s", timeString, threadId, fmt);
#endif

        va_list valist;
        va_start(valist, fmt);
        vprintf(logString, valist);
        va_end(valist);
    }
}

logPrintFunc globalCustomLogPrintFn = defaultLogPrint;