/** Copyright 2017 Amazon.com. All rights reserved. */

#include "OngoingPutFrameState.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

void OngoingPutFrameState::noteDataAvailable(UINT64 duration_available, UINT64 size_available) {
    LOG_DEBUG("Note data received: duration(100ns): " << duration_available << " bytes: " << size_available);
    setDataAvailable(duration_available, size_available);
}

void OngoingPutFrameState::setDataAvailable(UINT64 duration_available, UINT64 size_available) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    duration_available_ = duration_available;
    bytes_available_ = size_available;
    data_ready_.notify_one();
}

size_t OngoingPutFrameState::awaitData(size_t read_size) {
    LOG_TRACE("Awaiting data...");
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        do {
            if (0 < bytes_available_ || isEndOfStream()) {
                // Fast path - bytes are available so return immediately
                size_t ret_size = bytes_available_;
                bytes_available_ -= MIN(read_size, bytes_available_);
                return ret_size;
            }
            // Slow path - block until data arrives.
            // Wait may return without any data available due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            data_ready_.wait(lock, [this]() { return (0 < bytes_available_ || isEndOfStream()); });
        } while (true);
    }
}

void OngoingPutFrameState::notifyStateCurrent() {
    LOG_TRACE("Notify state current");
    {
        std::lock_guard<std::mutex> lock(state_current_mutex_);
        active_ = true;
        state_current_.notify_one();
    }
}

void OngoingPutFrameState::awaitCurrent() {
    LOG_TRACE("Awaiting state becoming current...");
    {
        std::unique_lock<std::mutex> lock(state_current_mutex_);
        do {
            if (active_) {
                LOG_TRACE("State is current");
                return;
            }

            // Blocking path
            // Wait may return without any data available due to a spurious wake up.
            // See: https://en.wikipedia.org/wiki/Spurious_wakeup
            state_current_.wait(lock, [this]() { return active_; });
        } while (true);
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
