/** Copyright 2017 Amazon.com. All rights reserved. */

#include "DefaultCallbackProvider.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::move;
using std::unique_ptr;
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

#define CURL_CLOSE_HANDLE_DELAY_IN_MILLIS               200
#define MAX_CUSTOM_USER_AGENT_STRING_LENGTH             128
#define CPP_SDK_CUSTOM_USERAGENT                        "CPPSDK"

UINT64 DefaultCallbackProvider::getCurrentTimeHandler(UINT64 custom_data) {
    UNUSED_PARAM(custom_data);
    return std::chrono::duration_cast<std::chrono::nanoseconds>(systemCurrentTime().time_since_epoch())
            .count() / DEFAULT_TIME_UNIT_IN_NANOS;
}

STATUS DefaultCallbackProvider::createDeviceHandler(
        UINT64 custom_data, PCHAR device_name, PServiceCallContext service_call_ctx) {
    UNUSED_PARAM(custom_data);
    UNUSED_PARAM(device_name);
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

STATUS DefaultCallbackProvider::streamDataAvailableHandler(UINT64 custom_data,
                                                           STREAM_HANDLE stream_handle,
                                                           PCHAR stream_name,
                                                           UPLOAD_HANDLE stream_upload_handle,
                                                           UINT64 duration_available,
                                                           UINT64 size_available) {
    LOG_TRACE("streamDataAvailableHandler invoked for stream: "
                      << stream_name
                      << " and stream upload handle: "
                      << stream_upload_handle);

    auto this_obj = reinterpret_cast<DefaultCallbackProvider *>(custom_data);

    auto stream_data_available_callback = this_obj->stream_callback_provider_->getStreamDataAvailableCallback();
    if (nullptr != stream_data_available_callback) {
        return stream_data_available_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                              stream_handle,
                                              stream_name,
                                              stream_upload_handle,
                                              duration_available,
                                              size_available);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamClosedHandler(UINT64 custom_data,
                                                    STREAM_HANDLE stream_handle,
                                                    UPLOAD_HANDLE stream_upload_handle) {
    LOG_DEBUG("streamClosedHandler invoked for upload handle: " << stream_upload_handle);

    auto this_obj = reinterpret_cast<DefaultCallbackProvider *>(custom_data);

    auto stream_eos_callback = this_obj->stream_callback_provider_->getStreamClosedCallback();
    if (nullptr != stream_eos_callback) {
        STATUS status = stream_eos_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                            stream_handle,
                                            stream_upload_handle);
        if (STATUS_FAILED(status)) {
            LOG_ERROR("streamClosedHandler failed with: " << status);
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Handles stream fragment errors.
 *
 * @param custom_data Custom handle passed by the caller (this class)
 * @param STREAM_HANDLE stream handle for the stream
 * @param UPLOAD_HANDLE the current stream upload handle
 * @param UINT64 errored fragment timecode
 * @param STATUS status code of the failure
 * @return Status of the callback
 */
STATUS DefaultCallbackProvider::streamErrorHandler(UINT64 custom_data,
                                                   STREAM_HANDLE stream_handle,
                                                   UPLOAD_HANDLE upload_handle,
                                                   UINT64 fragment_timecode,
                                                   STATUS status) {
    LOG_DEBUG("streamErrorHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto stream_error_callback = this_obj->stream_callback_provider_->getStreamErrorReportCallback();
    if (nullptr != stream_error_callback) {
        return stream_error_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                     stream_handle,
                                     upload_handle,
                                     fragment_timecode,
                                     status);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::clientReadyHandler(UINT64 custom_data, CLIENT_HANDLE client_handle) {
    LOG_DEBUG("clientReadyHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto client_ready_callback = this_obj->client_callback_provider_->getClientReadyCallback();
    if (nullptr != client_ready_callback) {
        return client_ready_callback(this_obj->client_callback_provider_->getCallbackCustomData(), client_handle);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::storageOverflowPressureHandler(UINT64 custom_data, UINT64 bytes_remaining) {
    LOG_DEBUG("storageOverflowPressureHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto storage_pressure_callback = this_obj->client_callback_provider_->getStorageOverflowPressureCallback();
    if (nullptr != storage_pressure_callback) {
        return storage_pressure_callback(this_obj->client_callback_provider_->getCallbackCustomData(), bytes_remaining);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamUnderflowReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle) {
    LOG_DEBUG("streamUnderflowReportHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto stream_underflow_callback = this_obj->stream_callback_provider_->getStreamUnderflowReportCallback();
    if (nullptr != stream_underflow_callback) {
        return stream_underflow_callback(this_obj->stream_callback_provider_->getCallbackCustomData(), stream_handle);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data,
                                                             STREAM_HANDLE stream_handle,
                                                             UINT64 buffer_duration) {
    LOG_DEBUG("streamLatencyPressureHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto stream_latency_callback = this_obj->stream_callback_provider_->getStreamLatencyPressureCallback();
    if (nullptr != stream_latency_callback) {
        return stream_latency_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                       stream_handle,
                                       buffer_duration);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::droppedFrameReportHandler(UINT64 custom_data,
                                                          STREAM_HANDLE stream_handle,
                                                          UINT64 timecode) {
    LOG_DEBUG("droppedFrameReportHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto dropped_frame_callback = this_obj->stream_callback_provider_->getDroppedFrameReportCallback();
    if (nullptr != dropped_frame_callback) {
        return dropped_frame_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                      stream_handle,
                                      timecode);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::droppedFragmentReportHandler(UINT64 custom_data,
                                                             STREAM_HANDLE stream_handle,
                                                             UINT64 timecode) {
    LOG_DEBUG("droppedFragmentReportHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto dropped_fragment_callback = this_obj->stream_callback_provider_->getDroppedFragmentReportCallback();
    if (nullptr != dropped_fragment_callback) {
        return dropped_fragment_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                         stream_handle,
                                         timecode);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::bufferDurationOverflowPressureHandler(UINT64 custom_data,
                                                                      STREAM_HANDLE stream_handle,
                                                                      UINT64 remaining_duration) {
    LOG_DEBUG("bufferDurationOverflowPressureHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto buffer_duration_overflow_pressure_callback = this_obj->stream_callback_provider_->getBufferDurationOverflowPressureCallback();
    if (nullptr != buffer_duration_overflow_pressure_callback) {
        return buffer_duration_overflow_pressure_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                                          stream_handle,
                                                          remaining_duration);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                             STREAM_HANDLE stream_handle,
                                                             UINT64 last_ack_duration) {
    LOG_DEBUG("streamConnectionStaleHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto connection_stale_callback = this_obj->stream_callback_provider_->getStreamConnectionStaleCallback();
    if (nullptr != connection_stale_callback) {
        return connection_stale_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                         stream_handle,
                                         last_ack_duration);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::streamReadyHandler(UINT64 custom_data, STREAM_HANDLE stream_handle) {
    LOG_DEBUG("streamReadyHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto stream_ready_callback = this_obj->stream_callback_provider_->getStreamReadyCallback();
    if (nullptr != stream_ready_callback) {
        return stream_ready_callback(this_obj->stream_callback_provider_->getCallbackCustomData(), stream_handle);
    } else {
        return STATUS_SUCCESS;
    }
}

STATUS DefaultCallbackProvider::fragmentAckReceivedHandler(UINT64 custom_data,
                                                           STREAM_HANDLE stream_handle,
                                                           UPLOAD_HANDLE uploadHandle,
                                                           PFragmentAck fragment_ack) {
    LOG_DEBUG("fragmentAckReceivedHandler invoked");
    auto this_obj = reinterpret_cast<DefaultCallbackProvider*>(custom_data);

    // Call the client callback if any specified
    auto fragment_ack_callback = this_obj->stream_callback_provider_->getFragmentAckReceivedCallback();
    if (nullptr != fragment_ack_callback) {
        return fragment_ack_callback(this_obj->stream_callback_provider_->getCallbackCustomData(),
                                     stream_handle,
                                     uploadHandle,
                                     fragment_ack);
    } else {
        return STATUS_SUCCESS;
    }
}

VOID DefaultCallbackProvider::logPrintHandler(UINT32 level, PCHAR tag, PCHAR fmt, ...) {
    static log4cplus::LogLevel picLevelToLog4cplusLevel[] = {
            log4cplus::TRACE_LOG_LEVEL,
            log4cplus::TRACE_LOG_LEVEL,
            log4cplus::DEBUG_LOG_LEVEL,
            log4cplus::INFO_LOG_LEVEL,
            log4cplus::WARN_LOG_LEVEL,
            log4cplus::ERROR_LOG_LEVEL,
            log4cplus::FATAL_LOG_LEVEL};
    UNUSED_PARAM(tag);
    va_list valist;
    log4cplus::LogLevel logLevel = log4cplus::TRACE_LOG_LEVEL;
    if (level >= LOG_LEVEL_VERBOSE && level <= LOG_LEVEL_FATAL) {
        logLevel = picLevelToLog4cplusLevel[level];
    }

    va_start(valist, fmt);
    auto logger = KinesisVideoLogger::getInstance();

    // This implementation is pulled from LOG4CPLUS_MACRO_FMT_BODY
    // Modified _snpbuf.print_va_list(va_list) to accept va_list instead of _snpbuf.print(arg...)
    LOG4CPLUS_SUPPRESS_DOWHILE_WARNING()
    do {
        log4cplus::Logger const & _l
            = log4cplus::detail::macros_get_logger (logger);
        if (_l.isEnabledFor (logLevel)) {
            LOG4CPLUS_MACRO_INSTANTIATE_SNPRINTF_BUF (_snpbuf);
            log4cplus::tchar const * _logEvent;
            _snpbuf.print_va_list (_logEvent, fmt, valist);
            log4cplus::detail::macro_forced_log (_l,
                logLevel, _logEvent,
                __FILE__, __LINE__, LOG4CPLUS_MACRO_FUNCTION ());
        }
    } while(0);
    LOG4CPLUS_RESTORE_DOWHILE_WARNING()

    va_end(valist);
}

DefaultCallbackProvider::DefaultCallbackProvider(
        unique_ptr <ClientCallbackProvider> client_callback_provider,
        unique_ptr <StreamCallbackProvider> stream_callback_provider,
        unique_ptr <CredentialProvider> credentials_provider,
        const string& region,
        const string& control_plane_uri,
        const std::string &user_agent_name,
        const std::string &custom_user_agent,
        const std::string &cert_path,
        bool is_caching_endpoint,
        uint64_t caching_update_period)
        : region_(region),
          service_(std::string(KINESIS_VIDEO_SERVICE_NAME)),
          control_plane_uri_(control_plane_uri),
          cert_path_(cert_path) {
    STATUS retStatus = STATUS_SUCCESS;
    client_callback_provider_ = move(client_callback_provider);
    stream_callback_provider_ = move(stream_callback_provider);
    credentials_provider_ = move(credentials_provider);
    PStreamCallbacks pContinuoutsRetryStreamCallbacks = NULL;
    std::string custom_user_agent_ = CPP_SDK_CUSTOM_USERAGENT + custom_user_agent;

    if (control_plane_uri_.empty()) {
        // Create a fully qualified URI
        control_plane_uri_ = CONTROL_PLANE_URI_PREFIX
                             + std::string(KINESIS_VIDEO_SERVICE_NAME)
                             + "."
                             + region_
                             + CONTROL_PLANE_URI_POSTFIX;
    }

    getStreamCallbacks();
    getProducerCallbacks();
    getPlatformCallbacks();
    if (STATUS_FAILED(retStatus = createAbstractDefaultCallbacksProvider(
            DEFAULT_CALLBACK_CHAIN_COUNT,
            API_CALL_CACHE_TYPE_ALL,
            caching_update_period,
            STRING_TO_PCHAR(region),
            STRING_TO_PCHAR(control_plane_uri),
            STRING_TO_PCHAR(cert_path),
            (PCHAR) DEFAULT_USER_AGENT_NAME,
            STRING_TO_PCHAR(custom_user_agent_),
            &client_callbacks_))) {
        std::stringstream status_strstrm;
        status_strstrm << std::hex << retStatus;
        LOG_AND_THROW("Unable to create default callback provider. Error status: 0x" + status_strstrm.str());
    }
    auth_callbacks_ = credentials_provider_->getCallbacks(client_callbacks_);
    addStreamCallbacks(client_callbacks_, &stream_callbacks_);
    addProducerCallbacks(client_callbacks_, &producer_callbacks_);
    setPlatformCallbacks(client_callbacks_, &platform_callbacks_);
    createContinuousRetryStreamCallbacks(client_callbacks_, &pContinuoutsRetryStreamCallbacks);
}

DefaultCallbackProvider::~DefaultCallbackProvider() {
    freeCallbacksProvider(&client_callbacks_);
}

StreamCallbacks DefaultCallbackProvider::getStreamCallbacks() {
    MEMSET(&stream_callbacks_, 0, SIZEOF(stream_callbacks_));
    stream_callbacks_.customData = reinterpret_cast<uintptr_t>(this);
    stream_callbacks_.version = STREAM_CALLBACKS_CURRENT_VERSION;  // from kinesis video cproducer include
    stream_callbacks_.streamReadyFn = getStreamReadyCallback();
    stream_callbacks_.streamClosedFn = getStreamClosedCallback();
    stream_callbacks_.streamLatencyPressureFn = getStreamLatencyPressureCallback();
    stream_callbacks_.streamErrorReportFn = getStreamErrorReportCallback();
    stream_callbacks_.streamConnectionStaleFn = getStreamConnectionStaleCallback();
    stream_callbacks_.streamDataAvailableFn = getStreamDataAvailableCallback();
    stream_callbacks_.streamUnderflowReportFn = getStreamUnderflowReportCallback();
    stream_callbacks_.streamShutdownFn = getStreamShutdownCallback();
    stream_callbacks_.droppedFragmentReportFn = getDroppedFragmentReportCallback();
    stream_callbacks_.droppedFrameReportFn = getDroppedFrameReportCallback();
    stream_callbacks_.bufferDurationOverflowPressureFn = getBufferDurationOverflowPressureCallback();
    stream_callbacks_.fragmentAckReceivedFn = getFragmentAckReceivedCallback();
    return stream_callbacks_;
}

ProducerCallbacks DefaultCallbackProvider::getProducerCallbacks() {
    MEMSET(&producer_callbacks_, 0, SIZEOF(producer_callbacks_));
    producer_callbacks_.customData = reinterpret_cast<uintptr_t>(this);
    producer_callbacks_.version = PRODUCER_CALLBACKS_CURRENT_VERSION;  // from kinesis video cproducer include
    producer_callbacks_.storageOverflowPressureFn = getStorageOverflowPressureCallback();
    producer_callbacks_.clientReadyFn = getClientReadyCallback();
    producer_callbacks_.clientShutdownFn = getClientShutdownCallback();
    return producer_callbacks_;
}

PlatformCallbacks DefaultCallbackProvider::getPlatformCallbacks() {
    MEMSET(&platform_callbacks_, 0, SIZEOF(platform_callbacks_));
    platform_callbacks_.customData = reinterpret_cast<uintptr_t>(this);
    platform_callbacks_.version = PLATFORM_CALLBACKS_CURRENT_VERSION;  // from kinesis video cproducer include
    platform_callbacks_.logPrintFn = getLogPrintCallback();
    return platform_callbacks_;
}

DefaultCallbackProvider::callback_t DefaultCallbackProvider::getCallbacks() {
    return *client_callbacks_;
}

GetStreamingTokenFunc DefaultCallbackProvider::getStreamingTokenCallback() {
    return auth_callbacks_.getStreamingTokenFn;
}

GetSecurityTokenFunc DefaultCallbackProvider::getSecurityTokenCallback() {
    return auth_callbacks_.getSecurityTokenFn;
}

DeviceCertToTokenFunc DefaultCallbackProvider::getDeviceCertToTokenCallback() {
    return auth_callbacks_.deviceCertToTokenFn;
}

GetDeviceCertificateFunc DefaultCallbackProvider::getDeviceCertificateCallback() {
    return auth_callbacks_.getDeviceCertificateFn;
}

GetDeviceFingerprintFunc DefaultCallbackProvider::getDeviceFingerprintCallback() {
     return auth_callbacks_.getDeviceFingerprintFn;
}

GetCurrentTimeFunc DefaultCallbackProvider::getCurrentTimeCallback() {
    return getCurrentTimeHandler;
}

DroppedFragmentReportFunc DefaultCallbackProvider::getDroppedFragmentReportCallback() {
    return droppedFragmentReportHandler;
}

BufferDurationOverflowPressureFunc DefaultCallbackProvider::getBufferDurationOverflowPressureCallback(){
    return bufferDurationOverflowPressureHandler;
}

StreamReadyFunc DefaultCallbackProvider::getStreamReadyCallback() {
    return streamReadyHandler;
}

StreamClosedFunc DefaultCallbackProvider::getStreamClosedCallback() {
    return streamClosedHandler;
}

FragmentAckReceivedFunc DefaultCallbackProvider::getFragmentAckReceivedCallback() {
    return fragmentAckReceivedHandler;
}

StreamUnderflowReportFunc DefaultCallbackProvider::getStreamUnderflowReportCallback() {
    return streamUnderflowReportHandler;
}

StorageOverflowPressureFunc DefaultCallbackProvider::getStorageOverflowPressureCallback() {
    return storageOverflowPressureHandler;
}

StreamLatencyPressureFunc DefaultCallbackProvider::getStreamLatencyPressureCallback() {
    return streamLatencyPressureHandler;
}

DroppedFrameReportFunc DefaultCallbackProvider::getDroppedFrameReportCallback() {
    return droppedFrameReportHandler;
}

StreamErrorReportFunc DefaultCallbackProvider::getStreamErrorReportCallback() {
    return streamErrorHandler;
}

ClientReadyFunc DefaultCallbackProvider::getClientReadyCallback() {
    return clientReadyHandler;
}

CreateDeviceFunc DefaultCallbackProvider::getCreateDeviceCallback() {
    return createDeviceHandler;
}

StreamDataAvailableFunc DefaultCallbackProvider::getStreamDataAvailableCallback() {
    return streamDataAvailableHandler;
}

StreamConnectionStaleFunc DefaultCallbackProvider::getStreamConnectionStaleCallback() {
    return streamConnectionStaleHandler;
}

CreateStreamFunc DefaultCallbackProvider::getCreateStreamCallback() {
    return nullptr;
}

DescribeStreamFunc DefaultCallbackProvider::getDescribeStreamCallback() {
    return nullptr;
}

GetStreamingEndpointFunc DefaultCallbackProvider::getStreamingEndpointCallback() {
    return nullptr;
}

PutStreamFunc DefaultCallbackProvider::getPutStreamCallback() {
    return nullptr;
}

TagResourceFunc DefaultCallbackProvider::getTagResourceCallback() {
    return nullptr;
}

LogPrintFunc DefaultCallbackProvider::getLogPrintCallback() {
    return logPrintHandler;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
