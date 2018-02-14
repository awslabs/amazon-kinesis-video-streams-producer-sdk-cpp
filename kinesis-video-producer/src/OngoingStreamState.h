/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <cstddef>

#include <algorithm>
#include <condition_variable>
#include <utility>
#include <curl/curl.h>
#include <string>
#include <mutex>

#include "Logger.h"
#include "com/amazonaws/kinesis/video/client/Include.h"
#include "CallbackProvider.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Internal class to wrap a condition variable that signals the curl callback that data is ready and to try to read
 * data off Kinesis Video PIC's internal data buffer in order to post it over the network.
 *
 * Additionally, this class contains all the state that is needed by the postBodyStreamingFunc and is passed in as
 * the void* custom_data parameter to that class.
 */
class Response;
class OngoingStreamState {
public:
    OngoingStreamState(CallbackProvider* callback_provider,
                         UINT64 upload_handle,
                         STREAM_HANDLE stream_handle,
                         std::string stream_name)
            : stream_handle_(stream_handle), duration_available_(0),
              bytes_available_(0), stream_name_(stream_name),
              end_of_stream_(false), shutdown_(false),
              upload_handle_(upload_handle),
              callback_provider_(callback_provider) {}

    ~OngoingStreamState() = default;

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
     * Sets the current CURL response object
     */
    void setResponse(std::shared_ptr<Response> response) {
        curl_response_ = response;
    }

    /**
     * Gets the current CURL response object
     */
    std::shared_ptr<Response> getResponse() {
        return curl_response_;
    }

    /**
     * Returns the stream upload handle
     */
    UINT64 getUploadHandle() {
        return upload_handle_;
    }

    /**
     * An implementation of the CURLOPT_READFUNCTION callback: https://curl.haxx.se/libcurl/c/CURLOPT_READFUNCTION.html.
     *
     * From the documentation:
     * This callback function gets called by libcurl as soon as it needs to read data in order to send it to the peer -
     * like if you ask it to upload or post data to the server.
     * The data area pointed at by the pointer buffer should be filled up with at most item_size multiplied with n_items
     * number
     * of bytes by your function.
     * This function must then return the actual number of bytes that it stored in that memory area. Returning 0 will
     * signal end-of-file to the library and cause it to stop the current transfer.
     *
     * @param buffer Curl buffer to be filled
     * @param item_size size of each item in the buffer in bytes
     * @param n_items Max number of items of size item_size that can fit in the buffer
     * @return The number of bytes written to the buffer
     */
    size_t postBodyStreamingReadFunc(char *buffer, size_t item_size, size_t n_items);

    /**
     * An implementation of the CURLOPT_HEADERFUNCTION callback:
     * https://curl.haxx.se/libcurl/c/CURLOPT_HEADERFUNCTION.html.
     *
     * From the documentation:
     * This function gets called by libcurl as soon as it has received header data. The header callback will be called
     * once for each header and only complete header lines are passed on to the callback. Parsing headers is very easy
     * using this. The size of the data pointed to by buffer is size multiplied with nmemb. Do not assume that the
     * header line is zero terminated! The pointer named userdata is the one you set with the CURLOPT_HEADERDATA
     * option. This callback function must return the number of bytes actually taken care of. If that amount differs
     * from the amount passed in to your function, it'll signal an error to the library. This will cause the transfer
     * to get aborted and the libcurl function in progress will return CURLE_WRITE_ERROR.
     *
     * @param buffer Curl buffer to be filled
     * @param item_size size of each item in the buffer in bytes
     * @param n_items Max number of items of size item_size that can fit in the buffer
     * @return The number of bytes written to the buffer
     */
    size_t postHeaderReadFunc(char *buffer, size_t item_size, size_t n_items);

    /**
     * An implementation of the CURLOPT_READFUNCTION callback: https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html.
     *
     * From the documentation:
     * This callback function gets called by libcurl as soon as there is data received that needs to be saved. ptr
     * points to the delivered data, and the size of that data is size multiplied with nmemb.
     *
     * The callback function will be passed as much data as possible in all invokes, but you must not make any assumptions.
     * It may be one byte, it may be thousands. The maximum amount of body data that will be passed to the write
     * callback is defined in the curl.h header file: CURL_MAX_WRITE_SIZE (the usual default is 16K).
     *
     * If CURLOPT_HEADER is enabled, which makes header data get passed to the write callback, you can get up to
     * CURL_MAX_HTTP_HEADER bytes of header data passed into it. This usually means 100K.
     *
     * @param buffer Curl buffer to read from
     * @param item_size size of each item in the buffer in bytes
     * @param n_items Max number of items of size item_size available in the buffer
     * @return The number of bytes read from the buffer
     */
    size_t postBodyStreamingWriteFunc(char *buffer, size_t item_size, size_t n_items);

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
     * Stream upload handle
     */
    UINT64 upload_handle_;

    /**
     * Mutex needed for the condition variable for data available locking.
     */
    std::mutex data_mutex_;

    /**
     * Condition variable used to signal between the Kinesis Video PIC state machine execution context and the network thread that
     * data is ready or to await data.
     */
    std::condition_variable data_ready_;

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
    std::shared_ptr<Response> curl_response_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

