#include "ConnectionStaleStateMachine.h"
#include "Logger.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace std;

void ConnectionStaleStateMachine::toResetConnectionState() {
    LOG_INFO("Stream Latency State Machine move to RESET_CONNECTION_STATE");

    current_state = ConnectionStaleHandlingState::RESET_CONNECTION_STATE;
    update_timestamp();

    if (data->stream_ready.load()) {
        data->kinesis_video_stream->resetConnection();
    } else {
        LOG_ERROR("Stream not ready.");
    }
}

void ConnectionStaleStateMachine::update_timestamp() {
    quiet_time = (curr_time + std::chrono::milliseconds(GRACE_PERIOD_MILLISECOND));
    back_to_normal_time = quiet_time + std::chrono::milliseconds(VERIFICATION_PERIOD_MILLISECOND);
}

void ConnectionStaleStateMachine::handleConnectionStale() {
    curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            systemCurrentTime().time_since_epoch());
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