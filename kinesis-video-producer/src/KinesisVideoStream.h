/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "Logger.h"
#include <mutex>
#include <iostream>
#include <utility>
#include <condition_variable>

#include "com/amazonaws/kinesis/video/client/Include.h"
#include "KinesisVideoProducer.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
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
    virtual ~KinesisVideoStream();

    /**
     * @return A pointer to the Kinesis Video STREAM_HANDLE for this instance.
     */
    PSTREAM_HANDLE getStreamHandle() {
        return &stream_handle_;
    }

    /**
     * Encodes and streams the frame to Kinesis Video service.
     *
     * @param frame The frame to be encoded and streamed.
     * @return true if the encoder accepted the frame and false otherwise.
     */
    bool putFrame(KinesisVideoFrame frame) const;

    /**
     * Initializes the encoder with a hex-encoded codec private data
     * and puts the stream in a state that it is ready to receive frames via putFrame().
     */
    bool start(const std::string& hexEncodedCodecPrivateData);

    /**
     * Initializes the encoder with a binary codec private data
     * and puts the stream in a state that it is ready to receive frames via putFrame().
     */
    bool start(const unsigned char* codecPrivateData, size_t codecPrivateDataSize);

    /**
     * Initializes the encoder and puts the stream in a state that it is ready to receive frames via putFrame().
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
     * Stops the the stream. Consecutive calls will fail until start is called again.
     *
     * NOTE: The function is async and will return immediately but the stream buffer
     * will continue emptying until it's finished and the close stream will be called.
     */
    bool stop();

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

    std::mutex& getStreamMutex() {
        return stream_ready_mutex_;
    }

    std::condition_variable& getStreamReadyVar() {
        return stream_ready_var_;
    }

    bool isReady() const {
        return stream_ready_;
    }

    void streamReady() {
        stream_ready_ = true;
    }

    std::string getStreamName() {
        return stream_name_;
    }

    const KinesisVideoProducer& getProducer() const {
        return kinesis_video_producer_;
    }

    void getStreamMetrics(StreamMetrics& metrics);

private:
    KinesisVideoStream(const KinesisVideoProducer& kinesis_video_producer, const std::string stream_name);

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
     * Whether the stream is ready
     */
    volatile bool stream_ready_;

    /**
     * Mutex needed for the condition variable for stream ready locking.
     */
    std::mutex stream_ready_mutex_;

    /**
     * Condition variable used to signal the stream being ready.
     */
    std::condition_variable stream_ready_var_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
