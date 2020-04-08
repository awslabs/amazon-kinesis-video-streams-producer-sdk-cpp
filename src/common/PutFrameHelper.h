#ifndef __PUT_FRAME_HELPER_H__
#define __PUT_FRAME_HELPER_H__

#include "KinesisVideoProducer.h"
#include <queue>
#include <vector>

namespace {
    const uint32_t DEFAULT_MAX_AUDIO_QUEUE_SIZE = 200;
    const uint32_t DEFAULT_MAX_VIDEO_QUEUE_SIZE = 50;
    const uint64_t DEFAULT_MKV_TIMECODE_SCALE_NS = 1000000;
    const uint32_t DEFAULT_BUFFER_SIZE_AUDIO = 50 * 1024;
    const uint32_t DEFAULT_BUFFER_SIZE_VIDEO = 100 * 1024;
}

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * @Deprecated
 *
 * Since audio and video frames from gstreamer dont arrive in the order of their timestamps,
 * this PutFrameHelper class provides functionality to synchronize audio and video putFrame calls
 * so that sdk does not generate overlapping clusters. The assumption is that audio pts grows monotonically, video
 * pts also grows monotonically exception at b frames, but when inter-laced, audio and video might get out of order.
 * To ensure no cluster overlapping, before putting a video key frame, it must not put any frame whose pts is greater than
 * or equal to the video key frame's pts. Therefore we must queue up every audio frame before a video frame is enqueued.
 * if the video frame is not a key frame, everything can be put into the stream. If the video frame is a key frame, we
 * need to wait until an audio frame whose pts is greater than or equal to video key frame's pts is enqueued, then the
 * video key frame can be put into the stream.
 */
class PutFrameHelper {
    shared_ptr<KinesisVideoStream> kinesis_video_stream;
    bool put_frame_status;
    uint8_t* data_buffer;
    uint32_t data_buffer_size;
public:
    PutFrameHelper(
            shared_ptr<KinesisVideoStream> kinesis_video_stream,
            uint64_t mkv_timecode_scale_ns = DEFAULT_MKV_TIMECODE_SCALE_NS,
            uint32_t max_audio_queue_size = DEFAULT_MAX_AUDIO_QUEUE_SIZE,
            uint32_t max_video_queue_size = DEFAULT_MAX_VIDEO_QUEUE_SIZE,
            uint32_t initial_buffer_size_audio = DEFAULT_BUFFER_SIZE_AUDIO,
            uint32_t initial_buffer_size_video = DEFAULT_BUFFER_SIZE_VIDEO);

    ~PutFrameHelper();

    /*
     * application should call getFrameDataBuffer() to get a buffer to store frame data before calling putFrameMultiTrack()
     */
    uint8_t *getFrameDataBuffer(uint32_t requested_buffer_size, bool isVideo);

    /*
     * application should call putFrameMultiTrack() to pass over the frame. The frame will be put into the kinesis video
     * stream when the time is right.
     */
    void putFrameMultiTrack(Frame frame, bool isVideo);

    /*
     * application should call flush() at end of file to release any frames still in queue.
     */
    void flush();

    bool putFrameFailed();

    void putEofr();
};

}
}
}
}



#endif //__PUT_FRAME_HELPER_H__
