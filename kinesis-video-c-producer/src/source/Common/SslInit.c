/**
 * Security sub-module initializer
 */
#define LOG_CLASS "SslInit"
#include "Include_i.h"

#if defined(KVS_USE_OPENSSL) && defined(OPENSSL_THREADS) && defined(SET_SSL_CALLBACKS)

// Unfortunately, this should be global due to the OpenSSL interfaces
static MUTEX* gOpenSslMutexes = NULL;

/**
 * Callback to provide mutex on a resource.
 * Must be able to handle up to CRYPTO_num_locks() different mutex locks.
 * It sets the n-th lock if mode & CRYPTO_LOCK, and releases it otherwise.
 */
VOID openSslLockCallback(INT32 mode, INT32 n, PCHAR file, INT32 line)
{
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    CHECK_EXT(gOpenSslMutexes != NULL, "Calling SSL callback on un-initialized globals");

    if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(gOpenSslMutexes[n]);
    } else {
        MUTEX_UNLOCK(gOpenSslMutexes[n]);
    }
}

/**
 * Creates resources for a CRYPTO_dynlock_value.
 */
CRYPTO_dynlock_value* openSslDynCreateFunction(PCHAR file, INT32 line)
{
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);

    CRYPTO_dynlock_value* pDynLockVal = MEMALLOC(SIZEOF(CRYPTO_dynlock_value));
    if (NULL != pDynLockVal) {
        pDynLockVal->mutex = MUTEX_CREATE(FALSE);
    } else {
        DLOGE("Failed to allocated memory for openssl dynamic lock");
    }

    return pDynLockVal;
}

/**
 * Frees the resources obtained in openSslDynCreateFunction().
 */
VOID openSslDynDestroyFunction(CRYPTO_dynlock_value* pDynLockVal, PCHAR file, INT32 line)
{
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);

    CHECK_EXT(pDynLockVal != NULL, "Invalid dynamic lock value specified in the callback");

    MUTEX_FREE(pDynLockVal->mutex);
    MEMFREE(pDynLockVal);
}

/**
 * Callback to provide mutex on a resource. Works on a user defined CRYPTO_dynlock_value instead
 * of an array of mutexes like openSslLockCallback().
 */
VOID openSslDynLockCallback(INT32 mode, CRYPTO_dynlock_value* pDynLockVal, PCHAR file, INT32 line)
{
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);

    CHECK_EXT(pDynLockVal != NULL, "Invalid dynamic lock value specified in the callback");

    if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(pDynLockVal->mutex);
    } else {
        MUTEX_UNLOCK(pDynLockVal->mutex);
    }
}

/**
 * Sets the currently-executing thread ID.
 */
VOID openSslThreadIdCallback(CRYPTO_THREADID* id)
{
    CRYPTO_THREADID_set_numeric(id, GETTID());
}

STATUS initializeSslCallbacks()
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, count;

    // Skip if it's already been initialized - effectively making it a singleton
    CHK(gOpenSslMutexes == NULL, retStatus);

    // Allocate the storage for the mutex array
    // The mutex array must support up to CRYPTO_num_locks().
    count = CRYPTO_num_locks();
    CHK(NULL != (gOpenSslMutexes = (MUTEX*) MEMCALLOC(count, SIZEOF(MUTEX))), STATUS_NOT_ENOUGH_MEMORY);
    for (i = 0; i < count; i++) {
        gOpenSslMutexes[i] = MUTEX_CREATE(FALSE);
    }

    CRYPTO_set_locking_callback(openSslLockCallback);
    CRYPTO_THREADID_set_callback(openSslThreadIdCallback);
    CRYPTO_set_dynlock_create_callback(openSslDynCreateFunction);
    CRYPTO_set_dynlock_lock_callback(openSslDynLockCallback);
    CRYPTO_set_dynlock_destroy_callback(openSslDynDestroyFunction);

CleanUp:

    return retStatus;
}

STATUS releaseSslCallbacks()
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;

    // Fast check for call idempotence
    CHK(gOpenSslMutexes != NULL, retStatus);

    CRYPTO_set_locking_callback(NULL);
    CRYPTO_THREADID_set_callback(NULL);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        if (IS_VALID_MUTEX_VALUE(gOpenSslMutexes[i])) {
            MUTEX_FREE(gOpenSslMutexes[i]);
        }
    }

    SAFE_MEMFREE(gOpenSslMutexes);

CleanUp:

    return retStatus;
}

#else
STATUS initializeSslCallbacks()
{
    return STATUS_SUCCESS;
}

STATUS releaseSslCallbacks()
{
    return STATUS_SUCCESS;
}
#endif