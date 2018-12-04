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

PVOID ProducerTestBase::basicProducerRoutine(KinesisVideoStream* kinesis_video_stream) {
    UINT32 index = 0, persistentMetadataIndex = 0;
    UINT64 timestamp = 0;
    Frame frame;
    std::string metadataNameStr;

    // Loop until cancelled
    frame.duration = FRAME_DURATION_IN_MICROS * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
    frame.frameData = frameBuffer_;
    MEMSET(frame.frameData, 0x55, SIZEOF(frameBuffer_));

    while (!stop_producer_) {
        // Produce frames
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                systemCurrentTime().time_since_epoch()).count() / DEFAULT_TIME_UNIT_IN_NANOS;
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

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));

        // Sleep a while
        THREAD_SLEEP(FRAME_DURATION_IN_MICROS);
    }

    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    EXPECT_TRUE(kinesis_video_stream->stopSync()) << "Timed out awaiting for the stream stop notification";


    // The signalling should be handled per stream.
    // This is for a demo purpose only!!!
    // IMPORTANT NOTE: There is a slight race condition between the stopSync completion
    // and the callback function getting triggered. This is normal as we are just trying
    // to ensure that any of the stream callbacks has fired so we need to introduce a
    // short sleep which is longer than the CURL await sleep in order to ensure the
    // callback had a chance to fire for validation.
    THREAD_SLEEP(15000);
    EXPECT_TRUE(gProducerApiTest->stop_called_) << "Status of stopped state " << gProducerApiTest->stop_called_;

    return NULL;
}

TEST_F(ProducerApiTest, create_free_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

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

TEST_F(ProducerApiTest, create_produce_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!access_key_set_) {
        return;
    }

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
    THREAD_SLEEP(3000000);
    LOG_DEBUG("Stopping the streams");
    stop_producer_ = true;
    LOG_DEBUG("Waiting for the streams to finish and close...");
    THREAD_SLEEP(10000000);

    stop_producer_ = false;

    LOG_DEBUG("Freeing the streams.");

    // Free the streams
    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        kinesis_video_producer_->freeStream(move(streams_[i]));
    }

    LOG_DEBUG("Starting the streams again");

    // Create new streams
    for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        streams_[i] = CreateTestStream(i);

        // Spin off the producer
        EXPECT_EQ(0, pthread_create(&producer_thread_, NULL, staticProducerRoutine, reinterpret_cast<PVOID> (streams_[i].get())));
    }
#endif


    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION_IN_MICROS);

    // Indicate the cancel for the threads
    stop_producer_ = true;

    // Join the thread and wait to exit.
    // NOTE: THis is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    EXPECT_EQ(STATUS_SUCCESS, THREAD_JOIN(producer_thread_, NULL));

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primities to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    THREAD_SLEEP(100000ull);

    freeStreams();
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com