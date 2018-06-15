/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

using namespace std;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Wraps around the client metrics class
*/
class KinesisVideoProducerMetrics {

public:

    /**
     * Default constructor
     */
    KinesisVideoProducerMetrics() {
        memset(&client_metrics_, 0x00, sizeof(::ClientMetrics));
        client_metrics_.version = CLIENT_METRICS_CURRENT_VERSION;
    }

    /**
     * Returns the overall content store size in bytes
     */
    uint64_t getContentStoreSizeSize() const {
        return client_metrics_.contentStoreSize;
    }

    /**
     * Returns the content store available size in bytes
     */
    uint64_t getContentStoreAvailableSize() const {
        return client_metrics_.contentStoreAvailableSize;
    }

    /**
     * Returns the content store allocated size in bytes
     */
    uint64_t getContentStoreAllocatedSize() const {
        return client_metrics_.contentStoreAllocatedSize;
    }

    /**
     * Returns the size in bytes allocated for all content view object for all streams
     */
    uint64_t getTotalContentViewsSize() const {
        return client_metrics_.totalContentViewsSize;
    }

    /**
     * Returns the observed frame rate in frames per second for all streams
     */
    uint64_t getTotalFrameRate() const {
        return client_metrics_.totalFrameRate;
    }

    /**
     * Returns the observed transfer rate in bytes per second for all streams
     */
    uint64_t getTotalTransferRate() const {
        return client_metrics_.totalTransferRate;
    }

    const ::ClientMetrics* getRawMetrics() const {
        return &client_metrics_;
    }

private:
    /**
     * Underlying metrics object
     */
    ::ClientMetrics client_metrics_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
