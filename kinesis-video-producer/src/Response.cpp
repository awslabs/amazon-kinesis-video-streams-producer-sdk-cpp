#include "Response.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

namespace {

using namespace com::amazonaws::kinesis::video;
using std::string;

static inline void ltrim(string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](int c) {
                return !std::isspace(c);
            }));
}

static inline void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         [](int c) {
                             return !std::isspace(c);
                         }).base(),
            s.end());
}

static inline void itrim(string &s) {
    s.erase(std::unique(s.begin(),
                        s.end(),
                        [](char lhs, char rhs) {
                            return (lhs == rhs) && std::isspace(static_cast<int>(lhs));
                        }),
            s.end());
}

static inline void trim_all(string &s) {
    ltrim(s);
    rtrim(s);
    itrim(s);
}

size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    Response::HeaderMap &response_headers = *static_cast<Response::HeaderMap *>(userp);
    size_t dataSize = size * nmemb;

    const char *headerDelim = static_cast<const char *>(std::memchr(ptr, ':', dataSize));
    if (headerDelim) {
        // parse the header name
        size_t headerSize = headerDelim - (char *) ptr;
        std::string header((char *) ptr, headerSize);

        // exclude the delimiter from the header map
        ++headerDelim;

        // parse the header value
        size_t valueSize = dataSize - headerSize - 1;
        std::string value(headerDelim, valueSize);
        trim_all(value);

        // insert header into map
        response_headers[header] = value;
    }

    return dataSize;
}

size_t write_response_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    std::string &response = *static_cast<std::string *>(userp);
    size_t dataSize = size * nmemb;
    response.append((char *) ptr, dataSize);
    return dataSize;
}

} // anonymous namespace

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::shared_ptr;
using std::move;

shared_ptr<Response> Response::create(Request &request) {

    // create curl handle
    shared_ptr<Response> response(new Response());
    response->curl_ = curl_easy_init();

    // set up the friendly error message buffer
    response->error_buffer_[0] = '\0';
    curl_easy_setopt(response->curl_, CURLOPT_ERRORBUFFER, response->error_buffer_);

    curl_easy_setopt(response->curl_, CURLOPT_URL, request.get_url().c_str());
    curl_easy_setopt(response->curl_, CURLOPT_NOSIGNAL, 1);

    // Curl will abort if the connection drops to below 10 bytes/s for 10 seconds.
    curl_easy_setopt(response->curl_, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(response->curl_, CURLOPT_LOW_SPEED_LIMIT, 10L);

    // add headers
    for (HeaderMap::const_iterator i = request.getHeaders().begin(); i != request.getHeaders().end(); ++i) {
        std::ostringstream oss;
        oss << i->first << ": " << i->second;
        response->request_headers_ = curl_slist_append(response->request_headers_, oss.str().c_str());
    }
    curl_easy_setopt(response->curl_, CURLOPT_HTTPHEADER, response->request_headers_);

    // set no verification for SSL connections
    if (request.getScheme() == "https") {
        // Use the default cert store at /etc/ssl in most common platforms
        // TODO have a way to set the certificate store path
        //curl_easy_setopt(response->m_curl, CURLOPT_CAPATH, >>> client should pass in path <<<);

        // Enforce the public cert verification - even though this is the default
        curl_easy_setopt(response->curl_, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(response->curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(response->curl_, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    }

    // set request completion timeout in milliseconds
    long completion_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
            request.getRequestCompletionTimeout()).count();
    curl_easy_setopt(response->curl_, CURLOPT_TIMEOUT_MS, completion_timeout);

    long connection_timeout = std::chrono::duration_cast<std::chrono::seconds>(
            request.getConnectionTimeout()).count();

    curl_easy_setopt(response->curl_, CURLOPT_CONNECTTIMEOUT, connection_timeout);
    curl_easy_setopt(response->curl_, CURLOPT_TCP_NODELAY, 1);

    // set header callback
    curl_easy_setopt(response->curl_, CURLOPT_HEADERFUNCTION, write_header_callback);
    curl_easy_setopt(response->curl_, CURLOPT_HEADERDATA, &response->response_headers_);

    if (request.getVerb() == Request::GET) {
        curl_easy_setopt(response->curl_, CURLOPT_HTTPGET, 1L);
    } else if (request.getVerb() == Request::PUT) {
        curl_easy_setopt(response->curl_, CURLOPT_PUT, 1L);
    } else if (request.getVerb() == Request::POST) {
        curl_easy_setopt(response->curl_, CURLOPT_POST, 1L);
        if (request.isStreaming()) {
            // Set the read callback from the request
            curl_easy_setopt(response->curl_, CURLOPT_HEADERFUNCTION, request.getPostHeaderCallback());
            curl_easy_setopt(response->curl_, CURLOPT_HEADERDATA, &request);

            // Set the read callback from the request
            curl_easy_setopt(response->curl_, CURLOPT_READFUNCTION, request.getPostReadCallback());
            curl_easy_setopt(response->curl_, CURLOPT_READDATA, &request);

            // Set the write callback from the request
            curl_easy_setopt(response->curl_, CURLOPT_WRITEFUNCTION, request.getPostWriteCallback());
            curl_easy_setopt(response->curl_, CURLOPT_WRITEDATA, &request);
        } else {
            // Set the read callback
            curl_easy_setopt(response->curl_, CURLOPT_POSTFIELDSIZE, request.getBody().size());
            curl_easy_setopt(response->curl_, CURLOPT_POSTFIELDS, request.getBody().c_str());

            // Set response callback
            curl_easy_setopt(response->curl_, CURLOPT_WRITEFUNCTION, write_response_callback);
            curl_easy_setopt(response->curl_, CURLOPT_WRITEDATA, &response->response_);
        }
    }

    return response;
}

Response::Response()
        : curl_(NULL),
          request_headers_(NULL),
          http_status_code_(0),
          terminated_(false),
          service_call_result_(SERVICE_CALL_RESULT_OK),
          start_time_(std::chrono::system_clock::now()) {
}

Response::~Response() {
    closeCurlHandles();
}

void Response::completeSync() {
    CURLcode result = curl_easy_perform(curl_);
    if (terminated_) {
        // The transmission has been force terminated.
        http_status_code_ = OK;
        service_call_result_ = SERVICE_CALL_RESULT_OK;
    } else {
        if (result != CURLE_OK) {
            const char *url;
            curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &url);
            LOG_ERROR("curl perform failed for url " << url
                                                     << " with result "
                                                     << curl_easy_strerror(result)
                                                     << ": "
                                                     << error_buffer_);
            service_call_result_ = getServiceCallResultFromCurlStatus(result);
        } else {
            // get the response code and note the request completion time
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_status_code_);
            service_call_result_ = getServiceCallResultFromHttpStatus(http_status_code_);
        }
    }

    end_time_ = std::chrono::system_clock::now();

    // warn and log request/response info if there was an error return code
    if (OK != http_status_code_) {
        const char *url;
        curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &url);
        std::ostringstream headers;
        for (const curl_slist *header = request_headers_; header; header = header->next) {
            headers << "\n    " << header->data;
        }
        LOG_WARN("HTTP Error " << http_status_code_ << ": Response: " << response_ << "\nRequest URL: "
                               << url << "\nRequest Headers:" << headers.str());
    }

    // release any curl handles
    closeCurlHandles();
}

void Response::terminate() {
    LOG_INFO("Force stopping the curl connection");

    // Currently, it seems that the only "good" way to stop CURL is to set
    // the timeout to a small value which will timeout the connection.
    terminated_ = true;
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, TIMEOUT_AFTER_STREAM_STOPPED);
}

void Response::closeCurlHandles() {
    std::unique_lock<std::mutex> lock(termination_mutex_);
    if (request_headers_) {
        curl_slist_free_all(request_headers_);
        request_headers_ = NULL;
    }
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }
}

std::chrono::system_clock::time_point Response::getStartTime() const {
    return start_time_;
}

std::chrono::system_clock::time_point Response::getEndTime() const {
    return end_time_;
}

const std::string *Response::getHeader(const std::string &header) const {
    HeaderMap::const_iterator i = response_headers_.find(header);
    if (response_headers_.end() == i) {
        return NULL;
    }
    return &i->second;
}

const Response::HeaderMap &Response::getHeaders() const {
    return response_headers_;
}

const std::string &Response::getData() const {
    return response_;
}

int Response::getStatusCode() const {
    return http_status_code_;
}

SERVICE_CALL_RESULT Response::getServiceCallResult() const {
    return service_call_result_;
}

inline SERVICE_CALL_RESULT Response::getServiceCallResultFromHttpStatus(int http_status) {
    switch (http_status) {
        case SERVICE_CALL_RESULT_OK:
        case SERVICE_CALL_INVALID_ARG:
        case SERVICE_CALL_RESOURCE_NOT_FOUND:
        case SERVICE_CALL_FORBIDDEN:
        case SERVICE_CALL_RESOURCE_DELETED:
        case SERVICE_CALL_NOT_AUTHORIZED:
        case SERVICE_CALL_NOT_IMPLEMENTED:
        case SERVICE_CALL_INTERNAL_ERROR:
            return static_cast<SERVICE_CALL_RESULT>(http_status);
        default:
            return SERVICE_CALL_UNKNOWN;
    }
}

inline SERVICE_CALL_RESULT Response::getServiceCallResultFromCurlStatus(CURLcode curl_status) {
    switch (curl_status) {
        case CURLE_OK:
            return SERVICE_CALL_RESULT_OK;
        case CURLE_UNSUPPORTED_PROTOCOL:
            return SERVICE_CALL_INVALID_ARG;
        case CURLE_OPERATION_TIMEDOUT:
            return SERVICE_CALL_NETWORK_CONNECTION_TIMEOUT;
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CACERT:
            return SERVICE_CALL_NOT_AUTHORIZED;
        default:
            return SERVICE_CALL_UNKNOWN;
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
