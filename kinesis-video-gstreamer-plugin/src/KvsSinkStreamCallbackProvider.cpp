#include "KvsSinkStreamCallbackProvider.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

STATUS
KvsSinkStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                            STREAM_HANDLE stream_handle,
                                                            UINT64 time_since_last_buffering_ack) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamConnectionStale callback for stream handle " << stream_handle << ". Time since last buffering ack in 100ns: " << time_since_last_buffering_ack);
    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data,
                                                        STREAM_HANDLE stream_handle,
                                                        UPLOAD_HANDLE upload_handle,
                                                        UINT64 errored_timecode,
                                                        STATUS status_code) {
    LOG_ERROR("Reported stream error. Errored timecode: " << errored_timecode << " Status: 0x" << std::hex << status_code);
    auto customDataObj = reinterpret_cast<KvsSinkCustomData*>(custom_data);

    // ignore if the sdk can recover from the error
    if (!IS_RECOVERABLE_ERROR(status_code)) {
        customDataObj->stream_status = status_code;
    }

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data,
                                                         STREAM_HANDLE stream_handle,
                                                         UINT64 dropped_frame_timecode) {
    UNUSED_PARAM(custom_data);
    LOG_WARN("Reported droppedFrame callback for stream handle " << stream_handle << ". Dropped frame timecode in 100ns: " << dropped_frame_timecode);
    return STATUS_SUCCESS; // continue streaming
}

STATUS
KvsSinkStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data,
                                                            STREAM_HANDLE stream_handle,
                                                            UINT64 current_buffer_duration) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamLatencyPressure callback for stream handle " << stream_handle << ". Current buffer duration in 100ns: " << current_buffer_duration);
    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamClosedHandler(UINT64 custom_data,
                                                   STREAM_HANDLE stream_handle,
                                                   UPLOAD_HANDLE upload_handle) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamClosed callback for stream handle " << stream_handle << ". Upload handle " << upload_handle);
    return STATUS_SUCCESS;
}


