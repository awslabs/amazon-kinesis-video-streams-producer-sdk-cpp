#include "AwsV4Signer.h"
#include "Response.h"
#include "Logger.h"
#include <fstream>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <cstdio>

namespace {
    const std::string HEADER_AMZ_DATE = "X-Amz-Date";
    const std::string HEADER_AMZ_SECURITY_TOKEN = "x-amz-security-token";
    const std::string HEADER_AUTH = "Authorization";
    const std::string HEADER_HOST = "Host";

    const std::string ALGORITHM = "AWS4-HMAC-SHA256";
    const std::string SIGNATURE_END = "aws4_request";
} // anonymous namespace

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::unique_ptr;
using std::move;

unique_ptr<const RequestSigner> AwsV4Signer::Create(
        const std::string &region,
        const std::string &service,
        unique_ptr<CredentialProvider> credentialProvider) {

    return unique_ptr<const RequestSigner>(new AwsV4Signer(
            region,
            service,
            move(credentialProvider),
            AwsV4Signer::V4Standard));
}

unique_ptr<const RequestSigner> AwsV4Signer::CreateStreaming(
        const std::string &region,
        const std::string &service,
        unique_ptr<CredentialProvider> credentialProvider) {

    return unique_ptr<const RequestSigner>(new AwsV4Signer(
            region,
            service,
            move(credentialProvider),
            AwsV4Signer::V4Streaming));
}

AwsV4Signer::AwsV4Signer(
        const std::string &region,
        const std::string &service,
        unique_ptr<CredentialProvider> credential_provider,
        SigningVariant signing_variant)
        : region_(region),
          service_(service),
          signing_variant_(signing_variant) {
    
    credential_provider_ = move(credential_provider);
}

void AwsV4Signer::signRequest(Request &request) const {
    // retrieve credentials from provider
    Credentials credentials;
    credential_provider_->getCredentials(credentials);

    std::string date_time_string = generateTimestamp("%Y%m%dT%H%M%SZ");
    std::string dateString = date_time_string.substr(0, 8);

    // ensure that host and date headers are set as they're required fields in the canonical header string
    request.setHeader(HEADER_HOST, request.getHost());
    request.setHeader(HEADER_AMZ_DATE, date_time_string);

    // create V4 string to sign
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-create-string-to-sign.html
    std::string credentialScope = generateCredentialScope(dateString);
    std::string canonicalRequest = generateCanonicalRequest(request);
    std::ostringstream stringToSign;
    stringToSign << ALGORITHM << '\n'
                 << date_time_string << '\n'
                 << credentialScope << '\n'
                 << hashStringSHA256(canonicalRequest);

    // create V4 signature
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-calculate-signature.html
    std::string signatureStart = "AWS4" + credentials.getSecretKey();
    byte_vector hmac(signatureStart.begin(), signatureStart.end());
    generateHMAC(hmac, dateString, hmac);
    generateHMAC(hmac, region_, hmac);
    generateHMAC(hmac, service_, hmac);
    generateHMAC(hmac, SIGNATURE_END, hmac);
    generateHMAC(hmac, stringToSign.str(), hmac);
    std::string signature = hexEncode(hmac);

    // set the auth header
    // http://docs.aws.amazon.com/general/latest/gr/sigv4-add-signature-to-request.html
    std::ostringstream authHeader;
    authHeader << ALGORITHM << " Credential=" << credentials.getAccessKey() << '/' << credentialScope << ", "
               << "SignedHeaders=" << generateSignedHeaderList(request.getHeaders()) << ", "
               << "Signature=" << signature;
    request.setHeader(HEADER_AUTH, authHeader.str());

    // set the security token header if provided
    if (!credentials.getSessionToken().empty()) {
        request.setHeader(HEADER_AMZ_SECURITY_TOKEN, credentials.getSessionToken());
    }
}


std::string AwsV4Signer::generateTimestamp(const char *format_string) {
    auto now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timestamp[18] = {0};
    strftime(reinterpret_cast<char *>(&timestamp), 18, format_string, std::gmtime(&now_time));
    return std::string(timestamp);
}


void AwsV4Signer::generateHMAC(const byte_vector &key, const std::string &message, byte_vector &out) {
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmacLength = 0;

    const EVP_MD *evp = EVP_sha256();
    ::HMAC(evp, 
        &key[0],
        (INT32)key.size(),
        reinterpret_cast<const unsigned char *>(message.c_str()),
        message.size(),
        hmac,
        &hmacLength);
    out.assign(hmac, hmac + hmacLength);
}

std::string AwsV4Signer::hashStringSHA256(const std::string &message) {
    byte_vector hash;
    hash.resize(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const uint8_t *>(message.data()), message.size(), hash.data());
    return hexEncode(hash);
}


std::string AwsV4Signer::hexEncode(const byte_vector &input) {
    static const char hex_char[] = "0123456789abcdef";

    std::string dest;

    dest.clear();
    dest.resize(input.size() * 2);

    for (size_t i = 0; i < input.size(); ++i) {
        // High nybble
        dest[i << 1] = hex_char[(input[i] >> 4) & 0x0f];
        // Low nybble
        dest[(i << 1) + 1] = hex_char[input[i] & 0x0f];
    }

    return dest;
}


bool AwsV4Signer::isCanonicalHeader(const std::string &headerName) {
    return headerName != HEADER_AMZ_SECURITY_TOKEN
           && headerName != HEADER_AUTH;
}

std::string AwsV4Signer::generateCredentialScope(const std::string &dateString) const {
    std::ostringstream credentialScope;
    credentialScope << dateString << '/' << region_ << '/' << service_ << '/' << SIGNATURE_END;
    return credentialScope.str();
}

std::string AwsV4Signer::generateCanonicalRequest(const Request &request) const {
    const char *requestMethod = NULL;
    switch (request.getVerb()) {
        case Request::GET:
            requestMethod = "GET";
            break;
        case Request::POST:
            requestMethod = "POST";
            break;
        case Request::PUT:
            requestMethod = "PUT";
            break;
        default:
            throw std::runtime_error("unrecognized request method");
    }

    // http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html
    std::ostringstream canonicalRequest;
    canonicalRequest << requestMethod << '\n'
                     << generateCanonicalURI(request) << '\n'
                     << generateCanonicalQuery(request) << '\n'
                     << generateCanonicalHeaders(request.getHeaders()) << '\n'
                     << generateSignedHeaderList(request.getHeaders()) << '\n';

    if (V4Streaming == signing_variant_) {
        // Streaming treats this portion as if the body were empty
        canonicalRequest << hashStringSHA256(std::string());
    } else {
        // standard signing
        canonicalRequest << hashStringSHA256(request.getBody());
    }

    return canonicalRequest.str();
}

std::string AwsV4Signer::generateCanonicalURI(const Request &request) const {
    const std::string &path = request.getPath();
    if (path.empty()) {
        return "/";
    }
    return path;
}

std::string AwsV4Signer::generateCanonicalQuery(const Request &request) const {
    const std::string &query = request.getQuery();

    // add query params to a vector
    std::vector<std::string> paramList;
    for (size_t paramPos = 0; paramPos < query.size();) {
        // add the next parameter to the vector
        size_t nextParamPos = query.find('&', paramPos);
        paramList.push_back(query.substr(paramPos, nextParamPos - paramPos));
        paramPos = nextParamPos;

        // skip the param delimiter
        if (nextParamPos != std::string::npos) {
            ++paramPos;
        }
    }

    // sort the query params
    std::sort(paramList.begin(), paramList.end(), Request::icase_less());

    // create the canonical query
    std::ostringstream canonicalQuery;
    for (size_t i = 0; i < paramList.size(); ++i) {
        if (0 != i) {
            canonicalQuery << '&';
        }
        canonicalQuery << paramList[i];
    }
    return canonicalQuery.str();
}

inline std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    std::string out = str.substr(first, (last - first + 1));
    return out;
}

inline std::string to_lower_copy(const std::string &str) {
    std::string cpy = str;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), ::tolower);
    return cpy;
}

std::string AwsV4Signer::generateCanonicalHeaders(const Request::HeaderMap &headers) const {
    std::ostringstream canonicalHeaders;
    for (Request::HeaderMap::const_iterator i = headers.begin(); i != headers.end(); ++i) {
        const std::string headerName = to_lower_copy(i->first);
        const std::string headerValue = trim(i->second);
        if (isCanonicalHeader(headerName)) {
            canonicalHeaders << headerName << ':' << headerValue << '\n';
        }
    }
    return canonicalHeaders.str();
}

std::string AwsV4Signer::generateSignedHeaderList(const Request::HeaderMap &headers) const {
    std::ostringstream signedHeaders;
    for (Request::HeaderMap::const_iterator i = headers.begin(); i != headers.end(); ++i) {
        const std::string &headerName = i->first;
        if (isCanonicalHeader(headerName)) {
            if (signedHeaders.tellp() > 0) {
                signedHeaders << ';';
            }
            signedHeaders << to_lower_copy(headerName);
        }
    }
    return signedHeaders.str();
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
