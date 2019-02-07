/** Copyright 2019 Amazon.com. All rights reserved. */

#include "CachingEndpointOnlyCallbackProvider.h"
#include "Version.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::move;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::thread;
using Json::FastWriter;

STATUS CachingEndpointOnlyCallbackProvider::createStreamHandler(
        UINT64 custom_data,
        PCHAR device_name,
        PCHAR stream_name,
        PCHAR content_type,
        PCHAR kms_arn,
        UINT64 retention_period,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("createStreamHandler invoked");

    UNUSED_PARAM(device_name);
    UNUSED_PARAM(content_type);
    UNUSED_PARAM(kms_arn);
    UNUSED_PARAM(retention_period);

    string stream_name_str = string(stream_name);
    auto this_obj = reinterpret_cast<CachingEndpointOnlyCallbackProvider*>(custom_data);

    auto async_call = [](CachingEndpointOnlyCallbackProvider* this_obj,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {

        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> (call_after_time);
        sleepUntilWithTimeCallback(time_point);

        // We will return stream name as the arm
        STATUS status = createStreamResultEvent(custom_data, SERVICE_CALL_RESULT_OK, const_cast<PCHAR>(stream_name_str.c_str()));

        this_obj->notifyResult(status, custom_data);
    };

    thread worker(async_call, this_obj, stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS CachingEndpointOnlyCallbackProvider::tagResourceHandler(
        UINT64 custom_data, PCHAR stream_arn, UINT32 num_tags, PTag tags,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("tagResourceHandler invoked for stream: " << stream_arn);

    UNUSED_PARAM(num_tags);
    UNUSED_PARAM(tags);

    string stream_arn_str = string(stream_arn);

    auto this_obj = reinterpret_cast<CachingEndpointOnlyCallbackProvider*>(custom_data);

    auto async_call = [](CachingEndpointOnlyCallbackProvider* this_obj,
                         string stream_arn_str,
                         PServiceCallContext service_call_ctx) -> auto {

        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> (call_after_time);
        sleepUntilWithTimeCallback(time_point);

        STATUS status = tagResourceResultEvent(custom_data, SERVICE_CALL_RESULT_OK);
        this_obj->notifyResult(status, custom_data);
    };

    thread worker(async_call, this_obj, stream_arn_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS CachingEndpointOnlyCallbackProvider::describeStreamHandler(
        UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx) {

    LOG_DEBUG("describeStreamHandler invoked");
    auto this_obj = reinterpret_cast<CachingEndpointOnlyCallbackProvider*>(custom_data);

    string stream_name_str = string(stream_name);

    auto async_call = [](CachingEndpointOnlyCallbackProvider* this_obj,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> (call_after_time);
        sleepUntilWithTimeCallback(time_point);

        StreamDescription stream_description;
        stream_description.version = STREAM_DESCRIPTION_CURRENT_VERSION;

        // set the device name
        string synth_str("DEVICE_NAME");
        std::memcpy(&(stream_description.deviceName), synth_str.c_str(), synth_str.size());
        stream_description.deviceName[synth_str.size()] = '\0';

        // set the stream name
        std::memcpy(&(stream_description.streamName), stream_name_str.c_str(), stream_name_str.size());
        stream_description.streamName[stream_name_str.size()] = '\0';

        // Set the content type
        synth_str = "MIME_TYPE";
        std::memcpy(&(stream_description.contentType), synth_str.c_str(), synth_str.size());
        stream_description.contentType[synth_str.size()] = '\0';

        // Set the update version
        synth_str = "Version";
        std::memcpy(&(stream_description.updateVersion), synth_str.c_str(), synth_str.size());
        stream_description.updateVersion[synth_str.size()] = '\0';

        // Set the ARN
        LOG_INFO("Discovered existing Kinesis Video stream: " << stream_name_str);
        std::memcpy(&(stream_description.streamArn), stream_name_str.c_str(), stream_name_str.size());
        stream_description.streamArn[stream_name_str.size()] = '\0';

        stream_description.streamStatus = STREAM_STATUS_ACTIVE;

        // Set the creation time
        stream_description.creationTime = getCurrentTimeHandler(custom_data);

        STATUS status = describeStreamResultEvent(custom_data, SERVICE_CALL_RESULT_OK, &stream_description);

        this_obj->notifyResult(status, custom_data);
    };

    thread worker(async_call, this_obj, stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS CachingEndpointOnlyCallbackProvider::streamingEndpointHandler(
        UINT64 custom_data, PCHAR stream_name, PCHAR api_name,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("streamingEndpointHandler invoked");

    auto this_obj = reinterpret_cast<CachingEndpointOnlyCallbackProvider*>(custom_data);

    string stream_name_str = string(stream_name);

    // Check if we can return the cached value
    if (!this_obj->streaming_endpoint_.empty() && std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - this_obj->last_update_time_).count() < this_obj->cache_update_period_) {
        // return the cached data
        auto async_call = [](CachingEndpointOnlyCallbackProvider* this_obj,
                             string stream_name_str,
                             PServiceCallContext service_call_ctx) -> auto {
            uint64_t custom_data = service_call_ctx->customData;

            // Wait for the specified amount of time before calling
            auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
            auto time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> (call_after_time);
            sleepUntilWithTimeCallback(time_point);

            STATUS status = getStreamingEndpointResultEvent(custom_data,
                                                            SERVICE_CALL_RESULT_OK,
                                                            const_cast<PCHAR>(this_obj->streaming_endpoint_.c_str()));
            this_obj->notifyResult(status, custom_data);
        };

        thread worker(async_call, this_obj, stream_name_str, service_call_ctx);
        worker.detach();
        return STATUS_SUCCESS;
    }

    Json::Value args = Json::objectValue;
    args["StreamName"] = stream_name_str;
    args["APIName"] = api_name;

    FastWriter jsonWriter;
    string post_body(jsonWriter.write(args));

    // De-serialize the credentials from the context
    Credentials credentials;
    SerializedCredentials::deSerialize(service_call_ctx->pAuthInfo->data, service_call_ctx->pAuthInfo->size, credentials);

    // New static credentials provider to go with the signer object
    auto staticCredentialProvider = make_unique<StaticCredentialProvider> (credentials);
    auto request_signer = AwsV4Signer::Create(this_obj->region_, this_obj->service_, move(staticCredentialProvider));

    string endpoint = this_obj->getControlPlaneUri();
    string url = endpoint + "/getDataEndpoint";
    unique_ptr<Request> request = make_unique<Request>(Request::POST, url, (STREAM_HANDLE) service_call_ctx->customData);
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", endpoint);
    request->setHeader("user-agent", this_obj->user_agent_);
    request->setBody(post_body);

    auto async_call = [](CachingEndpointOnlyCallbackProvider* this_obj,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> (call_after_time);
        sleepUntilWithTimeCallback(time_point);

        // Perform a sync call
        shared_ptr<Response> response = this_obj->ccm_.call(move(request), move(request_signer), this_obj);

        // Remove the response from the active response map
        {
            std::unique_lock<std::recursive_mutex> lock(this_obj->ongoing_responses_mutex_);
            this_obj->ongoing_responses_.remove((STREAM_HANDLE) service_call_ctx->customData);
        }

        LOG_DEBUG("getStreamingEndpoint response: " << response->getData());
        if (!response->terminated()) {
            string streaming_endpoint = "";
            char streaming_endpoint_chars[MAX_URI_CHAR_LEN];
            streaming_endpoint_chars[0] = '\0';
            if (HTTP_OK == response->getStatusCode()) {
                Json::Reader reader;
                Json::Value json_response = Json::nullValue;
                if (!reader.parse(response->getData(), json_response)) {
                    LOG_AND_THROW(
                            "Unable to parse response from kinesis video get streaming endpoint call as json. Data: " +
                            string(response->getData()));
                }

                // set the device name
                streaming_endpoint = json_response["DataEndpoint"].asString();
                assert(MAX_URI_CHAR_LEN > streaming_endpoint.size());
                strcpy(streaming_endpoint_chars, const_cast<PCHAR>(streaming_endpoint.c_str()));

                LOG_INFO("streaming to endpoint: " << string(reinterpret_cast<char *>(streaming_endpoint_chars)));
            }

            SERVICE_CALL_RESULT service_call_result = response->getServiceCallResult();
            STATUS status = getStreamingEndpointResultEvent(custom_data,
                                                            service_call_result,
                                                            streaming_endpoint_chars);

            // Store the data on success
            if (STATUS_SUCCEEDED(status)) {
                this_obj->streaming_endpoint_ = streaming_endpoint;
                this_obj->last_update_time_ = std::chrono::system_clock::now();
            }

            this_obj->notifyResult(status, custom_data);
        }
    };

    thread worker(async_call, this_obj, move(request), move(request_signer), stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

CachingEndpointOnlyCallbackProvider::CachingEndpointOnlyCallbackProvider(
        unique_ptr <ClientCallbackProvider> client_callback_provider,
        unique_ptr <StreamCallbackProvider> stream_callback_provider,
        unique_ptr <CredentialProvider> credentials_provider,
        const string& region,
        const string& control_plane_uri,
        const std::string &user_agent_name,
        const std::string &custom_user_agent,
        const std::string &cert_path,
        uint64_t cache_update_period) : DefaultCallbackProvider(
                move(client_callback_provider),
                move(stream_callback_provider),
                move(credentials_provider),
                region,
                control_plane_uri,
                user_agent_name,
                custom_user_agent,
                cert_path) {

    // Empty endpoint will invalidate the cache
    streaming_endpoint_ = "";

    // Store the last update time
    last_update_time_ = std::chrono::system_clock::now();

    // Store the update period
    cache_update_period_ = cache_update_period;
}

CachingEndpointOnlyCallbackProvider::~CachingEndpointOnlyCallbackProvider() {
}

CreateStreamFunc CachingEndpointOnlyCallbackProvider::getCreateStreamCallback() {
    return createStreamHandler;
}

DescribeStreamFunc CachingEndpointOnlyCallbackProvider::getDescribeStreamCallback() {
    return describeStreamHandler;
}

GetStreamingEndpointFunc CachingEndpointOnlyCallbackProvider::getStreamingEndpointCallback() {
    return streamingEndpointHandler;
}

TagResourceFunc CachingEndpointOnlyCallbackProvider::getTagResourceCallback() {
    return tagResourceHandler;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
