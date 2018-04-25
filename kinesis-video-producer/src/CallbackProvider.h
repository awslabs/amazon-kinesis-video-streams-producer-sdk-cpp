/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Interface extracted from the callbacks that the Kinesis Video SDK exposes for implementation by clients.
* Some of the callbacks are optional and if left null will have defaults from the SDK used. Other callbacks must be
* implemented by the user of the SDK.
* The key to understanding the Kinesis Video SDK is that it is at its heart a state machine. The calls from the
* application drive the state machine and move it via public API calls from one state to the next.
* In order for the state machine to transition from one state to the next, it often needs the client to do work
* because the SDK itself has no threads and no place to do any meaningful work (i.e. network IO).
* This is where the callbacks defined in this class come in. They do the heavy lifting for calls that are OS
* dependant or might require a thread to handle a RPC call that may block. Most of the callbacks fall into this
* category; however there are others, that are notification callbacks which provide feedback to the application
* about a stream in the running state. These callbacks must be defined but the aren't strictly required to do anything
* interesting, but your application can take advantage of them to implement smarter congestion avoidance.
*
* The break down is as follows
*
* Required callbacks:
*
*   Authentication callback (you must implement *one* out of this group depending on your auth method):
*     getDeviceCertificateCallback();
*     getSecurityTokenCallback();
*     getDeviceFingerprintCallback();
*
*   Stream notification callbacks (you must define them, but you are not required to do anything to run):
*    getStreamUnderflowReportCallback();
*    getStorageOverflowPressureCallback();
*    getStreamLatencyPressureCallback();
*    getStreamConnectionStaleCallback();
*    getDroppedFrameReportCallback();
*    getDroppedFragmentReportCallback();
*    getStreamErrorReportCallback();
*    getStreamReadyCallback();
*    getStreamDataAvailableCallback();
*
*   State Machine Driven Callbacks (you must implement all of them and they must do the right thing):
*     getCreateStreamCallback();
*     getDescribeStreamCallback();
*     getStreamingEndpointCallback();
*     getStreamingTokenCallback();
*     getPutStreamCallback();
*     getTagResourceCallback();
*
*    Device/Client level Callbacks
*
*     getCreateDeviceCallback();
*     getDeviceCertToTokenCallback();
*     getClientReadyCallback();
*
* Optional callbacks:
*
*   OS dependent implementations (you don't have to define these):
*     getCreateMutexCallback();
*     getLockMutexCallback();
*     getUnlockMutexCallback();
*     getTryLockMutexCallback();
*     getFreeMutexCallback();
*     getCurrentTimeCallback();
*     getRandomNumberCallback();
*     getLogPrintCallback();
*
* The optional callbacks are virtual, but there are default implementations defined for them that return nullptr,
* which will therefore use the defaults provided by the Kinesis Video SDK.
*/
class CallbackProvider {
public:
    using callback_t = ClientCallbacks;

    /**
     * Gets the callbacks
     * @return ClientCallbacks
     */
    virtual callback_t getCallbacks();

    /**
     * Shutting down
     */
    virtual void shutdown();

    /**
     * Stream is being freed
     */
    virtual void shutdownStream(STREAM_HANDLE stream_handle);

    /**
     * @return Kinesis Video client default implementation
     */
    virtual CreateMutexFunc getCreateMutexCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual LockMutexFunc getLockMutexCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual UnlockMutexFunc getUnlockMutexCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual TryLockMutexFunc getTryLockMutexCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual FreeMutexFunc getFreeMutexCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual GetCurrentTimeFunc getCurrentTimeCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual GetRandomNumberFunc getRandomNumberCallback();

    /**
     * @return Kinesis Video client default implementation
     */
    virtual LogPrintFunc getLogPrintCallback();

    /**
     * The function returned by this callback takes three arguments:
     *  - UINT64 custom_data: A handle to this class.
     *  - PBYTE* buffer: A pointer that is to be set with a pointer to a buffer containing the STS token.
     *  - PUINT32 size: The size of the buffer.
     *  - PUINT64 expiration: Device certificate expiration.
     *
     * Optional Callback.
     *
     *  The implementation must populate the buffer with the STS token to be used to authenticate the device.
     *  The size shall be set according to the size populated in the buffer.
     *
     * @return a function pointer conforming to the description above.
     */
    virtual GetSecurityTokenFunc getSecurityTokenCallback();

    /**
     * The function returned by this callback takes three arguments:
     *  - UINT64 custom_data: A handle to this class.
     *  - PBYTE* buffer: A pointer that is to be set with a pointer to a buffer containing the device certificate.
     *  - PUINT32 size: The size of the buffer.
     *  - PUINT64 expiration: Device certificate expiration.
     *
     * Optional Callback.
     *
     *  The implementation must populate the buffer with the client certificate to be used to authenticate the device.
     *  The size shall be set according to the size populated in the buffer.
     *
     * @return a function pointer conforming to the description above.
     */
    virtual GetDeviceCertificateFunc getDeviceCertificateCallback();

    /**
     * The function returned by this callback takes twp arguments:
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 PCHAR* - Device fingerprint - NULL terminated.
     *
     * Optional Callback.
     *
     *  The implementation must provide a string defining unique device fingerprint to be used for provisioning.
     *
     * @return a function pointer conforming to the description above.
     */
    virtual GetDeviceFingerprintFunc getDeviceFingerprintCallback();

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting underflow.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle a stream underflow
     * event.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamUnderflowReportFunc getStreamUnderflowReportCallback();

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: A handle to this class.
     * - UINT64 bytes_remaining: The number of bytes left before storage is exhausted.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the local storage
     * pressure.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StorageOverflowPressureFunc getStorageOverflowPressureCallback();

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting latency pressure.
     * - UINT64 buffer_depth: The current buffer backlog depth (duration of time) in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the latency
     * pressure.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamLatencyPressureFunc getStreamLatencyPressureCallback();

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting received ACK.
     * - PFragmentAck fragment ack: The fragment ACK received
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to get notified
     * when an ack is received and processed.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual FragmentAckReceivedFunc getFragmentAckReceivedCallback();

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting a dropped frame.
     * - UINT64 timestamp: Timestamp of the dropped frame in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the dropped frame.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DroppedFrameReportFunc getDroppedFrameReportCallback();

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which reports an error for.
     * - UINT64 timestamp: Timestamp of the errored fragment in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the errored stream.
     * The client should terminate the stream as the inlet host would have/has already terminated the connection.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamErrorReportFunc getStreamErrorReportCallback();

    /**
    * The function returned by this callback takes three arguments:
    * - UINT64 custom_data: A handle to this class.
    * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting a stale connection.
    * - UINT64 timestamp: Duration of the last buffering ACK received in 100ns.
    *
    * Optional Callback.
    *
    * The callback returned shall take the appropriate action (decided by the implementor) to handle the stale connection.
    *
    *  @return a function pointer conforming to the description above.
    */
    virtual StreamConnectionStaleFunc getStreamConnectionStaleCallback();

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting a dropped fragment.
     * - UINT64 timestamp: Timestamp of the dropped fragment in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the dropped
     * fragment.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DroppedFragmentReportFunc getDroppedFragmentReportCallback();

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video handle for the stream which is ready.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the stream ready
     * event.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamReadyFunc getStreamReadyCallback();

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: A handle to this class.
     * - STREAM_HANDLE stream_handle: Kinesis Video handle for the stream for which EOS is reported.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle
     * the stream closed notification
     * event.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamClosedFunc getStreamClosedCallback();

    /**
     * The function returned by this callback takes five arguments:
     * - UINT64 custom_data: A handle to this class.
     * - PCHAR device_name: Device name.
     * - PCHAR stream_name: Stream name.
     * - PCHAR content_type: Content type.
     * - PCHAR kms_arn: KMS Key ID ARN Optional (to be filled in, you still need the parameter).
     * - UINT64 retention_period: Stream retention period in 100ns.
     * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
     *
     * The callback returned shall invoke the Kinesis Video create stream API routine and
     * return control to Kinesis Video client by invoking the createStreamResultEvent()
     * call with the customData field from the service context and the service
     * call result returned by Kinesis Video service.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual CreateStreamFunc getCreateStreamCallback() = 0;

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: A handle to this class.
     * - PCHAR stream_name: Stream name.
     * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
     *
     * The callback returned shall invoke the Kinesis Video describe stream API routine and
     * return control to Kinesis Video client by invoking the describeStreamResultEvent() call
     * with the customData field from the service context and the service
     * call result returned by Kinesis Video service.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DescribeStreamFunc getDescribeStreamCallback() = 0;

    /**
     * The function returned by this callback takes four arguments:
     * - UINT64 custom_data: A handle to this class.
     * - PCHAR stream_name: Stream name.
     * - PCHAR api_name: API enum
     * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
     *
     * The callback returned shall lookup the Kinesis Video endpoint to use based on the API enum and
     * return control to Kinesis Video client by invoking the getStreamingEndpointResultEvent() call
     * with the customData field from the service context, the service call result, and the endpoint retrieved.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual GetStreamingEndpointFunc getStreamingEndpointCallback() = 0;

    /**
     * The function returned by this callback takes four arguments:
     * - UINT64 custom_data: A handle to this class.
     * - PCHAR stream_name: Stream name.
     * - STREAM_ACCESS_MODE  access_made: whether this stream will be in read or write mode..
     * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
     *
     * The callback returned shall exchange the security token in the context with a streaming token which might have
     * limited expiration period as well as limited privilege allowing only putting media into a specified stream.
     * Kinesis Video client by invoking the getStreamingTokenResultEvent() call
     * with the customData field from the service context, the service call result, and the streaming token retrieved.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual GetStreamingTokenFunc getStreamingTokenCallback() = 0;

    /**
    * The function returned by this callback takes six arguments:
    * - UINT64 custom_data: A handle to this class.
    * - PCHAR stream_name: Name of the stream to which data will be put.
    * - PCHAR container_type: Container type enum.
    * - UINT64 start_timestamp: Stream start timestamp.
    * - BOOL do_ack: Application level ACK required.
    * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
    *
    * The callback shall invoke the Kinesis Video put stream API routine and maintain
     * a persistent connection to the Kinesis Video service.
    * NOTE the function will be called once each time the connection is established.
    *
    *  @return a function pointer conforming to the description above.
    */
    virtual PutStreamFunc getPutStreamCallback() = 0;

    /**
    * The function returned by this callback takes five arguments:
    * - UINT64 custom_data: A handle to this class.
    * - PCHAR stream_arn: ARN of the stream to tag.
    * - UINT32 num_tags: The number of tags contained in the tags array.
    * - PTag tags: array of resource tags corresponding to the num_tags.
    * - PServiceCallContext service_call_ctx: Service call context struct used by Kinesis Video.
    *
    * The callback shall invoke the Kinesis Video tag resource API routine and pass the tags specified by the tags
    * parameter in the POST body and the return control to the Kinesis Video client by invoking
    * the tagResourceResultEvent() call with the customData field from the service context and
    * the service call result  from the API call.
    *
    *  @return a function pointer conforming to the description above.
    */
    virtual TagResourceFunc getTagResourceCallback() = 0;

    /**
     * Reports a ready state for the client.
     *
     * Optional callback.
     *
     * The function returned by this callback takes two arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 CLIENT_HANDLE - The client handle.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual ClientReadyFunc getClientReadyCallback();

    /**
     * Reports data available for the stream. Can be used to unblock a reading thread.
     *
     * Optional callback.
     *
     * The function returned by this callback takes four arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - The duration of content currently available in 100ns.
     * @param 4 UINT64 - The size of content in bytes currently available.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamDataAvailableFunc getStreamDataAvailableCallback();

    /**
     * Creates a device in the cloud. This is an "upsert" operation.
     *
     * The function returned by this callback takes three arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 PCHAR - Device name.
     * @param 3 PServiceCallContext - Service call context.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual CreateDeviceFunc getCreateDeviceCallback() = 0;

    /**
     * Gets a security token from the device certificate
     *
     * The function returned by this callback takes three arguments:
     *
     * Optional Callback.
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 PCHAR - Device name.
     * @param 3 PServiceCallContext - Service call context.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DeviceCertToTokenFunc getDeviceCertToTokenCallback();
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
