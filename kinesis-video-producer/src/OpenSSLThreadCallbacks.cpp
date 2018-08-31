#include <assert.h>
#include "com/amazonaws/kinesis/video/client/Include.h"
#include <openssl/crypto.h>
#include "Logger.h"
#include "OpenSSLThreadCallbacks.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

/**
 * This struct must live in the global namespace so that it plays nicely with the opaque struct
 * required by the openSSL callback function signatures. It simply wraps a pthread_mutex_t.
 */
typedef struct CRYPTO_dynlock_value {
    MUTEX mutex;
} CRYPTO_dynlock_value;

namespace OpenSSLThreadCallbacks {

static MUTEX *mutexes = NULL;

/**
 * Callback to provide mutex on a resource.
 * Must be able to handle up to CRYPTO_num_locks() different mutex locks.
 * It sets the n-th lock if mode & CRYPTO_LOCK, and releases it otherwise.
 */
static void openSSLLockCallback(int mode, int n, const char * file, int line) {
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(mutexes[n]);
    } else {
        MUTEX_UNLOCK(mutexes[n]);
    }
}

/**
 * Creates resources for a CRYPTO_dynlock_value.
 */
static CRYPTO_dynlock_value *openSSLDynCreateFunction(const char *file, int line) {
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    CRYPTO_dynlock_value *dl = new CRYPTO_dynlock_value();
    if (NULL != dl) {
        dl->mutex = MUTEX_CREATE(true);
    } else {
        LOG_FATAL("Failed to allocated memory for openssl dynamic lock");
    }
    return dl;
}

/**
 * Frees the resources obtained in openSSLDynCreateFunction().
 */
static void openSSLDynDestroyFunction(CRYPTO_dynlock_value *dl, const char *file, int line) {
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    MUTEX_FREE(dl->mutex);
    delete dl;
}

/**
 * Callback to provide mutex on a resource. Works on a user defined CRYPTO_dynlock_value instead
 * of an array of mutexes like openSSLLockCallback().
 */
static void openSSLDynLockCallback(int mode, CRYPTO_dynlock_value *dl, const char * file, int line) {
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(dl->mutex);
    } else {
        MUTEX_UNLOCK(dl->mutex);
    }
}

/**
 * Sets the currently-executing thread ID.
 */
static void openSSLThreadIdCallback(CRYPTO_THREADID *id) {
    CRYPTO_THREADID_set_numeric(id, static_cast<unsigned long>(GETTID()));
}

/**
 * Initialize the array of mutexes via pthread_mutex_init().
 * The function may be called multiple times, but the array
 * will only be initialized once.
 */
static void initOpenSSLMutexes(void) {
    if (NULL == mutexes) {
        // The mutex array must support up to CRYPTO_num_locks().
        mutexes = new MUTEX[CRYPTO_num_locks()];
        for (int i = 0; i < CRYPTO_num_locks(); i++) {
            mutexes[i] = MUTEX_CREATE(FALSE);
        }
    }
}

void registerThreadingCallbacks(void) {
    initOpenSSLMutexes();
    CRYPTO_set_locking_callback(openSSLLockCallback);
    CRYPTO_THREADID_set_callback(openSSLThreadIdCallback);
    CRYPTO_set_dynlock_create_callback(openSSLDynCreateFunction);
    CRYPTO_set_dynlock_lock_callback(openSSLDynLockCallback);
    CRYPTO_set_dynlock_destroy_callback(openSSLDynDestroyFunction);
}
} // OpenSSLThreadCallbacks
