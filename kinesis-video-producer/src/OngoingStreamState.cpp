/** Copyright 2017 Amazon.com. All rights reserved. */

#include "OngoingStreamState.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::string;

void OngoingStreamState::noteDataAvailable(UINT64 duration_available, UINT64 size_available) {
    LOG_DEBUG("Note data received: duration(100ns): "
                      << duration_available
                      << " bytes: "
                      << size_available
                      << " for stream handle: "
                      << upload_handle_);
    setDataAvailable(duration_available, size_available);
}

void OngoingStreamState::setDataAvailable(UINT64 duration_available, UINT64 size_available) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    duration_available_ = duration_available;
    bytes_available_ = size_available;
    data_ready_.notify_one();
}

size_t OngoingStreamState::awaitData(size_t read_size) {
    LOG_TRACE("Awaiting data...");
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        do {
            if (0 < bytes_available_ || isEndOfStream()) {
                // Fast path - bytes are available so return immediately
                size_t ret_size = bytes_available_;
                bytes_available_ -= MIN(read_size, bytes_available_);
                return ret_size;
            }
            // Slow path - block until data arrives.
            // Wait may return without any data available due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            data_ready_.wait(lock, [this]() { return (0 < bytes_available_ || isEndOfStream()); });
        } while (true);
    }
}

size_t OngoingStreamState::postHeaderReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_DEBUG("postHeaderReadFunc (curl callback) invoked");

    size_t data_size = item_size * n_items;

    LOG_DEBUG("Curl post header write function returned:" << string(buffer, data_size));

    return data_size;
}

size_t OngoingStreamState::postBodyStreamingReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_DEBUG("postBodyStreamingReadFunc (curl callback) invoked");

    // Check for end-of-stream
    if (isEndOfStream()) {
        // Returning 0 will close the connection
        LOG_INFO("Closing connection for upload stream handle: " << getUploadHandle());
        return 0;
    }

    size_t buffer_size = item_size * n_items;

    // Block until data is available.
    size_t bytes_written = 0;
    while (bytes_written == 0 && !isEndOfStream()) {
        size_t available_bytes = awaitData(buffer_size);

        if (isShutdown()) {
            return CURL_READFUNC_ABORT;
        }

        UINT64 client_stream_handle = 0;
        bytes_written = 0;
        STATUS retStatus = getKinesisVideoStreamData(
                getStreamHandle(),
                &client_stream_handle,
                reinterpret_cast<PBYTE>(buffer),
                buffer_size,
                reinterpret_cast<PUINT32>(&bytes_written));

        LOG_TRACE("Available bytes to read: " << available_bytes
                                              << " buffer size: "
                                              << buffer_size
                                              << " written bytes: "
                                              << bytes_written
                                              << " for client stream handle: "
                                              << client_stream_handle);

        // The return should be OK, no more data or an end of stream
        switch (retStatus) {
            case STATUS_SUCCESS:
            case STATUS_NO_MORE_DATA_AVAILABLE:

                // This is an OK case - ensure that we reset the available bytes in the state if we got 0 returned
                if (0 == bytes_written) {
                    LOG_DEBUG("Resetting the available data for the streaming state object.");
                    setDataAvailable(0, 0);
                }

                break;

            case STATUS_END_OF_STREAM:
                LOG_INFO("Reported end-of-stream for " << getStreamName() << ". Upload handle: " << getUploadHandle());

                // Output the remaining bytes and signal the EOS
                endOfStream();
                break;

            default:
                LOG_ERROR("Failed to get data from the stream with an error: " << retStatus);

                // Terminate and close the connection
                bytes_written = CURL_READFUNC_ABORT;
        }
    }

    LOG_DEBUG("Wrote " << bytes_written << " bytes to Kinesis Video. Upload stream handle: " << getUploadHandle());

    return bytes_written;
}

size_t OngoingStreamState::postBodyStreamingWriteFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_DEBUG("postBodyStreamingWriteFunc (curl callback) invoked");

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
        status = kinesisVideoStreamParseFragmentAck(getStreamHandle(), getUploadHandle(), buffer, data_size);
        if (STATUS_FAILED(status)) {
            LOG_ERROR("Failed to submit ACK: "
                              << data_as_string
                              << " with status code: "
                              << status);
        } else {
            LOG_DEBUG("Processed ACK OK.");
        }
    }

    return data_size;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
