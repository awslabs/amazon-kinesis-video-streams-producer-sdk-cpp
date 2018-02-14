#include "CurlCallManager.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::unique_ptr;
using std::shared_ptr;
using std::move;

CurlCallManager &CurlCallManager::getInstance() {
    static CurlCallManager singleton_instance;
    return singleton_instance;
}

CurlCallManager::CurlGlobalInitializer::CurlGlobalInitializer() {
    LOG_INFO("Initializing curl.");
    signal(SIGPIPE, SIG_IGN);
    curl_global_init(CURL_GLOBAL_ALL);
}

CurlCallManager::CurlGlobalInitializer::~CurlGlobalInitializer() {
    curl_global_cleanup();
    LOG_INFO("Curl shutdown.");
}

CurlCallManager::CurlCallManager() {
}

CurlCallManager::~CurlCallManager() {
}

shared_ptr<Response> CurlCallManager::call(unique_ptr<Request> request,
                                           unique_ptr<const RequestSigner> request_signer) const {
    return call(move(request), move(request_signer), nullptr);
}

shared_ptr<Response> CurlCallManager::call(unique_ptr<Request> request,
                                           unique_ptr<const RequestSigner> request_signer,
                                           std::shared_ptr<OngoingStreamState> ongoing_state) const {
    move(request_signer)->signRequest(*request);
    shared_ptr<Response> response = Response::create(*request);

    // Set the response object on state
    if (nullptr != ongoing_state) {
        ongoing_state->setResponse(response);
    }

    // Perform sync call
    response->completeSync();

    return response;
}

void CurlCallManager::dumpCurlEasyInfo(CURL *curl_easy) {
    double tt;
    CURLcode ret;

    ret = curl_easy_getinfo(curl_easy, CURLINFO_TOTAL_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("TOTAL_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump TOTAL_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_NAMELOOKUP_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("NAMELOOKUP_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump NAMELOOKUP_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_CONNECT_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("CONNECT_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump CONNECT_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_APPCONNECT_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("APPCONNECT_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump APPCONNECT_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_PRETRANSFER_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("PRETRANSFER_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump PRETRANSFER_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_STARTTRANSFER_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("STARTTRANSFER_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump STARTTRANSFER_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_REDIRECT_TIME, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("REDIRECT_TIME: " << tt);
    } else {
        LOG_WARN("Unable to dump REDIRECT_TIME");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_SPEED_UPLOAD, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("CURLINFO_SPEED_UPLOAD: " << tt);
    } else {
        LOG_WARN("Unable to dump CURLINFO_SPEED_UPLOAD");
    }

    ret = curl_easy_getinfo(curl_easy, CURLINFO_SIZE_UPLOAD, &tt);
    if (ret == CURLE_OK) {
        LOG_INFO("CURLINFO_SIZE_UPLOAD: " << tt);
    } else {
        LOG_WARN("Unable to dump CURLINFO_SIZE_UPLOAD");
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
