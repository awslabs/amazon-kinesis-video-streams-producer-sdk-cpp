#include "ConnectionStaleStateMachine.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace std;

const uint32_t THROTTLING_PERIOD_MILLISECOND = 20000;

void ConnectionStaleStateMachine::toResetConnectionState() {
    LOG_INFO("Stream Latency State Machine move to RESET_CONNECTION_STATE");

    current_state = ConnectionStaleHandlingState::RESET_CONNECTION_STATE;
    update_timestamp();

    auto stream = data->kinesis_video_stream_map.get(stream_handle);
    if (NULL != stream) {
        stream->resetConnection();
    } else {
        LOG_ERROR("No stream found for given stream handle: " << stream_handle);
    }
}

void ConnectionStaleStateMachine::update_timestamp() {
    quiet_time = (curr_time + std::chrono::milliseconds(GRACE_PERIOD_MILLISECOND));
    back_to_normal_time = quiet_time + std::chrono::milliseconds(VERIFICATION_PERIOD_MILLISECOND);
}

void ConnectionStaleStateMachine::handleConnectionStale() {
    curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    LOG_INFO("curr_time: " << curr_time.count() << ", quiet_time: " << quiet_time.count()
                           << ", back_to_normal_time: " << back_to_normal_time.count());
    if (quiet_time < curr_time){
        switch(current_state) {
            case ConnectionStaleHandlingState::NORMAL_STATE:
                LOG_INFO("Stream Latency State Machine starting from NORMAL_STATE");
                toResetConnectionState();
                break;
            case ConnectionStaleHandlingState::RESET_CONNECTION_STATE:
                LOG_INFO("Stream Latency State Machine starting from RESET_CONNECTION_STATE");
                LOG_INFO("Stream Latency State Machine aborting");
                update_timestamp();
                break;
        }
    }
}