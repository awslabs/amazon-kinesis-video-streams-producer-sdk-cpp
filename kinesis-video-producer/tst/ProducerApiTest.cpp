#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
class ProducerApiTest : public ProducerTestBase {
};

extern ProducerTestBase* gProducerApiTest;

PVOID staticProducerRoutine(PVOID arg)
{
    auto kinesis_video_stream = reinterpret_cast<KinesisVideoStream*> (arg);
    return gProducerApiTest->basicProducerRoutine(kinesis_video_stream);
}

PVOID ProducerTestBase::basicProducerRoutine(KinesisVideoStream* kinesis_video_stream) {
    UINT32 index = 0;
    UINT64 timestamp = 0;
    Frame frame;

    // Loop until cancelled
    frame.duration = FRAME_DURATION_IN_MICROS * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
    frame.size = SIZEOF(frameBuffer_);
    frame.frameData = frameBuffer_;
    MEMSET(frame.frameData, 0x55, frame.size);

    while (!stop_producer_) {
        // Produce frames
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

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

        EXPECT_TRUE(kinesis_video_stream->putFrame(frame));

        // Sleep a while
        usleep(FRAME_DURATION_IN_MICROS);
    }

    LOG_DEBUG("Stopping the stream: " << kinesis_video_stream->getStreamName());
    kinesis_video_stream->stop();

    // The awaiting should really be handled properly with signalling per stream.
    // We will wait for 10 seconds only
    // This is for a demo purpose only!!!
    index = 0;
    while (!gProducerApiTest->stop_called_ && index < 1000) {
        LOG_DEBUG("Awaiting for the stopped notification... Status of stopped state " << gProducerApiTest->stop_called_);
        usleep(10000L);
        index++;
    }

    return NULL;
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
        EXPECT_EQ(0, pthread_create(&producer_thread_, NULL, staticProducerRoutine, reinterpret_cast<PVOID> (streams_[i].get())));
    }

#if 0
    // This section will simply demonstrate a stream termination, waiting for the termination and then cleanup and
    // a consequent re-creation and restart. Normally, after stopping, the customers application will have to
    // await until the stream stopped event is called.
    sleep(3);
    LOG_DEBUG("Stopping the streams");
    stop_producer_ = true;
    LOG_DEBUG("Waiting for the streams to finish and close...");
    sleep(10);

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
    sleep(TEST_EXECUTION_DURATION_IN_SECONDS);

    // Indicate the cancel for the threads
    stop_producer_ = true;

    // Join the thread and wait to exit.
    // NOTE: THis is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    EXPECT_EQ(0, pthread_join(producer_thread_, NULL));

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primities to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    usleep(100000L);

    freeStreams();
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com