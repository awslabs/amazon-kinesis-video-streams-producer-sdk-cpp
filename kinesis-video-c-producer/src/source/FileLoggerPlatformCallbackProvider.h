#ifndef __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__
#define __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

// For tight packing


/////////////////////////////////////////
// File logging functionality
/////////////////////////////////////////

/**
 * file logger declaration
 */
typedef struct {
    // string buffer. once the buffer is full, its content will be flushed to file
    PCHAR stringBuffer;

    // Size of the buffer in bytes
    // This will point to the end of the FileLogger to allow for single allocation and preserve the processor cache locality
    UINT64 stringBufferLen;

    // lock protecting the print operation
    MUTEX lock;

    // bytes starting from beginning of stringBuffer that contains valid data
    UINT64 currentOffset;

    // directory to put the log file
    CHAR logFileDir[MAX_PATH_LEN + 1];

    // file to store last log file index
    CHAR indexFilePath[MAX_PATH_LEN + 1];

    // max number of log file allowed
    UINT64 maxFileCount;

    // index for next log file
    UINT64 currentFileIndex;

    // print log to stdout too
    BOOL printLog;

    // file logger logPrint callback
    logPrintFunc fileLoggerLogPrintFn;
} FileLogger, *PFileLogger;

#define MAX_FILE_LOGGER_STRING_BUFFER_SIZE          3 * 1024 * 1024
#define MIN_FILE_LOGGER_STRING_BUFFER_SIZE          10 * 1024
#define MAX_FILE_LOGGER_LOG_FILE_COUNT              10 * 1024

////////////////////////////////////////////////////////////////////////
// file logger function implementations
////////////////////////////////////////////////////////////////////////

/**
 * Flushes currentOffset number of chars from stringBuffer into logfile.
 * If maxFileCount is exceeded, the earliest file is deleted before writing to the new file.
 * After flushLogToFile finishes, currentOffset is set to 0, whether the status of execution was success or not.
 *
 * @return - STATUS of execution
 */
STATUS flushLogToFile();
/**
 * This function initializes the static pFileLogger in FileLoggerPlatformCallbacks.c
 * @param - UINT64 - IN - Size of string buffer in file logger. When the string buffer is full the logger will flush everything into a new file
 * @param - UINT64 - IN - Max number of log file. When exceeded, the oldest file will be deleted when new one is generated
 * @param - PCHAR - IN - Directory in which the log file will be generated
 * @param - BOOL - IN - print log to std out too
 *
 * @return - STATUS of execution
 */
STATUS createFileLogger(UINT64, UINT64, PCHAR, BOOL);
/**
 * This function frees the static pFileLogger in FileLoggerPlatformCallbacks.c
 *
 * @return - STATUS of execution
 */
STATUS freeFileLogger();

/**
 * This callback is supposed to be called when callbacks are getting freed. It will free the underlying PFileLogger.
 *
 * @return - STATUS of execution
 */
STATUS freeFileLoggerPlatformCallbacksFunc(PUINT64);



#ifdef  __cplusplus
}
#endif
#endif /* __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__ */