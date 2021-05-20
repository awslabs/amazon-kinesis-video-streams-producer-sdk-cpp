/**
 * Kinesis Video Producer File Logger functionality
 */
#define LOG_CLASS "FileLogger"
#include "Include_i.h"

STATUS freeFileLoggerPlatformCallbacksFunc(PUINT64 customData)
{
    STATUS retStatus = STATUS_SUCCESS;

    UNUSED_PARAM(customData);
    CHK_STATUS(freeFileLogger());

CleanUp:

    return retStatus;
}

STATUS addFileLoggerPlatformCallbacksProvider(PClientCallbacks pClientCallbacks, UINT64 stringBufferSize, UINT64 maxLogFileCount, PCHAR logFileDir,
                                              BOOL printLog)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCallbacksProvider pCallbackProvider = (PCallbacksProvider) pClientCallbacks;
    PlatformCallbacks fileLoggerPlatformCallbacks;
    logPrintFunc logFn;

    CHK(pCallbackProvider != NULL, STATUS_NULL_ARG);

    CHK_STATUS(createFileLogger(stringBufferSize, maxLogFileCount, logFileDir, printLog, FALSE, &logFn));

    MEMSET(&fileLoggerPlatformCallbacks, 0x00, SIZEOF(PlatformCallbacks));
    fileLoggerPlatformCallbacks.customData = (UINT64) NULL;
    fileLoggerPlatformCallbacks.version = PLATFORM_CALLBACKS_CURRENT_VERSION;
    fileLoggerPlatformCallbacks.logPrintFn = logFn;
    fileLoggerPlatformCallbacks.freePlatformCallbacksFn = freeFileLoggerPlatformCallbacksFunc;

    CHK_STATUS(setPlatformCallbacks(pClientCallbacks, &fileLoggerPlatformCallbacks));

CleanUp:

    if (!STATUS_SUCCEEDED(retStatus)) {
        freeFileLogger();
    }

    // Need to fix-up the status code from the common for back compat
    if (retStatus == STATUS_FILE_LOGGER_INDEX_FILE_INVALID_SIZE) {
        retStatus = STATUS_FILE_LOGGER_INDEX_FILE_TOO_LARGE;
    }
    return retStatus;
}
