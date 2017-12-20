/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <cstddef>

#include <algorithm>
#include <condition_variable>
#include <utility>
#include <curl/curl.h>
#include <string>
#include <mutex>

#include "com/amazonaws/kinesis/video/client/Include.h"
#include "CallbackProvider.h"
#include "Response.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Internal class to wrap a condition variable that signals the curl callback that data is ready and to try to read
 * data off Kinesis Video PIC's internal data buffer in order to post it over the network.
 *
 * Additionally, this class contains all the state that is needed by the postBodyStreamingFunc and is passed in as
 * the void* custom_data parameter to that class.
 */
class OngoingPutFrameState {
public:
    OngoingPutFrameState(CallbackProvider* callback_provider,
                         STREAM_HANDLE stream_handle,
                         std::string stream_name)
            : stream_handle_(stream_handle), duration_available_(0),
              bytes_available_(0), stream_name_(stream_name),
              end_of_stream_(false), next_state_(nullptr),
              active_(false), shutdown_(false),
              callback_provider_(callback_provider) {}

    ~OngoingPutFrameState() = default;

    /**
     *
     * Notify the condition variable that data was received and is ready for transit over the network thread.
     *
     * @param duration_available duration of the data available in 100ns.
     * @param size_available the size available for reading off Kinesis Video PIC's buffer in bytes.
     */
    void noteDataAvailable(UINT64 duration_available, UINT64 size_available);

    /**
     * Blocks until this.noteDataReceived() is invoked.
     * @param buffer_size The max buffer size to read
     * @return the bytes available for reading
     */
    size_t awaitData(size_t buffer_size);

    /**
     * Notifies the conditional variable that the state is current to unblock the data processing
     */
    void notifyStateCurrent();

    /**
     * Blocks until this.notifyStateCurrent() is invoked.
     */
    void awaitCurrent();

    /**
     * Returns the current stream handle we are streaming for
     *
     * @return STREAM_HANDLE
     */
    STREAM_HANDLE getStreamHandle() const {
        return stream_handle_;
    }

    /**
     * Returns the callback provider
     *
     * @return shared pointer to CallbackProvider object
     */
    CallbackProvider* getCallbackProvider() const {
        return callback_provider_;
    }

    /**
     * Returns the stream name
     * @return stream name
     */
    std::string getStreamName() {
        return stream_name_;
    }

    /**
     * Returns whether we have reached end-of-stream
     */
    bool isEndOfStream() {
        return end_of_stream_;
    }

    /**
     * Signals the end of the stream
     */
    void endOfStream() {
        end_of_stream_ = true;
    }

    /**
     * Returns whether CURL is in shutdown state
     */
    bool isShutdown() {
        return shutdown_;
    }

    /**
     * Signals the CURL shutdown
     */
    void shutdown() {
        end_of_stream_ = true;
        shutdown_ = true;
    }

    /**
     * Sets the size and duration of the available data
     */
    void setDataAvailable(UINT64 duration_available, UINT64 size_available);

    /**
     * Returns the next state if any
     */
    OngoingPutFrameState* getNextState() {
        return next_state_;
    }

    /**
     * Sets the next state
     */
    void setNextState(OngoingPutFrameState* next_state) {
        // Iterate until we find the last entry
        auto cur_state = this;
        while(cur_state->next_state_ != nullptr) {
            cur_state = cur_state->next_state_;
        }

        cur_state->next_state_ = next_state;
    }

    /**
     * Returns whether it's active
     */
    bool isActive() {
        return active_;
    }

    /**
     * Sets the current CURL response object
     */
    void setResponse(Response* response) {
        curl_response_ = response;
    }

    /**
     * Gets the current CURL response object
     */
    Response* getResponse() {
        return curl_response_;
    }

private:

    /**
     * Stream handle
     */
    STREAM_HANDLE stream_handle_;

    /**
     * The main owner object
     */
    CallbackProvider* callback_provider_;

    /**
     * The duration of the data available to put in 100 nanoseconds.
     * NOTE: in the future we may want to support "awaitDuration()" method call which behaves
     * similarly to awaitData(), but that it blocks until a duration is reached, instead of bytes.
     */
    volatile UINT64 duration_available_;

    /**
     * The size of the data available to send to Kinesis Video PIC in bytes.
     */
    volatile UINT64 bytes_available_;

    /**
     * Stream name
     */
    std::string stream_name_;

    /**
     * Mutex needed for the condition variable for data available locking.
     */
    std::mutex data_mutex_;

    /**
     * Mutex needed for the condition variable for state being current locking.
     */
    std::mutex state_current_mutex_;

    /**
     * Condition variable used to signal between the Kinesis Video PIC state machine execution context and the network thread that
     * data is ready or to await data.
     */
    std::condition_variable data_ready_;

    /**
     * Condition variable used to signal when the state is current so data processing can continue
     */
    std::condition_variable state_current_;

    /**
     * Whether the state is active
     */
    volatile bool active_;

    /**
     * Whether we have reached end-of-stream and the connection needs to be closed
     */
    volatile bool end_of_stream_;

    /**
     * CURL is shutting down
     */
    volatile bool shutdown_;

    /**
     * Ongoing CURL response object
     */
    Response* curl_response_;

    /**
     * Next state
     */
    OngoingPutFrameState* next_state_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

