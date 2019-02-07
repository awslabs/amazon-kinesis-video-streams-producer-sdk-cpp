#include "TestDefaultCallbackProvider.h"
#include "Logger.h"
#include <thread>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

STATUS TestDefaultCallbackProvider::putStreamHandler(UINT64 custom_data, PCHAR stream_name,
                             PCHAR container_type, UINT64 start_timestamp, BOOL absolute_fragment_timestamp,
                             BOOL do_ack, PCHAR streaming_endpoint, PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<TestDefaultCallbackProvider *>(custom_data);

    if (this_obj->fault_inject_curl && this_obj->total_curl_termination > 0 && this_obj->at_token_rotation) {
        UPLOAD_HANDLE handle_to_terminate = this_obj->terminate_previous_upload_at_putStream ? this_obj->current_upload_handle_ - 1: this_obj->current_upload_handle_;
        auto state = this_obj->active_streams_.get(handle_to_terminate);
        if (!this_obj->terminate_previous_upload_at_putStream || state != nullptr) {
            auto async_call = [](std::shared_ptr<OngoingStreamState> state, int terminate_curl_timeout, UPLOAD_HANDLE handle_to_terminate, TestDefaultCallbackProvider *this_obj) -> auto {
                while (state == nullptr) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    state = this_obj->active_streams_.get(handle_to_terminate);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(terminate_curl_timeout));
                if (state->getResponse() != nullptr) {
                    LOG_DEBUG("fault injecting curl");
                    state->getResponse()->terminate();
                }

            };
            auto worker = std::thread(async_call, state, this_obj->terminate_curl_timeout, handle_to_terminate, this_obj);
            worker.detach();
        }
        this_obj->total_curl_termination--;
    }

    if (!this_obj->at_token_rotation) {
        this_obj->at_token_rotation = true;
    }

    return DefaultCallbackProvider::putStreamHandler(custom_data, stream_name, container_type, start_timestamp,
                                                    absolute_fragment_timestamp, do_ack, streaming_endpoint, service_call_ctx);

}

PutStreamFunc TestDefaultCallbackProvider::getPutStreamCallback() {
    return putStreamHandler;
}

}
}
}
}