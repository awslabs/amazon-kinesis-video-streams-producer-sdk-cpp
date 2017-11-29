#pragma once

#include <map>
#include <string>
#include <memory>
#include <chrono>
#include <strings.h>
#include "com/amazonaws/kinesis/video/common/CommonDefs.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class Response;

/// Provides an interface for setting HTTP request parameters.
///
/// Requests are executed by the CurlCallManager::call or callAsync.
class Request {
public:
    typedef size_t (*CurlHeaderCallbackFn)(char *, size_t, size_t, void *);
    typedef size_t (*CurlReadCallbackFn)(char *, size_t, size_t, void *);
    typedef size_t (*CurlWriteCallbackFn)(char *, size_t, size_t, void *);

    enum Verb {
        GET, POST, PUT
    };

    /// Used to sort header keys using case-insensitive comparisons.
    struct icase_less {
        bool operator()(const std::string &lhs, const std::string &rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

    typedef std::map<std::string, std::string, icase_less> HeaderMap; ///< Map of header names to values.

    Request(Verb verb, const std::string &url);

    Request(Verb verb, const std::string &url,
            CurlHeaderCallbackFn post_header_callback,
            CurlReadCallbackFn post_read_callback,
            void *post_read_callback_custom_data,
            CurlWriteCallbackFn post_write_callback = nullptr,
            void *post_write_callback_custom_data = nullptr);

    virtual ~Request();

    void setBody(const void *body, size_t body_size); ///< Set the request body.
    void setBody(const std::string &body); ///< Set the request body.
    void setHeader(const std::string &header_name, const std::string &header_value); ///< Set a header value.
    void setRequestCompletionTimeout(
            std::chrono::duration<double, std::milli> timeout); ///< Set the request timeout duration.
    void setConnectionTimeout(
            std::chrono::duration<double, std::milli> timeout); ///< Set the connect timeout duration.
    void setUrl(const std::string &url); ///< Set the request URL.
    void setVerb(Verb verb); ///< Set the HTTP request method.

    const std::string &getBody() const; ///< Get the request body.
    const std::chrono::system_clock::time_point getCreationTime() const; ///< Get the request creation time.
    const std::string *getHeader(const std::string &header) const; ///< Get header value. Returns null if not set.
    const HeaderMap &getHeaders() const; ///< Get a map of all headers set on this request.
    const std::chrono::duration<double, std::milli> getRequestCompletionTimeout() const; ///< Get the request timeout duration.
    const std::chrono::duration<double, std::milli> getConnectionTimeout() const; ///< Get the connection timeout duration.
    const std::string &get_url() const; ///< Get the full request URL.
    Verb getVerb() const; ///< Get the HTTP request method.

    std::string getScheme() const; ///< Get the scheme portion of the URL.
    std::string getHost() const; ///< Get the host portion of the URL.
    std::string getPath() const; ///< Get the path portion of the URL.
    std::string getQuery() const; ///< Get the query  portion of the URL.

    bool isStreaming() const; ///< Return true if the request transfers the body as chunks; false otherwise.

    /**
     * @return A function pointer conforming to: https://curl.haxx.se/libcurl/c/CURLOPT_READFUNCTION.html
     */
    CurlReadCallbackFn getPostReadCallback() const;

    /**
     * @return A function pointer conforming to: https://curl.haxx.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
     */
    CurlReadCallbackFn getPostHeaderCallback() const;

    /**
     * @return A custom data pointer which will be passed as the 4th argument to the function returned by
     *         getPostReadCallback().
     */
    void *getPostReadCallbackCustomData() const;

    /**
     * @return A function pointer conforming to: https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
     */
    CurlWriteCallbackFn getPostWriteCallback() const;

    /**
     * @return A custom data pointer which will be passed as the 4th argument to the function returned by
     *         getPostWriteCallback().
     */
    void *getPostWriteCallbackCustomData() const;

private:
    Request();

    // noncopyable
    Request(const Request &);
    Request &operator=(const Request &);

    const std::chrono::system_clock::time_point creation_time_;
    Verb verb_;
    std::string url_;
    HeaderMap headers_;
    std::string body_;
    std::chrono::duration<double, std::milli> request_completion_timeout_;
    std::chrono::duration<double, std::milli> connection_timeout_;

    bool is_streaming_;
    CurlReadCallbackFn post_read_callback_;
    CurlHeaderCallbackFn post_header_callback_;
    void *post_read_callback_custom_data_;
    CurlWriteCallbackFn post_write_callback_;
    void *post_write_callback_custom_data_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
