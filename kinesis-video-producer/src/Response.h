#pragma once

#include <com/amazonaws/kinesis/video/client/Include.h>
#include <curl/curl.h>
#include <mutex>
#include <thread>
#include "Response.h"
#include "Auth.h"
#include "Logger.h"
#include "Request.h"

// forward-declare CURL types to restrict visibility of curl.h
extern "C" {
typedef void CURL;
struct curl_slist;
}

namespace com { namespace amazonaws { namespace kinesis { namespace video {
#define HTTP_OK 200

// Setting this timeout to terminate CURL connection
#define TIMEOUT_AFTER_STREAM_STOPPED 1L

/// Provides an interface for retrieving HTTP response data.
class Response {
public:
    typedef Request::HeaderMap HeaderMap;

    static std::shared_ptr<Response> create(Request &request);

    ~Response();

    std::chrono::system_clock::time_point
    getStartTime() const; ///< Get the response start time (start of request processing).
    std::chrono::system_clock::time_point getEndTime() const; ///< Get the response completion time.
    const std::string* getHeader(const std::string &header) const; ///< Get header value. Returns null if not set.
    const HeaderMap &getHeaders() const; ///< Get a map reponse headers.
    const std::string &getData() const; ///< Get the response payload.
    int getStatusCode() const; ///< Get the response status code.
    SERVICE_CALL_RESULT getServiceCallResult() const; ///< Get the response overall result as a service call result.

    void completeSync();

    // Force closes the CURL connection
    void terminate();

    /**
     * Convenience method to convert HTTP statuses to SERVICE_CALL_RESULT status.
     *
     * @param http_status the HTTP status code of the call
     * @return The HTTP status translated into a SERVICE_CALL_RESULT value.
     */
    static SERVICE_CALL_RESULT getServiceCallResultFromHttpStatus(int http_status);

    /**
     * Convenience method to convert CURL return status to SERVICE_CALL_RESULT status.
     *
     * @param curl_status the CURL status code of the call
     * @return The CURL code translated into a SERVICE_CALL_RESULT value.
     */
    static SERVICE_CALL_RESULT getServiceCallResultFromCurlStatus(CURLcode curl_status);

private:
    Response();

    void closeCurlHandles();

    // noncopyable
    Response(const Response &);

    Response &operator=(const Response &);

    std::mutex termination_mutex_;
    CURL *curl_;
    volatile bool terminated_;
    char error_buffer_[CURL_ERROR_SIZE];
    curl_slist *request_headers_;
    HeaderMap response_headers_;
    std::string response_;
    long http_status_code_;
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    SERVICE_CALL_RESULT service_call_result_;
};

enum HTTP_STATUS {
    OK = 200
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
