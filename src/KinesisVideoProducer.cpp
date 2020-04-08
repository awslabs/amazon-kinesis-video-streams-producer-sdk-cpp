#include "KinesisVideoProducer.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::create(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<ClientCallbackProvider> client_callback_provider,
        unique_ptr<StreamCallbackProvider> stream_callback_provider,
        unique_ptr<CredentialProvider> credential_provider,
        const std::string &region,
        const std::string &control_plane_uri,
        const std::string &user_agent_name) {

    unique_ptr<DefaultCallbackProvider> callback_provider(new DefaultCallbackProvider(move(client_callback_provider),
            move(stream_callback_provider),
            move(credential_provider),
            region,
            control_plane_uri,
            user_agent_name,
            device_info_provider->getCustomUserAgent()));

    return KinesisVideoProducer::create(move(device_info_provider), move(callback_provider));
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::create(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<CallbackProvider> callback_provider) {

    CLIENT_HANDLE client_handle;
    DeviceInfo device_info = device_info_provider->getDeviceInfo();

    // Create the producer object
    std::unique_ptr<KinesisVideoProducer> kinesis_video_producer(new KinesisVideoProducer());

    auto callbacks = callback_provider->getCallbacks();

    STATUS status = createKinesisVideoClient(&device_info, &callbacks, &client_handle);
    if (STATUS_FAILED(status)) {
        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_AND_THROW(" Unable to create Kinesis Video client. Error status: 0x" + status_strstrm.str());
    }

    kinesis_video_producer->client_handle_ = client_handle;
    kinesis_video_producer->callback_provider_ = move(callback_provider);

    return kinesis_video_producer;
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::createSync(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<ClientCallbackProvider> client_callback_provider,
        unique_ptr<StreamCallbackProvider> stream_callback_provider,
        unique_ptr<CredentialProvider> credential_provider,
        const std::string &region,
        const std::string &control_plane_uri,
        const std::string &user_agent_name,
        bool is_caching_endpoint,
        uint64_t caching_update_period) {

    unique_ptr<DefaultCallbackProvider> callback_provider(new DefaultCallbackProvider(move(client_callback_provider),
            move(stream_callback_provider),
            move(credential_provider),
            region,
            control_plane_uri,
            user_agent_name,
            device_info_provider->getCustomUserAgent(),
            device_info_provider->getCertPath(),
            is_caching_endpoint,
            caching_update_period));

    return KinesisVideoProducer::createSync(move(device_info_provider), move(callback_provider));
}

unique_ptr<KinesisVideoProducer> KinesisVideoProducer::createSync(
        unique_ptr<DeviceInfoProvider> device_info_provider,
        unique_ptr<CallbackProvider> callback_provider) {

    CLIENT_HANDLE client_handle;
    DeviceInfo device_info = device_info_provider->getDeviceInfo();

    // Create the producer object
    std::unique_ptr<KinesisVideoProducer> kinesis_video_producer(new KinesisVideoProducer());

    auto callbacks = callback_provider->getCallbacks();

    STATUS status = createKinesisVideoClientSync(&device_info, &callbacks, &client_handle);
    if (STATUS_FAILED(status)) {
        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_AND_THROW(" Unable to create Kinesis Video client. Error status: 0x" + status_strstrm.str());
    }

    kinesis_video_producer->client_handle_ = client_handle;
    kinesis_video_producer->callback_provider_ = move(callback_provider);

    return kinesis_video_producer;
}

shared_ptr<KinesisVideoStream> KinesisVideoProducer::createStream(unique_ptr<StreamDefinition> stream_definition) {
    assert(stream_definition.get());

    if (stream_definition->getTrackCount() > MAX_SUPPORTED_TRACK_COUNT_PER_STREAM) {
        LOG_AND_THROW("Exceeded maximum track count: " + std::to_string(MAX_SUPPORTED_TRACK_COUNT_PER_STREAM));
    }
    StreamInfo stream_info = stream_definition->getStreamInfo();
    std::shared_ptr<KinesisVideoStream> kinesis_video_stream(new KinesisVideoStream(*this, stream_definition->getStreamName()), KinesisVideoStream::videoStreamDeleter);
    STATUS status = createKinesisVideoStream(client_handle_, &stream_info, kinesis_video_stream->getStreamHandle());

    if (STATUS_FAILED(status)) {
        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_AND_THROW(" Unable to create Kinesis Video stream. " + stream_definition->getStreamName() +
                  " Error status: 0x" + status_strstrm.str());
    }

    // Add to the map
    active_streams_.put(*kinesis_video_stream->getStreamHandle(), kinesis_video_stream);

    return kinesis_video_stream;
}

shared_ptr<KinesisVideoStream> KinesisVideoProducer::createStreamSync(unique_ptr<StreamDefinition> stream_definition) {

    assert(stream_definition.get());

    if (stream_definition->getTrackCount() > MAX_SUPPORTED_TRACK_COUNT_PER_STREAM) {
        LOG_AND_THROW("Exceeded maximum track count: " + std::to_string(MAX_SUPPORTED_TRACK_COUNT_PER_STREAM));
    }
    StreamInfo stream_info = stream_definition->getStreamInfo();
    std::shared_ptr<KinesisVideoStream> kinesis_video_stream(new KinesisVideoStream(*this, stream_definition->getStreamName()), KinesisVideoStream::videoStreamDeleter);
    STATUS status = createKinesisVideoStreamSync(client_handle_, &stream_info, kinesis_video_stream->getStreamHandle());

    if (STATUS_FAILED(status)) {
        stringstream status_strstrm;
        status_strstrm << std::hex << status;
        LOG_AND_THROW(" Unable to create Kinesis Video stream. " + stream_definition->getStreamName() +
                  " Error status: 0x" + status_strstrm.str());
    }

    // Add to the map
    active_streams_.put(*kinesis_video_stream->getStreamHandle(), kinesis_video_stream);

    return kinesis_video_stream;
}

void KinesisVideoProducer::freeStream(std::shared_ptr<KinesisVideoStream> kinesis_video_stream) {
    if (nullptr == kinesis_video_stream) {
        LOG_AND_THROW("Kinesis Video stream can't be null");
    }

    // Get and save the stream handle
    STREAM_HANDLE stream_handle = *kinesis_video_stream->getStreamHandle();

    // Stop the ongoing CURL operations
    callback_provider_->shutdownStream(stream_handle);

    // Free the stream object itself
    kinesis_video_stream->free();

    // Find the stream and remove it from the map
    active_streams_.remove(stream_handle);
}

void KinesisVideoProducer::freeStreams() {
    {
        std::lock_guard<std::mutex> lock(free_client_mutex_);
        auto num_streams = active_streams_.getMap().size();

        for (auto i = 0; i < num_streams; i++) {
            auto stream = active_streams_.getAt(0);
            freeStream(stream);
        }
    }
}

KinesisVideoProducer::~KinesisVideoProducer() {
    // Free the streams
    freeStreams();

    // Freeing the underlying client object
    freeKinesisVideoClient();
}

KinesisVideoProducerMetrics KinesisVideoProducer::getMetrics() const {
    STATUS status = ::getKinesisVideoMetrics(client_handle_, (PClientMetrics) client_metrics_.getRawMetrics());
    LOG_AND_THROW_IF(STATUS_FAILED(status), "Failed to get producer metrics with: " << status);

    return client_metrics_;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
