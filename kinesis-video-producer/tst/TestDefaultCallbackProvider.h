#ifndef __TEST_DEFAULT_CALLBACK_PROVIDER_H__
#define __TEST_DEFAULT_CALLBACK_PROVIDER_H__

#include <DefaultCallbackProvider.h>
#include <chrono>
#define DEFAUL_CURL_TIMEOUT_MS 5

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class TestDefaultCallbackProvider : public DefaultCallbackProvider {

public:
    explicit TestDefaultCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider):
                DefaultCallbackProvider(
                    move(client_callback_provider),
                    move(stream_callback_provider),
                    move(credentials_provider)),
                at_token_rotation(false),
                terminate_previous_upload_at_putStream(false),
                fault_inject_curl(false),
                terminate_curl_timeout(DEFAUL_CURL_TIMEOUT_MS),
                total_curl_termination(0) {}

    UPLOAD_HANDLE getCurrentUploadHandle() {
        return current_upload_handle_;
    }

    void limitSendSpeedForCurrentUploadHandle(uint32_t bytes_per_second);
    void limitReceiveSpeedForCurrentUploadHandle(uint32_t bytes_per_second);

    std::shared_ptr<OngoingStreamState> getStreamState(UPLOAD_HANDLE upload_handle) {
        return active_streams_.get(upload_handle);
    }

    PutStreamFunc getPutStreamCallback() override;

    static STATUS
    putStreamHandler(UINT64 custom_data,
                     PCHAR stream_name,
                     PCHAR container_type,
                     UINT64 start_timestamp,
                     BOOL absolute_fragment_timestamp,
                     BOOL do_ack,
                     PCHAR streaming_endpoint,
                     PServiceCallContext service_call_ctx);

    bool at_token_rotation;
    int total_curl_termination;
    int terminate_curl_timeout;
    bool terminate_previous_upload_at_putStream;
    bool fault_inject_curl;
};
}
}
}
}

#endif //__TEST_DEFAULT_CALLBACK_PROVIDER_H__
