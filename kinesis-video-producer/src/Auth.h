#pragma once

#include "Request.h"

#include <chrono>
#include <mutex>

#include <cstdlib>
#include <cstring>
#include <assert.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Simple data object around aws credentials
*/
class Credentials {
public:
    Credentials() {}

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

struct SerializedCredentials {
    uint32_t access_key_offset_;
    uint32_t access_key_length_;
    uint32_t secret_key_offset_;
    uint32_t secret_key_length_;
    uint32_t session_token_offset_;
    uint32_t session_token_length_;
    uint64_t expiration_seconds_;

public:

    static void serialize(const Credentials &credentials, uint8_t **ppBuffer, uint32_t *pSize) {
        auto access_key = credentials.getAccessKey();
        auto access_key_len = access_key.length();
        auto secret_key = credentials.getSecretKey();
        auto secret_key_len = secret_key.length();
        auto session_token = credentials.getSessionToken();
        auto session_token_len = session_token.length();
        auto expiration_seconds = credentials.getExpiration().count();

        auto size = sizeof(SerializedCredentials) + access_key_len + secret_key_len + session_token_len;

        char *pBuffer = reinterpret_cast<char *>(malloc(size));

        if (!pBuffer) {
            throw std::runtime_error("Failed to allocate a buffer for the serialized credentials.");
        }

        SerializedCredentials *serializedCredentials = reinterpret_cast<SerializedCredentials *>(pBuffer);

        serializedCredentials->access_key_offset_ = sizeof(SerializedCredentials);
        serializedCredentials->access_key_length_ = access_key_len;
        serializedCredentials->secret_key_offset_ =
                serializedCredentials->access_key_offset_ + serializedCredentials->access_key_length_;
        serializedCredentials->secret_key_length_ = secret_key_len;
        serializedCredentials->session_token_offset_ =
                serializedCredentials->secret_key_offset_ + serializedCredentials->secret_key_length_;
        serializedCredentials->session_token_length_ = session_token_len;
        serializedCredentials->expiration_seconds_ = expiration_seconds;

        char *pCurPtr = pBuffer + serializedCredentials->access_key_offset_;
        std::memcpy(pCurPtr, access_key.c_str(), access_key_len);
        pCurPtr += access_key_len;
        std::memcpy(pCurPtr, secret_key.c_str(), secret_key_len);
        pCurPtr += secret_key_len;
        std::memcpy(pCurPtr, session_token.c_str(), session_token_len);
        pCurPtr += session_token_len;
        assert(pCurPtr <= pBuffer + size);

        *ppBuffer = reinterpret_cast<uint8_t *>(pBuffer);
        *pSize = size;
    }

    static void deSerialize(uint8_t *pBuffer, uint32_t size, Credentials &credentials) {
        std::string accessKey = "";
        std::string secretKey = "";
        std::string sessionToken = "";
        std::chrono::duration<uint64_t> expiration = std::chrono::seconds(MAX_UINT64);

        if (pBuffer != NULL && size != 0) {
            // Extract the credentials only if the buffer is not empty
            SerializedCredentials *pCreds = reinterpret_cast<SerializedCredentials *>(pBuffer);
            if (sizeof(SerializedCredentials) > size ||
                    (uint64_t) pCreds->access_key_offset_ + pCreds->access_key_length_ > size ||
                    (uint64_t) pCreds->secret_key_offset_ + pCreds->secret_key_length_ > size ||
                    (uint64_t) pCreds->session_token_offset_ + pCreds->session_token_length_ > size) {
                throw std::runtime_error("invalid serialized credentials.");
            }

            accessKey = std::string(reinterpret_cast<char *>(pBuffer + pCreds->access_key_offset_),
                                    pCreds->access_key_length_);
            secretKey = std::string(reinterpret_cast<char *>(pBuffer + pCreds->secret_key_offset_),
                                    pCreds->secret_key_length_);
            sessionToken = std::string(reinterpret_cast<char *>(pBuffer + pCreds->session_token_offset_),
                                       pCreds->session_token_length_);
            expiration = std::chrono::seconds(pCreds->expiration_seconds_);
        }

        credentials.setAccessKey(accessKey);
        credentials.setSecretKey(secretKey);
        credentials.setSessionToken(sessionToken);
        credentials.setExpiration(expiration);
    }
};

class CredentialProvider {
public:
    void getCredentials(Credentials& credentials);
    void getUpdatedCredentials(Credentials& credentials);
    virtual ~CredentialProvider() {}
    
protected:
    CredentialProvider();

private:
    void refreshCredentials(bool forceUpdate = false);

    virtual void updateCredentials(Credentials& credentials) = 0;

    std::mutex credential_mutex_;
    std::chrono::duration<uint64_t> next_rotation_time_;
    Credentials credentials_;
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
        credentials = credentials_;
    }

    const Credentials credentials_;
};

class RequestSigner {
public:

    virtual ~RequestSigner() {}

    /// Sign the request by setting appropriate auth headers.
    virtual void signRequest(Request &request) const = 0;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
