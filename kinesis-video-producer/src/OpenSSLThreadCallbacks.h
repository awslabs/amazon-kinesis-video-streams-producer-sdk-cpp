#pragma once

/**
 * Provides a minimal pthread implementation of callbacks required by openssl for thread safety.
 * See: https://www.openssl.org/docs/manmaster/crypto/threads.html
 */
namespace OpenSSLThreadCallbacks {

    /**
     * Register the CRYPTO_set_locking_callback() and CRYPTO_set_id_callback() with openssl.
     * This is the minimal requirement for openssl thread safety:
     */
    void registerThreadingCallbacks(void);
}
