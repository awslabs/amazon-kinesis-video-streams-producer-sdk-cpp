/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"
#include "AwsV4Signer.h"
#include "CurlCallManager.h"
#include "Logger.h"
#include "CallbackProvider.h"
#include "ClientCallbackProvider.h"
#include "StreamCallbackProvider.h"
#include "ThreadSafeMap.h"
#include "OngoingPutFrameState.h"

#include <algorithm>
#include <memory>
#include <thread>
#include <future>
#include <list>
#include <cstdint>
#include <string>
#include <mutex>

namespace {
/**
 * Defining the defaults for the service
 */
const std::string DEFAULT_AWS_REGION  = "us-west-2";
const std::string KINESIS_VIDEO_SERVICE_NAME = "kinesisvideo";
const std::string DEFAULT_CONTROL_PLANE_URI = "https://kinesisvideo.us-west-2.amazonaws.com";
const std::string CONTROL_PLANE_URI_PREFIX = "https://";
const std::string CONTROL_PLANE_URI_POSTFIX = ".amazonaws.com";
}

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class DefaultCallbackProvider : public CallbackProvider {
public:
    explicit DefaultCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider =
                std::make_unique<EmptyCredentialProvider>(),
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "");

    virtual ~DefaultCallbackProvider();

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
     * @return A mutable reference map from stream name to the OngoingPutFrameState associated with that stream name.
     */
    ThreadSafeMap<STREAM_HANDLE, OngoingPutFrameState*> &getActiveStreams();

    /**
     * Gets the current time in 100ns from some timestamp.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     *
     * @return Current time value in 100ns
     */
    static UINT64 getCurrentTimeHandler(UINT64 custom_data);

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
    static STATUS createStreamHandler(
            UINT64 custom_data,
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
    static STATUS
    describeStreamHandler(UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx);

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
    static STATUS streamingEndpointHandler(
            UINT64 custom_data, PCHAR stream_name, PCHAR api_name,
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
    static STATUS streamingTokenHandler(
            UINT64 custom_data, PCHAR stream_name, STREAM_ACCESS_MODE access_mode,
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
    putStreamHandler(UINT64 custom_data, PCHAR stream_name, PCHAR container_type, UINT64 start_timestamp,
                     BOOL absolute_fragment_timestamp, BOOL do_ack, PCHAR streaming_endpoint,
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
    static STATUS tagResourceHandler(
            UINT64 custom_data,
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
     * @param UINT64 errored fragment timecode
     * @param STATUS status code of the failure
     * @return Status of the callback
     */
    static STATUS
    streamErrorHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 fragment_timecode, STATUS status);

    /**
     * Gets triggered when data becomes available
     *
     * @param custom_data Must be a handle to the implementation of the CallbackProvider class.
     * @param stream_handle opaque handle to the stream
     * @param stream_name - the name of the stream that has data available.
     * @param duration_available_in_hundred_nanos
     * @param size_available_in_bytes
     * @return
     */
    static STATUS streamDataAvailableHandler(
            UINT64 custom_data,
            STREAM_HANDLE stream_handle,
            PCHAR stream_name,
            UINT64 duration_available,
            UINT64 size_available);

    /**
     * Gets triggered the closing of the stream
     *
     * @param custom_data Must be a handle to the implementation of the CallbackProvider class.
     * @param stream_handle opaque handle to the stream
     * @return
     */
    static STATUS streamClosedHandler(
            UINT64 custom_data,
            STREAM_HANDLE stream_handle);

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
     * @param custom_data Custom handle passed by the caller (an instance of OngoingPutFrameState*)
     * @return The number of bytes written to the buffer
     */
    static size_t postBodyStreamingReadFunc(char *buffer, size_t item_size, size_t n_items, void *custom_data);

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
     * @param custom_data Custom handle passed by the caller (an instance of OngoingPutFrameState*)
     * @return The number of bytes written to the buffer
     */
    static size_t postHeaderReadFunc(char *buffer, size_t item_size, size_t n_items, void *custom_data);

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
     * @param custom_data Custom handle passed by the caller
     * @return The number of bytes read from the buffer
     */
    static size_t postBodyStreamingWriteFunc(char *buffer, size_t item_size, size_t n_items, void *custom_data);

protected:

    /**
     * Convenience method to convert Kinesis Video string statuses to their corresponding enum value.
     * If the status is not recognized, a std::runtime_exception will be thrown.
     *
     * @param status The status returned by Kinesis Video as a string
     * @return The KINESIS_VIDEO_STREAM_STATUS value corresponding to the status
     */
    static STREAM_STATUS getStreamStatusFromString(const std::string &status);

    /**
     * Safe frees a buffer
     * @param buffer
     */
    static void safeFreeBuffer(uint8_t** ppBuffer);

    /**
     * Returns the fully combined service URI
     */
    std::string& getControlPlaneUri() {
        return control_plane_uri_;
    }

    /**
     * Removes the current state and sets the next one in the active states
     */
    void removeActiveState(OngoingPutFrameState* state);

    /**
     * Finds the next state and makes it active
     */
    void activateNextState(OngoingPutFrameState* state);

    /**
     * Upserts the new stream state object
     */
    void insertActiveState(OngoingPutFrameState* state);

    /**
     * Notifies the client callback on an error status
     */
    void notifyResult(STATUS status, STREAM_HANDLE stream_handle) const;

    /**
     * SIGV4 request signer used by curl call manager to sign HTTP requests.
     */
    CurlCallManager &ccm_;

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
     * Storage for the serialized security token
     */
    uint8_t* security_token_;

    /**
     * Mutex needed for locking the states for atomic operations
     */
    std::recursive_mutex active_streams_mutex_;

    /**
     * A map which holds a reference mapping the stream handle to th OngoingPutFrameState instance associated with that
     * stream name.
     *
     * This map exists so that a raw pointer (void*) of OngoingPutFrameState can be passed into the curl callback, as
     * required by the curl API. Upon exit of the async task for put stream, the map entry is removed in the thunk
     * returned by the pending_tasks_ future. At that point the OngoingPutFrameState shared pointer falls out of scope.
     *
     */
    ThreadSafeMap<STREAM_HANDLE, OngoingPutFrameState*> active_streams_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
