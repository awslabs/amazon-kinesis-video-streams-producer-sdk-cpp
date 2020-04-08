/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

using namespace std;
using namespace std::chrono;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Wraps around the stream metrics class
*/
class KinesisVideoStreamMetrics {

public:

    /**
     * Default constructor
     */
    KinesisVideoStreamMetrics() {
        memset(&stream_metrics_, 0x00, sizeof(::StreamMetrics));
        stream_metrics_.version = STREAM_METRICS_CURRENT_VERSION;
    }

    /**
     * Returns the current view duration in millis
     */
    duration<uint64_t, milli> getCurrentViewDuration() const {
        return milliseconds(stream_metrics_.currentViewDuration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    /**
     * Returns the overall view duration in millis
     */
    duration<uint64_t, milli> getOverallViewDuration() const {
        return milliseconds(stream_metrics_.overallViewDuration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    }

    /**
     * Returns the current view size in bytes
     */
    uint64_t getCurrentViewSize() const {
        return stream_metrics_.currentViewSize;
    }

    /**
     * Returns the overall view size in bytes
     */
    uint64_t getOverallViewSize() const {
        return stream_metrics_.overallViewSize;
    }

    /**
     * Returns the observed frame rate in frames per second
     */
    double getCurrentFrameRate() const {
        return stream_metrics_.currentFrameRate;
    }

    /**
     * Returns the observed transfer rate in bytes per second
     */
    uint64_t getCurrentTransferRate() const {
        return stream_metrics_.currentTransferRate;
    }

    const ::StreamMetrics* getRawMetrics() const {
        return &stream_metrics_;
    }

private:
    /**
     * Underlying metrics object
     */
    ::StreamMetrics stream_metrics_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
