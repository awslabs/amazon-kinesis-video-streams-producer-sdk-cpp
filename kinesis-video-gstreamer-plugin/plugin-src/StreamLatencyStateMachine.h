#ifndef __STREAM_LATENCY_STATE_MACHINE_H__
#define __STREAM_LATENCY_STATE_MACHINE_H__

#include "gstkvssink.h"
#include <chrono>

enum class StreamLatencyHandlingState {
    NORMAL_STATE,
    RESET_CONNECTION_STATE,
    THROTTLE_PIPELINE_STATE,
    INFINITE_RETRY_STATE
};

class StreamLatencyStateMachine {
    const uint32_t THROTTLE_DELAY_NANOSECOND = 3000000;
    const uint32_t GRACE_PERIOD_MILLISECOND = 30000;
    const uint32_t VERIFICATION_PERIOD_MILLISECOND = 60000;

    std::shared_ptr<CustomData> data;
    STREAM_HANDLE stream_handle;
    StreamLatencyHandlingState current_state;
    std::chrono::milliseconds curr_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds quiet_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds back_to_normal_time = std::chrono::milliseconds(0);
    std::thread undo_throttle;
public:
    StreamLatencyStateMachine(std::shared_ptr<CustomData> data, STREAM_HANDLE stream_handle): current_state(StreamLatencyHandlingState::NORMAL_STATE),
                                                                              data(data), stream_handle(stream_handle) {}
    void handleStreamLatency();
private:
    void toResetConnectionState();
    void toThrottlePipelineState();
    void toInfiniteRetryState();
    void update_timestamp();
    void doInfiniteRetry();
};


#endif //__STREAM_LATENCY_STATE_MACHINE_H__
