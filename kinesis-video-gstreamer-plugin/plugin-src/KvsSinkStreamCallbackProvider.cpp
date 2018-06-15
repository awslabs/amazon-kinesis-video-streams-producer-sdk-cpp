#include "KvsSinkStreamCallbackProvider.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

STATUS
KvsSinkStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                                  STREAM_HANDLE stream_handle,
                                                                  UINT64 last_buffering_ack) {
    auto customDataObj = reinterpret_cast<CustomData*>(custom_data);
    auto stateMachine = customDataObj->callback_state_machine_map.get(stream_handle);
    if (NULL != stateMachine) {
        stateMachine->connection_stale_state_machine->handleConnectionStale();
    } else {
        LOG_ERROR("No state machine found for given stream handle: " << stream_handle);
    }

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                           UINT64 errored_timecode, STATUS status_code) {
    LOG_ERROR("Reporting stream error. Errored timecode: " << errored_timecode << " Status: 0x" << std::hex << status_code);
    auto customDataObj = reinterpret_cast<CustomData*>(custom_data);
    if (status_code == static_cast<UINT32>(STATUS_DESCRIBE_STREAM_CALL_FAILED)) {
        std::lock_guard<std::mutex> lk(customDataObj->closing_stream_handles_queue_mtx);
        customDataObj->closing_stream_handles_queue.push(stream_handle);
    }

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                            UINT64 dropped_frame_timecode) {
    LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
    return STATUS_SUCCESS; // continue streaming
}

STATUS
KvsSinkStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                               UINT64 current_buffer_duration) {
    auto customDataObj = reinterpret_cast<CustomData*>(custom_data);
    auto stateMachine = customDataObj->callback_state_machine_map.get(stream_handle);
    if (NULL != stateMachine) {
        stateMachine->stream_latency_state_machine->handleStreamLatency();
    } else {
        LOG_ERROR("No state machine found for given stream handle: " << stream_handle);
    }

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle) {
    return STATUS_SUCCESS;
}


