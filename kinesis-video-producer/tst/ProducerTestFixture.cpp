#include "ProducerTestFixture.h"

using namespace std;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Global write-once pointer to the tests
 */
class ProducerTestBase;
ProducerTestBase* gProducerApiTest;

STATUS TestClientCallbackProvider::storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes) {
    UNUSED_PARAM(custom_handle);
    LOG_WARN("Reporting storage overflow. Bytes remaining " << remaining_bytes);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 last_buffering_ack) {
    LOG_WARN("Reporting stream stale. Last ACK received " << last_buffering_ack);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 errored_timecode, STATUS status) {
    LOG_ERROR("Reporting stream error. Errored timecode " << errored_timecode << " with status code " << status);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 dropped_frame_timecode) {
    LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 duration) {
    LOG_WARN("Reporting stream latency pressure. Current buffer duration " << duration);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 stream_upload_handle) {
    LOG_INFO("Reporting stream stopped.");
    if (nullptr != gProducerApiTest) {
        // This should really be done per-stream - this is a wrong way of doing it!!!
        gProducerApiTest->stop_called_ = true;
    }
    return STATUS_SUCCESS;
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
