/**
 * Implementation of counting semaphores that support operation timeouts using condition variables.
 * Useful on platforms whose runtime library lacks a sem_timedwait() implementation.
 */

#ifndef __TIMED_SEMAPHORE_H__
#define __TIMED_SEMAPHORE_H__

class TimedSemaphore
{
    TimedSemaphore(const TimedSemaphore&); // Prevent copy construction
    TimedSemaphore& operator=(const TimedSemaphore&); // Prevent assignment
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
    uint32_t mCount;

public:

    TimedSemaphore() : mCount(0)
    {
        if (pthread_mutex_init(&mMutex, NULL) != 0)
        {
            CRASH("Fatal error creating a pthreads mutex");
        }
        if (pthread_cond_init(&mCond, NULL) != 0)
        {
            CRASH("Fatal error creating a pthreads condition variable");
        }
    }

    ~TimedSemaphore()
    {
        pthread_cond_destroy(&mCond);
        pthread_mutex_destroy(&mMutex);
    }

    /**
     * Waits until the semaphore count becomes positive, if necessary, then decrements it.
     */
    void wait()
    {
        pthread_mutex_lock(&mMutex);

        while (mCount <= 0)
        {
            if (pthread_cond_wait(&mCond, &mMutex) != 0)
            {
                CRASH("Fatal error in semaphore wait");
                break;
            }
        }
        mCount--;

        pthread_mutex_unlock(&mMutex);
    }

    /**
     * Decrements the semaphore count and returns true iff it is still positive.  Does not block.
     */
    bool tryWait()
    {
        pthread_mutex_lock(&mMutex);

        bool sem = (mCount > 0);
        if (sem)
        {
            mCount--;
        }

        pthread_mutex_unlock(&mMutex);
        return sem;
    }

    /**
     * Waits until the semaphore count becomes positive, if necessary, then decrements it and returns true.
     * If the wait takes longer than the specified period of time, it gives up and returns false.
     */
    bool timedWait(uint32_t relTimeOutMs)
    {
        pthread_mutex_lock(&mMutex);

        // Compute absolute timeout time by offsetting the current time by the relative timeout time.
        // Use milliseconds as an intermediate value to keep the offset math simple.
        timeval now; gettimeofday(&now, NULL);
        uint64_t nowMs = now.tv_sec * 1000ull + now.tv_usec / 1000ull;
        uint64_t absTimeOutMs = nowMs + relTimeOutMs;
        timespec waitUntil;
        waitUntil.tv_sec = absTimeOutMs / 1000ull;
        waitUntil.tv_nsec = (absTimeOutMs % 1000ull) * 1000ull * 1000ull;

        while (mCount == 0)
        {
            int result = pthread_cond_timedwait(&mCond, &mMutex, &waitUntil);
            if (result == ETIMEDOUT)
            {
                pthread_mutex_unlock(&mMutex);
                return false;
            }
            // Any other failure is fatal
            if (result != 0)
            {
                pthread_mutex_unlock(&mMutex);
                CRASH("Fatal error in timed semaphore wait");
                return false;
            }
        }
        mCount--;

        pthread_mutex_unlock(&mMutex);
        return true;
    }

    /**
     * Increments the semaphore count, and if it was zero, marks a thread (if any) currently waiting on this semaphore
     * as ready-to-run. If multiple threads are waiting, only one will be woken, but it is not specified which.
     *
     * This method never blocks.
     */
    void post()
    {
        pthread_mutex_lock(&mMutex);

        mCount++;

        if (mCount == 1 && pthread_cond_signal(&mCond) != 0)
        {
            CRASH("Signaling a condition variable failed; it probably wasn't initialized (errno = %d)", errno);
        }

        pthread_mutex_unlock(&mMutex);
    }
};

#endif // __TIMED_SEMAPHORE_H__
