#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Stub Mutex library functions
//
INLINE MUTEX stubCreateMutex(BOOL reentrant)
{
    UNUSED_PARAM(reentrant);
    return (MUTEX) 0;
}

INLINE VOID stubLockMutex(MUTEX mutex)
{
    UNUSED_PARAM(mutex);
}

INLINE VOID stubUnlockMutex(MUTEX mutex)
{
    UNUSED_PARAM(mutex);
}

INLINE VOID stubTryLockMutex(MUTEX mutex)
{
    UNUSED_PARAM(mutex);
}

INLINE VOID stubFreeMutex(MUTEX mutex)
{
    UNUSED_PARAM(mutex);
}

createMutex globalCreateMutex = stubCreateMutex;
lockMutex globalLockMutex = stubLockMutex;
unlockMutex globalUnlockMutex = stubUnlockMutex;
tryLockMutex globalTryLockMutex = stubTryLockMutex;
freeMutex globalFreeMutex = stubFreeMutex;

#else

// Definition of the static global mutexes
// NOTE!!! Some of the libraries don't have a definition of PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
pthread_mutex_t globalReentrantMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
pthread_mutex_t globalReentrantMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif
pthread_mutex_t globalNonReentrantMutex = PTHREAD_MUTEX_INITIALIZER;

INLINE MUTEX defaultCreateMutex(BOOL reentrant)
{
    pthread_mutex_t* pMutex;
    pthread_mutexattr_t mutexAttributes;

    // Allocate the mutex
    pMutex = (pthread_mutex_t*) MEMCALLOC(1, SIZEOF(pthread_mutex_t));
    if (NULL == pMutex) {
        return (reentrant ? GLOBAL_REENTRANT_MUTEX : GLOBAL_NON_REENTRANT_MUTEX);
    }

    if (0 != pthread_mutexattr_init(&mutexAttributes) ||
        0 != pthread_mutexattr_settype(&mutexAttributes, reentrant ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL) ||
        0 != pthread_mutex_init(pMutex, &mutexAttributes))
    {
        // In case of an error return the global mutexes
        MEMFREE(pMutex);
        return (reentrant ? GLOBAL_REENTRANT_MUTEX : GLOBAL_NON_REENTRANT_MUTEX);
    }

    return (MUTEX) pMutex;
}

INLINE VOID defaultLockMutex(MUTEX mutex)
{
    pthread_mutex_lock((pthread_mutex_t*) mutex);
}

INLINE VOID defaultUnlockMutex(MUTEX mutex)
{
    pthread_mutex_unlock((pthread_mutex_t*) mutex);
}

INLINE VOID defaultTryLockMutex(MUTEX mutex)
{
    pthread_mutex_trylock((pthread_mutex_t*) mutex);
}

INLINE VOID defaultFreeMutex(MUTEX mutex)
{
    pthread_mutex_destroy((pthread_mutex_t*) mutex);

    // De-allocate the memory if it's not a well-known mutex - aka if we had allocated it previously
    if (mutex != GLOBAL_REENTRANT_MUTEX && mutex != GLOBAL_NON_REENTRANT_MUTEX) {
        MEMFREE((PVOID) mutex);
    }
}

createMutex globalCreateMutex = defaultCreateMutex;
lockMutex globalLockMutex = defaultLockMutex;
unlockMutex globalUnlockMutex = defaultUnlockMutex;
tryLockMutex globalTryLockMutex = defaultTryLockMutex;
freeMutex globalFreeMutex = defaultFreeMutex;

#endif
