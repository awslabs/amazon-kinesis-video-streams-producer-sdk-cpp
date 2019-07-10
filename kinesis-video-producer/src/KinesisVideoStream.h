/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <mutex>
#include <iostream>
#include <utility>
#include <condition_variable>

#include "KinesisVideoProducer.h"
#include "KinesisVideoStreamMetrics.h"
#include "StreamDefinition.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Stream stop timeout duration.
 * This needs to be extended to allow post stream stop stream.
 **/
#define STREAM_CLOSED_TIMEOUT_DURATION_IN_SECONDS 120

#define DEBUG_DUMP_FRAME_INFO "DEBUG_DUMP_FRAME_INFO"

/**
* This definition comes from the Kinesis Video PIC, the typedef is to allow differentiation in case of other "Frame" definitions.
*/
using KinesisVideoFrame = ::Frame;

/**
* KinesisVideoStream is responsible for streaming any type of data into KinesisVideo service
*
* Example Usage:
* @code:
* auto client(KinesisVideoClient<DeviceInfoProviderImpl, CallbackProviderImpl>::getInstance());
* auto stream(client.createStream(StreamInfo(...));
* stream.start()
*
* // Assumes you have a keepStreaming() and getFrameFromSrc() methods that do what their name sounds like they do.
* while(keepStreaming()) {
*   stream.putFrame(getFrameFromSrc());
* }
* stream.stop();
* @endcode
*/
class KinesisVideoProducer;
class KinesisVideoStream {
    friend KinesisVideoProducer;
public:

    /**
     * @return A pointer to the Kinesis Video STREAM_HANDLE for this instance.
     */
    PSTREAM_HANDLE getStreamHandle() {
        return &stream_handle_;
    }

    /**
     * Packages and streams the frame to Kinesis Video service.
     *
     * @param frame The frame to be packaged and streamed.
     * @return true if the encoder accepted the frame and false otherwise.
     */
    bool putFrame(KinesisVideoFrame frame) const;

    /**
     * Gets the stream metrics.
     *
     * @return Stream metrics object to be filled with the current metrics data.
     */
    KinesisVideoStreamMetrics getMetrics() const;

    /**
     * Appends a "metadata" - a key/value string pair into the stream.
     *
     * NOTE: The metadata is modeled as MKV tags and are not immediately put into the stream as
     * it might break the fragment.
     * This is a limitation of MKV format as Tags are level 1 elements.
     * Instead, they will be accumulated and inserted in-between the fragments and at the end of the stream.
     * @param 1 name - the metadata name.
     * @param 2 value - the metadata value.
     * @param 3 persistent - whether the metadata is persistent.
     *
     */
    bool putFragmentMetadata(const std::string& name, const std::string& value, bool persistent = true);

    /**
     * Initializes the track identified by trackId with a hex-encoded codec private data
     * and puts the stream in a state that it is ready to receive frames via putFrame().
     */
    bool start(const std::string& hexEncodedCodecPrivateData, uint64_t trackId = DEFAULT_TRACK_ID);

    /**
     * Initializes the track identified by trackId  with a binary codec private data
     * and puts the stream in a state that it is ready to receive frames via putFrame().
     */
    bool start(const unsigned char* codecPrivateData, size_t codecPrivateDataSize, uint64_t trackId = DEFAULT_TRACK_ID);

    /**
     * Initializes the stream and puts the stream in a state that it is ready to receive frames via putFrame().
     */
    bool start();

    /**
     * Pulses the current upload stream. This will effectively inject a stream termination event into the stream
     * causing it to re-set the upload stream and re-acquire a new connection.
     * This is useful in situations when the current connection goes stale and the stream latency pressure
     * notification gets invoked. The listener can pulse the stream to acquire a new connection to re-stream
     * the accumulated frames in the buffer without loosing them.
     */
    bool resetConnection();

    /**
     * Restart/Reset a stream by dropping remaining data and reset stream state machine.
     * This would be dropping all frame data in current buffer and restart the stream with new incoming frame data.
     */
    bool resetStream();

    /**
     * Stops the the stream. Consecutive calls will fail until start is called again.
     *
     * NOTE: The function is async and will return immediately but the stream buffer
     * will continue emptying until it's finished and the close stream will be called.
     */
    bool stop();

    /**
     * Stops the the stream and awaits until the buffer is depleted or a timeout occured.
     */
    bool stopSync();

    bool operator==(const KinesisVideoStream &rhs) const {
        return stream_handle_ == rhs.stream_handle_ &&
               stream_name_ == rhs.stream_name_;
    }

    bool operator!=(const KinesisVideoStream &rhs) const {
        return !(rhs == *this);
    }

    KinesisVideoStream(const KinesisVideoStream &rhs)
            : stream_handle_(rhs.stream_handle_),
              kinesis_video_producer_(rhs.kinesis_video_producer_),
              stream_name_(rhs.stream_name_) {}

    std::string getStreamName() {
        return stream_name_;
    }

    const KinesisVideoProducer& getProducer() const {
        return kinesis_video_producer_;
    }

protected:
    /**
     * Non-public constructor as streams should be only created by the producer client
     */
    KinesisVideoStream(const KinesisVideoProducer& kinesis_video_producer, const std::string stream_name);

    /**
     * Non-public destructor as the streams should be de-allocated by the producer client
     */
    virtual ~KinesisVideoStream();

    /**
     * Static function to call destructor needed for the shared_ptr in the producer client object
     */
    static void videoStreamDeleter(KinesisVideoStream* kinesis_video_stream) {
        delete kinesis_video_stream;
    }

    /**
     * Stops the the stream immediately and frees the resources.
     * Consecutive calls will fail.
     */
    void free();

    /**
     * Pointer to an opaque Kinesis Video stream.
     */
    STREAM_HANDLE stream_handle_;

    /**
     * Stores the parent object
     */
    const KinesisVideoProducer& kinesis_video_producer_;

    /**
     * Stream name for reference
     */
    const std::string stream_name_;

    /**
     * Flag used to ensure idempotency of freeKinesisVideoStream().
     */
    std::once_flag free_kinesis_video_stream_flag_;

    /**
     * Whether the stream is closed
     */
    volatile bool stream_closed_;

    /**
     * Stream metrics
     */
    KinesisVideoStreamMetrics stream_metrics_;

    /**
     * Whether to dump frame info into file.
     */
    bool debug_dump_frame_info_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
