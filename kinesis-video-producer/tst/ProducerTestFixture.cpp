#include "ProducerTestFixture.h"

using namespace std;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Global write-once pointer to the tests
 */

STATUS TestClientCallbackProvider::storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes) {
    UNUSED_PARAM(custom_handle);
    LOG_WARN("Reporting storage overflow. Bytes remaining " << remaining_bytes);
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 last_buffering_ack) {
    LOG_WARN("Reporting stream stale. Last ACK received " << last_buffering_ack);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 errored_timecode, STATUS status) {
    LOG_ERROR("Reporting stream error. Errored timecode " << errored_timecode << " with status code " << status);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 dropped_frame_timecode) {
    LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 duration) {
    LOG_WARN("Reporting stream latency pressure. Current buffer duration " << duration);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::fragmentAckReceivedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, PFragmentAck fragment_ack) {
    LOG_TRACE("Reporting fragment ack");
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamDataAvailableHandler(UINT64 custom_data,
                                                              STREAM_HANDLE stream_handle,
                                                              PCHAR stream_name,
                                                              UPLOAD_HANDLE stream_upload_handle,
                                                              UINT64 duration_available,
                                                              UINT64 size_available) {
    LOG_TRACE("Reporting stream data available");
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 stream_upload_handle) {
    LOG_INFO("Reporting stream stopped.");

    TestStreamCallbackProvider* streamCallbackProvider = reinterpret_cast<TestStreamCallbackProvider*> (custom_data);
    EXPECT_TRUE(streamCallbackProvider != nullptr);
    if (streamCallbackProvider == nullptr) {
        return STATUS_INVALID_OPERATION;
    }

    ProducerTestBase* testBase = streamCallbackProvider->getTestBase();
    EXPECT_TRUE(testBase != nullptr);
    if (testBase == nullptr) {
        return STATUS_INVALID_OPERATION;
    }

    // This should really be done per-stream - this is a wrong way of doing it!!!
    testBase->stop_called_ = true;

    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::validateCallback(UINT64 custom_data) {
    TestStreamCallbackProvider* streamCallbackProvider = reinterpret_cast<TestStreamCallbackProvider*> (custom_data);
    EXPECT_TRUE(streamCallbackProvider != nullptr);
    if (streamCallbackProvider == nullptr) {
        return STATUS_INVALID_OPERATION;
    }

    ProducerTestBase* testBase = streamCallbackProvider->getTestBase();
    EXPECT_TRUE(testBase != nullptr);
    if (testBase == nullptr) {
        return STATUS_INVALID_OPERATION;
    }

    EXPECT_EQ(TEST_MAGIC_NUMBER, streamCallbackProvider->getTestMagicNumber());

    return STATUS_SUCCESS;
}


}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
