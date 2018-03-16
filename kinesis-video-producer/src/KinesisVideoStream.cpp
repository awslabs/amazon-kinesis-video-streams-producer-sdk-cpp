#include "Logger.h"
#include "KinesisVideoStream.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

KinesisVideoStream::KinesisVideoStream(const KinesisVideoProducer& kinesis_video_producer, const std::string stream_name)
        : stream_handle_(INVALID_STREAM_HANDLE_VALUE),
          stream_name_(stream_name),
          kinesis_video_producer_(kinesis_video_producer),
          stream_ready_(false) {
    LOG_INFO("Creating Kinesis Video Stream " << stream_name_);
    // the handle is NULL to start. We will set it later once Kinesis Video PIC gives us a stream handle.
}

bool KinesisVideoStream::putFrame(KinesisVideoFrame frame) const {
    if (!isReady()) {
        LOG_ERROR("Kinesis Video stream is not ready.");
        return false;
    }

    assert(0 != stream_handle_);
    STATUS status = putKinesisVideoFrame(stream_handle_, &frame);
    if (STATUS_FAILED(status)) {
        std::stringstream status_strstrm;
        status_strstrm << "0x" << std::hex << status;
        LOG_ERROR("Failed to submit frame to Kinesis Video client. status: " << status_strstrm.str()
                                                                      << " decoding timestamp: "
                                                                      << frame.decodingTs
                                                                      << " presentation timestamp: "
                                                                      << frame.presentationTs);
        return false;
    }

    // Print metrics on every key-frame
    if ((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) {
        StreamMetrics stream_metrics;
        stream_metrics.version = STREAM_METRICS_CURRENT_VERSION;

        ClientMetrics kinesis_video_client_metrics;
        kinesis_video_client_metrics.version = CLIENT_METRICS_CURRENT_VERSION;

        // Extract metrics and print out
        status = getKinesisVideoMetrics(kinesis_video_producer_.getClientHandle(), &kinesis_video_client_metrics);
        LOG_AND_THROW_IF(STATUS_FAILED(status), "Failed to get client metrics with: " << status);

        status = getKinesisVideoStreamMetrics(stream_handle_, &stream_metrics);
        LOG_AND_THROW_IF(STATUS_FAILED(status), "Failed to get stream metrics with: " << status);

        LOG_DEBUG("Kinesis Video client and stream metrics"
                          << "\n\t>> Overall storage size: " << kinesis_video_client_metrics.contentStoreSize
                          << "\n\t>> Available storage size: " << kinesis_video_client_metrics.contentStoreAvailableSize
                          << "\n\t>> Allocated storage size: " << kinesis_video_client_metrics.contentStoreAllocatedSize
                          << "\n\t>> Total view allocation size: " << kinesis_video_client_metrics.totalContentViewsSize
                          << "\n\t>> Total streams frame rate: " << kinesis_video_client_metrics.totalFrameRate
                          << "\n\t>> Total streams transfer rate: " << kinesis_video_client_metrics.totalTransferRate
                          << "\n\t>> Current view duration: " << stream_metrics.currentViewDuration
                          << "\n\t>> Overall view duration: " << stream_metrics.overallViewDuration
                          << "\n\t>> Current view size: " << stream_metrics.currentViewSize
                          << "\n\t>> Overall view size: " << stream_metrics.overallViewSize
                          << "\n\t>> Current frame rate: " << stream_metrics.currentFrameRate
                          << "\n\t>> Current transfer rate: " << stream_metrics.currentTransferRate);
    }

    return true;
}

bool KinesisVideoStream::start(const std::string& hexEncodedCodecPrivateData) {
    // Hex-decode the string
    const char* pStrCpd = hexEncodedCodecPrivateData.c_str();
    UINT32 size = 0;
    PBYTE pBuffer = nullptr;
    STATUS status = STATUS_SUCCESS;

    if (STATUS_FAILED(status = hexDecode((PCHAR) pStrCpd, NULL, &size))) {
        LOG_ERROR("Failed to get the size of the buffer for hex decoding the codec private data with: " << status);
        return false;
    }

    // Allocate the buffer needed
    pBuffer = reinterpret_cast<PBYTE>(malloc(size));
    if (nullptr == pBuffer) {
        LOG_ERROR("Failed to allocate enough buffer for hex decoding. Size: " << size);
        return false;
    }

    if (STATUS_FAILED(status = hexDecode((PCHAR) pStrCpd, pBuffer, &size))) {
        LOG_ERROR("Failed to hex decode the codec private data with: " << status);
        ::free(pBuffer);
        return false;
    }

    // Start the stream with the binary codec private data buffer
    bool retVal = start(reinterpret_cast<unsigned char*> (pBuffer), size);

    // Free the allocated buffer before returning
    ::free(pBuffer);

    return retVal;
}

bool KinesisVideoStream::start(const unsigned char* codecPrivateData, size_t codecPrivateDataSize) {
    STATUS status = STATUS_SUCCESS;

    if (STATUS_FAILED(status = kinesisVideoStreamFormatChanged(stream_handle_, codecPrivateDataSize,
                                                               (PBYTE) codecPrivateData))) {
        LOG_ERROR("Failed to set the codec private data with: " << status);
        return false;
    }

    // Call the start after setting the CPD
    return start();
}

bool KinesisVideoStream::start() {
    // No-op for now

    return true;
}

bool KinesisVideoStream::resetConnection() {
    STATUS status = STATUS_SUCCESS;

    if (STATUS_FAILED(status = kinesisVideoStreamTerminated(stream_handle_,
                                                            INVALID_UPLOAD_HANDLE_VALUE,
                                                            SERVICE_CALL_RESULT_OK))) {
        LOG_ERROR("Failed to reset the connection with: " << status);
        return false;
    }

    return true;
}

void KinesisVideoStream::free() {
    // Set the ready indicator to false
    stream_ready_ = false;

    LOG_INFO("Freeing Kinesis Video Stream " << stream_name_);

    // Free the underlying stream
    std::call_once(free_kinesis_video_stream_flag_, freeKinesisVideoStream, getStreamHandle());
}

bool KinesisVideoStream::stop() {
    STATUS status = STATUS_SUCCESS;

    if (STATUS_FAILED(status = stopKinesisVideoStream(stream_handle_))) {
        LOG_ERROR("Failed to stop the stream with: " << status);
        return false;
    }

    // Set the ready indicator to false
    stream_ready_ = false;

    return true;
}

void KinesisVideoStream::getStreamMetrics(StreamMetrics& metrics) {
    STATUS status = ::getKinesisVideoStreamMetrics(stream_handle_, &metrics);
    LOG_AND_THROW_IF(STATUS_FAILED(status), "Failed to get stream metrics with: " << status);
}

KinesisVideoStream::~KinesisVideoStream() {
    free();
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
