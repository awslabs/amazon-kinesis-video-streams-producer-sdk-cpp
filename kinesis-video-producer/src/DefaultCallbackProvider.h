/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "CallbackProvider.h"
#include "ClientCallbackProvider.h"
#include "StreamCallbackProvider.h"
#include "ThreadSafeMap.h"
#include "GetTime.h"

#include "Auth.h"

#include <algorithm>
#include <memory>
#include <thread>
#include <future>
#include <list>
#include <cstdint>
#include <string>
#include <mutex>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class DefaultCallbackProvider : public CallbackProvider {
public:
    using callback_t = ClientCallbacks;
    explicit DefaultCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider = (std::unique_ptr<CredentialProvider>) new EmptyCredentialProvider(),
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "",
            const std::string &user_agent_name = "",
            const std::string &custom_user_agent = "",
            const std::string &cert_path = "",
            bool is_caching_endpoint = false,
            uint64_t caching_update_period = DEFAULT_ENDPOINT_CACHE_UPDATE_PERIOD);

    virtual ~DefaultCallbackProvider();

    callback_t getCallbacks() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCurrentTimeCallback()
     */
    GetCurrentTimeFunc getCurrentTimeCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getSecurityTokenCallback()
     */
    GetSecurityTokenFunc getSecurityTokenCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDeviceCertificateCallback()
     */
    GetDeviceCertificateFunc getDeviceCertificateCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDeviceFingerprintCallback()
     */
    GetDeviceFingerprintFunc getDeviceFingerprintCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamUnderflowReportCallback()
     */
    StreamUnderflowReportFunc getStreamUnderflowReportCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStorageOverflowPressureCallback()
     */
    StorageOverflowPressureFunc getStorageOverflowPressureCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamLatencyPressureCallback()
     */
    StreamLatencyPressureFunc getStreamLatencyPressureCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDroppedFrameReportCallback()
     */
    DroppedFrameReportFunc getDroppedFrameReportCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDroppFragmentReportCallback()
     */
    DroppedFragmentReportFunc getDroppedFragmentReportCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamErrorReportCallback()
     */
    StreamErrorReportFunc getStreamErrorReportCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamReadyCallback()
     */
    StreamReadyFunc getStreamReadyCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamClosedCallback()
     */
    StreamClosedFunc getStreamClosedCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getFragmentAckReceivedCallback()
     */
    FragmentAckReceivedFunc getFragmentAckReceivedCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamingTokenCallback()
     */
    GetStreamingTokenFunc getStreamingTokenCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getClientReadyCallback()
     */
    ClientReadyFunc getClientReadyCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateDeviceCallback()
     */
    CreateDeviceFunc getCreateDeviceCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDeviceCertToTOkenCallback()
     */
    DeviceCertToTokenFunc getDeviceCertToTokenCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamDataAvailableCallback()
     */
    StreamDataAvailableFunc getStreamDataAvailableCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamConnectionStaleCallback()
     */
    StreamConnectionStaleFunc getStreamConnectionStaleCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getBufferDurationOverflowPressureCallback()
     */
    BufferDurationOverflowPressureFunc getBufferDurationOverflowPressureCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    CreateStreamFunc getCreateStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    DescribeStreamFunc getDescribeStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    GetStreamingEndpointFunc getStreamingEndpointCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    PutStreamFunc getPutStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    TagResourceFunc getTagResourceCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getLogPrintCallback()
     */
    LogPrintFunc getLogPrintCallback() override;

    /**
     * Gets the current time in 100ns from some timestamp.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     *
     * @return Current time value in 100ns
     */
    static UINT64 getCurrentTimeHandler(UINT64 custom_data);

    /**
     * Reports a ready state for the client.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 CLIENT_HANDLE - The client handle.
     *
     * @return Status of the callback
     */
    static STATUS clientReadyHandler(UINT64 custom_data, CLIENT_HANDLE client_handle);

    /**
     * Reports storage pressure.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 UINT64 - Remaining bytes.
     *
     * @return Status of the callback
     */
    static STATUS storageOverflowPressureHandler(UINT64 custom_data, UINT64 bytes_remaining);

    /**
     * Reports an underflow for the stream.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - Reporting for this stream.
     *
     * @return Status of the callback
     */
    static STATUS streamUnderflowReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle);

    /**
     * Reports stream latency excess.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - The current buffer duration/depth in 100ns.
     *
     * @return Status of the callback
     */
    static STATUS streamLatencyPressureHandler(UINT64 custom_data,
                                               STREAM_HANDLE stream_handle,
                                               UINT64 buffer_duration);

    /**
     * Reports a dropped frame for the stream.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - The timecode of the dropped frame in 100ns.
     *
     * @return Status of the callback
     */
    static STATUS droppedFrameReportHandler(UINT64 custom_data,
                                            STREAM_HANDLE stream_handle,
                                            UINT64 timecode);

    /**
     * Reports temporal buffer pressure.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - Reporting for this stream.
     * @param 3 UINT64 - Remaining duration in hundreds of nanos.
     *
     * @return Status of the callback
     */
    static STATUS bufferDurationOverflowPressureHandler(UINT64 custom_data,
                                                        STREAM_HANDLE stream_handle,
                                                        UINT64 remaining_duration);

    /**
     * Reports a dropped fragment for the stream.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - The timecode of the dropped fragment in 100ns.
     *
     * @return Status of the callback
     */
    static STATUS droppedFragmentReportHandler(UINT64 custom_data,
                                               STREAM_HANDLE stream_handle,
                                               UINT64 timecode);

    /**
     * Reports stream staleness as the last buffering ack is greater than
     * a specified duration in the stream caps.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - Duration of the last buffering ACK received in 100ns.
     *
     * @return Status of the callback
     */
    static STATUS streamConnectionStaleHandler(UINT64 custom_data,
                                               STREAM_HANDLE stream_handle,
                                               UINT64 last_ack_duration);

    /**
     * Reports a ready state for the stream.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     *
     * @return Status of the callback
     */
    static STATUS streamReadyHandler(UINT64 custom_data, STREAM_HANDLE stream_handle);

    /**
     * Reports a received fragment ack for the stream.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UPLOAD_HANDLE - the current stream upload handle.
     * @param 4 PFragmentAck - The constructed fragment ack.
     *
     * @return Status of the callback
    */
    static STATUS fragmentAckReceivedHandler(UINT64 custom_data,
                                             STREAM_HANDLE stream_handle,
                                             UPLOAD_HANDLE upload_handle,
                                             PFragmentAck fragment_ack);

    /**
     * Creates/updates the device in the cloud. Currently, no-op.
     * createDeviceResultEvent() callback to drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param device_name The device name
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS
    createDeviceHandler(UINT64 custom_data, PCHAR device_name, PServiceCallContext service_call_ctx);

    /**
     * Handles stream fragment errors.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param STREAM_HANDLE stream handle for the stream
     * @param UPLOAD_HANDLE the current stream upload handle.
     * @param UINT64 errored fragment timecode
     * @param STATUS status code of the failure
     * @return Status of the callback
     */
    static STATUS
    streamErrorHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 fragment_timecode, STATUS status);

    /**
     * Gets triggered when data becomes available
     *
     * @param custom_data Must be a handle to the implementation of the CallbackProvider class.
     * @param stream_handle opaque handle to the stream
     * @param stream_name - the name of the stream that has data available.
     * @param stream_upload_handle - the current stream upload handle.
     * @param duration_available_in_hundred_nanos
     * @param size_available_in_bytes
     * @return
     */
    static STATUS streamDataAvailableHandler(UINT64 custom_data,
                                             STREAM_HANDLE stream_handle,
                                             PCHAR stream_name,
                                             UPLOAD_HANDLE stream_upload_handle,
                                             UINT64 duration_available,
                                             UINT64 size_available);

    /**
     * Gets triggered the closing of the stream
     *
     * @param custom_data Must be a handle to the implementation of the CallbackProvider class.
     * @param stream_handle opaque handle to the stream
     * @param stream_upload_handle opaque handle to the current stream upload from the client.
     * @return
     */
    static STATUS streamClosedHandler(UINT64 custom_data,
                                      STREAM_HANDLE stream_handle,
                                      UPLOAD_HANDLE stream_upload_handle);

    /**
     * Use log4cplus to print the logs
     *
     * @param level Log level.
     * @param tag Log tag.
     * @param fmt Log format.
     */
    static VOID logPrintHandler(UINT32 level, PCHAR tag, PCHAR fmt, ...);

protected:
    StreamCallbacks getStreamCallbacks();
    ProducerCallbacks getProducerCallbacks();
    PlatformCallbacks getPlatformCallbacks();

    /**
     * Stores the region for the service
     */
    std::string region_;

    /**
     * Stores the service URI
     */
    std::string control_plane_uri_;

    /**
     * Stores the service name
     */
    std::string service_;

    /**
     * Certificate directory path
     */
    const std::string cert_path_;

    /**
     * Stores the credentials provider
     */
    std::unique_ptr <CredentialProvider> credentials_provider_;

    /**
     * Stores the client level callbacks
     */
    std::unique_ptr <ClientCallbackProvider> client_callback_provider_;

    /**
     * Stores the stream level API
     */
    std::unique_ptr <StreamCallbackProvider> stream_callback_provider_;

    /**
     * Stores all callbacks from PIC
     */
    PClientCallbacks client_callbacks_;

    /**
     * Stores all auth callbacks from C++
     */
    AuthCallbacks auth_callbacks_;

    /**
     * Stores all producer callbacks from C++
     */
    ProducerCallbacks producer_callbacks_;

    /**
     * Stores all stream callbacks from C++
     */
    StreamCallbacks stream_callbacks_;

    /**
     * Stores all platform callbacks from C++
     */
    PlatformCallbacks platform_callbacks_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
