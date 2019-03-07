#include "PutFrameHelper.h"
#include <Logger.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

PutFrameHelper::PutFrameHelper(
        shared_ptr<KinesisVideoStream> kinesis_video_stream,
        uint64_t mkv_timecode_scale_ns,
        uint32_t max_audio_queue_size,
        uint32_t max_video_queue_size,
        uint32_t initial_buffer_size_audio,
        uint32_t initial_buffer_size_video) :
            kinesis_video_stream(kinesis_video_stream),
            MAX_AUDIO_QUEUE_SIZE(max_audio_queue_size),
            MAX_VIDEO_QUEUE_SIZE(max_video_queue_size),
            MKV_TIMECODE_SCALE_NS(mkv_timecode_scale_ns),
            INITIAL_BUFFER_SIZE_AUDIO(initial_buffer_size_audio),
            INITIAL_BUFFER_SIZE_VIDEO(initial_buffer_size_video),
            next_available_buffer_audio(0),
            next_available_buffer_video(0),
            put_frame_status(true),
            is_processing_eofr(false) {

    for(uint32_t i = 0; i < MAX_VIDEO_QUEUE_SIZE; i++) {
        FrameDataBuffer frameDataBuffer;
        frameDataBuffer.size = INITIAL_BUFFER_SIZE_VIDEO;
        frameDataBuffer.buffer = new uint8_t[INITIAL_BUFFER_SIZE_VIDEO];
        video_data_buffers.push_back(frameDataBuffer);
    }

    for(uint32_t i = 0; i < MAX_AUDIO_QUEUE_SIZE; i++) {
        FrameDataBuffer frameDataBuffer;
        frameDataBuffer.size = INITIAL_BUFFER_SIZE_AUDIO;
        frameDataBuffer.buffer = new uint8_t[INITIAL_BUFFER_SIZE_AUDIO];
        audio_data_buffers.push_back(frameDataBuffer);
    }
}

void PutFrameHelper::putFrameMultiTrack(Frame frame, bool isVideo) {
    if (CHECK_FRAME_FLAG_END_OF_FRAGMENT(frame.flags)) {
        put_frame_status = false;
        LOG_ERROR("Client should not use putFrameMultiTrack to put EOFR frame. Use putEofr instead.");
        return;
    }

    if (isVideo) {
        if (video_frame_queue.size() == MAX_VIDEO_QUEUE_SIZE) {
            video_frame_queue.pop();
            LOG_WARN("Dropped front of video frame queue due to overflow");
        }
        video_frame_queue.push(frame);
    } else {
        if (audio_frame_queue.size() == MAX_AUDIO_QUEUE_SIZE) {
            audio_frame_queue.pop();
            LOG_WARN("Dropped front of audio frame queue due to overflow");
        }
        audio_frame_queue.push(frame);
    }

    if (video_frame_queue.empty()) {
        return;
    }

    Frame video_front = video_frame_queue.front();
    // compare using mkv timestamp
    uint64_t video_pts = video_front.presentationTs * DEFAULT_TIME_UNIT_IN_NANOS / MKV_TIMECODE_SCALE_NS;
    // release all audio with pts less than video_front' pts
    while (!audio_frame_queue.empty()) {
        Frame audio_frame = audio_frame_queue.front();
        uint64_t audio_pts = audio_frame.presentationTs * DEFAULT_TIME_UNIT_IN_NANOS / MKV_TIMECODE_SCALE_NS;
        if (audio_pts >= video_pts) {
            break;
        }

        // If user is using eofr frames, then they should not set any FRAME_FLAG_KEY_FRAME in frame flags.
        // First frame to be put after the eofr frame will have the FRAME_FLAG_KEY_FRAME flag.
        if (is_processing_eofr) {
            audio_frame.flags = (FRAME_FLAGS) (audio_frame.flags | FRAME_FLAG_KEY_FRAME);
            is_processing_eofr = false;
        }

        if (!kinesis_video_stream->putFrame(audio_frame)) {
            put_frame_status = false;
            LOG_WARN("Failed to put audio frame");
        }
        audio_frame_queue.pop();
    }

    // Sync audio frames to video frames. audio_frame_queue being not empty here means there is a audio frame whose timestamp
    // is greater than video_front.
    if (!audio_frame_queue.empty()) {

        // If user is using eofr frames, then they should not set any FRAME_FLAG_KEY_FRAME in frame flags.
        // First frame to be put after the eofr frame will have the FRAME_FLAG_KEY_FRAME flag.
        if (is_processing_eofr) {
            video_front.flags = (FRAME_FLAGS) (video_front.flags | FRAME_FLAG_KEY_FRAME);
            is_processing_eofr = false;
        }

        if (!kinesis_video_stream->putFrame(video_front)) {
            put_frame_status = false;
            LOG_WARN("Failed to put video frame. Frame flag: " << video_front.flags);
        }
        video_frame_queue.pop();
    }
}

void PutFrameHelper::flush() {
    while (!video_frame_queue.empty()) {
        Frame video_frame = video_frame_queue.front();
        uint64_t video_pts = video_frame.presentationTs * DEFAULT_TIME_UNIT_IN_NANOS / MKV_TIMECODE_SCALE_NS;

        while (!audio_frame_queue.empty()) {
            Frame audio_frame = audio_frame_queue.front();
            uint64_t audio_pts = audio_frame.presentationTs * DEFAULT_TIME_UNIT_IN_NANOS / MKV_TIMECODE_SCALE_NS;
            if (audio_pts >= video_pts) {
                break;
            }
            if (!kinesis_video_stream->putFrame(audio_frame)) {
                put_frame_status = false;
                LOG_WARN("Failed to put audio frame");
            }
            audio_frame_queue.pop();
        }
        if (!kinesis_video_stream->putFrame(video_frame)) {
            put_frame_status = false;
            LOG_WARN("Failed to put video frame. Frame flag: " << video_frame.flags);
        }
        video_frame_queue.pop();
    }

    while (!audio_frame_queue.empty()) {
        Frame audio_frame = audio_frame_queue.front();
        if (!kinesis_video_stream->putFrame(audio_frame)) {
            put_frame_status = false;
            LOG_WARN("Failed to put audio frame");
        }
        audio_frame_queue.pop();
    }
}

uint8_t *PutFrameHelper::getFrameDataBuffer(uint32_t requested_buffer_size, bool isVideo) {
    FrameDataBuffer *frameDataBuffer;

    if (isVideo) {
        if (video_frame_queue.size() == MAX_VIDEO_QUEUE_SIZE) {
            return nullptr;
        }
        frameDataBuffer = &video_data_buffers[next_available_buffer_video];
        next_available_buffer_video = (next_available_buffer_video + 1) % MAX_VIDEO_QUEUE_SIZE;
    } else {
        if (audio_frame_queue.size() == MAX_AUDIO_QUEUE_SIZE) {
            return nullptr;
        }
        frameDataBuffer = &audio_data_buffers[next_available_buffer_audio];
        next_available_buffer_audio = (next_available_buffer_audio + 1) % MAX_AUDIO_QUEUE_SIZE;
    }

    // if current buffer does not fit, free and allocate a larger one.
    if (frameDataBuffer->size < requested_buffer_size) {
        delete [] frameDataBuffer->buffer;
        frameDataBuffer->size = requested_buffer_size + requested_buffer_size / 2;
        frameDataBuffer->buffer = new uint8_t[frameDataBuffer->size];
    }

    return frameDataBuffer->buffer;
}

bool PutFrameHelper::putFrameFailed() {
    return put_frame_status;
}

void PutFrameHelper::putEofr() {
    is_processing_eofr = true;
    Frame frame = EOFR_FRAME_INITIALIZER;
    if (!kinesis_video_stream->putFrame(frame)) {
        put_frame_status = false;
        LOG_WARN("Failed to put eofr frame");
        is_processing_eofr = false;
    }
}

PutFrameHelper::~PutFrameHelper() {
    for(uint32_t i = 0; i < MAX_VIDEO_QUEUE_SIZE; i++) {
        delete [] video_data_buffers[i].buffer;
    }

    for(uint32_t i = 0; i < MAX_AUDIO_QUEUE_SIZE; i++) {
        delete [] audio_data_buffers[i].buffer;
    }
}

}
}
}
}