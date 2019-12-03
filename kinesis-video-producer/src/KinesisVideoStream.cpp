#include "Logger.h"
#include "KinesisVideoStream.h"
#include "KinesisVideoStreamMetrics.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

KinesisVideoStream::KinesisVideoStream(const KinesisVideoProducer& kinesis_video_producer, const std::string stream_name)
        : stream_handle_(INVALID_STREAM_HANDLE_VALUE),
          stream_name_(stream_name),
          kinesis_video_producer_(kinesis_video_producer),
          debug_dump_frame_info_(false) {
    LOG_INFO("Creating Kinesis Video Stream " << stream_name_);
    // the handle is NULL to start. We will set it later once Kinesis Video PIC gives us a stream handle.

    if (getenv(DEBUG_DUMP_FRAME_INFO)) {
        debug_dump_frame_info_ = true;
    }
}

bool KinesisVideoStream::putFrame(KinesisVideoFrame frame) const {
    if (debug_dump_frame_info_) {
        LOG_DEBUG("pts: " << frame.presentationTs << ", dts: " << frame.decodingTs << ", duration: " << frame.duration << ", size: " << frame.size << ", trackId: " << frame.trackId
                          << ", isKey: " << CHECK_FRAME_FLAG_KEY_FRAME(frame.flags));
    }

    assert(0 != stream_handle_);
    STATUS status = putKinesisVideoFrame(stream_handle_, &frame);
    if (STATUS_FAILED(status)) {
        return false;
    }

    // Print metrics on every key-frame
    // TODO: this will create too much spam in case of audio
    if (CHECK_FRAME_FLAG_KEY_FRAME(frame.flags)) {
        // Extract metrics and print out
        auto stream_metrics = getMetrics();
        auto client_metrics = kinesis_video_producer_.getMetrics();
        auto total_transfer_tate = 8 * client_metrics.getTotalTransferRate();
        auto transfer_rate = 8 * stream_metrics.getCurrentTransferRate();

        LOG_DEBUG("Kinesis Video client and stream metrics"
                          << "\n\t>> Overall storage byte size: " << client_metrics.getContentStoreSizeSize()
                          << "\n\t>> Available storage byte size: " << client_metrics.getContentStoreAvailableSize()
                          << "\n\t>> Allocated storage byte size: " << client_metrics.getContentStoreAllocatedSize()
                          << "\n\t>> Total view allocation byte size: " << client_metrics.getTotalContentViewsSize()
                          << "\n\t>> Total streams frame rate (fps): " << client_metrics.getTotalFrameRate()
                          << "\n\t>> Total streams transfer rate (bps): " <<  total_transfer_tate << " (" << total_transfer_tate / 1024 << " Kbps)"
                          << "\n\t>> Current view duration (ms): " << stream_metrics.getCurrentViewDuration().count()
                          << "\n\t>> Overall view duration (ms): " << stream_metrics.getOverallViewDuration().count()
                          << "\n\t>> Current view byte size: " << stream_metrics.getCurrentViewSize()
                          << "\n\t>> Overall view byte size: " << stream_metrics.getOverallViewSize()
                          << "\n\t>> Current frame rate (fps): " << stream_metrics.getCurrentFrameRate()
                          << "\n\t>> Current transfer rate (bps): " << transfer_rate << " (" << transfer_rate / 1024 << " Kbps)");
    }

    return true;
}

bool KinesisVideoStream::start(const std::string& hexEncodedCodecPrivateData, uint64_t trackId) {
    // Hex-decode the string
    const char* pStrCpd = hexEncodedCodecPrivateData.c_str();
    UINT32 size = 0;
    PBYTE pBuffer = nullptr;
    STATUS status;

    if (STATUS_FAILED(status = hexDecode((PCHAR) pStrCpd, 0, NULL, &size))) {
        LOG_ERROR("Failed to get the size of the buffer for hex decoding the codec private data with: " << status);
        return false;
    }

    // Allocate the buffer needed
    pBuffer = reinterpret_cast<PBYTE>(malloc(size));
    if (nullptr == pBuffer) {
        LOG_ERROR("Failed to allocate enough buffer for hex decoding. Size: " << size);
        return false;
    }

    if (STATUS_FAILED(status = hexDecode((PCHAR) pStrCpd, 0, pBuffer, &size))) {
        LOG_ERROR("Failed to hex decode the codec private data with: " << status);
        ::free(pBuffer);
        return false;
    }

    // Start the stream with the binary codec private data buffer
    bool retVal = start(reinterpret_cast<unsigned char*> (pBuffer), size, trackId);

    // Free the allocated buffer before returning
    ::free(pBuffer);

    return retVal;
}

bool KinesisVideoStream::start(const unsigned char* codecPrivateData, size_t codecPrivateDataSize, uint64_t trackId) {
    STATUS status;

    if (STATUS_FAILED(status = kinesisVideoStreamFormatChanged(stream_handle_, (UINT32) codecPrivateDataSize,
                                                               (PBYTE) codecPrivateData, (UINT64) trackId))) {
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

    if (STATUS_FAILED(status = kinesisVideoStreamResetConnection(stream_handle_))) {
        LOG_ERROR("Failed to reset the connection with: " << status);
        return false;
    }

    return true;
}

bool KinesisVideoStream::resetStream() {
    STATUS status = STATUS_SUCCESS;

    if (STATUS_FAILED(status = kinesisVideoStreamResetStream(stream_handle_))) {
        LOG_ERROR("Failed to reset the stream with: " << status);
        return false;
    }

    return true;
}

void KinesisVideoStream::free() {
    LOG_INFO("Freeing Kinesis Video Stream " << stream_name_);

    // Free the underlying stream
    std::call_once(free_kinesis_video_stream_flag_, freeKinesisVideoStream, getStreamHandle());
}

bool KinesisVideoStream::stop() {
    STATUS status;

    if (STATUS_FAILED(status = stopKinesisVideoStream(stream_handle_))) {
        LOG_ERROR("Failed to stop the stream with: " << status);
        return false;
    }

    return true;
}

bool KinesisVideoStream::stopSync() {
    STATUS status;

    if (STATUS_FAILED(status = stopKinesisVideoStreamSync(stream_handle_))) {
        LOG_ERROR("Failed to stop the stream with: " << status);
        return false;
    }

    return true;
}

KinesisVideoStreamMetrics KinesisVideoStream::getMetrics() const {
    STATUS status = ::getKinesisVideoStreamMetrics(stream_handle_, (PStreamMetrics) stream_metrics_.getRawMetrics());
    LOG_AND_THROW_IF(STATUS_FAILED(status), "Failed to get stream metrics with: " << status);

    return stream_metrics_;
}

bool KinesisVideoStream::putFragmentMetadata(const std::string &name, const std::string &value, bool persistent){
    const char* pMetadataName = name.c_str();
    const char* pMetadataValue = value.c_str();
    STATUS status = ::putKinesisVideoFragmentMetadata(stream_handle_, (PCHAR) pMetadataName, (PCHAR) pMetadataValue, persistent);
    if (STATUS_FAILED(status)) {
        LOG_ERROR("Failed to insert fragment metadata with: " << status);
        return false;
    }

    return true;
}

KinesisVideoStream::~KinesisVideoStream() {
    free();
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
