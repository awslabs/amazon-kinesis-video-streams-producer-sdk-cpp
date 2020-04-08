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
    MUTEX mMutex;
    CVAR mCond;
    UINT32 mCount;

public:

    TimedSemaphore() : mCount(0)
    {
        mMutex = MUTEX_CREATE(FALSE);
        mCond = CVAR_CREATE();
    }

    ~TimedSemaphore()
    {
        MUTEX_FREE(mMutex);
        CVAR_FREE(mCond);
    }

    /**
     * Waits until the semaphore count becomes positive, if necessary, then decrements it.
     */
    void wait()
    {
        MUTEX_LOCK(mMutex);

        while (mCount <= 0)
        {
            if (STATUS_FAILED(CVAR_WAIT(mCond, mMutex, INFINITE_TIME_VALUE)))
            {
                CRASH("Fatal error in semaphore wait");
                break;
            }
        }

        mCount--;

        MUTEX_UNLOCK(mMutex);
    }

    /**
     * Decrements the semaphore count and returns true iff it is still positive.  Does not block.
     */
    bool tryWait()
    {
        MUTEX_LOCK(mMutex);

        bool sem = (mCount > 0);
        if (sem)
        {
            mCount--;
        }

        MUTEX_UNLOCK(mMutex);
        return sem;
    }

    /**
     * Waits until the semaphore count becomes positive, if necessary, then decrements it and returns true.
     * If the wait takes longer than the specified period of time, it gives up and returns false.
     */
    bool timedWait(UINT32 relTimeOutMs)
    {
        MUTEX_LOCK(mMutex);
        BOOL retVal = true;

        // Create default units duration
        UINT64 duration = relTimeOutMs * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;

        while (mCount == 0)
        {
            STATUS status = CVAR_WAIT(mCond, mMutex, duration);
            if (status == STATUS_OPERATION_TIMED_OUT)
            {
                retVal = false;
                break;
            }

            // Any other failure is fatal
            if (STATUS_FAILED(status))
            {
                CRASH("Fatal error in timed semaphore wait");
                
                // Unreachable - just to keep some static code analysis tools happy
                break;
            }
        }
        mCount--;

        MUTEX_UNLOCK(mMutex);
        return retVal;
    }

    /**
     * Increments the semaphore count, and if it was zero, marks a thread (if any) currently waiting on this semaphore
     * as ready-to-run. If multiple threads are waiting, only one will be woken, but it is not specified which.
     *
     * This method never blocks.
     */
    void post()
    {
        MUTEX_LOCK(mMutex);

        mCount++;

        if (mCount == 1 && STATUS_FAILED(CVAR_SIGNAL(mCond))) {
            CRASH("Signaling a condition variable failed; it probably wasn't initialized (errno = %d)", errno);
        }

        MUTEX_UNLOCK(mMutex);
    }
};

#endif // __TIMED_SEMAPHORE_H__
