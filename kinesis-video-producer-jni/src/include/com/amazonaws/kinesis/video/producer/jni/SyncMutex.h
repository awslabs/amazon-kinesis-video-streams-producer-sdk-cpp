/**
 * A version of Android's Mutex object that can be entered multiple times by the same thread and can log its activity.
 * Typically used to protect code regions from concurrent entry by more than one thread.
 */

#ifndef __SYNC_MUTEX_H__
#define __SYNC_MUTEX_H__

static const size_t gMaxMutexDescriptionSize = 100;
static const char* const gDefaultMutexDescription = "mutex";

class SyncMutex
{
    // Logging behavior
    char mMutexDescription[gMaxMutexDescriptionSize];
    bool mLogsEnabled;

    // Mutex implementation primitives
    pthread_mutex_t mMutex;
    pthread_mutexattr_t mMutexAttributes;
    pthread_cond_t mCondition;

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
        if (pthread_mutexattr_init(&mMutexAttributes) != 0 ||
            pthread_mutexattr_settype(&mMutexAttributes, PTHREAD_MUTEX_RECURSIVE) != 0 ||
            pthread_mutex_init(&mMutex, &mMutexAttributes) != 0 ||
            pthread_cond_init(&mCondition, NULL) != 0)
        {
            CRASH("Mutex initialization failed");
        }
    }

    ~SyncMutex()
    {
        int status = pthread_mutex_destroy(&mMutex);
        CHECK_EXT(status == 0, "pthread_mutex_destroy() returned Unix errno %d", status);
    }

    // Set the mutex name to be shown in log messages.
    void setName(const char* mutexName)
    {
        CHECK(mutexName);
        strncpy(mMutexDescription, mutexName, sizeof mMutexDescription);
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

        int status = pthread_mutex_lock(&mMutex);
        CHECK_EXT(status == 0, "pthread_mutex_lock() returned Unix errno %d", status);
    }

    // Release the mutex.
    void unlock(const char* function)
    {
        if (mLogsEnabled)
        {
            DLOGI("%s: unlocking %s", function, mMutexDescription);
        }

        int status = pthread_mutex_unlock(&mMutex);
        CHECK_EXT(status == 0, "pthread_mutex_unlock() returned Unix errno %d", status);
    }

    // Acquire the mutex and wait on the condition variable.
    void wait(const char* function)
    {
        long before_ms = 0;
        if (mLogsEnabled)
        {
            DLOGI("%s: waiting on %s", function, mMutexDescription);
            timeval before;
            gettimeofday(&before, NULL);
            before_ms = before.tv_sec * 1000 + before.tv_usec / 1000;
        }

        int status = pthread_cond_wait(&mCondition, &mMutex);
        CHECK_EXT(status == 0, "pthread_cond_wait() returned Unix errno %d", status);

        if (mLogsEnabled)
        {
            timeval after;
            gettimeofday(&after, NULL);
            long after_ms = after.tv_sec * 1000 + after.tv_usec / 1000;
            long elapsed_ms = after_ms - before_ms;
            DLOGI("%s: waited %ldms for %s", function, elapsed_ms, mMutexDescription);
        }
    }

    // Signal the condition variable, allowing all blocked threads to continue.
    void notifyAll(const char* function)
    {
        if (mLogsEnabled)
        {
            DLOGI("%s: signalling %s", function, mMutexDescription);
        }

        int status = pthread_cond_broadcast(&mCondition);
        CHECK_EXT(status == 0, "pthread_cond_broadcast() returned Unix errno %d", status);
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
