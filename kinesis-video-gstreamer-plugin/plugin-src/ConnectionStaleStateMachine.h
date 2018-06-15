#ifndef __CONNECTION_STALE_STATE_MACHINE_H__
#define __CONNECTION_STALE_STATE_MACHINE_H__

#include "gstkvssink.h"
#include <chrono>

enum class ConnectionStaleHandlingState {
    NORMAL_STATE,
    RESET_CONNECTION_STATE
};

class ConnectionStaleStateMachine {
    const uint32_t GRACE_PERIOD_MILLISECOND = 30000;
    const uint32_t VERIFICATION_PERIOD_MILLISECOND = 60000;

    std::shared_ptr<CustomData> data;
    STREAM_HANDLE stream_handle;
    ConnectionStaleHandlingState current_state;
    std::chrono::milliseconds quiet_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds back_to_normal_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds curr_time = std::chrono::milliseconds(0);
public:
    ConnectionStaleStateMachine(std::shared_ptr<CustomData> data, STREAM_HANDLE stream_handle):
            current_state(ConnectionStaleHandlingState::NORMAL_STATE), data(data), stream_handle(stream_handle) {}
    void handleConnectionStale();
private:
    void toResetConnectionState();
    void update_timestamp();
};


#endif //__STREAM_LATENCY_STATE_MACHINE_H__
