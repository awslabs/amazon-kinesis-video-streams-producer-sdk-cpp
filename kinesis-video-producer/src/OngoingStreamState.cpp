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
        : stream_handle_(stream_handle),
          stream_name_(stream_name),
          end_of_stream_(false), shutdown_(false),
          upload_handle_(upload_handle),
          debug_dump_file_(debug_dump_file),
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
    // Un-pause the curl
    if (curl_response_ != nullptr) {
        curl_response_->unPause();
    }
}

size_t OngoingStreamState::postHeaderReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_TRACE("postHeaderReadFunc (curl callback) invoked");

    size_t data_size = item_size * n_items;

    LOG_DEBUG("Curl post header write function returned:" << string(buffer, data_size));

    return data_size;
}

size_t OngoingStreamState::postBodyStreamingReadFunc(char *buffer, size_t item_size, size_t n_items) {
    LOG_TRACE("postBodyStreamingReadFunc (curl callback) invoked");

    UPLOAD_HANDLE upload_handle = getUploadHandle();

    size_t buffer_size = item_size * n_items;

    // Block until data is available.
    size_t bytes_written = 0;
    while (bytes_written == 0) {
        // Check for end-of-stream before the await
        if (isEndOfStream()) {
            // Returning 0 will close the connection
            LOG_INFO("Closing connection for upload stream handle: " << upload_handle);
            return 0;
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

        LOG_TRACE("Get Stream data returned: " << " buffer size: " << buffer_size
                                               << " written bytes: " << bytes_written
                                               << " for client stream handle: " << client_stream_handle
                                               << " current stream handle: " << upload_handle);

        // The return should be OK, no more data or an end of stream
        switch (retStatus) {
            case STATUS_SUCCESS:
            case STATUS_NO_MORE_DATA_AVAILABLE:
                // Check if we the return is for this handle
                if (client_stream_handle != upload_handle) {
                    // Pulse the 'other' stream handler
                    pulseUploadHandle(client_stream_handle);

                    bytes_written = 0;
                } else {
                    // Media pipeline thread might be blocked due to heap or temporal limit.
                    // Pause curl read and wait for persisted ack.
                    if (0 == bytes_written) {
                        LOG_DEBUG("Pausing CURL read for upload handle: " << upload_handle);
                        bytes_written = CURL_READFUNC_PAUSE;
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
                // Check if we the return is for this handle
                if (client_stream_handle != upload_handle) {
                    // Pulse the 'other' stream handler
                    pulseUploadHandle(client_stream_handle);

                    bytes_written = 0;
                } else {
                    LOG_DEBUG("Pausing CURL read to await ACK for upload handle: " << upload_handle);
                    bytes_written = CURL_READFUNC_PAUSE;
                }
                break;

            default:
                LOG_ERROR("Failed to get data from the stream with an error: " << retStatus);

                // Terminate and close the connection
                bytes_written = CURL_READFUNC_ABORT;
        }
    }

    if (bytes_written != CURL_READFUNC_ABORT && bytes_written != CURL_READFUNC_PAUSE) {
        LOG_DEBUG("Wrote " << bytes_written << " bytes to Kinesis Video. Upload stream handle: " << upload_handle);

        if (bytes_written != 0 &&
            debug_dump_file_) {
            debug_dump_file_stream_.write(buffer, bytes_written);
        }
    }

    if (bytes_written == CURL_READFUNC_PAUSE && curl_response_ != nullptr) {
        curl_response_->pause();
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
    return curl_response_ == nullptr ? false : curl_response_->unPause();
}

void OngoingStreamState::pulseUploadHandle(uint64_t upload_handle) {
    auto callback_provider = getCallbackProvider();
    auto custom_data = callback_provider->getCallbacks().customData;
    auto data_available_callback = callback_provider->getStreamDataAvailableCallback();
    if (nullptr != data_available_callback) {
        CHAR stream_name[MAX_STREAM_NAME_LEN + 1];
        STRNCPY(stream_name, getStreamName().c_str(), MAX_STREAM_NAME_LEN);
        stream_name[MAX_STREAM_NAME_LEN] = '\0';
        LOG_DEBUG("Indicating data available for a different upload handle: " << upload_handle);
        data_available_callback(custom_data,
                                getStreamHandle(),
                                stream_name,
                                upload_handle,
                                0,
                                0);
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
