#include "StreamLatencyStateMachine.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace std;

const uint32_t THROTTLING_PERIOD_MILLISECOND = 20000;

void turnOffThrottle(std::shared_ptr<CustomData> data) {
    LOG_INFO("Stream Latency State Machine turn off throttling");
    this_thread::sleep_for(std::chrono::milliseconds(THROTTLING_PERIOD_MILLISECOND));
    gst_base_sink_set_throttle_time (GST_BASE_SINK (data->kvsSink), 0);
}

void StreamLatencyStateMachine::update_timestamp() {
    quiet_time = (curr_time + std::chrono::milliseconds(GRACE_PERIOD_MILLISECOND));
    back_to_normal_time = quiet_time + std::chrono::milliseconds(VERIFICATION_PERIOD_MILLISECOND);
}

void StreamLatencyStateMachine::toResetConnectionState() {
    LOG_INFO("Stream Latency State Machine move to RESET_CONNECTION_STATE");

    current_state = StreamLatencyHandlingState::RESET_CONNECTION_STATE;
    update_timestamp();
    auto stream = data->kinesis_video_stream_map.get(stream_handle);
    if (NULL != stream) {
        stream->resetConnection();
    } else {
        LOG_ERROR("No stream found for given stream handle: " << stream_handle);
    }
}

void StreamLatencyStateMachine::toThrottlePipelineState() {
    LOG_INFO("Stream Latency State Machine move to THROTTLE_PIPELINE_STATE");
    current_state = StreamLatencyHandlingState::THROTTLE_PIPELINE_STATE;
    update_timestamp();
    gst_base_sink_set_throttle_time (GST_BASE_SINK (data->kvsSink), THROTTLE_DELAY_NANOSECOND);
    if (undo_throttle.joinable()){
        undo_throttle.join();
    }
    undo_throttle = std::thread(turnOffThrottle, data);
}

void StreamLatencyStateMachine::toInfiniteRetryState() {
    LOG_INFO("Stream Latency State Machine move to ABORTING_STATE");
    current_state = StreamLatencyHandlingState::INFINITE_RETRY_STATE;
}

void StreamLatencyStateMachine::doInfiniteRetry() {
    update_timestamp();
    auto stream = data->kinesis_video_stream_map.get(stream_handle);
    if (NULL != stream) {
        stream->resetConnection();
    } else {
        LOG_ERROR("No stream found for given stream handle: " << stream_handle);
    }
}

void StreamLatencyStateMachine::handleStreamLatency() {
    curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    LOG_INFO("curr_time: " << curr_time.count() << ", quiet_time: " << quiet_time.count()
                           << ", back_to_normal_time: " << back_to_normal_time.count());
    if (curr_time > back_to_normal_time) {
        current_state = StreamLatencyHandlingState::NORMAL_STATE;
    }

    if (curr_time > quiet_time){
        switch(current_state) {
            case StreamLatencyHandlingState::NORMAL_STATE:
                LOG_INFO("Stream Latency State Machine starting from NORMAL_STATE");
                toResetConnectionState();
                break;
            case StreamLatencyHandlingState::RESET_CONNECTION_STATE:
                LOG_INFO("Stream Latency State Machine starting from RESET_CONNECTION_STATE");
                toThrottlePipelineState();
                break;
            case StreamLatencyHandlingState::THROTTLE_PIPELINE_STATE:
                LOG_INFO("Stream Latency State Machine starting from THROTTLE_PIPELINE_STATE");
                toInfiniteRetryState();
                break;
            case StreamLatencyHandlingState::INFINITE_RETRY_STATE:
                LOG_INFO("Stream Latency State Machine starting from INFINITE_RETRY_STATE");
                doInfiniteRetry();
                break;
        }
    }
}