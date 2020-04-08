/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Kinesis Video Stream level callback provider
*
*    getStreamUnderflowReportCallback();
*    getStreamLatencyPressureCallback();
*    getStreamConnectionStaleCallback();
*    getDroppedFrameReportCallback();
*    getDroppedFragmentReportCallback();
*    getStreamErrorReportCallback();
*    getStreamReadyCallback();
*    getStreamClosedCallback();
*    getStreamDataAvailableCallback();
*    getFragmentAckReceivedCallback();
*
* The optional callbacks are virtual, but there are default implementations defined for them that return nullptr,
* which will therefore use the defaults provided by the Kinesis Video SDK.
*/
class StreamCallbackProvider {
public:
    /**
     * Returns the custom data for this object to be used with the callbacks.
     *
     * @return Custom data
     */
    virtual UINT64 getCallbackCustomData() = 0;

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video metadata for the stream which is reporting underflow.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle a stream underflow
     * event.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamUnderflowReportFunc getStreamUnderflowReportCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC metadata for the stream which is reporting latency pressure.
     * - UINT64 buffer_depth: The current buffer backlog depth (duration of time) in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the latency
     * pressure.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamLatencyPressureFunc getStreamLatencyPressureCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC metadata for the stream which is reporting a dropped frame.
     * - UINT64 timestamp: Timestamp of the dropped frame in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the dropped frame.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DroppedFrameReportFunc getDroppedFrameReportCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC metadata for the stream which is reporting a stale connection.
     * - UINT64 timestamp: Duration of the last buffering ACK received in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the stale connection.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamConnectionStaleFunc getStreamConnectionStaleCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC metadata for the stream which is reporting a dropped fragment.
     * - UINT64 timestamp: Timestamp of the dropped fragment in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the dropped
     * fragment.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual DroppedFragmentReportFunc getDroppedFragmentReportCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes three arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC metadata for the stream which is reporting an errored fragment for.
     * - UINT64 timestamp: Timestamp of the errored fragment in 100ns.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the errored
     * fragment.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamErrorReportFunc getStreamErrorReportCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC handle for the stream which is ready.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the stream
     * ready event.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamReadyFunc getStreamReadyCallback() {
        return nullptr;
    };

    /**
     * The function returned by this callback takes two arguments:
     * - UINT64 custom_data: Custom handle passed by the caller.
     * - STREAM_HANDLE stream_handle: Kinesis Video PIC handle for the stream which is ready.
     *
     * Optional Callback.
     *
     * The callback returned shall take the appropriate action (decided by the implementor) to handle the
     * stream closed notification.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamClosedFunc getStreamClosedCallback() {
        return nullptr;
    };

    /**
     * Reports data available for the stream. Can be used to unblock a reading thread.
     *
     * Optional callback.
     *
     * The function returned by this callback takes the following arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 PCHAR - the name of the stream that has data available.
     * @param 4 UINT64 - The duration of content currently available in 100ns.
     * @param 5 UINT64 - The size of content in bytes currently available.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StreamDataAvailableFunc getStreamDataAvailableCallback() {
        return nullptr;
    };

    /**
     * Reports FragmentAck received for the stream. Can be used for diagnostic purposes.
     *
     * Optional callback.
     *
     * The function returned by this callback takes the following arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 PFragmentAck - the fragment ack received.
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual FragmentAckReceivedFunc getFragmentAckReceivedCallback() {
        return nullptr;
    };

    /**
     * Reports buffer duration is about to overflow
     *
     * Optional callback.
     *
     * The function returned by this callback takes the following arguments:
     *
     * @param 1 UINT64 - Custom handle passed by the caller.
     * @param 2 STREAM_HANDLE - The stream to report for.
     * @param 3 UINT64 - The remaining duration available in buffer in 100ns
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual BufferDurationOverflowPressureFunc getBufferDurationOverflowPressureCallback() {
        return nullptr;
    };

    virtual ~StreamCallbackProvider() {};
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
