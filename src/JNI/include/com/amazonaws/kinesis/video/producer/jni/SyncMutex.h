/**
 * A version of Android's Mutex object that can be entered multiple times by the same thread and can log its activity.
 * Typically used to protect code regions from concurrent entry by more than one thread.
 */

#ifndef __SYNC_MUTEX_H__
#define __SYNC_MUTEX_H__

#include <com/amazonaws/kinesis/video/client/Include.h>

static const size_t gMaxMutexDescriptionSize = 100;
static const char* const gDefaultMutexDescription = "mutex";

class SyncMutex
{
    // Logging behavior
    char mMutexDescription[gMaxMutexDescriptionSize];
    bool mLogsEnabled;

    // Mutex implementation primitives
    MUTEX mMutex;
    CVAR mCondition;

    SyncMutex(const SyncMutex&); // Prevent copies
    SyncMutex& operator=(const SyncMutex&); // Prevent assignment

public:

    SyncMutex()
    {
        initialize();
    }

    SyncMutex(const char* mutexName)
    {
        initialize();
        setName(mutexName);
    }

    void initialize()
    {
        // Default logging configuration: none
        setName(gDefaultMutexDescription);
        mLogsEnabled = false;

        // Prepare pthreads primitives
        mMutex = MUTEX_CREATE(TRUE);
        mCondition = CVAR_CREATE();
    }

    ~SyncMutex()
    {
        MUTEX_FREE(mMutex);
        CVAR_FREE(mCondition);
    }

    // Set the mutex name to be shown in log messages.
    void setName(const char* mutexName)
    {
        CHECK(mutexName);
        STRNCPY(mMutexDescription, mutexName, sizeof mMutexDescription);
        mMutexDescription[sizeof mMutexDescription - 1] = '\0'; // Don't trust strncpy() to NUL-terminate
    }

    // Set the mutex name and associated media type to be shown in log messages.
    void setName(const char* mutexName, const char* mediaType)
    {
        CHECK(mutexName);
        CHECK(mediaType);
        snprintf(mMutexDescription, sizeof mMutexDescription, "%s[%s]", mutexName, mediaType);
        mMutexDescription[sizeof mMutexDescription - 1] = '\0'; // Don't trust snprintf() to NUL-terminate
    }

    // Enable or disable detailed activity traces.
    void setLoggingEnabled(bool enabled)
    {
        mLogsEnabled = enabled;
    }

    // Acquire the mutex.
    void lock(const char* function)
    {
        if (mLogsEnabled)
        {
            DLOGI("%s: locking %s", function, mMutexDescription);
        }

        MUTEX_LOCK(mMutex);
    }

    // Release the mutex.
    void unlock(const char* function)
    {
        if (mLogsEnabled)
        {
            DLOGI("%s: unlocking %s", function, mMutexDescription);
        }

        MUTEX_UNLOCK(mMutex);
    }

    // Acquire the mutex and wait on the condition variable.
    void wait(const char* function)
    {
        MUTEX_LOCK(mMutex);

        UINT64 before = 0;
        if (mLogsEnabled)
        {
            DLOGI("%s: waiting on %s", function, mMutexDescription);
            UINT64 before = GETTIME();
        }

        int status = CVAR_WAIT(mCondition, mMutex, INFINITE_TIME_VALUE);
        CHECK_EXT(status == 0, "pthread_cond_wait() returned Unix errno %d", status);

        if (mLogsEnabled)
        {
            UINT64 after = GETTIME();
            UINT64 elapsed_ms = (after - before) / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
            DLOGI("%s: waited %ldms for %s", function, elapsed_ms, mMutexDescription);
        }

        MUTEX_UNLOCK(mMutex);
    }

    // Signal the condition variable, allowing all blocked threads to continue.
    void notifyAll(const char* function)
    {
        if (mLogsEnabled)
        {
            DLOGI("%s: signalling %s", function, mMutexDescription);
        }

        STATUS status = CVAR_BROADCAST(mCondition);
        CHECK_EXT(STATUS_SUCCEEDED(status), "pthread_cond_broadcast() returned Unix errno %d", status);
    }

    /**
     * Autolock: Helper object that locks a given mutex on creation and unlocks it again on destruction.  This is
     * typically used with automatic scope as a way to ensure that the mutex is unlocked no matter how the scope is
     * exited (goto, return, etc).
     */
    class Autolock {

        SyncMutex& mLock;
        const char* mFunction;

    public:
        Autolock(SyncMutex& mutex, const char* function) : mLock(mutex), mFunction(function) {mLock.lock(function);}
        Autolock(SyncMutex* mutex, const char* function) : mLock(*mutex), mFunction(function) {mLock.lock(function);}
        ~Autolock() {mLock.unlock(mFunction);}
    };

};

#endif // __SYNC_MUTEX_H__
