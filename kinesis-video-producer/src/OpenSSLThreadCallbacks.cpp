#include <assert.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include "Logger.h"
#include "OpenSSLThreadCallbacks.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

/**
* This struct must live in the global namespace so that it plays nicely with the opaque struct
* required by the openSSL callback function signatures. It simply wraps a pthread_mutex_t.
*/
typedef struct CRYPTO_dynlock_value {
    pthread_mutex_t mutex;
} CRYPTO_dynlock_value;

namespace OpenSSLThreadCallbacks {

static pthread_mutex_t *mutexes = NULL;

/** Callback to provide mutex on a resource.
* Must be able to handle up to CRYPTO_num_locks() different mutex locks.
* It sets the n-th lock if mode & CRYPTO_LOCK, and releases it otherwise.
*/
static void openSSLLockCallback(int mode, int n, const char * /*file*/, int /*line*/) {
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&mutexes[n]);
    } else {
        pthread_mutex_unlock(&mutexes[n]);
    }
}

/** Creates resources for a CRYPTO_dynlock_value. */
static CRYPTO_dynlock_value *openSSLDynCreateFunction(const char *file, int line) {
    CRYPTO_dynlock_value *dl = new CRYPTO_dynlock_value();
    if (NULL != dl) {
        int status = pthread_mutex_init(&dl->mutex, NULL);
        if (0 != status) {
            LOG_FATAL("Alakazam failed to initialize dynamic mutex structure");
            delete (dl);
            dl = NULL;
        }
    } else {
        LOG_FATAL("Alakazam failed to allocated memory for openssl dynamic lock");
    }
    return dl;
}

/** Frees the resources obtained in openSSLDynCreateFunction(). */
static void openSSLDynDestroyFunction(CRYPTO_dynlock_value *dl, const char *file, int line) {
    pthread_mutex_destroy(&dl->mutex);
    delete dl;
}

/** Callback to provide mutex on a resource. Works on a user defined CRYPTO_dynlock_value instead
* of an array of mutexes like openSSLLockCallback().
*/
static void openSSLDynLockCallback(int mode, CRYPTO_dynlock_value *dl, const char * /*file*/, int/*line*/) {
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&dl->mutex);
    } else {
        pthread_mutex_unlock(&dl->mutex);
    }
}

/** Sets the currently-executing thread ID. */
static void openSSLThreadIdCallback(CRYPTO_THREADID *id) {
    CRYPTO_THREADID_set_numeric(id, reinterpret_cast<unsigned long>(pthread_self()));
}

/**
* Initialize the array of mutexes via pthread_mutex_init().
* The function may be called multiple times, but the array
* will only be initialized once.
*/
static void initOpenSSLMutexes(void) {
    if (NULL == mutexes) {
// The mutex array must support up to CRYPTO_num_locks().
        mutexes = new pthread_mutex_t[CRYPTO_num_locks()];
        for (int i = 0; i < CRYPTO_num_locks(); i++) {
            pthread_mutex_init(&mutexes[i], NULL);
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
