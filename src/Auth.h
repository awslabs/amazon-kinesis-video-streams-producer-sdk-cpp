#pragma once

#include <chrono>
#include <mutex>

#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <string>
#include "com/amazonaws/kinesis/video/cproducer/Include.h"

#include <Logger.h>
#include "GetTime.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

#define STRING_TO_PCHAR(s) ((PCHAR) ((s).c_str()))

/**
* Simple data object around aws credentials
*/
class Credentials {
public:
    Credentials() : access_key_(""), secret_key_(""), session_token_(""), expiration_(std::chrono::seconds(MAX_UINT64)) {}

    /**
    * Initializes object with access_key, secret_key, session_token and expiration.
     * NOTE: Session token defaults to empty.
     * NOTE: Expiration defaults to max uint 64
    */
    Credentials(const std::string& access_key,
                const std::string& secret_key,
                const std::string& session_token = "",
                const std::chrono::duration<uint64_t> expiration = std::chrono::seconds(MAX_UINT64)) :
    access_key_(access_key), secret_key_(secret_key), session_token_(session_token), expiration_(expiration)
    {
    }

    /**
     * Virtual destructor
     */
    virtual ~Credentials() {}

    /**
    * Gets the underlying access key credential
    */
    inline const std::string& getAccessKey() const
    {
        return access_key_;
    }

    /**
    * Gets the underlying secret key credential
    */
    inline const std::string& getSecretKey() const
    {
        return secret_key_;
    }

    /**
    * Gets the underlying session token
    */
    inline const std::string& getSessionToken() const
    {
        return session_token_;
    }

    /**
    * Gets the underlying session token, return NULL if not exist
    */
    inline const PCHAR getSessionTokenIfExist() const
    {
        return session_token_ == "" ? NULL : (PCHAR) session_token_.c_str();
    }

    /**
    * Gets the underlying session tokens expiration
    */
    inline const std::chrono::duration<uint64_t>& getExpiration() const
    {
        return expiration_;
    }

    /**
    * Sets the underlying access key credential. Copies from parameter access_key.
    */
    inline void setAccessKey(const std::string& access_key)
    {
        access_key_ = access_key;
    }

    /**
    * Sets the underlying secret key credential. Copies from parameter secret_key
    */
    inline void setSecretKey(const std::string& secret_key)
    {
        secret_key_ = secret_key;
    }

    /**
    * Sets the underlying session token. Copies from parameter session_token
    */
    inline void setSessionToken(const std::string& session_token)
    {
        session_token_ = session_token;
    }

    /**
    * Sets the underlying session tokens expiration. Copies from parameter expiration
    */
    inline void setExpiration(const std::chrono::duration<uint64_t> expiration)
    {
        expiration_ = expiration;
    }

    Credentials& operator=(const Credentials& other) {
        access_key_ = other.getAccessKey();
        secret_key_ = other.getSecretKey();
        session_token_ = other.getSessionToken();
        expiration_ = other.expiration_;

        return *this;
    }

private:
    std::string access_key_;
    std::string secret_key_;
    std::string session_token_;
    std::chrono::duration<uint64_t> expiration_;
};

class CredentialProvider {
public:
    using callback_t = AuthCallbacks;
    virtual void getCredentials(Credentials& credentials);
    virtual void getUpdatedCredentials(Credentials& credentials);
    virtual ~CredentialProvider();

    /**
     * Gets the callbacks
     *
     * @param PClientCallbacks - Required client callbacks to create auth callbacks
     *
     * @return AuthCallbacks
     */
    virtual callback_t getCallbacks(PClientCallbacks);

    /**
     * @return Kinesis Video credential provider default implementation
     */
    virtual GetDeviceCertificateFunc getDeviceCertificateCallback();

    /**
     * @return Kinesis Video credential provider default implementation
     */
    virtual GetDeviceFingerprintFunc getDeviceFingerPrintCallback();

    /**
     * @return Kinesis Video credential provider default implementation
     */
    virtual GetSecurityTokenFunc getSecurityTokenCallback();
    /**
     * @return Kinesis Video credential provider default implementation
     */
    virtual GetStreamingTokenFunc getStreamingTokenCallback();

    /**
     * @return Kinesis Video credential provider default implementation
     */
    virtual DeviceCertToTokenFunc deviceCertToTokenCallback();

protected:
    CredentialProvider();

    const std::chrono::duration<uint64_t> CredentialProviderGracePeriod = std::chrono::seconds(5 + (MIN_STREAMING_TOKEN_EXPIRATION_DURATION + STREAMING_TOKEN_EXPIRATION_GRACE_PERIOD) / HUNDREDS_OF_NANOS_IN_A_SECOND);

private:
    void refreshCredentials(bool forceUpdate = false);

    virtual void updateCredentials(Credentials& credentials) = 0;

    std::mutex credential_mutex_;
    std::chrono::duration<uint64_t> next_rotation_time_;
    Credentials credentials_;
    /**
     * Storage for the serialized security token
     */
    PAwsCredentials security_token_;

    callback_t callbacks_;

    static STATUS getStreamingTokenHandler(UINT64, PCHAR, STREAM_ACCESS_MODE, PServiceCallContext);
    static STATUS getSecurityTokenHandler(UINT64, PBYTE*, PUINT32, PUINT64);
};

class EmptyCredentialProvider : public CredentialProvider {
private:
    void updateCredentials(Credentials& credentials) override {
        credentials.setAccessKey("");
        credentials.setSecretKey("");
        credentials.setSessionToken("");
        credentials.setExpiration(std::chrono::seconds(MAX_UINT64));
    }
};

class StaticCredentialProvider : public CredentialProvider {
public:

    StaticCredentialProvider(const Credentials& credentials) : credentials_(credentials) {}

protected:

    void updateCredentials(Credentials& credentials) override {
        // Copy the stored creds forward
        credentials = credentials_;

        // Always use maximum expiration for static credentials
        credentials.setExpiration(std::chrono::seconds(MAX_UINT64));
    }

    const Credentials credentials_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
