/**
 * Kinesis Video Client Callbacks
 */
#define LOG_CLASS "KinesisVideoClientCallbacks"
#include "Include_i.h"

/**
 * Default get current time implementation
 */
UINT64 kinesisVideoStreamDefaultGetCurrentTime(UINT64 customData)
{
    UNUSED_PARAM(customData);
    return GETTIME();
}

/**
 * Default random number generator functionality
 */
UINT32 kinesisVideoStreamDefaultGetRandomNumber(UINT64 customData)
{
    UNUSED_PARAM(customData);
    return RAND();
}

/**
 * Default create mutex functionality
 */
MUTEX kinesisVideoStreamDefaultCreateMutex(UINT64 customData, BOOL reentrant)
{
    UNUSED_PARAM(customData);
    return MUTEX_CREATE(reentrant);
}

/**
 * Default lock mutex functionality
 */
VOID kinesisVideoStreamDefaultLockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_LOCK(mutex);
}

/**
 * Default unlock mutex functionality
 */
VOID kinesisVideoStreamDefaultUnlockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_UNLOCK(mutex);
}

/**
 * Default try unlock mutex functionality
 */
BOOL kinesisVideoStreamDefaultTryLockMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    return MUTEX_TRYLOCK(mutex);
}

/**
 * Default free mutex functionality
 */
VOID kinesisVideoStreamDefaultFreeMutex(UINT64 customData, MUTEX mutex)
{
    UNUSED_PARAM(customData);
    MUTEX_FREE(mutex);
}

/**
 * Default create condition variable functionality
 */
CVAR kinesisVideoStreamDefaultCreateConditionVariable(UINT64 customData)
{
    UNUSED_PARAM(customData);
    return CVAR_CREATE();
}

/**
 * Default signal condition variable functionality
 */
STATUS kinesisVideoStreamDefaultSignalConditionVariable(UINT64 customData, CVAR cvar)
{
    UNUSED_PARAM(customData);
    return CVAR_SIGNAL(cvar);
}

/**
 * Default broadcast condition variable functionality
 */
STATUS kinesisVideoStreamDefaultBroadcastConditionVariable(UINT64 customData, CVAR cvar)
{
    UNUSED_PARAM(customData);
    return CVAR_BROADCAST(cvar);
}

/**
 * Default wait for condition variable functionality
 */
STATUS kinesisVideoStreamDefaultWaitConditionVariable(UINT64 customData, CVAR cvar, MUTEX mutex, UINT64 timeout)
{
    UNUSED_PARAM(customData);
    return CVAR_WAIT(cvar, mutex, timeout);
}

/**
 * Default free cvar functionality
 */
VOID kinesisVideoStreamDefaultFreeConditionVariable(UINT64 customData, CVAR cvar)
{
    UNUSED_PARAM(customData);
    CVAR_FREE(cvar);
}

/**
 * Default stream ready callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultStreamReady(UINT64 customData, STREAM_HANDLE streamHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    return STATUS_SUCCESS;
}

/**
 * Default end of stream callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultEndOfStream(UINT64 customData, STREAM_HANDLE streamHandle, UPLOAD_HANDLE streamUploadHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(streamUploadHandle);
    return STATUS_SUCCESS;
}

/**
 * Default client ready callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultClientReady(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(clientHandle);
    return STATUS_SUCCESS;
}

/**
 * Default stream data available callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultStreamDataAvailable(UINT64 customData, STREAM_HANDLE streamHandle, PCHAR streamName, UPLOAD_HANDLE uploadHandle,
                                                    UINT64 duration, UINT64 size)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(streamName);
    UNUSED_PARAM(uploadHandle);
    UNUSED_PARAM(duration);
    UNUSED_PARAM(size);
    return STATUS_SUCCESS;
}

/**
 * Default client shutdown callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultClientShutdown(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(clientHandle);
    return STATUS_SUCCESS;
}

/**
 * Default stream shutdown callback functionality - NOOP
 */
STATUS kinesisVideoStreamDefaultStreamShutdown(UINT64 customData, STREAM_HANDLE streamHandle, BOOL resetStream)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(streamHandle);
    UNUSED_PARAM(resetStream);
    return STATUS_SUCCESS;
}