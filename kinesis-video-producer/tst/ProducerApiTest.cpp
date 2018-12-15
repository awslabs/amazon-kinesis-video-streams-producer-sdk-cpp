#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
class ProducerApiTest : public ProducerTestBase {
};

ProducerTestBase* gProducerApiTest;

PVOID staticProducerRoutine(PVOID arg)
{
    auto kinesis_video_stream = reinterpret_cast<KinesisVideoStream*> (arg);
    return gProducerApiTest->basicProducerRoutine(kinesis_video_stream);
}

PVOID staticProducerRoutineOffline(PVOID arg)
{
    auto kinesis_video_stream = reinterpret_cast<KinesisVideoStream*> (arg);
    return gProducerApiTest->basicProducerRoutine(kinesis_video_stream, STREAMING_TYPE_OFFLINE);
}

PVOID ProducerTestBase::basicProducerRoutine(KinesisVideoStream* kinesis_video_stream,
                                             STREAMING_TYPE streaming_type) {
    UINT32 index = 0, persistentMetadataIndex = 0;
    UINT64 timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / DEFAULT_TIME_UNIT_IN_NANOS;
    Frame frame;
    std::string metadataNameStr;

    // Set an indicator that the producer is not stopped
    producer_stopped_ = false;

    // Loop until cancelled
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = frameBuffer_;
    frame.trackId = DEFAULT_TRACK_ID;
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    while (!stop_producer_) {
        // Produce frames
        if (IS_OFFLINE_STREAMING_MODE(streaming_type)) {
            timestamp += frame.duration;
        } else {
            timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        }

        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Add small variation to the frame size (if larger frames
        frame.size = SIZEOF(frameBuffer_);
        if (frame.size > 100) {
            frame.size -= RAND() % 100;
        }

        // Key frame every 50th
        frame.flags = (frame.index % 50 == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

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

        // Apply some non-persistent metadata every few frames
        if (frame.index % 20 == 0) {
            std::ostringstream metadataName, metadataValue;
            metadataName << "MetadataNameForFrame_" << frame.index;
            metadataValue << "MetadataValueForFrame_" << frame.index;
            EXPECT_TRUE(kinesis_video_stream->putFragmentMetadata(metadataName.str(), metadataValue.str(), false));
        }

        // Apply some persistent metadata on a larger intervals to span fragments
        if (frame.index % 60 == 0) {
            std::ostringstream metadataName, metadataValue;
            std::string metadataValueStr;

            metadataName << "PersistentMetadataName_" << persistentMetadataIndex;
            metadataValue << "PersistentMetadataValue_" << persistentMetadataIndex;

            // Set or clear persistent metadata every other time.
            if (persistentMetadataIndex % 2 == 0) {
                metadataNameStr = metadataName.str();
                metadataValueStr = metadataValue.str();
            } else {
                metadataValueStr = std::string();
            }

            persistentMetadataIndex++;
            EXPECT_TRUE(kinesis_video_stream->putFragmentMetadata(metadataNameStr, metadataValueStr, true));
        }

#if 0
        // Simulate EoFr first
        if (frame.index % 50 == 0 && frame.index != 0) {
            Frame eofr = EOFR_FRAME_INITIALIZER;
            LOG_DEBUG("Putting End-of-Fragment frame");
            EXPECT_TRUE(kinesis_video_stream->putFrame(eofr));
            LOG_DEBUG("Finished End-of-Fragment frame");
        }
#endif

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));

        // Sleep a while for non-offline modes
        if (streaming_type != STREAMING_TYPE_OFFLINE) {
            THREAD_SLEEP(TEST_FRAME_DURATION);
        }
    }

    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";


    // The signalling should be handled per stream.
    // This is for a demo purpose only!!!
    EXPECT_TRUE(gProducerApiTest->stop_called_) << "Status of stopped state " << gProducerApiTest->stop_called_;

    // Indicate that the producer routine had stopped
    producer_stopped_ = true;

    return NULL;
}

TEST_F(ProducerApiTest, create_free_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    CreateProducer();

    // Create streams
    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        streams_[i] = CreateTestStream(i);
    }

    // Free the streams
    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Free the stream
        kinesis_video_producer_->freeStream(streams_[i]);

        // Re-create again
        streams_[i] = CreateTestStream(i);
    }

    // Free all the streams
    kinesis_video_producer_->freeStreams();

    // Re-create the streams
    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        streams_[i] = CreateTestStream(i);
    }

    // The destructor should clear the streams again.
}

TEST_F(ProducerApiTest, DISABLED_create_produce_offline_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    CreateProducer();

    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        streams_[i] = CreateTestStream(i, STREAMING_TYPE_OFFLINE);

        // Spin off the producer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&producer_thread_, staticProducerRoutineOffline, reinterpret_cast<PVOID> (streams_[i].get())));
    }

    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION);

    // Indicate the cancel for the threads
    stop_producer_ = true;

    // Join the thread and wait to exit.
    // NOTE: This is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    // We will wait for 10 seconds for the thread to terminate
    int32_t index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index < 100 && !producer_stopped_);

    EXPECT_TRUE(producer_stopped_) << "Producer thread failed to stop cleanly";

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primitives to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();
}

TEST_F(ProducerApiTest, create_produce_start_stop_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    key_frame_interval_ = 50;
    setFps(25);

    CreateProducer();

    UINT64 timestamp;
    Frame frame;
    frame.duration = frame_duration_;
    frame.frameData = frameBuffer_;
    frame.size = SIZEOF(frameBuffer_);
    frame.trackId = DEFAULT_TRACK_ID;

    // Set the value of the frame buffer
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    for (uint32_t i = 0; i < 10; i++) {
        // Create stream
        streams_[0] = CreateTestStream(0);
        shared_ptr<KinesisVideoStream> kinesis_video_stream = streams_[0];

        // Start streaming
        for (uint32_t index = 0; index < TEST_START_STOP_ITERATION_COUT; index++) {
            // Produce frames
            timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() / DEFAULT_TIME_UNIT_IN_NANOS;
            frame.index = index++;
            frame.decodingTs = timestamp;
            frame.presentationTs = timestamp;

            // Key frame every 50th
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

            THREAD_SLEEP(frame_duration_);
        }

        LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
        EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";
        EXPECT_TRUE(gProducerApiTest->stop_called_) << "Status of stopped state " << gProducerApiTest->stop_called_;

        kinesis_video_producer_->freeStream(move(streams_[0]));
        streams_[0] = nullptr;
    }
}

TEST_F(ProducerApiTest, create_produce_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

    CreateProducer();

    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        streams_[i] = CreateTestStream(i);

        // Spin off the producer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&producer_thread_, staticProducerRoutine, reinterpret_cast<PVOID> (streams_[i].get())));
    }

#if 0
    // This section will simply demonstrate a stream termination, waiting for the termination and then cleanup and
    // a consequent re-creation and restart. Normally, after stopping, the customers application will have to
    // await until the stream stopped event is called.
    for (uint32_t iter = 0; iter < 10; iter++) {
        THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        LOG_DEBUG("Stopping the streams");
        stop_producer_ = true;
        LOG_DEBUG("Waiting for the streams to finish and close...");
        THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

        LOG_DEBUG("Freeing the streams.");

        // Free the streams
        for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
            kinesis_video_producer_->freeStream(move(streams_[i]));
        }

        LOG_DEBUG("Starting the streams again");
        stop_producer_ = false;

        // Create new streams
        for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
            // Create the stream
            streams_[i] = CreateTestStream(i);

            DLOGI("Stream has been created");

            // Spin off the producer
            EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&producer_thread_, staticProducerRoutine,
                                                    reinterpret_cast<PVOID> (streams_[i].get())));
        }
    }
#endif

    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION);

    // Indicate the cancel for the threads
    stop_producer_ = true;

    // Join the thread and wait to exit.
    // NOTE: This is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    // We will wait for 10 seconds for the thread to terminate
    int32_t index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index < 100 && !producer_stopped_);

    EXPECT_TRUE(producer_stopped_) << "Producer thread failed to stop cleanly";

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primitives to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();
}

TEST_F(ProducerApiTest, exceed_max_track_count)
{
    char stream_name[MAX_STREAM_NAME_LEN];
    sprintf(stream_name, "ScaryTestStream");
    const string testTrackName = "testTrackName", testCodecId = "testCodecId";

    // add 4 tracks
    auto stream_definition = make_unique<StreamDefinition>(stream_name,
                                                           hours(2),
                                                           nullptr,
                                                           "",
                                                           STREAMING_TYPE_REALTIME,
                                                           "video/h264",
                                                           milliseconds(TEST_MAX_STREAM_LATENCY_IN_MILLIS),
                                                           seconds(2),
                                                           milliseconds(1),
                                                           true,
                                                           true,
                                                           true);
    stream_definition->addTrack(1, testTrackName, testCodecId, MKV_TRACK_INFO_TYPE_VIDEO);
    stream_definition->addTrack(2, testTrackName, testCodecId, MKV_TRACK_INFO_TYPE_AUDIO);
    stream_definition->addTrack(3, testTrackName, testCodecId, MKV_TRACK_INFO_TYPE_VIDEO);
    EXPECT_EQ(nullptr, kinesis_video_producer_->createStream(move(stream_definition)));
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com