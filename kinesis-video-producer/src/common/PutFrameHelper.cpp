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
            data_buffer_size(max_video_queue_size),
            put_frame_status(true) {
    data_buffer = new uint8_t[data_buffer_size];
}

void PutFrameHelper::putFrameMultiTrack(Frame frame, bool isVideo) {
    if (!kinesis_video_stream->putFrame(frame)) {
        put_frame_status = false;
        LOG_WARN("Failed to put normal frame");
    }
}

void PutFrameHelper::flush() {
    // no-op
}

uint8_t *PutFrameHelper::getFrameDataBuffer(uint32_t requested_buffer_size, bool isVideo) {
    if (requested_buffer_size > data_buffer_size) {
        delete [] data_buffer;
        data_buffer_size = requested_buffer_size + requested_buffer_size / 2;
        data_buffer = new uint8_t[data_buffer_size];
    }
    return data_buffer;
}

bool PutFrameHelper::putFrameFailed() {
    return put_frame_status;
}

void PutFrameHelper::putEofr() {
    Frame frame = EOFR_FRAME_INITIALIZER;
    if (!kinesis_video_stream->putFrame(frame)) {
        put_frame_status = false;
        LOG_WARN("Failed to put eofr frame");
    }
}

PutFrameHelper::~PutFrameHelper() {
    delete [] data_buffer;
}

}
}
}
}