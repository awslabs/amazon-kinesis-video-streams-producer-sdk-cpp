#ifndef __FILE_OUTPUT_CALLBACK_PROVIDER_H__
#define __FILE_OUTPUT_CALLBACK_PROVIDER_H__

#include "CallbackProvider.h"
#include <fstream>

namespace com { namespace amazonaws { namespace kinesis { namespace video {
class FileOutputCallbackProvider : public CallbackProvider {

public:
    explicit FileOutputCallbackProvider();

    virtual ~FileOutputCallbackProvider();

    UPLOAD_HANDLE getUploadHandle() {
        return current_upload_handle_++;
    }

    /**
    * Stream is being freed
    */
    void shutdownStream(STREAM_HANDLE stream_handle) override;

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
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    CreateStreamFunc getCreateStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDescribeStreamCallback()
     */
    DescribeStreamFunc getDescribeStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamingEndpointCallback()
     */
    GetStreamingEndpointFunc getStreamingEndpointCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamingTokenCallback()
     */
    GetStreamingTokenFunc getStreamingTokenCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getPutStreamCallback()
     */
    PutStreamFunc getPutStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getTagResourceCallback()
     */
    TagResourceFunc getTagResourceCallback() override;

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
     * Retrieve the security token to authenticate the client to AWS.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param buffer Device certificate bits
     * @param size Size of the buffer
     * @param expiration certificate expiration time.
     * @return Status of the callback
     */
    static STATUS getSecurityTokenHandler(UINT64 custom_data, PBYTE *buffer, PUINT32 size, PUINT64 expiration);

    /**
     * Invoked when the Kinesis Video SDK determines that the stream, defined by the stream_name,
     * does not exist.
     *
     * The handler spawns an async task and makes a network call on that thread of execution to the createStream
     * API. If the HTTP status code returned is not 200, the response body and the HTTP status code is logged
     * and a std::runtime_exception is thrown from the async thread execution context.
     * On successful completion of the createStream call, the createStreamResultEvent() callback is invoked to
     * drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param device_name The name of this device (compute ID)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param kms_arn Optional kms key ARN to be used in encrypting the data at rest.
     * @param retention_period Archival retention period in 100ns. A value of 0 means no retention.
     * @param content_type MIME type (e.g. video/x-matroska)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS createStreamHandler(UINT64 custom_data,
                                      PCHAR device_name,
                                      PCHAR stream_name,
                                      PCHAR content_type,
                                      PCHAR kms_arn,
                                      UINT64 retention_period,
                                      PServiceCallContext service_call_ctx);

    /**
     * Invoked when the Kinesis Video SDK to check if the stream, defined by stream_name, exists.
     *
     * The handler spawns an async task and makes a network call on that thread of execution to the
     * describeStream API.
     * On successful completion of the describeStream call, the describeStreamResultEvent() callback is invoked to
     * drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @param service_call_ctx
     * @return Status of the callback
     */
    static STATUS describeStreamHandler(UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx);

    /**
     * Gets the streaming endpoint.
     * getStreamingEndpointResultEvent() callback to drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param api_name API name to call (currently PUT_MEDIA)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS streamingEndpointHandler(UINT64 custom_data,
                                           PCHAR stream_name,
                                           PCHAR api_name,
                                           PServiceCallContext service_call_ctx);

    /**
     * Currently this handler does nothing but echo the params received back to Kinesis Video by invoking
     * getStreamingTokenResultEvent() callback with data retrieved from the service_call_ctx.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param access_mode Read or write (always write in this case)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS streamingTokenHandler(UINT64 custom_data,
                                        PCHAR stream_name,
                                        STREAM_ACCESS_MODE access_mode,
                                        PServiceCallContext service_call_ctx);

    /**
     * This handler is invoked once per stream on start up of the stream to Kinesis Video.
     * An async task is spawned as the network thread and does not return until the TCP connection interrupted, the
     * stream runs out of data beyond the retry period, or the process is shut down. The internal implementation inside
     * the network thread is that it continues to invoke getKinesisVideoStreamData() to fill the POST body with a chunked
     * encoded buffer that is potentially infinite. As such, the HTTP status code is set to 200 regardless of the
     * actual return code from the server and the handler returns promptly to Kinesis Video PIC while the network task continues
     * as long as the conditions for return from the task are not reached.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param container_type Container type (set by Kinesis Video SDK) which will become the value of the
     *                       x-amzn-kinesisvideo-container-type header.
     * @param start_timestamp Start timestamp of the stream in Epoch time in 100ns units
     * @param absolute_fragment_timestamp Whether the fragment timestamps are absolute or relative
     * @param do_ack True if application level ack is required and false otherwise
     * @param streaming_endpoint Streaming endpoint to use
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS
    putStreamHandler(UINT64 custom_data,
                     PCHAR stream_name,
                     PCHAR container_type,
                     UINT64 start_timestamp,
                     BOOL absolute_fragment_timestamp,
                     BOOL do_ack,
                     PCHAR streaming_endpoint,
                     PServiceCallContext service_call_ctx);

    /**
     * Tags a stream with a tag.
     * tagResourceResultEvent() callback to drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_arn The stream arn
     * @param num_tags Number of tags that follow
     * @param tags The tags array
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS tagResourceHandler(UINT64 custom_data,
                                     PCHAR stream_arn,
                                     UINT32 num_tags,
                                     PTag tags,
                                     PServiceCallContext service_call_ctx);

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

protected:
    UPLOAD_HANDLE current_upload_handle_;
    std::ofstream debug_dump_file_stream_;
    uint8_t *buffer_;

};
}
}
}
}
#endif //__FILE_OUTPUT_CALLBACK_PROVIDER_H__
