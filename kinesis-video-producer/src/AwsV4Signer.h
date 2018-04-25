#pragma once

#include "Auth.h"
#include <memory>
#include <string>
#include <vector>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/// Implements the AWS V4 signing procedure.
///
/// See http://docs.aws.amazon.com/general/latest/gr/sigv4_signing.html
class AwsV4Signer : public RequestSigner {
public:
    static std::unique_ptr<const RequestSigner> Create(
            const std::string &region,
            const std::string &service,
            std::unique_ptr<CredentialProvider> credentialProvider);

    /// Create an instance which implements the signing procedure used by streaming calls.
    static std::unique_ptr<const RequestSigner> CreateStreaming(
            const std::string &region,
            const std::string &service,
            std::unique_ptr<CredentialProvider> credentialProvider);

    virtual void signRequest(Request &request) const;

private:
    typedef std::vector<uint8_t> byte_vector;

    enum SigningVariant {
        V4Standard,
        V4Streaming
    };

    AwsV4Signer(
            const std::string &region,
            const std::string &service,
            std::unique_ptr<CredentialProvider> credentialProvider,
            SigningVariant signingVariant);

    static std::string generateTimestamp(const char *formatString);

    static void generateHMAC(
            const byte_vector &key,
            const std::string &message,
            byte_vector &out);

    static std::string hashStringSHA256(const std::string &data);

    static std::string hexEncode(const byte_vector &input);

    static bool isCanonicalHeader(const std::string &headerName);

    std::string generateCredentialScope(const std::string &dateString) const;

    std::string generateCanonicalRequest(const Request &request) const;

    std::string generateCanonicalURI(const Request &request) const;

    std::string generateCanonicalQuery(const Request &request) const;

    std::string generateCanonicalHeaders(const Request::HeaderMap &headers) const;

    std::string generateSignedHeaderList(const Request::HeaderMap &headers) const;

    const std::string& region_;                                   ///< The service region.
    const std::string& service_;                                  ///< The service name.
    std::unique_ptr<CredentialProvider> credential_provider_;   ///< The credential provider.
    SigningVariant signing_variant_;                        ///< The signing variant to use.
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
