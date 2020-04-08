#include "ProducerTestFixture.h"

using namespace std;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

STATUS getProducerTestBase(UINT64 custom_handle, ProducerTestBase** ppTestBase) {
    TestClientCallbackProvider* clientCallbackProvider = reinterpret_cast<TestClientCallbackProvider*> (custom_handle);
    EXPECT_TRUE(clientCallbackProvider != nullptr);
    if (clientCallbackProvider == nullptr) {
        return STATUS_INVALID_OPERATION;
    }
    ProducerTestBase* testBase = clientCallbackProvider->getTestBase();
    EXPECT_TRUE(testBase != nullptr);
    if (testBase == nullptr) {
        return STATUS_INVALID_OPERATION;
    }
    *ppTestBase = testBase;
    return STATUS_SUCCESS;
}


/**
 * Global write-once pointer to the tests
 */
STATUS TestClientCallbackProvider::storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes) {
    UNUSED_PARAM(custom_handle);
    LOG_WARN("Reporting storage overflow. Bytes remaining " << remaining_bytes);
    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_handle, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
    }
    testBase->storage_overflow_ = true;
    return STATUS_SUCCESS;
}

STATUS TestStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 last_buffering_ack) {
    UNUSED_PARAM(stream_handle);
    LOG_WARN("Reporting stream stale. Last ACK received " << last_buffering_ack);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 errored_timecode, STATUS status) {
    UNUSED_PARAM(stream_handle);
    LOG_ERROR("Reporting stream error. Errored timecode " << errored_timecode << " with status code " << status);
    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_data, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
    }
    testBase->setErrorStatus(status);

    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 dropped_frame_timecode) {
    UNUSED_PARAM(stream_handle);
    LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_data, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
    }

    testBase->frame_dropped_ = true;
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 duration) {
    UNUSED_PARAM(stream_handle);
    LOG_WARN("Reporting stream latency pressure. Current buffer duration " << duration);
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::bufferDurationOverflowPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 remaining_duration) {
    LOG_WARN("Reporting buffer duration overflow pressure. remaining duration " << remaining_duration);
    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_data, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
    }
    testBase->buffer_duration_pressure_ = true;
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::fragmentAckReceivedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, PFragmentAck fragment_ack) {
    UNUSED_PARAM(stream_handle);
    UNUSED_PARAM(fragment_ack);
    LOG_TRACE("Reporting fragment ack");
    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_data, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
    }

    if (fragment_ack->ackType == FRAGMENT_ACK_TYPE_BUFFERING) {
        if (testBase->previous_buffering_ack_timestamp_.find(upload_handle) != testBase->previous_buffering_ack_timestamp_.end() &&
            fragment_ack->timestamp != testBase->previous_buffering_ack_timestamp_[upload_handle] && // timestamp can be same when retransmit happens
            fragment_ack->timestamp - testBase->previous_buffering_ack_timestamp_[upload_handle] > testBase->getFragmentDurationMs()) {
            LOG_ERROR("Buffering ack not in sequence. Previous ack ts: " << testBase->previous_buffering_ack_timestamp_[upload_handle] << " Current ack ts: " << fragment_ack->timestamp);
            testBase->buffering_ack_in_sequence_ = false;
        }

        testBase->previous_buffering_ack_timestamp_[upload_handle] = fragment_ack->timestamp;
    }

    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamDataAvailableHandler(UINT64 custom_data,
                                                              STREAM_HANDLE stream_handle,
                                                              PCHAR stream_name,
                                                              UPLOAD_HANDLE stream_upload_handle,
                                                              UINT64 duration_available,
                                                              UINT64 size_available) {
    UNUSED_PARAM(stream_handle);
    UNUSED_PARAM(stream_name);
    UNUSED_PARAM(stream_upload_handle);
    UNUSED_PARAM(duration_available);
    UNUSED_PARAM(size_available);
    LOG_TRACE("Reporting stream data available");
    return validateCallback(custom_data);
}

STATUS TestStreamCallbackProvider::streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 stream_upload_handle) {
    UNUSED_PARAM(stream_handle);
    UNUSED_PARAM(stream_upload_handle);
    LOG_INFO("Reporting stream stopped.");

    ProducerTestBase* testBase;
    STATUS ret = getProducerTestBase(custom_data, &testBase);
    if (STATUS_FAILED(ret)) {
        return ret;
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
