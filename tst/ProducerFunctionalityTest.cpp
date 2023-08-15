#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

#define TEST_CURL_FAULT_INJECT_COUNT                      2
#define WAIT_5_SECONDS_FOR_ACKS                           5 * HUNDREDS_OF_NANOS_IN_A_SECOND

class ProducerFunctionalityTest : public ProducerTestBase {
};

TEST_F(ProducerFunctionalityTest, start_stopsync_terminate) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
        return;
    }

    CreateProducer();

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME);
    auto kinesis_video_stream = streams_[0];
    EXPECT_TRUE(kinesis_video_stream->stopSync());

    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}

TEST_F(ProducerFunctionalityTest, offline_upload_limited_buffer_duration) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
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
        LOG_WARN("Creds not set");
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
        LOG_WARN("Creds not set");
        return;
    }

    const uint32_t clip_duration_seconds = 15;
    const uint32_t clip_count = 20;
    const uint32_t frames_per_clip = clip_duration_seconds * (uint32_t) fps_;
    const uint32_t total_frames = frames_per_clip * clip_count;
    const uint32_t pause_between_clip_seconds[] = {2, 15};

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

            // pause at the last frame of each clip
            if (i > 0 && (i + 1) % frames_per_clip == 0) {
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
        LOG_WARN("Creds not set");
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
        LOG_WARN("Creds not set");
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

/**
 * Set short max latency in the stream info.
 * Send a few fragments then pause as in an intermittent
 * scenario. Put an EoFR before pausing to ensure we terminate the fragment on the backend
 * which will issue a persistent ACK, causing the state machine to not rollback
 * on the next frame produced after the pause. The pause will cause the connection timeout
 * which would then set the state machine to change state to issue a new session. The new
 * session will not rollback due to the previous session being closed and persisted ACK
 * received. If, however, this is not the case, the rollback would cause a latency pressure.
 */
TEST_F(ProducerFunctionalityTest, realtime_intermittent_no_latency_pressure_eofr) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
        return;
    }

    CreateProducer();

    buffering_ack_in_sequence_ = true;
    key_frame_interval_ = 60;
    total_frame_count_ = 6 * key_frame_interval_;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = 16 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;
    Frame eofr = EOFR_FRAME_INITIALIZER;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME,
                                   15000,
                                   120);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Pause on the 5th
        if (i == 5 * key_frame_interval_) {
            EXPECT_TRUE(kinesis_video_stream->putFrame(eofr));

            // Make sure we hit the connection idle timeout
            THREAD_SLEEP(60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }

        timestamp = GETTIME();
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

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

        THREAD_SLEEP(30 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_EQ(0, latency_pressure_count_) << "Should not have latency pressure events";
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}

/**
 * Set short max latency in the stream info.
 * Set an automatic intermittent producer case handling (intra-frame timeout/closing fragment).
 * Send a few fragments then pause as in an intermittent
 * scenario. No EoFR produced before pausing.
 * Automatic fragment closing fired on timeout to ensure we terminate the fragment on the backend
 * which will issue a persistent ACK, causing the state machine to not rollback
 * on the next frame produced after the pause. The pause will cause the connection timeout
 * which would then set the state machine to change state to issue a new session. The new
 * session will not rollback due to the previous session being closed and persisted ACK
 * received. If, however, this is not the case, the rollback would cause a latency pressure.
 */
 /**
  * TODO: This is disabled until we implement the auto fragment closing feature
  */
TEST_F(ProducerFunctionalityTest, DISABLED_realtime_intermittent_no_latency_pressure_auto) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
        return;
    }

    CreateProducer();

    buffering_ack_in_sequence_ = true;
    key_frame_interval_ = 60;
    total_frame_count_ = 6 * key_frame_interval_;

    UINT64 timestamp = 0;
    Frame frame;
    frame.duration = 16 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME,
                                   15000,
                                   120);
    auto kinesis_video_stream = streams_[0];

    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Pause on the 5th
        if (i == 5 * key_frame_interval_) {
            // Make sure we hit the connection idle timeout
            THREAD_SLEEP(60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }

        timestamp = GETTIME();
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

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

        THREAD_SLEEP(30 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_EQ(0, latency_pressure_count_) << "Should not have latency pressure events";
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_); // all fragments should be sent
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}

/**
 * This test assumes client info AUTOMATIC_STREAMING_FLAGS set to AUTOMATIC_STREAMING_ALWAYS_CONTINUOUS
 * Set short max latency in the stream info.
 * Send a few fragments then pause as in an intermittent
 * scenario. No EoFR produced before pausing.
 * The backend doesn't terminate the fragment so no ACKs will be issued.
 * The pause will cause the connection timeout
 * which would then set the state machine to change state to issue a new session. The new
 * session will not rollback due to the previous session being closed and persisted ACK
 * received.
 * As we haven't received a persistent ACK, the rollback will roll it back to the previous
 * fragment and re-stream. The previous fragment timestamps are in the past causing latency
 * pressure callback to fire.
 */
TEST_F(ProducerFunctionalityTest, realtime_intermittent_latency_pressure) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
        return;
    }

    KinesisVideoLogger::getInstance().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);

    CreateProducer(false, AUTOMATIC_STREAMING_ALWAYS_CONTINUOUS);

    buffering_ack_in_sequence_ = true;
    key_frame_interval_ = 60;
    total_frame_count_ = 7 * key_frame_interval_;
    frame_duration_ = 16 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;

    UINT64 startTime = 0;
    UINT64 timestamp = 0;
    UINT64 delta = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME,
                                   15000,
                                   120);
    auto kinesis_video_stream = streams_[0];

    startTime = GETTIME();
    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Pause on the 5th
        if (i == 5 * key_frame_interval_) {
            // Make sure we hit the connection idle timeout
            UINT64 start = GETTIME();
            THREAD_SLEEP(60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            delta = GETTIME() - start;
        } else if ( i < 5 * key_frame_interval_ ) {
            // This should not flip to false until after the 60s sleep when we start putting frames
            EXPECT_TRUE(buffering_ack_in_sequence_);
        }

        timestamp = startTime + i*frame_duration_ + delta;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

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

        THREAD_SLEEP(frame_duration_);
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_NE(0, latency_pressure_count_) << "Should fire latency pressure events";
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_FALSE(buffering_ack_in_sequence_);
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}

/**
 * This test assumes client info AUTOMATIC_STREAMING_FLAGS set to AUTOMATIC_STREAMING_INTERMITTENT_PRODUCER (default)
 * Set short max latency in the stream info.
 * Send a few fragments then pause as in an intermittent
 * scenario. EoFR should be automatically produced after pause
 * Backend should NOT timeout so we should get an ACK
 */
TEST_F(ProducerFunctionalityTest, realtime_auto_intermittent_latency_pressure) {
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        LOG_WARN("Creds not set");
        return;
    }

    KinesisVideoLogger::getInstance().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);

    CreateProducer();

    buffering_ack_in_sequence_ = true;
    key_frame_interval_ = 60;
    total_frame_count_ = 6 * key_frame_interval_;
    frame_duration_ = 16 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;

    UINT64 startTime = 0;
    UINT64 timestamp = 0;
    UINT64 delta = 0;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    streams_[0] = CreateTestStream(0, STREAMING_TYPE_REALTIME,
                                   15000,
                                   120);
    auto kinesis_video_stream = streams_[0];

    startTime = GETTIME();
    for(uint32_t i = 0; i < total_frame_count_; i++) {
        frame.index = i;
        frame.flags = (frame.index % key_frame_interval_ == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Pause on the 5th
        if (i == 5 * key_frame_interval_) {
            // Make sure we hit the connection idle timeout
            UINT64 start = GETTIME();
            THREAD_SLEEP(60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            delta = GETTIME() - start;
        }

        timestamp = startTime + i*frame_duration_ + delta;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

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

        THREAD_SLEEP(frame_duration_);
    }

    THREAD_SLEEP(WAIT_5_SECONDS_FOR_ACKS);
    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
    EXPECT_FALSE(frame_dropped_) << "Status of frame drop " << frame_dropped_;
    EXPECT_EQ(0, latency_pressure_count_) << "Should fire latency pressure events";
    EXPECT_TRUE(STATUS_SUCCEEDED(getErrorStatus())) << "Status of stream error " << getErrorStatus();
    EXPECT_TRUE(buffering_ack_in_sequence_);
    kinesis_video_producer_->freeStreams();
    streams_[0] = nullptr;
}



}
}
}
}

