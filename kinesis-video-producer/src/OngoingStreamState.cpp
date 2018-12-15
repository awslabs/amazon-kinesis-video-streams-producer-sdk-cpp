/** Copyright 2017 Amazon.com. All rights reserved. */

#include "OngoingStreamState.h"
#include "Response.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

#define DEFAULT_DATA_READY_CV_TIMEOUT_MS 10
#define DEFAULT_AWAIT_DATA_LOOP_CYCLE 100
#define DEFAULT_OFFLINE_DATA_READY_CV_TIMEOUT_HOURS 10

LOGGER_TAG("com.amazonaws.kinesis.video");

using namespace std;
using std::string;

OngoingStreamState::OngoingStreamState(CallbackProvider* callback_provider,
                                       UPLOAD_HANDLE upload_handle,
                                       STREAM_HANDLE stream_handle,
                                       std::string stream_name,
                                       bool debug_dump_file)
        : stream_handle_(stream_handle), duration_available_(0),
          bytes_available_(0), stream_name_(stream_name),
          end_of_stream_(false), shutdown_(false),
          upload_handle_(upload_handle),
          debug_dump_file_(debug_dump_file),
          awaiting_persisted_ack_(false),
          callback_provider_(callback_provider) {
    if (debug_dump_file) {
        std::ostringstream dump_file_name;
        dump_file_name << stream_name << "_" << upload_handle << ".mkv";
        debug_dump_file_stream_.open(dump_file_name.str(), ios::out | ios::trunc | ios::binary);
    }
}

void OngoingStreamState::noteDataAvailable(UINT64 duration_available, UINT64 size_available) {
    LOG_TRACE("Note data received: duration(100ns): "
                      << duration_available
                      << " bytes: "
                      << size_available
                      << " for stream handle: "
                      << upload_handle_);

    // Special handling for EOS triggering from close handle
    if (duration_available == 0 && size_available == 0) {
        endOfStream();
    }

    setDataAvailable(duration_available, size_available);
}

void OngoingStreamState::setDataAvailable(UINT64 duration_available, UINT64 size_available) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    duration_available_ = duration_available;
    bytes_available_ = size_available;
    data_ready_.notify_one();
}

size_t OngoingStreamState::awaitData(size_t read_size) {
    int32_t iterations = 0;
    size_t ret_size = 0;
    LOG_TRACE("Awaiting data...");
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        do {
            if (0 < bytes_available_ || isEndOfStream()) {
                // Fast path - bytes are available so return immediately
                ret_size = (size_t)bytes_available_;
                bytes_available_ -= MIN(read_size, bytes_available_);
                break;
            }

            // Slow path - block until data arrives.
            // Wait may return without any data available due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            PStreamInfo pStreamInfo;
            kinesisVideoStreamGetStreamInfo(stream_handle_, &pStreamInfo);
            auto time_out = IS_OFFLINE_STREAMING_MODE(pStreamInfo->streamCaps.streamingType) ?
                            chrono::milliseconds(DEFAULT_DATA_READY_CV_TIMEOUT_MS) :
                            chrono::hours(DEFAULT_OFFLINE_DATA_READY_CV_TIMEOUT_HOURS);
            data_ready_.wait_for(lock, time_out,
                                 [this]() { return (0 < bytes_available_ || isEndOfStream()); });
            iterations++;
        } while (iterations < DEFAULT_AWAIT_DATA_LOOP_CYCLE);
    }

    return ret_size;
}

size_t OngoingStreamState::postHeaderReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_TRACE("postHeaderReadFunc (curl callback) invoked");

    size_t data_size = item_size * n_items;

    LOG_DEBUG("Curl post header write function returned:" << string(buffer, data_size));

    return data_size;
}

size_t OngoingStreamState::postBodyStreamingReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_TRACE("postBodyStreamingReadFunc (curl callback) invoked");

    if (awaiting_persisted_ack_) {
        // If we are awaiting for the persisted ACK then we need to pause the upload
        LOG_TRACE("Awaiting the persisted ACK. Pausing the upload.");
        return CURL_READFUNC_PAUSE;
    }

    UPLOAD_HANDLE upload_handle = getUploadHandle();

    size_t buffer_size = item_size * n_items;

    // Block until data is available.
    size_t bytes_written = 0;
    while (bytes_written == 0) {
        // Check for end-of-stream before the await
        if (isEndOfStream()) {
            // Returning 0 will close the connection
            LOG_INFO("Closing connection for upload stream handle: " << upload_handle);
            break;
        }

        size_t available_bytes = awaitData(buffer_size);

        // Check for EOS and shutdown after the await
        if (isEndOfStream()) {
            // Returning 0 will close the connection
            LOG_INFO("Closing connection for upload stream handle: " << upload_handle);
            break;
        }

        if (isShutdown()) {
            LOG_WARN("Aborting the connection for upload stream handle: " << upload_handle);
            return CURL_READFUNC_ABORT;
        }

        UINT64 client_stream_handle = 0;
        UINT32 retrieved_size = 0;
        bytes_written = 0;
        STATUS retStatus = getKinesisVideoStreamData(
                getStreamHandle(),
                &client_stream_handle,
                reinterpret_cast<PBYTE>(buffer),
                (UINT32)buffer_size,
                &retrieved_size);
        bytes_written = (size_t)retrieved_size;

        LOG_TRACE("Available bytes to read: " << available_bytes
                                              << " buffer size: "
                                              << buffer_size
                                              << " written bytes: "
                                              << bytes_written
                                              << " for client stream handle: "
                                              << client_stream_handle
                                              << " current stream handle: "
                                              << upload_handle);

        // The return should be OK, no more data or an end of stream
        switch (retStatus) {
            case STATUS_SUCCESS:
            case STATUS_NO_MORE_DATA_AVAILABLE:
                // Check if we the return is for this handle
                if (client_stream_handle != upload_handle) {
                    // Pulse the 'other' stream handler
                    auto callback_provider = getCallbackProvider();
                    auto custom_data = callback_provider->getCallbacks().customData;
                    auto data_available_callback = callback_provider->getStreamDataAvailableCallback();
                    if (nullptr != data_available_callback) {
                        CHAR stream_name[MAX_STREAM_NAME_LEN + 1];
                        STRNCPY(stream_name, getStreamName().c_str(), MAX_STREAM_NAME_LEN);
                        stream_name[MAX_STREAM_NAME_LEN] = '\0';
                        LOG_DEBUG("Indicating data available for a different upload handle: " << client_stream_handle);
                        data_available_callback(custom_data,
                                                getStreamHandle(),
                                                stream_name,
                                                client_stream_handle,
                                                0,
                                                0);
                    }

                    bytes_written = 0;
                } else {
                    // This is an OK case - ensure that we reset the available bytes in the state if we got 0 returned
                    if (0 == bytes_written) {
                        LOG_DEBUG("Resetting the available data for the streaming state object.");
                        setDataAvailable(0, 0);

                        // awaitData timed out. Media pipeline thread might be blocked due to heap limit.
                        // Pause curl read and wait for persisted ack.
                        if (0 == available_bytes) {
                            // Pause sending data
                            LOG_DEBUG("Pausing curl read");
                            return CURL_READFUNC_PAUSE;
                        }
                    }
                }

                break;

            case STATUS_END_OF_STREAM:
                // Check if we the return is for this handle
                if (client_stream_handle != upload_handle) {
                    // Terminate the other stream
                    auto callback_provider = getCallbackProvider();
                    auto custom_data = callback_provider->getCallbacks().customData;
                    auto stream_closed_callback = callback_provider->getStreamClosedCallback();
                    if (nullptr != stream_closed_callback) {
                        LOG_DEBUG("Indicating end-of-stream for a different upload handle: " << client_stream_handle);
                        stream_closed_callback(custom_data, getStreamHandle(), client_stream_handle);
                    }

                    bytes_written = 0;
                } else {
                    LOG_INFO("Reported end-of-stream for " << getStreamName() << ". Upload handle: " << upload_handle);

                    // Output the remaining bytes and signal the EOS
                    endOfStream();
                }

                break;

            case STATUS_AWAITING_PERSISTED_ACK:
                // Send out the remaining bits and pause the upload
                awaiting_persisted_ack_ = true;
                break;

            default:
                LOG_ERROR("Failed to get data from the stream with an error: " << retStatus);

                // Terminate and close the connection
                bytes_written = CURL_READFUNC_ABORT;
        }
    }

    LOG_DEBUG("Wrote " << bytes_written << " bytes to Kinesis Video. Upload stream handle: " << upload_handle);

    if (bytes_written != 0 &&
        debug_dump_file_ &&
        (bytes_written != CURL_READFUNC_ABORT && bytes_written != CURL_READFUNC_PAUSE)) {
        debug_dump_file_stream_.write(buffer, bytes_written);
    }

    return bytes_written;
}

size_t OngoingStreamState::postBodyStreamingWriteFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_TRACE("postBodyStreamingWriteFunc (curl callback) invoked");

    STATUS status = STATUS_SUCCESS;
    size_t data_size = item_size * n_items;
    string data_as_string(buffer, data_size);

    LOG_INFO("Curl post body write function for stream: "
                     << getStreamName()
                     << " and upload handle: "
                     << getUploadHandle()
                     << " returned: "
                     << data_as_string);

    // The data can be passed in an arbitrary size so we can't make any assumptions.
    // What we do is the following. We start with the initial state of expecting to have an open curly brace.
    // We will accumulate bits until the closing curly brace. All of our ACKs have a form of
    // {"ackEventType":"PERSISTED","fragmentTimecode":3400,"fragmentNumber":"91343852344490898070324846249945695249403494059"}
    // and do not contain curlies inside as they are simple jsons. This assumption is valid for now but if it changes
    // we will need to re-implement this parsing logic. We will be stream parsing the data as it comes.

    if (!isShutdown()) {
        status = kinesisVideoStreamParseFragmentAck(getStreamHandle(), getUploadHandle(), buffer, (UINT32)data_size);
        if (STATUS_FAILED(status)) {
            LOG_ERROR("Failed to submit ACK: "
                              << data_as_string
                              << " with status code: "
                              << status);
        } else {
            LOG_TRACE("Processed ACK OK.");
        }
    }

    return data_size;
}

bool OngoingStreamState::unPause() {
    return curl_response_->unPause();
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
