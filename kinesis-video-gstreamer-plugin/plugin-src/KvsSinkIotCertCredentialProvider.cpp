#include "KvsSinkIotCertCredentialProvider.h"
#include <json/json.h>
#include <iomanip>
#include "Util/KvsSinkUtil.h"

#define REQUEST_COMPLETION_TIMEOUT_MS 3000
#define CURL_ERROR_SIZE 256
#define HTTP_OK 200
#define SERVICE_CALL_PREFIX "https://"
#define ROLE_ALIASES_PATH "/role-aliases"
#define CREDENTIAL_SERVICE "/credentials"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

size_t write_response_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    std::string &response = *static_cast<std::string *>(userp);
    size_t dataSize = size * nmemb;
    response.append((char *) ptr, dataSize);
    return dataSize;
}

void KvsSinkIotCertCredentialProvider::dumpCurlEasyInfo(CURL *curl_easy) {
    double time_in_seconds;
    CURLcode ret;

    ret = curl_easy_getinfo(curl_easy, CURLINFO_TOTAL_TIME, &time_in_seconds);
    if (ret == CURLE_OK) {
        LOG_INFO("CURL_TOTAL_TIME: " << time_in_seconds << " seconds.");
    } else {
        LOG_WARN("Unable to dump TOTAL_TIME");
    }
}

void KvsSinkIotCertCredentialProvider::updateCredentials(Credentials &credentials) {

    CURL *curl;
    CURLcode res;
    std::string response_data;
    long http_status_code;
    char error_buffer[CURL_ERROR_SIZE];
    error_buffer[0] = '\0';
    const std::string service_url = SERVICE_CALL_PREFIX +
                                    iot_get_credential_endpoint_ +
                                    ROLE_ALIASES_PATH + '/' +
                                    role_alias_ +
                                    CREDENTIAL_SERVICE;

    std::chrono::duration<uint64_t> expiration;
    std::string access_key_str, session_token_str, secret_access_key_str, expiration_str;
    std::stringstream ss;
    Json::Value response_json;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_URL, service_url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLCERT, cert_path_.c_str());
    curl_easy_setopt(curl, CURLOPT_SSLKEY, private_key_path_.c_str());
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_cert_path_.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    long completion_timeout = (long)std::chrono::milliseconds(REQUEST_COMPLETION_TIMEOUT_MS).count();
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, completion_timeout);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        const char *url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        LOG_ERROR("curl perform failed for url " << url
                                                 << " with result "
                                                 << curl_easy_strerror(res)
                                                 << ": "
                                                 << error_buffer);
        goto CleanUp;

    }

    dumpCurlEasyInfo(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
    if (HTTP_OK != http_status_code) {
        const char *url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
        LOG_WARN("HTTP Error " << http_status_code << ": Response: " << response_data << "\nRequest URL: "
                               << url);
        goto CleanUp;
    }

    ss << response_data;
    ss >> response_json;
    response_json = response_json["credentials"];

    if (response_json.empty()) {
        LOG_ERROR("credentials not found in response. \nResponse: " << response_data);
        goto CleanUp;
    }

    access_key_str = response_json.get("accessKeyId", "").asString();
    session_token_str = response_json.get("sessionToken", "").asString();
    secret_access_key_str = response_json.get("secretAccessKey", "").asString();
    expiration_str = response_json.get("expiration", "").asString();

    if (access_key_str.empty() || session_token_str.empty() || secret_access_key_str.empty() ||
            expiration_str.empty()) {
        LOG_ERROR("Invalid credentials. \nResponse: " << response_data);
        goto CleanUp;
    }



    if (kvs_sink_util::parseTimeStr(expiration_str, expiration) == false) {
        LOG_ERROR("Failed to parse AWS_TOKEN_EXPIRATION. \nResponse: " << response_data);
        goto CleanUp;
    }

    LOG_INFO("Successfully obtained aws credentials using iot certificate. Expiration: " << expiration_str);

    credentials.setAccessKey(access_key_str);
    credentials.setSecretKey(secret_access_key_str);
    credentials.setSessionToken(session_token_str);
    credentials.setExpiration(expiration);

CleanUp:
    curl_easy_cleanup(curl);
    return;
}