/** Copyright 2017 Amazon.com. All rights reserved. */

#include "DefaultCallbackProvider.h"

#include "json/json.h"

#include <fstream>
#include <curl/curl.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::move;
using std::unique_ptr;
using std::make_unique;
using std::string;
using std::thread;
using std::shared_ptr;
using std::make_shared;
using std::future;
using std::function;
using std::vector;
using std::mutex;
using std::lock_guard;
using std::chrono::seconds;
using std::future_status;
using std::condition_variable;
using std::tuple;
using std::async;
using std::launch;
using Json::FastWriter;

#define CURL_CLOSE_HANDLE_DELAY_IN_MILLIS 10

/**
* As we store the credentials provider in the object itself we will return the pointer in the buffer
* which we will later use to access the credentials
*/
STATUS DefaultCallbackProvider::getSecurityTokenHandler(UINT64 custom_data, PBYTE *buffer, PUINT32 size, PUINT64 expiration) {
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    Credentials credentials;
    this_obj->credentials_provider_.get()->getCredentials(credentials);

    uint32_t bufferSize;

    // Safe free the buffer
    safeFreeBuffer(&this_obj->security_token_);

    // Store the buffer so we can release it at the end
    SerializedCredentials::serialize(credentials, &this_obj->security_token_, &bufferSize);

    // Credentials expiration count is in seconds. Need to set the expiration in Kinesis Video time
    *expiration = credentials.getExpiration().count() * HUNDREDS_OF_NANOS_IN_A_SECOND;

    *buffer = this_obj->security_token_;
    *size = bufferSize;

    return STATUS_SUCCESS;
}

UINT64 DefaultCallbackProvider::getCurrentTimeHandler(UINT64 custom_data) {
    UNUSED_PARAM(custom_data);
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count() / DEFAULT_TIME_UNIT_IN_NANOS;
}

STATUS DefaultCallbackProvider::createDeviceHandler(
        UINT64 custom_data, PCHAR device_name, PServiceCallContext service_call_ctx) {
    LOG_DEBUG("createDeviceHandler invoked");
    // TODO: Implement the upsert of the device in the backend. Returning a dummy arn
    string device_arn = "arn:aws:kinesisvideo:us-west-2:11111111111:mediastream/device";
    STATUS status = createDeviceResultEvent(service_call_ctx->customData, SERVICE_CALL_RESULT_OK,
                                            const_cast<PCHAR>(device_arn.c_str()));
    if (STATUS_FAILED(status)) {
        LOG_ERROR("createDeviceResultEvent failed with: " << status);
    }

    return status;
}

STATUS DefaultCallbackProvider::createStreamHandler(
        UINT64 custom_data,
        PCHAR device_name,
        PCHAR stream_name,
        PCHAR content_type,
        PCHAR kms_arn,
        UINT64 retention_period,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("createStreamHandler invoked");

    string stream_name_str = string(stream_name);

    Json::Value args = Json::objectValue;
    args["DeviceName"] = string(device_name);
    args["StreamName"] = stream_name_str;
    args["MediaType"] = string(content_type);

    // KMS key id is an optional param
    if (kms_arn != NULL && kms_arn[0] != '\0') {
        args["KmsKeyId"] = string(kms_arn);
    }

    // Expressed in hours
    args["DataRetentionInHours"] = (Json::UInt64) (retention_period / HUNDREDS_OF_NANOS_IN_A_SECOND / 60 / 60);
    FastWriter jsonWriter;
    string post_body(jsonWriter.write(args));

    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // De-serialize the credentials from the context
    Credentials credentials;
    SerializedCredentials::deSerialize(service_call_ctx->pAuthInfo->data, service_call_ctx->pAuthInfo->size, credentials);

    // New static credentials provider to go with the signer object
    auto staticCredentialProvider = make_unique<StaticCredentialProvider> (credentials);
    auto request_signer = AwsV4Signer::Create(this_obj->region_, this_obj->service_, move(staticCredentialProvider));

    auto endpoint = this_obj->getControlPlaneUri();
    auto url = endpoint + "/createStream";
    unique_ptr<Request> request = make_unique<Request>(Request::POST, url);
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", endpoint);
    request->setHeader("content-type", "application/json");
    request->setBody(post_body);

    LOG_DEBUG("createStreamHandler post body: " << post_body);

    auto async_call = [](const CurlCallManager* ccm,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> (call_after_time);
        std::this_thread::sleep_until(time_point);

        // Perform a sync call
        unique_ptr<Response> response = ccm->call(std::move(request), request_signer.get());

        SERVICE_CALL_RESULT service_call_result = response->getServiceCallResult();

        LOG_DEBUG("createStream response: " << response->getData());

        if (HTTP_OK != response->getStatusCode()) {
            LOG_AND_THROW("Creation of stream: " << stream_name_str << " failed. "
                                                 << "Status code: " << response->getStatusCode()
                                                 << "Response body: " << response->getData());
        }

        Json::Reader reader;
        Json::Value json_response = Json::nullValue;
        if (!reader.parse(response->getData(), json_response)) {
            LOG_AND_THROW("Unable to parse response from kinesis video create stream call as json. Data: " +
                          string(response->getData()));
        }

        string stream_arn(json_response["StreamARN"].asString());
        LOG_INFO("Created new Kinesis Video stream: " << stream_arn);
        STATUS status = createStreamResultEvent(custom_data, service_call_result,
                                                const_cast<PCHAR>(stream_arn.c_str()));

        if (STATUS_FAILED(status)) {
            LOG_ERROR("createStreamResultEvent failed with: " << status);
        }
    };

    thread worker(async_call, &this_obj->ccm_, move(request), move(request_signer), stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS DefaultCallbackProvider::tagResourceHandler(
        UINT64 custom_data, PCHAR stream_arn, UINT32 num_tags, PTag tags,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("tagResourceHandler invoked for stream: " << stream_arn);

    // Extract the tags into json format
    Json::Value json_tags;
    for (int i = 0; i < num_tags; ++i) {
        Tag &tag = tags[i];
        json_tags[tag.name] = tag.value;
    }

    string stream_arn_str = string(stream_arn);

    Json::Value args = Json::objectValue;
    args["StreamARN"] = stream_arn_str;
    args["Tags"] = json_tags;
    FastWriter jsonWriter;
    string post_body(jsonWriter.write(args));

    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // De-serialize the credentials from the context
    Credentials credentials;
    SerializedCredentials::deSerialize(service_call_ctx->pAuthInfo->data, service_call_ctx->pAuthInfo->size, credentials);

    // New static credentials provider to go with the signer object
    auto staticCredentialProvider = make_unique<StaticCredentialProvider> (credentials);
    auto request_signer = AwsV4Signer::Create(this_obj->region_, this_obj->service_, move(staticCredentialProvider));

    auto endpoint = this_obj->getControlPlaneUri();
    auto url = endpoint + "/tagStream";
    unique_ptr<Request> request = make_unique<Request>(Request::POST, url);
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", endpoint);
    request->setHeader("content-type", "application/json");
    request->setBody(post_body);

    LOG_DEBUG("tagResourceHandler post body: " << post_body);

    auto async_call = [](const CurlCallManager* ccm,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_arn_str,
                         PServiceCallContext service_call_ctx) -> auto {

        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> (call_after_time);
        std::this_thread::sleep_until(time_point);

        // Perform a sync call
        unique_ptr<Response> response = ccm->call(std::move(request), request_signer.get());

        if (HTTP_OK != response->getStatusCode()) {
            LOG_ERROR("Failed to set tags on Kinesis Video stream: " << string(stream_arn_str)
                                                              << " status: " << response->getStatusCode()
                                                              << " response: "
                                                              << string(response->getData()));
        }

        SERVICE_CALL_RESULT service_call_result = response->getServiceCallResult();
        STATUS status = tagResourceResultEvent(custom_data, service_call_result);
        if (STATUS_FAILED(status)) {
            LOG_ERROR("tagResourceResultEvent failed with: " << status);
        }
    };

    thread worker(async_call, &this_obj->ccm_, move(request), move(request_signer), stream_arn_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS DefaultCallbackProvider::describeStreamHandler(
        UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx) {

    LOG_DEBUG("describeStreamHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    string stream_name_str = string(stream_name);

    Json::Value args = Json::objectValue;
    args["StreamName"] = stream_name_str;
    FastWriter jsonWriter;
    string post_body(jsonWriter.write(args));


    // De-serialize the credentials from the context
    Credentials credentials;
    SerializedCredentials::deSerialize(service_call_ctx->pAuthInfo->data, service_call_ctx->pAuthInfo->size, credentials);

    // New static credentials provider to go with the signer object
    auto staticCredentialProvider = make_unique<StaticCredentialProvider> (credentials);
    auto request_signer = AwsV4Signer::Create(this_obj->region_, this_obj->service_, move(staticCredentialProvider));

    auto endpoint = this_obj->getControlPlaneUri();
    auto url = endpoint + "/describeStream";
    unique_ptr<Request> request = make_unique<Request>(Request::POST, url);
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", endpoint);
    request->setHeader("content-type", "application/json");
    request->setBody(post_body);

    auto async_call = [](const CurlCallManager* ccm,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> (call_after_time);
        std::this_thread::sleep_until(time_point);

        // Perform a sync call
        unique_ptr<Response> response = ccm->call(std::move(request), request_signer.get());

        LOG_DEBUG("describeStream response: " << response->getData());
        StreamDescription stream_description;
        PStreamDescription stream_description_ptr = nullptr;
        if (HTTP_OK == response->getStatusCode()) {
            Json::Reader reader;
            Json::Value json_response = Json::nullValue;
            if (!reader.parse(response->getData(), json_response)) {
                LOG_AND_THROW("Unable to parse response from Kinesis Video describe stream call as json. Data: " +
                              string(response->getData()));
            }

            stream_description.version = STREAM_DESCRIPTION_CURRENT_VERSION;

            // set the device name
            const string device_name(json_response["StreamInfo"]["DeviceName"].asString());
            assert(MAX_DEVICE_NAME_LEN > device_name.size());
            std::memcpy(&(stream_description.deviceName), device_name.c_str(), device_name.size());
            stream_description.deviceName[device_name.size()] = '\0';

            // set the stream name
            const string stream_name(json_response["StreamInfo"]["StreamName"].asString());
            assert(MAX_STREAM_NAME_LEN > stream_name.size());
            std::memcpy(&(stream_description.streamName), stream_name.c_str(), stream_name.size());
            stream_description.streamName[stream_name.size()] = '\0';

            // Set the content type
            const string mime_type(json_response["StreamInfo"]["MimeType"].asString());
            assert(MAX_CONTENT_TYPE_LEN > mime_type.size());
            std::memcpy(&(stream_description.contentType), mime_type.c_str(), mime_type.size());
            stream_description.contentType[mime_type.size()] = '\0';

            // Set the update version
            const string update_version(json_response["StreamInfo"]["Version"].asString());
            assert(MAX_UPDATE_VERSION_LEN > update_version.size());
            std::memcpy(&(stream_description.updateVersion), update_version.c_str(), update_version.size());
            stream_description.updateVersion[update_version.size()] = '\0';

            // Set the ARN
            const string stream_arn(json_response["StreamInfo"]["StreamARN"].asString());
            LOG_INFO("Discovered existing Kinesis Video stream: " << stream_arn);
            assert(MAX_ARN_LEN > stream_arn.size());
            std::memcpy(&(stream_description.streamArn), stream_arn.c_str(), stream_arn.size());
            stream_description.streamArn[stream_arn.size()] = '\0';

            LOG_INFO("stream arn in stream_info struct: "
                             << string(reinterpret_cast<char *>(&(stream_description
                                     .streamArn))));

            stream_description.streamStatus = getStreamStatusFromString(
                    json_response["StreamInfo"]["Status"].asString());

            // Set the creation time
            DOUBLE creation_time = json_response["StreamInfo"]["CreationTime"].asDouble();
            UINT64 seconds = (UINT64) creation_time;
            DOUBLE fraction = creation_time - seconds;

            // Use only micros precision - chop the rest
            UINT64 micros = (UINT64) (fraction * 1000000);
            stream_description.creationTime = seconds * HUNDREDS_OF_NANOS_IN_A_SECOND +
                    micros * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;

            // set the pointer
            stream_description_ptr = &stream_description;
        } else {
            LOG_INFO("Describe stream did not find the stream " << stream_name_str
                                                                << " in Kinesis Video (stream will be created)");
        }

        SERVICE_CALL_RESULT service_call_result = response->getServiceCallResult();
        STATUS status = describeStreamResultEvent(custom_data,
                                                  service_call_result,
                                                  stream_description_ptr);
        if (STATUS_FAILED(status)) {
            LOG_ERROR("describeStreamResultEvent failed with: " << status);
        }
    };


    thread worker(async_call, &this_obj->ccm_, move(request), move(request_signer), stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS DefaultCallbackProvider::streamingEndpointHandler(
        UINT64 custom_data, PCHAR stream_name, PCHAR api_name,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("streamingEndpointHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    string stream_name_str = string(stream_name);

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
    unique_ptr<Request> request = make_unique<Request>(Request::POST, url);
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", endpoint);
    request->setBody(post_body);

    auto async_call = [](const CurlCallManager* ccm,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> (call_after_time);
        std::this_thread::sleep_until(time_point);

        // Perform a sync call
        unique_ptr<Response> response = ccm->call(std::move(request), request_signer.get());

        LOG_DEBUG("getStreamingEndpoint response: " << response->getData());

        char streaming_endpoint_chars[MAX_URI_CHAR_LEN];
        streaming_endpoint_chars[0] = '\0';
        if (HTTP_OK == response->getStatusCode()) {
            Json::Reader reader;
            Json::Value json_response = Json::nullValue;
            if (!reader.parse(response->getData(), json_response)) {
                LOG_AND_THROW("Unable to parse response from kinesis video get streaming endpoint call as json. Data: " +
                              string(response->getData()));
            }

            // set the device name
            const string streaming_endpoint(json_response["DataEndpoint"].asString());
            assert(MAX_URI_CHAR_LEN > streaming_endpoint.size());
            strcpy(streaming_endpoint_chars, const_cast<PCHAR>(streaming_endpoint.c_str()));

            LOG_INFO("streaming to endpoint: " << string(reinterpret_cast<char *>(streaming_endpoint_chars)));
        }

        SERVICE_CALL_RESULT service_call_result = response->getServiceCallResult();
        STATUS status = getStreamingEndpointResultEvent(custom_data,
                                                        service_call_result,
                                                        streaming_endpoint_chars);

        if (STATUS_FAILED(status)) {
            LOG_ERROR("getStreamingEndpointResultEvent failed with: " << status);
        }
    };

    thread worker(async_call, &this_obj->ccm_, move(request), move(request_signer), stream_name_str, service_call_ctx);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS DefaultCallbackProvider::streamingTokenHandler(
        UINT64 custom_data, PCHAR stream_name, STREAM_ACCESS_MODE access_mode,
        PServiceCallContext service_call_ctx) {
    LOG_DEBUG("streamingTokenHandler invoked");
    // TODO: (musheghm) Currently, we are not supporting scoped-down credentials for streaming. 
    // Will need to implement this once the backend support is enabled.
    // NOTE: For now, we will use the credentials provider to get the token
    // Assuming, the security token will allow streaming.

    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    Credentials credentials;
    this_obj->credentials_provider_.get()->getUpdatedCredentials(credentials);

    uint32_t bufferSize;
    uint8_t* buffer = nullptr;

    // Store the buffer so we can release it at the end
    SerializedCredentials::serialize(credentials, &buffer, &bufferSize);

    // Convert to Kinesis Video time
    auto expiration = std::chrono::duration_cast<std::chrono::nanoseconds>(credentials.getExpiration()).count()
                      / DEFAULT_TIME_UNIT_IN_NANOS;

    STATUS status = getStreamingTokenResultEvent(
            service_call_ctx->customData, SERVICE_CALL_RESULT_OK,
            reinterpret_cast<PBYTE>(buffer),
            bufferSize,
            expiration);

    if (STATUS_FAILED(status)) {
        LOG_ERROR("getStreamingTokenResultEvent failed with: " << status);
    }

    // Delete the allocated object
    safeFreeBuffer(&buffer);

    return status;
}

STATUS DefaultCallbackProvider::putStreamHandler(
        UINT64 custom_data, PCHAR stream_name, PCHAR container_type,
        UINT64 start_timestamp, BOOL absolute_fragment_timestamp,
        BOOL do_ack, PCHAR streaming_endpoint, PServiceCallContext service_call_ctx) {
    LOG_DEBUG("putStreamHandler invoked");

    auto this_obj = reinterpret_cast<DefaultCallbackProvider *>(custom_data);

    // De-serialize the credentials from the context
    Credentials credentials;
    SerializedCredentials::deSerialize(service_call_ctx->pAuthInfo->data, service_call_ctx->pAuthInfo->size, credentials);

    // New static credentials provider to go with the signer object
    auto staticCredentialProvider = make_unique<StaticCredentialProvider> (credentials);
    auto request_signer = AwsV4Signer::CreateStreaming(this_obj->region_, this_obj->service_, move(staticCredentialProvider));

    string stream_name_str(stream_name);

    // Create a new state
    auto state = make_shared<OngoingPutFrameState>(this_obj,
                                                   service_call_ctx->customData,
                                                   stream_name_str);

    // Upsert into the active state map
    this_obj->insertActiveState(state.get());

    // The state object interpreted as a client stream handle to be passed to PIC.
    UINT64 clientStreamHandle = reinterpret_cast<UINT64>(state.get());

    // Need to extract the URI fully qualified host path to set the "host" header
    auto put_media_endpoint = string(streaming_endpoint) + "/putMedia";

    // we are passing the raw pointer of state to the callback function. This is ok lifetime wise because we have
    // a reference to state saved in the active_streams_ map that is only removed with the future attached to
    // the return of this async_call exits.
    unique_ptr<Request> request = make_unique<Request>(Request::POST,
                    put_media_endpoint,
                    DefaultCallbackProvider::postHeaderReadFunc,
                    DefaultCallbackProvider::postBodyStreamingReadFunc,
                    reinterpret_cast<void *>(state.get()),
                    DefaultCallbackProvider::postBodyStreamingWriteFunc,
                    reinterpret_cast<void *>(state.get()));
    request->setConnectionTimeout(std::chrono::milliseconds(service_call_ctx->timeout / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
    request->setHeader("host", streaming_endpoint);
    request->setHeader("x-amzn-stream-name", stream_name);
    // Producer start time in putMedia call takes a format of "seconds_from_epoch.milliseconds"
    UINT64 timestamp_millis = start_timestamp / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    string timestamp = std::to_string(timestamp_millis / 1000) + "." + std::to_string(timestamp_millis % 1000);
    request->setHeader("x-amzn-producer-start-timestamp", timestamp);
    request->setHeader("x-amzn-fragment-acknowledgment-required", std::to_string(do_ack));
    request->setHeader("x-amzn-fragment-timecode-type", absolute_fragment_timestamp ? "ABSOLUTE" : "RELATIVE");
    request->setHeader("transfer-encoding", "chunked");
    request->setHeader("connection", "keep-alive");

    auto async_call = [](DefaultCallbackProvider* this_obj,
                         const CurlCallManager* ccm,
                         shared_ptr<OngoingPutFrameState> state,
                         std::unique_ptr<Request> request,
                         std::unique_ptr<const RequestSigner> request_signer,
                         string stream_name_str,
                         PServiceCallContext service_call_ctx) -> auto {
        uint64_t custom_data = service_call_ctx->customData;

        // Wait for the specified amount of time before calling
        auto call_after_time = std::chrono::nanoseconds(service_call_ctx->callAfter * DEFAULT_TIME_UNIT_IN_NANOS);
        auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> (call_after_time);
        std::this_thread::sleep_until(time_point);

        LOG_INFO("Creating new connection for Kinesis Video stream: " << stream_name_str);

        // Perform a sync call
        CURL* curl_handle;
        unique_ptr<Response> response = ccm->call(std::move(request), request_signer.get(), state);

        LOG_DEBUG("Connection for Kinesis Video stream: " << stream_name_str << " closed.");

        // does not return until the stream ends.
        // this behavior is important because it keeps the cb_data variable valid for the lifetime of the stream.
        LOG_INFO("Network thread for Kinesis Video stream: " << stream_name_str << " exited. http status: "
                                                             << response->getStatusCode());

        // Remove the state from the active list
        this_obj->removeActiveState(state.get());

        // If we terminated abnormally then terminate the stream
        if (!state->isEndOfStream()) {
            LOG_WARN("Stream for "
                             << stream_name_str
                             << " has exited without triggering end-of-stream. Service call result: "
                             << response->getServiceCallResult());

            kinesisVideoStreamTerminated(custom_data, response->getServiceCallResult());
        }
    };

    thread worker(async_call, this_obj, &this_obj->ccm_, state, move(request), move(request_signer), stream_name_str, service_call_ctx);
    worker.detach();

    // Return 200 to Kinesis Video SDK on successful connection establishment as the POST is theoretically infinite.
    STATUS status = putStreamResultEvent(service_call_ctx->customData, SERVICE_CALL_RESULT_OK, clientStreamHandle);
    if (STATUS_FAILED(status)) {
        LOG_ERROR("putStreamResultEvent failed with: " << status);
    }

    return status;
}

size_t DefaultCallbackProvider::postHeaderReadFunc(
        char *buffer, size_t item_size, size_t n_items, void *custom_data) {
    LOG_DEBUG("postHeaderReadFunc (curl callback) invoked");

    size_t data_size = item_size * n_items;

    LOG_DEBUG("Curl post header write function returned:" << string(buffer, data_size));

    return data_size;
}

size_t DefaultCallbackProvider::postBodyStreamingReadFunc(
        char *buffer, size_t item_size, size_t n_items, void *custom_data) {
    LOG_DEBUG("postBodyStreamingReadFunc (curl callback) invoked");
    OngoingPutFrameState *state = reinterpret_cast<OngoingPutFrameState *>(custom_data);
    DefaultCallbackProvider* this_obj = (DefaultCallbackProvider*) state->getCallbackProvider();

    // Block until the stream is current
    state->awaitCurrent();

    // Check for end-of-stream
    if (state->isEndOfStream()) {
        // Activate the next state so we won't wait till the end of stream termination
        this_obj->activateNextState(state);

        // Returning 0 will close the connection
        return 0;
    }

    size_t buffer_size = item_size * n_items;

    // Block until data is available.
    size_t bytes_written = 0;
    while (bytes_written == 0 && !state->isEndOfStream()) {
        size_t available_bytes = state->awaitData(buffer_size);

        UINT64 client_stream_handle = 0;
        bytes_written = 0;
        STATUS retStatus = getKinesisVideoStreamData(
                state->getStreamHandle(),
                &client_stream_handle,
                reinterpret_cast<PBYTE>(buffer),
                buffer_size,
                reinterpret_cast<PUINT32>(&bytes_written));

        LOG_TRACE("Available bytes to read: " << available_bytes
                                              << " buffer size: "
                                              << buffer_size
                                              << " written bytes: "
                                              << bytes_written
                                              << " for client stream handle: "
                                              << client_stream_handle);

        // The return should be OK, no more data or an end of stream
        switch (retStatus) {
            case STATUS_SUCCESS:
            case STATUS_NO_MORE_DATA_AVAILABLE:

                // This is an OK case - ensure that we reset the available bytes in the state if we got 0 returned
                if (0 == bytes_written) {
                    LOG_DEBUG("Resetting the available data for the streaming state object.");
                    state->setDataAvailable(0, 0);
                }

                break;

            case STATUS_END_OF_STREAM:
                LOG_INFO("Reported end-of-stream.");

                // Output the remaining bytes and signal the EOS
                state->endOfStream();
                break;

            default:
                LOG_ERROR("Failed to get data from the stream with an error: " << retStatus);

                // Terminate and close the connection
                bytes_written = CURL_READFUNC_ABORT;
        }
    }

    LOG_DEBUG("Wrote " << bytes_written << " bytes to Kinesis Video");
    if (0 == bytes_written || CURL_READFUNC_ABORT == bytes_written) {
        // Activate the next state
        this_obj->activateNextState(state);
    }

    return bytes_written;
}

void DefaultCallbackProvider::insertActiveState(OngoingPutFrameState* state) {
    std::unique_lock<std::mutex> lock(active_streams_mutex_);

    // Check if we already have an entry - this could be during token rotation
    auto stream_handle = state->getStreamHandle();
    auto current_state = active_streams_.get(stream_handle);
    if (nullptr != current_state) {
        // set the next for the current state
        while (nullptr != current_state->getNextState()) {
            current_state = current_state->getNextState();
        }

        current_state->setNextState(state);
        LOG_DEBUG("Already have an active stream state.");
    } else {
        // This is the current stream state so insert it into the map first and make it active
        active_streams_.put(stream_handle, state);
        state->notifyStateCurrent();
        LOG_DEBUG("No active streams states for " << state->getStreamName());
    }
}

void DefaultCallbackProvider::removeActiveState(OngoingPutFrameState* state) {
    std::unique_lock<std::mutex> lock(active_streams_mutex_);

    auto stream_handle = state->getStreamHandle();
    auto existing_state = active_streams_.get(stream_handle);
    CHECK_EXT(existing_state != nullptr, "Existing state can not be null");

    if (existing_state == state) {
        // Remove from the active states and promote the next state
        active_streams_.remove(stream_handle);

        // Promote the next if any
        auto next_state = existing_state->getNextState();
        if (nullptr != next_state) {
            // Insert into the map and activate the state
            LOG_DEBUG("Inserting a new active stream state");
            active_streams_.put(stream_handle, next_state);
            next_state->notifyStateCurrent();
        }
    } else {
        // Find it in the list
        auto cur_state = existing_state;
        while (nullptr != cur_state) {
            auto next_state = existing_state->getNextState();
            if (next_state == state) {
                cur_state->setNextState(next_state->getNextState());
                LOG_DEBUG("Making a new active stream state");
                cur_state->notifyStateCurrent();
                break;
            }

            cur_state = next_state;
        }
    }
}

void DefaultCallbackProvider::activateNextState(OngoingPutFrameState* state) {
    std::unique_lock<std::mutex> lock(active_streams_mutex_);

    auto stream_handle = state->getStreamHandle();
    auto existing_state = active_streams_.get(stream_handle);
    CHECK_EXT(existing_state != nullptr, "Existing state can not be null");

    if (existing_state == state) {
        // Activate the next if any
        auto next_state = existing_state->getNextState();
        if (nullptr != next_state) {
            // Insert into the map and activate the state
            LOG_DEBUG("Activating the next state of the existing");
            next_state->notifyStateCurrent();
        }
    } else {
        // Find it in the list
        auto cur_state = existing_state;
        while (nullptr != cur_state) {
            auto next_state = cur_state->getNextState();
            if (next_state == state) {
                cur_state = next_state->getNextState();
                if (nullptr != cur_state) {
                    LOG_DEBUG("Activating the next stream state");
                    cur_state->notifyStateCurrent();
                    break;
                }
            }

            cur_state = next_state;
        }
    }
}

size_t DefaultCallbackProvider::postBodyStreamingWriteFunc(
        char *buffer, size_t item_size, size_t n_items, void *custom_data) {
    LOG_DEBUG("postBodyStreamingWriteFunc (curl callback) invoked");

    STATUS status = STATUS_SUCCESS;
    size_t data_size = item_size * n_items;
    string data_as_string(buffer, data_size);

    LOG_INFO("Curl post body write function returned:" << data_as_string);

    // The data can be passed in an arbitrary size so we can't make any assumptions.
    // What we do is the following. We start with the initial state of expecting to have an open curly brace.
    // We will accumulate bits until the closing curly brace. All of our ACKs have a form of
    // {"ackEventType":"PERSISTED","fragmentTimecode":3400,"fragmentNumber":"91343852344490898070324846249945695249403494059"}
    // and do not contain curlies inside as they are simple jsons. This assumption is valid for now but if it changes
    // we will need to re-implement this parsing logic. We will be stream parsing the data as it comes.

    OngoingPutFrameState *state = reinterpret_cast<OngoingPutFrameState *>(custom_data);
    status = kinesisVideoStreamParseFragmentAck(state->getStreamHandle(), buffer, data_size);
    if (STATUS_FAILED(status)) {
        LOG_ERROR("Failed to submit ACK: "
                          << data_as_string
                          << " with status code: "
                          << status);
    } else {
        LOG_DEBUG("Processed ACK OK.");
    }

    return data_size;
}

STATUS DefaultCallbackProvider::streamDataAvailableHandler(UINT64 custom_data,
                                                          STREAM_HANDLE stream_handle,
                                                          PCHAR stream_name,
                                                          UINT64 duration_available,
                                                          UINT64 size_available) {
    LOG_DEBUG("streamDataAvailableHandler invoked");

    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    {
        std::unique_lock<std::mutex> lock(this_obj->active_streams_mutex_);

        auto state = this_obj->getActiveStreams().get(stream_handle);
        while (nullptr != state) {
            if (!state->isEndOfStream()) {
                state->noteDataAvailable(duration_available, size_available);
                break;
            }

            state = state->getNextState();
        }
    }

    auto client_stream_report_callback = this_obj->stream_callback_provider_->getStreamDataAvailableCallback();
    if (nullptr != client_stream_report_callback) {
        return client_stream_report_callback(custom_data, stream_handle, stream_name, duration_available, size_available);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamClosedHandler(UINT64 custom_data,
                                                    STREAM_HANDLE stream_handle) {
    LOG_DEBUG("streamClosedHandler invoked");

    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    {
        std::unique_lock<std::mutex> lock(this_obj->active_streams_mutex_);

        auto state = this_obj->getActiveStreams().get(stream_handle);
        while (nullptr != state) {
            if (state->isActive() && !state->isEndOfStream()) {
                state->endOfStream();
                state->noteDataAvailable(0, 0);
                break;
            }

            Response* curl_response = state->getResponse();
            // Close the connection
            if (nullptr != curl_response) {
                curl_response->terminate();
            }

            state = state->getNextState();
        }
    }

    auto client_eos_callback = this_obj->stream_callback_provider_->getStreamClosedCallback();
    if (nullptr != client_eos_callback) {
        // Await for some time for CURL to terminate properly before triggering the callback on another thread
        // as the calling thread is likely to be the curls thread and most implementations have a single threaded
        // pool which we can't block.
        auto async_call = [](const StreamClosedFunc client_eos_callback,
                             UINT64 custom_data,
                             STREAM_HANDLE stream_handle) -> auto {
            // Wait for the specified amount of time before calling the provided callback
            // NOTE: We will add an extra time for curl handle to settle and close the stream.
            auto time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>
                    (std::chrono::milliseconds(TIMEOUT_AFTER_STREAM_STOPPED + CURL_CLOSE_HANDLE_DELAY_IN_MILLIS));
            std::this_thread::sleep_until(time_point);

            STATUS status = client_eos_callback(custom_data, stream_handle);
            if (STATUS_FAILED(status)) {
                LOG_ERROR("streamClosedHandler failed with: " << status);
            }
        };

        thread worker(async_call, client_eos_callback, custom_data, stream_handle);
        worker.detach();
    }

    return STATUS_SUCCESS;
}

/**
 * Handles stream fragment errors.
 *
 * @param custom_data Custom handle passed by the caller (this class)
 * @param STREAM_HANDLE stream handle for the stream
 * @param UINT64 errored fragment timecode
 * @param STATUS status code of the failure
 * @return Status of the callback
 */
STATUS DefaultCallbackProvider::streamErrorHandler(UINT64 custom_data,
                                                   STREAM_HANDLE stream_handle,
                                                   UINT64 fragment_timecode,
                                                   STATUS status) {
    LOG_DEBUG("streamErrorHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Terminate the existing stream if any
    auto existingState = this_obj->active_streams_.get(stream_handle);
    if (existingState != nullptr) {
        existingState->endOfStream();
    }

    // Call the client callback if any specified
    auto client_stream_report_callback = this_obj->stream_callback_provider_->getStreamErrorReportCallback();
    if (nullptr != client_stream_report_callback) {
        return client_stream_report_callback(custom_data, stream_handle, fragment_timecode, status);
    } else {
        return STATUS_SUCCESS;
    }
}

DefaultCallbackProvider::DefaultCallbackProvider(
        unique_ptr <ClientCallbackProvider> client_callback_provider,
        unique_ptr <StreamCallbackProvider> stream_callback_provider,
        unique_ptr <CredentialProvider> credentials_provider,
        const string& region,
        const string& control_plane_uri)
        : ccm_(CurlCallManager::getInstance()),
          region_(region),
          service_(KINESIS_VIDEO_SERVICE_NAME),
          control_plane_uri_(control_plane_uri),
          security_token_(nullptr) {
    client_callback_provider_ = move(client_callback_provider);
    stream_callback_provider_ = move(stream_callback_provider);
    credentials_provider_ = move(credentials_provider);
    internal_stream_callback_provider_ = make_unique<StreamCallbackProvider>();

    if (control_plane_uri_.empty()) {
        // Create a fully qualified URI
        control_plane_uri_ = CONTROL_PLANE_URI_PREFIX
                             + KINESIS_VIDEO_SERVICE_NAME
                             + "."
                             + DEFAULT_AWS_REGION
                             + CONTROL_PLANE_URI_POSTFIX;
    }
}

void DefaultCallbackProvider::safeFreeBuffer(uint8_t** ppBuffer) {
    if (ppBuffer && *ppBuffer) {
        free(*ppBuffer);
        *ppBuffer = nullptr;
    }
}

DefaultCallbackProvider::~DefaultCallbackProvider() {
    DefaultCallbackProvider::safeFreeBuffer(&security_token_);
}

STREAM_STATUS DefaultCallbackProvider::getStreamStatusFromString(const std::string &status) {
    if ("ACTIVE" == status) { return STREAM_STATUS_ACTIVE; }
    if ("CREATING" == status) { return STREAM_STATUS_CREATING; }
    if ("UPDATING" == status) { return STREAM_STATUS_UPDATING; }
    if ("DELETING" == status) { return STREAM_STATUS_DELETING; }
    LOG_AND_THROW("Encountered unhandled stream status: " << status);
}

GetCurrentTimeFunc DefaultCallbackProvider::getCurrentTimeCallback() {
    return getCurrentTimeHandler;
}

DroppedFragmentReportFunc DefaultCallbackProvider::getDroppedFragmentReportCallback() {
    return stream_callback_provider_->getDroppedFragmentReportCallback();
}

StreamReadyFunc DefaultCallbackProvider::getStreamReadyCallback() {
    return stream_callback_provider_->getStreamReadyCallback();
}

StreamClosedFunc DefaultCallbackProvider::getStreamClosedCallback() {
    return streamClosedHandler;
}

CreateStreamFunc DefaultCallbackProvider::getCreateStreamCallback() {
    return createStreamHandler;
}

DescribeStreamFunc DefaultCallbackProvider::getDescribeStreamCallback() {
    return describeStreamHandler;
}

GetStreamingEndpointFunc DefaultCallbackProvider::getStreamingEndpointCallback() {
    return streamingEndpointHandler;
}

GetStreamingTokenFunc DefaultCallbackProvider::getStreamingTokenCallback() {
    return streamingTokenHandler;
}

PutStreamFunc DefaultCallbackProvider::getPutStreamCallback() {
    return putStreamHandler;
}

TagResourceFunc DefaultCallbackProvider::getTagResourceCallback() {
    return tagResourceHandler;
}

GetSecurityTokenFunc DefaultCallbackProvider::getSecurityTokenCallback() {
    return getSecurityTokenHandler;
}

StreamUnderflowReportFunc DefaultCallbackProvider::getStreamUnderflowReportCallback() {
    return stream_callback_provider_->getStreamUnderflowReportCallback();
}

StorageOverflowPressureFunc DefaultCallbackProvider::getStorageOverflowPressureCallback() {
    return client_callback_provider_->getStorageOverflowPressureCallback();
}

StreamLatencyPressureFunc DefaultCallbackProvider::getStreamLatencyPressureCallback() {
    return stream_callback_provider_->getStreamLatencyPressureCallback();
}

DroppedFrameReportFunc DefaultCallbackProvider::getDroppedFrameReportCallback() {
    return stream_callback_provider_->getDroppedFrameReportCallback();
}

StreamErrorReportFunc DefaultCallbackProvider::getStreamErrorReportCallback() {
    return streamErrorHandler;
}

GetDeviceCertificateFunc DefaultCallbackProvider::getDeviceCertificateCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

GetDeviceFingerprintFunc DefaultCallbackProvider::getDeviceFingerprintCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

ClientReadyFunc DefaultCallbackProvider::getClientReadyCallback() {
    return client_callback_provider_->getClientReadyCallback();
}

CreateDeviceFunc DefaultCallbackProvider::getCreateDeviceCallback() {
    return createDeviceHandler;
}

DeviceCertToTokenFunc DefaultCallbackProvider::getDeviceCertToTokenCallback() {
    // We are using a security token, so this callback should be null.
    return nullptr;
}

StreamDataAvailableFunc DefaultCallbackProvider::getStreamDataAvailableCallback() {
    return streamDataAvailableHandler;
}

StreamConnectionStaleFunc DefaultCallbackProvider::getStreamConnectionStaleCallback() {
    return stream_callback_provider_->getStreamConnectionStaleCallback();
}

ThreadSafeMap<STREAM_HANDLE, OngoingPutFrameState*>&
DefaultCallbackProvider::getActiveStreams() {
    return active_streams_;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
