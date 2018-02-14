#include "KinesisVideoProducer.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::create(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<ClientCallbackProvider> client_callback_provider,
        unique_ptr<StreamCallbackProvider> stream_callback_provider,
        unique_ptr<CredentialProvider> credential_provider,
        const std::string &region,
        const std::string &control_plane_uri) {

    auto callback_provider = make_unique<DefaultCallbackProvider>(move(client_callback_provider),
                                                                  move(stream_callback_provider),
                                                                  move(credential_provider),
                                                                  region,
                                                                  control_plane_uri);

    return KinesisVideoProducer::create(move(device_info_provider), move(callback_provider));
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::create(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<CallbackProvider> callback_provider) {

    CLIENT_HANDLE client_handle;
    DeviceInfo device_info = device_info_provider->getDeviceInfo();

    // Create the producer object
    auto kinesis_video_producer = new KinesisVideoProducer();

    kinesis_video_producer->stored_callbacks_ = callback_provider->getCallbacks();

    ClientCallbacks override_callbacks;
    override_callbacks.customData = reinterpret_cast<UINT64> (kinesis_video_producer);
    override_callbacks.version = CALLBACKS_CURRENT_VERSION;
    override_callbacks.getDeviceCertificateFn = kinesis_video_producer->stored_callbacks_.getDeviceCertificateFn == NULL ? NULL : KinesisVideoProducer::getDeviceCertificateFunc;
    override_callbacks.getSecurityTokenFn = kinesis_video_producer->stored_callbacks_.getSecurityTokenFn == NULL ? NULL : KinesisVideoProducer::getSecurityTokenFunc;
    override_callbacks.getDeviceFingerprintFn = kinesis_video_producer->stored_callbacks_.getDeviceFingerprintFn == NULL ? NULL : KinesisVideoProducer::getDeviceFingerprintFunc;
    override_callbacks.streamUnderflowReportFn = kinesis_video_producer->stored_callbacks_.streamUnderflowReportFn == NULL ? NULL : KinesisVideoProducer::streamUnderflowReportFunc;
    override_callbacks.storageOverflowPressureFn = kinesis_video_producer->stored_callbacks_.storageOverflowPressureFn == NULL ? NULL : KinesisVideoProducer::storageOverflowPressureFunc;
    override_callbacks.streamLatencyPressureFn = kinesis_video_producer->stored_callbacks_.streamLatencyPressureFn == NULL ? NULL : KinesisVideoProducer::streamLatencyPressureFunc;
    override_callbacks.droppedFrameReportFn = kinesis_video_producer->stored_callbacks_.droppedFrameReportFn == NULL ? NULL : KinesisVideoProducer::droppedFrameReportFunc;
    override_callbacks.streamClosedFn = kinesis_video_producer->stored_callbacks_.streamClosedFn == NULL ? NULL : KinesisVideoProducer::streamClosedFunc;
    override_callbacks.droppedFragmentReportFn = kinesis_video_producer->stored_callbacks_.droppedFragmentReportFn == NULL ? NULL : KinesisVideoProducer::droppedFragmentReportFunc;
    override_callbacks.streamErrorReportFn = kinesis_video_producer->stored_callbacks_.streamErrorReportFn == NULL ? NULL : KinesisVideoProducer::streamErrorReportFunc;
    override_callbacks.createStreamFn = kinesis_video_producer->stored_callbacks_.createStreamFn == NULL ? NULL : KinesisVideoProducer::createStreamFunc;
    override_callbacks.describeStreamFn = kinesis_video_producer->stored_callbacks_.describeStreamFn == NULL ? NULL : KinesisVideoProducer::describeStreamFunc;
    override_callbacks.getStreamingEndpointFn = kinesis_video_producer->stored_callbacks_.getStreamingEndpointFn == NULL ? NULL : KinesisVideoProducer::getStreamingEndpointFunc;
    override_callbacks.getStreamingTokenFn = kinesis_video_producer->stored_callbacks_.getStreamingTokenFn == NULL ? NULL : KinesisVideoProducer::getStreamingTokenFunc;
    override_callbacks.putStreamFn = kinesis_video_producer->stored_callbacks_.putStreamFn == NULL ? NULL : KinesisVideoProducer::putStreamFunc;
    override_callbacks.tagResourceFn = kinesis_video_producer->stored_callbacks_.tagResourceFn == NULL ? NULL : KinesisVideoProducer::tagResourceFunc;
    override_callbacks.createDeviceFn = kinesis_video_producer->stored_callbacks_.createDeviceFn == NULL ? NULL : KinesisVideoProducer::createDeviceFunc;
    override_callbacks.deviceCertToTokenFn = kinesis_video_producer->stored_callbacks_.deviceCertToTokenFn == NULL ? NULL : KinesisVideoProducer::deviceCertToTokenFunc;
    override_callbacks.streamConnectionStaleFn = kinesis_video_producer->stored_callbacks_.streamConnectionStaleFn == NULL ? NULL : KinesisVideoProducer::streamConnectionStaleFunc;
    override_callbacks.streamDataAvailableFn = kinesis_video_producer->stored_callbacks_.streamDataAvailableFn == NULL ? NULL : KinesisVideoProducer::streamDataAvailableFunc;
    override_callbacks.fragmentAckReceivedFn = kinesis_video_producer->stored_callbacks_.fragmentAckReceivedFn == NULL ? NULL : KinesisVideoProducer::fragmentAckReceivedFunc;
    override_callbacks.createMutexFn = kinesis_video_producer->stored_callbacks_.createMutexFn == NULL ? NULL : KinesisVideoProducer::createMutexFunc;
    override_callbacks.lockMutexFn = kinesis_video_producer->stored_callbacks_.lockMutexFn == NULL ? NULL : KinesisVideoProducer::lockMutexFunc;
    override_callbacks.unlockMutexFn = kinesis_video_producer->stored_callbacks_.unlockMutexFn == NULL ? NULL : KinesisVideoProducer::unlockMutexFunc;
    override_callbacks.tryLockMutexFn = kinesis_video_producer->stored_callbacks_.tryLockMutexFn == NULL ? NULL : KinesisVideoProducer::tryLockMutexFunc;
    override_callbacks.freeMutexFn = kinesis_video_producer->stored_callbacks_.freeMutexFn == NULL ? NULL : KinesisVideoProducer::freeMutexFunc;
    override_callbacks.getCurrentTimeFn = kinesis_video_producer->stored_callbacks_.getCurrentTimeFn == NULL ? NULL : KinesisVideoProducer::getCurrentTimeFunc;
    override_callbacks.getRandomNumberFn = kinesis_video_producer->stored_callbacks_.getRandomNumberFn == NULL ? NULL : KinesisVideoProducer::getRandomNumberFunc;

    // Override the client and the stream ready API
    override_callbacks.clientReadyFn = KinesisVideoProducer::clientReadyFunc;
    override_callbacks.streamReadyFn = KinesisVideoProducer::streamReadyFunc;


    STATUS status = createKinesisVideoClient(&device_info, &override_callbacks, &client_handle);
    if (STATUS_FAILED(status)) {
        // Delete the producer on error
        delete kinesis_video_producer;

        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_AND_THROW(" Unable to create Kinesis Video client. Error status: 0x" + status_strstrm.str());
    }

    kinesis_video_producer->client_handle_ = client_handle;
    kinesis_video_producer->callback_provider_ = move(callback_provider);

    return unique_ptr<KinesisVideoProducer>(kinesis_video_producer);
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::createSync(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<ClientCallbackProvider> client_callback_provider,
        unique_ptr<StreamCallbackProvider> stream_callback_provider,
        unique_ptr<CredentialProvider> credential_provider,
        const std::string &region,
        const std::string &control_plane_uri) {

    auto callback_provider = make_unique<DefaultCallbackProvider>(move(client_callback_provider),
                                                                  move(stream_callback_provider),
                                                                  move(credential_provider),
                                                                  region,
                                                                  control_plane_uri);

    return KinesisVideoProducer::createSync(move(device_info_provider), move(callback_provider));
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::createSync(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<CallbackProvider> callback_provider) {

    auto kinesis_video_producer = move(KinesisVideoProducer::create(move(device_info_provider), move(callback_provider)));

    if (nullptr == kinesis_video_producer) {
        LOG_AND_THROW("Failed to create Kinesis Video client");
    }

    {
        LOG_DEBUG("Awaiting for the producer to become ready...");
        unique_lock<mutex> lock(kinesis_video_producer->client_ready_mutex_);
        do {
            if (kinesis_video_producer->client_ready_) {
                LOG_DEBUG("Kinesis Video producer is Ready.");
                break;
            }

            // Blocking path
            // Wait may return without any data available due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            if (!kinesis_video_producer->client_ready_var_.wait_for(lock,
                                                             std::chrono::seconds(
                                                                     CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS),
                                                             [kinesis_video_producer = kinesis_video_producer.get()]() {
                                                                 return kinesis_video_producer->client_ready_;
                                                             })) {
                LOG_AND_THROW("Failed to create Kinesis Video Producer - timed out.");
                break;
            }
        } while (true);
    }

    return kinesis_video_producer;
}

shared_ptr<KinesisVideoStream> KinesisVideoProducer::createStream(unique_ptr<StreamDefinition> stream_definition) {
    assert(stream_definition.get());

    StreamInfo stream_info = stream_definition->getStreamInfo();
    std::shared_ptr<KinesisVideoStream> kinesis_video_stream(new KinesisVideoStream(*this, stream_definition->getStreamName()));
    STATUS status = createKinesisVideoStream(client_handle_, &stream_info, kinesis_video_stream->getStreamHandle());

    if (STATUS_FAILED(status)) {
        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_ERROR(" Unable to create Kinesis Video stream. " + stream_definition->getStreamName() +
                  " Error status: 0x" + status_strstrm.str());
        return nullptr;
    }

    // Add to the map
    active_streams_.put(*kinesis_video_stream->getStreamHandle(), kinesis_video_stream);

    return kinesis_video_stream;
}

shared_ptr<KinesisVideoStream> KinesisVideoProducer::createStreamSync(unique_ptr<StreamDefinition> stream_definition) {

    auto kinesis_video_stream = move(KinesisVideoProducer::createStream(move(stream_definition)));

    if (nullptr == kinesis_video_stream) {
        LOG_AND_THROW("Failed to create Kinesis Video stream");
    }

    {
        LOG_DEBUG("Awaiting for the stream to become ready...");
        unique_lock<mutex> lock(kinesis_video_stream->getStreamMutex());
        do {
            if (kinesis_video_stream->isReady()) {
                LOG_DEBUG("Kinesis Video stream " << kinesis_video_stream->getStreamName() << " is Ready.");
                break;
            }

            // Blocking path
            // Wait may return due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            if (!kinesis_video_stream->getStreamReadyVar().wait_for(lock,
                                                             std::chrono::seconds(
                                                                     STREAM_READY_TIMEOUT_DURATION_IN_SECONDS),
                                                             [kinesis_video_stream = kinesis_video_stream.get()]() {
                                                                 return kinesis_video_stream->isReady();
                                                             })) {
                LOG_AND_THROW("Failed to create Kinesis Video Stream - timed out.");
                break;
            }
        } while (true);
    }

    return kinesis_video_stream;
}

void KinesisVideoProducer::freeStream(std::shared_ptr<KinesisVideoStream> kinesis_video_stream) {
    if (nullptr == kinesis_video_stream) {
        LOG_AND_THROW("Kinesis Video stream can't be null");
    }

    // Stop the ongoing CURL operations
    callback_provider_->shutdownStream(*kinesis_video_stream->getStreamHandle());

    // Find the stream and remove it from the map
    active_streams_.remove(*kinesis_video_stream->getStreamHandle());
}

KinesisVideoProducer::~KinesisVideoProducer() {
    freeKinesisVideoClient();
}

/**
 * Client ready override callback
 */
STATUS KinesisVideoProducer::clientReadyFunc(UINT64 custom_data,
                                             CLIENT_HANDLE client_handle) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);

    // Trigger the client ready state
    {
        std::lock_guard<std::mutex> lock(this_obj->client_ready_mutex_);
        this_obj->client_ready_ = true;
        this_obj->client_ready_var_.notify_one();
    }

    // Call the stored callback if specified
    if (nullptr != this_obj->stored_callbacks_.clientReadyFn) {
        return this_obj->stored_callbacks_.clientReadyFn(this_obj->stored_callbacks_.customData,
                                                         client_handle);
    } else {
        return STATUS_SUCCESS;
    }
}

/**
 * Stream ready override callback
 */
STATUS KinesisVideoProducer::streamReadyFunc(UINT64 custom_data,
                                             STREAM_HANDLE stream_handle) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    auto kinesis_video_stream = this_obj->active_streams_.get(stream_handle);
    assert(nullptr != kinesis_video_stream);

    // Trigger the stream ready state
    {
        std::lock_guard<std::mutex> lock(kinesis_video_stream->getStreamMutex());
        kinesis_video_stream->streamReady();
        kinesis_video_stream->getStreamReadyVar().notify_one();
    }

    // Call the stored callback if specified
    if (nullptr != this_obj->stored_callbacks_.streamReadyFn) {
        return this_obj->stored_callbacks_.streamReadyFn(this_obj->stored_callbacks_.customData,
                                                         stream_handle);
    } else {
        return STATUS_SUCCESS;
    }
}

uint64_t KinesisVideoProducer::getAvailableStorageSize() const {
    ClientMetrics kinesis_video_client_metrics;

    kinesis_video_client_metrics.version = CLIENT_METRICS_CURRENT_VERSION;
    STATUS retStatus = ::getKinesisVideoMetrics(client_handle_, &kinesis_video_client_metrics);

    LOG_AND_THROW_IF(STATUS_FAILED(retStatus), "Failed to get available storage size with: " << retStatus);

    // Return the allocated storage size
    return kinesis_video_client_metrics.contentStoreAvailableSize;
}

///////////////////////////////////////////////////////////////////
// Rest of the default callback overrides
///////////////////////////////////////////////////////////////////
MUTEX KinesisVideoProducer::createMutexFunc(UINT64 custom_data,
                                            BOOL reentrant) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.createMutexFn(this_obj->stored_callbacks_.customData, 
                                                     reentrant);
}

VOID KinesisVideoProducer::lockMutexFunc(UINT64 custom_data,
                                         MUTEX mutex) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    this_obj->stored_callbacks_.lockMutexFn(this_obj->stored_callbacks_.customData, mutex);
}

VOID KinesisVideoProducer::unlockMutexFunc(UINT64 custom_data,
                                           MUTEX mutex){
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    this_obj->stored_callbacks_.unlockMutexFn(this_obj->stored_callbacks_.customData, mutex);
}
VOID KinesisVideoProducer::tryLockMutexFunc(UINT64 custom_data,
                                            MUTEX mutex) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    this_obj->stored_callbacks_.tryLockMutexFn(this_obj->stored_callbacks_.customData, mutex);
}
VOID KinesisVideoProducer::freeMutexFunc(UINT64 custom_data,
                                         MUTEX mutex) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    this_obj->stored_callbacks_.freeMutexFn(this_obj->stored_callbacks_.customData, mutex);
}

UINT64 KinesisVideoProducer::getCurrentTimeFunc(UINT64 custom_data) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getCurrentTimeFn(this_obj->stored_callbacks_.customData);
}

UINT32 KinesisVideoProducer::getRandomNumberFunc(UINT64 custom_data) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getRandomNumberFn(this_obj->stored_callbacks_.customData);
}

STATUS KinesisVideoProducer::getSecurityTokenFunc(UINT64 custom_data,
                                                  PBYTE *buffer,
                                                  PUINT32 size,
                                                  PUINT64 expiration) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getSecurityTokenFn(this_obj->stored_callbacks_.customData,
                                                          buffer,
                                                          size,
                                                          expiration);
}

STATUS KinesisVideoProducer::getDeviceCertificateFunc(UINT64 custom_data,
                                                      PBYTE* device_cert,
                                                      PUINT32 device_cert_size,
                                                      PUINT64 device_cert_expiration) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getDeviceCertificateFn(this_obj->stored_callbacks_.customData,
                                                              device_cert,
                                                              device_cert_size,
                                                              device_cert_expiration);
}

STATUS KinesisVideoProducer::getDeviceFingerprintFunc(UINT64 custom_data,
                                                      PCHAR* device_fingerprint) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getDeviceFingerprintFn(this_obj->stored_callbacks_.customData,
                                                              device_fingerprint);
}

STATUS KinesisVideoProducer::streamUnderflowReportFunc(UINT64 custom_data,
                                                       STREAM_HANDLE stream_handle) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamUnderflowReportFn(this_obj->stored_callbacks_.customData,
                                                               stream_handle);
}

STATUS KinesisVideoProducer::storageOverflowPressureFunc(UINT64 custom_data,
                                                         UINT64 bytes_remaining) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.storageOverflowPressureFn(this_obj->stored_callbacks_.customData,
                                                                 bytes_remaining);
}

STATUS KinesisVideoProducer::streamLatencyPressureFunc(UINT64 custom_data,
                                                       STREAM_HANDLE stream_handle,
                                                       UINT64 duration) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamLatencyPressureFn(this_obj->stored_callbacks_.customData,
                                                               stream_handle,
                                                               duration);
}

STATUS KinesisVideoProducer::droppedFrameReportFunc(UINT64 custom_data,
                                                    STREAM_HANDLE stream_handle,
                                                    UINT64 timecode) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.droppedFrameReportFn(this_obj->stored_callbacks_.customData,
                                                            stream_handle,
                                                            timecode);
}

STATUS KinesisVideoProducer::droppedFragmentReportFunc(UINT64 custom_data,
                                                       STREAM_HANDLE stream_handle,
                                                       UINT64 timecode) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.droppedFragmentReportFn(this_obj->stored_callbacks_.customData,
                                                               stream_handle,
                                                               timecode);
}

STATUS KinesisVideoProducer::streamClosedFunc(UINT64 custom_data,
                                              STREAM_HANDLE stream_handle,
                                              UINT64 stream_upload_handle) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamClosedFn(this_obj->stored_callbacks_.customData,
                                                      stream_handle,
                                                      stream_upload_handle);
}

STATUS KinesisVideoProducer::streamErrorReportFunc(UINT64 custom_data,
                                                   STREAM_HANDLE stream_handle,
                                                   UINT64 timecode,
                                                   STATUS status) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamErrorReportFn(this_obj->stored_callbacks_.customData,
                                                           stream_handle,
                                                           timecode,
                                                           status);
}

STATUS KinesisVideoProducer::createStreamFunc(UINT64 custom_data,
                                              PCHAR device_name,
                                              PCHAR stream_name,
                                              PCHAR content_type,
                                              PCHAR kms_arn,
                                              UINT64 retention_period,
                                              PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.createStreamFn(this_obj->stored_callbacks_.customData,
                                                      device_name,
                                                      stream_name,
                                                      content_type,
                                                      kms_arn,
                                                      retention_period,
                                                      service_call_ctx);
}

STATUS KinesisVideoProducer::describeStreamFunc(UINT64 custom_data,
                                                PCHAR stream_name,
                                                PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.describeStreamFn(this_obj->stored_callbacks_.customData,
                                                        stream_name,
                                                        service_call_ctx);
}

STATUS KinesisVideoProducer::getStreamingEndpointFunc(UINT64 custom_data,
                                                      PCHAR stream_name,
                                                      PCHAR api_name,
                                                      PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getStreamingEndpointFn(this_obj->stored_callbacks_.customData,
                                                              stream_name,
                                                              api_name,
                                                              service_call_ctx);
}

STATUS KinesisVideoProducer::getStreamingTokenFunc(UINT64 custom_data,
                                                   PCHAR stream_name,
                                                   STREAM_ACCESS_MODE access_mode,
                                                   PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.getStreamingTokenFn(this_obj->stored_callbacks_.customData,
                                                           stream_name,
                                                           access_mode,
                                                           service_call_ctx);
}

STATUS KinesisVideoProducer::putStreamFunc(UINT64 custom_data,
                                           PCHAR stream_name,
                                           PCHAR container_type,
                                           UINT64 start_timestamp,
                                           BOOL absolute_fragment_timestamp,
                                           BOOL do_ack,
                                           PCHAR streaming_endpoint,
                                           PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.putStreamFn(this_obj->stored_callbacks_.customData,
                                                   stream_name,
                                                   container_type,
                                                   start_timestamp,
                                                   absolute_fragment_timestamp,
                                                   do_ack,
                                                   streaming_endpoint,
                                                   service_call_ctx);
}

STATUS KinesisVideoProducer::tagResourceFunc(UINT64 custom_data,
                                             PCHAR stream_arn,
                                             UINT32 num_tags,
                                             PTag tags,
                                             PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.tagResourceFn(this_obj->stored_callbacks_.customData,
                                                     stream_arn,
                                                     num_tags,
                                                     tags,
                                                     service_call_ctx);
}

STATUS KinesisVideoProducer::createDeviceFunc(UINT64 custom_data,
                                              PCHAR device_name,
                                              PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.createDeviceFn(this_obj->stored_callbacks_.customData,
                                                      device_name,
                                                      service_call_ctx);
}

STATUS KinesisVideoProducer::deviceCertToTokenFunc(UINT64 custom_data,
                                                   PCHAR device_name,
                                                   PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.deviceCertToTokenFn(this_obj->stored_callbacks_.customData,
                                                           device_name,
                                                           service_call_ctx);
}

STATUS KinesisVideoProducer::streamDataAvailableFunc(UINT64 custom_data,
                                                     STREAM_HANDLE stream_handle,
                                                     PCHAR stream_name,
                                                     UINT64 stream_upload_handle,
                                                     UINT64 duration_available,
                                                     UINT64 size_available) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamDataAvailableFn(this_obj->stored_callbacks_.customData,
                                                             stream_handle,
                                                             stream_name,
                                                             stream_upload_handle,
                                                             duration_available,
                                                             size_available);
}

STATUS KinesisVideoProducer::streamConnectionStaleFunc(UINT64 custom_data,
                                                       STREAM_HANDLE stream_handle,
                                                       UINT64 duration) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.streamConnectionStaleFn(this_obj->stored_callbacks_.customData,
                                                               stream_handle,
                                                               duration);
}

STATUS KinesisVideoProducer::fragmentAckReceivedFunc(UINT64 custom_data,
                                                     STREAM_HANDLE stream_handle,
                                                     PFragmentAck fragment_ack) {
    auto this_obj = reinterpret_cast<KinesisVideoProducer*>(custom_data);
    return this_obj->stored_callbacks_.fragmentAckReceivedFn(this_obj->stored_callbacks_.customData,
                                                             stream_handle,
                                                             fragment_ack);
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
