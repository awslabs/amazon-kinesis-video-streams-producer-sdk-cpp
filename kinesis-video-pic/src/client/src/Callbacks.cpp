/**
 * Kinesis Video Client Callbacks
 */
#define LOG_CLASS "KinesisVideoClientCallbacks"
#include "Include_i.h"

/**
 * Default get current time implementation
 */
UINT64 defaultGetCurrentTime(UINT64 customData)
{
    UNUSED_PARAM(customData);
    return GETTIME();
}

/**
 * Default random number generator functionality
 */
UINT32 defaultGetRandomNumber(UINT64 customData)
{
    UNUSED_PARAM(customData);
    SRAND(GETTIME());
    return RAND();
}

/**
 * Default create mutex functionality
 */
MUTEX defaultCreateMutex(UINT64 customData, BOOL reentrant)
{
    UNUSED_PARAM(customData);
    return MUTEX_CREATE(reentrant);
}

/**
 * Default lock mutex functionality
 */
VOID defaultLockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_LOCK(mutex);
}

/**
 * Default unlock mutex functionality
 */
VOID defaultUnlockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_UNLOCK(mutex);
}

/**
 * Default try unlock mutex functionality
 */
VOID defaultTryLockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_TRYLOCK(mutex);
}

/**
 * Default free mutex functionality
 */
VOID defaultFreeMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_FREE(mutex);
}

/**
 * Default stream ready callback functionality - NOOP
 */
STATUS defaultStreamReady(UINT64 customData, STREAM_HANDLE streamHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    return STATUS_SUCCESS;
}

/**
 * Default end of stream callback functionality - NOOP
 */
STATUS defaultEndOfStream(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 streamUploadHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(streamUploadHandle);
    return STATUS_SUCCESS;
}

/**
 * Default client ready callback functionality - NOOP
 */
STATUS defaultClientReady(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(clientHandle);
    return STATUS_SUCCESS;
}