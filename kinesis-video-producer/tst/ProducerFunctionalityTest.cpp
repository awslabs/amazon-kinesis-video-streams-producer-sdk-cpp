#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

#define TEST_CURL_FAULT_INJECT_COUNT                      2
#define WAIT_5_SECONDS_FOR_ACKS                           5 * HUNDREDS_OF_NANOS_IN_A_SECOND

class ProducerFunctionalityTest : public ProducerTestBase {
};

TEST_F(ProducerFunctionalityTest, terminate_curl_at_token_rotation) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    int timeout_duration_ms[] = {4, 128};
    int timeout_duration_count = sizeof(timeout_duration_ms) / sizeof(int);

    CreateProducer();
    testDefaultCallbackProvider->fault_inject_curl = true;
    testDefaultCallbackProvider->terminate_previous_upload_at_putStream = false;

    for(int j = 0; j < timeout_duration_count; j++) {
        LOG_DEBUG("timing out in " << timeout_duration_ms[j] << "ms");

        testDefaultCallbackProvider->total_curl_termination = TEST_CURL_FAULT_INJECT_COUNT;
        testDefaultCallbackProvider->terminate_curl_timeout = timeout_duration_ms[j];

        buffering_ack_in_sequence_ = true;
        previous_buffering_ack_timestamp_.clear();

        UINT64 timestamp = 0;
        Frame frame;
        frame.duration = frame_duration_;
        frame.frameData = frameBuffer_;
        frame.size = SIZEOF(frameBuffer_);
        frame.trackId = DEFAULT_TRACK_ID;

        // Set the value of the frame buffer
        MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

        streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME);
        auto kinesis_video_stream = streams_[0];

        for(uint32_t i = 0; i < total_frame_count_; i++) {
            frame.index = i;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;

            frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

            std::stringstream strstrm;
            strstrm << " TID: 0x" << std::hex << GETTID();
            LOG_DEBUG("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                                   << strstrm.str()
                                                   << " Id: " << frame.index
                                                   << ", Key Frame: "
                                                   << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                       ? "true" : "false")
                                                   << ", Size: " << frame.size
                                                   << ", Dts: " << frame.decodingTs
                                                   << ", Pts: " << frame.presentationTs);

            EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
            timestamp += frame_duration_;
            THREAD_SLEEP(frame_duration_);
        }

        THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
        EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
        EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
        kinesis_video_producer_->freeStreams();
        streams_[0] = nullptr;
        testDefaultCallbackProvider->at_token_rotation = false;

        // Run the test once more. This time fault inject previous upload handle at putMedia callback
        if (j == timeout_duration_count && !testDefaultCallbackProvider->terminate_previous_upload_at_putStream) {
            j = 0;
            testDefaultCallbackProvider->terminate_previous_upload_at_putStream = true;
        }
    }
}

TEST_F(ProducerFunctionalityTest, intermittent_pausing_putFrame_and_terminate_curl_at_token_rotation) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    int timeout_duration_ms[] = {4, 128};
    int timeout_duration_count = sizeof(timeout_duration_ms) / sizeof(int);

    CreateProducer();
    testDefaultCallbackProvider->fault_inject_curl = true;
    testDefaultCallbackProvider->terminate_previous_upload_at_putStream = false;

    for(int j = 0; j < timeout_duration_count; j++) {
        LOG_DEBUG("timing out in " << timeout_duration_ms[j] << "ms");

        testDefaultCallbackProvider->total_curl_termination = TEST_CURL_FAULT_INJECT_COUNT;
        testDefaultCallbackProvider->terminate_curl_timeout = timeout_duration_ms[j];

        buffering_ack_in_sequence_ = true;
        previous_buffering_ack_timestamp_.clear();

        UINT64 timestamp = 0;
        UINT64 next_pause_time = timestamp + 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
        Frame frame;
        frame.duration = frame_duration_;
        frame.frameData = frameBuffer_;
        frame.size = SIZEOF(frameBuffer_);
        frame.trackId = DEFAULT_TRACK_ID;

        // Set the value of the frame buffer
        MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

        streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME);
        auto kinesis_video_stream = streams_[0];


        for(uint32_t i = 0; i < total_frame_count_; i++) {
            frame.index = i;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;

            frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

            std::stringstream strstrm;
            strstrm << " TID: 0x" << std::hex << GETTID();
            LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                                   << strstrm.str()
                                                   << " Id: " << frame.index
                                                   << ", Key Frame: "
                                                   << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                       ? "true" : "false")
                                                   << ", Size: " << frame.size
                                                   << ", Dts: " << frame.decodingTs
                                                   << ", Pts: " << frame.presentationTs);
            if (timestamp >= next_pause_time) {
                LOG_DEBUG("pausing putFrame for 5 seconds");
                THREAD_SLEEP(5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
                next_pause_time += 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
            }

            EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
            timestamp += frame_duration_;

            THREAD_SLEEP(frame_duration_);
        }

        THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
        EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
        EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
        kinesis_video_producer_->freeStreams();
        streams_[0] = nullptr;
        testDefaultCallbackProvider->at_token_rotation = false;

        // Run the test once more. This time fault inject previous upload handle at putMedia callback
        if (j == timeout_duration_count && !testDefaultCallbackProvider->terminate_previous_upload_at_putStream) {
            j = 0;
            testDefaultCallbackProvider->terminate_previous_upload_at_putStream = true;
        }
    }
}


TEST_F(ProducerFunctionalityTest, pressure_on_buffer_duration_terminate_curl_at_token_rotation) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    int timeout_duration_ms[] = {4, 128};
    int timeout_duration_count = sizeof(timeout_duration_ms) / sizeof(int);
    const int TEST_BUFFER_DURATION_SECONDS = 3;

    CreateProducer();
    testDefaultCallbackProvider->fault_inject_curl = true;
    testDefaultCallbackProvider->terminate_previous_upload_at_putStream = false;

    for(int j = 0; j < timeout_duration_count; j++) {
        LOG_DEBUG("timing out in " << timeout_duration_ms[j] << "ms");

        testDefaultCallbackProvider->total_curl_termination = TEST_CURL_FAULT_INJECT_COUNT;
        testDefaultCallbackProvider->terminate_curl_timeout = timeout_duration_ms[j];

        buffering_ack_in_sequence_ = true;
        previous_buffering_ack_timestamp_.clear();

        UINT64 timestamp = 0;
        Frame frame;
        frame.duration = frame_duration_;
        frame.frameData = frameBuffer_;
        frame.size = SIZEOF(frameBuffer_);
        frame.trackId = DEFAULT_TRACK_ID;

        // Set the value of the frame buffer
        MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

        streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME, TEST_MAX_STREAM_LATENCY_IN_MILLIS, TEST_BUFFER_DURATION_SECONDS);
        auto kinesis_video_stream = streams_[0];

        for(uint32_t i = 0; i < total_frame_count_; i++) {
            frame.index = i;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;
            frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

            std::stringstream strstrm;
            strstrm << " TID: 0x" << std::hex << GETTID();
            LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                                  << strstrm.str()
                                                  << " Id: " << frame.index
                                                  << ", Key Frame: "
                                                  << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                      ? "true" : "false")
                                                  << ", Size: " << frame.size
                                                  << ", Dts: " << frame.decodingTs
                                                  << ", Pts: " << frame.presentationTs);

            EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
            timestamp += frame_duration_;

            handlePressure(buffer_duration_pressure_, 0);

            if (current_pressure_state_ == PRESSURING) {
                LOG_DEBUG("slowing down");
                THREAD_SLEEP(frame_duration_);
            }
        }


        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
        EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
        EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
        EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
        kinesis_video_producer_->freeStreams();
        streams_[0] = nullptr;
        testDefaultCallbackProvider->at_token_rotation = false;

        // Run the test once more. This time fault inject previous upload handle at putMedia callback
        if (j == timeout_duration_count && !testDefaultCallbackProvider->terminate_previous_upload_at_putStream) {
            j = 0;
            testDefaultCallbackProvider->terminate_previous_upload_at_putStream = true;
        }
    }
}

TEST_F(ProducerFunctionalityTest, pressure_on_storage_terminate_curl_at_token_rotation) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    int timeout_duration_ms[] = {4, 128};
    int timeout_duration_count = sizeof(timeout_duration_ms) / sizeof(int);
    const int TEST_PRESSURE_GRACE_PERIOD_SECOND = 2;

    device_storage_size_ = 1 * 1024 * 1024; // 1MB of storage

    CreateProducer();
    testDefaultCallbackProvider->fault_inject_curl = true;
    testDefaultCallbackProvider->terminate_previous_upload_at_putStream = false;

    for(int j = 0; j < timeout_duration_count; j++) {
        LOG_DEBUG("timing out in " << timeout_duration_ms[j] << "ms");

        testDefaultCallbackProvider->total_curl_termination = TEST_CURL_FAULT_INJECT_COUNT;
        testDefaultCallbackProvider->terminate_curl_timeout = timeout_duration_ms[j];

        buffering_ack_in_sequence_ = true;
        previous_buffering_ack_timestamp_.clear();

        UINT64 timestamp = 0;
        Frame frame;
        frame.duration = frame_duration_;
        frame.frameData = frameBuffer_;
        frame.size = SIZEOF(frameBuffer_);
        frame.trackId = DEFAULT_TRACK_ID;

        // Set the value of the frame buffer
        MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

        streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME);
        auto kinesis_video_stream = streams_[0];

        for(uint32_t i = 0; i < total_frame_count_; i++) {
            frame.index = i;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;
            frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

            std::stringstream strstrm;
            strstrm << " TID: 0x" << std::hex << GETTID();
            LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                                  << strstrm.str()
                                                  << " Id: " << frame.index
                                                  << ", Key Frame: "
                                                  << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                      ? "true" : "false")
                                                  << ", Size: " << frame.size
                                                  << ", Dts: " << frame.decodingTs
                                                  << ", Pts: " << frame.presentationTs);

            EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
            timestamp += frame_duration_;

            handlePressure(storage_overflow_, TEST_PRESSURE_GRACE_PERIOD_SECOND);

            if (current_pressure_state_ == PRESSURING) {
                LOG_DEBUG("slowing down");
                THREAD_SLEEP(frame_duration_);
            }
        }

        THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
        EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
        EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
        kinesis_video_producer_->freeStreams();
        streams_[0] = nullptr;
        testDefaultCallbackProvider->at_token_rotation = false;

        // Run the test once more. This time fault inject previous upload handle at putMedia callback
        if (j == timeout_duration_count && !testDefaultCallbackProvider->terminate_previous_upload_at_putStream) {
            j = 0;
            testDefaultCallbackProvider->terminate_previous_upload_at_putStream = true;
        }
    }
}

TEST_F(ProducerFunctionalityTest, one_hundred_mpbs_test) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    device_storage_size_ = 3 * 1024 * 1024 * 1024llu;
    key_frame_interval_ = 5;
    const uint32_t test_frame_size = 650000; // should produce 104 mbps stream
    PBYTE test_frame_buffer = new BYTE[test_frame_size];
    CreateProducer();

    buffering_ack_in_sequence_ = true;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = test_frame_buffer;
    frame.size = test_frame_size;
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, test_frame_size);

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        std::stringstream strstrm;
        strstrm << " TID: 0x" << std::hex << GETTID();
        LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                              << strstrm.str()
                                              << " Id: " << frame.index
                                              << ", Key Frame: "
                                              << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                  ? "true" : "false")
                                              << ", Size: " << frame.size
                                              << ", Dts: " << frame.decodingTs
                                              << ", Pts: " << frame.presentationTs);

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
        timestamp += frame_duration_;

        THREAD_SLEEP(frame_duration_);
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
    delete [] test_frame_buffer;
}

TEST_F(ProducerFunctionalityTest, offline_upload_limited_buffer_duration) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    CreateProducer();

    buffering_ack_in_sequence_ = true;
    const int test_buffer_duration_seconds = 4;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_OFFLINE, TEST_MAX_STREAM_LATENCY_IN_MILLIS, test_buffer_duration_seconds);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        std::stringstream strstrm;
        strstrm << " TID: 0x" << std::hex << GETTID();
        LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                              << strstrm.str()
                                              << " Id: " << frame.index
                                              << ", Key Frame: "
                                              << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                  ? "true" : "false")
                                              << ", Size: " << frame.size
                                              << ", Dts: " << frame.decodingTs
                                              << ", Pts: " << frame.presentationTs);

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
        timestamp += frame_duration_;
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}

TEST_F(ProducerFunctionalityTest, offline_upload_limited_storage) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    device_storage_size_ = 1 * 1024 * 1024;
    const uint32_t test_frame_size = 12500; // each fragment contains 20 frames. Storage can only contain 4 fragments at the same time
    BYTE test_frame_data[test_frame_size];
    CreateProducer();

    buffering_ack_in_sequence_ = true;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = test_frame_data;
    frame.size = SIZEOF(test_frame_data);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(test_frame_data));

    streams_[0] = CreateTestStream(1, STREAMING_TYPE_OFFLINE);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        std::stringstream strstrm;
        strstrm << " TID: 0x" << std::hex << GETTID();
        LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                              << strstrm.str()
                                              << " Id: " << frame.index
                                              << ", Key Frame: "
                                              << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                  ? "true" : "false")
                                              << ", Size: " << frame.size
                                              << ", Dts: " << frame.decodingTs
                                              << ", Pts: " << frame.presentationTs);

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
        timestamp += frame_duration_;
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}


TEST_F(ProducerFunctionalityTest, intermittent_file_upload) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    const int clip_duration_seconds = 15;
    const int clip_count = 5;
    const int frames_per_clip = clip_duration_seconds * fps_;
    const int total_frames = frames_per_clip * clip_count;
    const int pause_between_clip_seconds[] = {2, 15};

    CreateProducer();

    for (auto pause_seconds : pause_between_clip_seconds) {
        buffering_ack_in_sequence_ = true;
        previous_buffering_ack_timestamp_.clear();

        UINT64 timestamp = 0;
        Frame frame;
        frame.duration = frame_duration_;
        frame.frameData = frameBuffer_;
        frame.size = SIZEOF(frameBuffer_);
        frame.trackId = DEFAULT_TRACK_ID;

        // Set the value of the frame buffer
        MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

        streams_[0] = CreateTestStream(0, STREAMING_TYPE_OFFLINE);
        auto kinesis_video_stream = streams_[0];

        for(uint32_t i = 0; i < total_frames; i++) {
            frame.index = i;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;
            frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

            std::stringstream strstrm;
            strstrm << " TID: 0x" << std::hex << GETTID();
            LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                                  << strstrm.str()
                                                  << " Id: " << frame.index
                                                  << ", Key Frame: "
                                                  << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                      ? "true" : "false")
                                                  << ", Size: " << frame.size
                                                  << ", Dts: " << frame.decodingTs
                                                  << ", Pts: " << frame.presentationTs);

            EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
            timestamp += frame_duration_;

            if (i > 0 && i % frames_per_clip == 0) {
                LOG_INFO("Pausing for next clip");
                THREAD_SLEEP(pause_seconds * HUNDREDS_OF_NANOS_IN_A_SECOND);
            }
        }

        THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
        EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
        EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
        kinesis_video_producer_->freeStreams();
        streams_[0] = nullptr;
    }
}

TEST_F(ProducerFunctionalityTest, high_fragment_rate_file_upload) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    key_frame_interval_ = 4;

    CreateProducer();

    buffering_ack_in_sequence_ = true;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_OFFLINE);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        std::stringstream strstrm;
        strstrm << " TID: 0x" << std::hex << GETTID();
        LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                              << strstrm.str()
                                              << " Id: " << frame.index
                                              << ", Key Frame: "
                                              << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                  ? "true" : "false")
                                              << ", Size: " << frame.size
                                              << ", Dts: " << frame.decodingTs
                                              << ", Pts: " << frame.presentationTs);

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
        timestamp += frame_duration_;
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;

}

TEST_F(ProducerFunctionalityTest, offline_mode_token_rotation_block_on_space) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    device_storage_size_ = 10 * 1024 * 1024llu; // 10MB heap
    uint64_t test_total_frame_count =  10 * 1000llu;
    CreateProducer();

    buffering_ack_in_sequence_ = true;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_OFFLINE);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < test_total_frame_count; i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        std::stringstream strstrm;
        strstrm << " TID: 0x" << std::hex << GETTID();
        LOG_INFO("Putting frame for stream: " << kinesis_video_stream->getStreamName()
                                              << strstrm.str()
                                              << " Id: " << frame.index
                                              << ", Key Frame: "
                                              << (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME)
                                                  ? "true" : "false")
                                              << ", Size: " << frame.size
                                              << ", Dts: " << frame.decodingTs
                                              << ", Pts: " << frame.presentationTs);

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));
        timestamp += frame_duration_;
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;

}

}
}
}
}

