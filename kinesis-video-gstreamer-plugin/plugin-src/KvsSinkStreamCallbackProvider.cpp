#include "KvsSinkStreamCallbackProvider.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

STATUS
KvsSinkStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                                  STREAM_HANDLE stream_handle,
                                                                  UINT64 last_buffering_ack) {
    auto customDataObj = reinterpret_cast<CustomData*>(custom_data);
    customDataObj->callback_state_machine->connection_stale_state_machine->handleConnectionStale();

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                        UPLOAD_HANDLE upload_handle, UINT64 errored_timecode, STATUS status_code) {
    LOG_ERROR("Reporting stream error. Errored timecode: " << errored_timecode << " Status: 0x" << std::hex << status_code);
    auto customDataObj = reinterpret_cast<CustomData*>(custom_data);

	// set stream error_code in the custom data object.  This code will be available for sending to the pipeline
	customDataObj->stream_error_code = status_code;

	// If stream is ready and an error occurs, set the stream_status. We will
	// not change stream status when the stream is not in a ready state.
	// The reason for that is that we might want to retry and get the system in
	// the ready state. If it's not possible, the calling function will have
	// to send the error message.
	if ( customDataObj->stream_ready.load() )
	{
		customDataObj->stream_status = status_code;
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
    customDataObj->callback_state_machine->stream_latency_state_machine->handleStreamLatency();

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle) {
    return STATUS_SUCCESS;
}
