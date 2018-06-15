/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <list>
#include <string>
#include <tuple>
#include <memory>
#include <functional>

#include "StreamTags.h"

using namespace std;
using namespace std::chrono;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Models all metadata necessary to describe a stream
*
*/
class StreamDefinition {

public:
    /**
     * @param stream_name Human readable name of the stream. Usually: <sensor ID>.camera_<stream_tag>
     * @param transforms The type transformation to be done in the encoder. (e.g. convert I420 -> h.264, etc.)
     * @param tags The Kinesis Video PIC stream tags which should be set for this stream.
     */
    StreamDefinition(
            string stream_name,
            duration<uint64_t, ratio<3600>> retention_period,
            const map<string, string>* tags = nullptr,
            string kms_key_id = "",
            STREAMING_TYPE streaming_type = STREAMING_TYPE_REALTIME,
            string content_type = "video/h264",
            duration<uint64_t, milli> max_latency = milliseconds::zero(),
            duration<uint64_t, milli> fragment_duration = milliseconds(2000),
            duration<uint64_t, milli> timecode_scale = milliseconds(1),
            bool key_frame_fragmentation = true,
            bool frame_timecodes = true,
            bool absolute_fragment_times = true,
            bool fragment_acks = true,
            bool restart_on_error = true,
            bool recalculate_metrics = true,
            uint32_t nal_adaptation_flags = NAL_ADAPTATION_ANNEXB_NALS | NAL_ADAPTATION_ANNEXB_CPD_NALS,
            uint32_t frame_rate = 25,
            uint32_t avg_bandwidth_bps = 4 * 1024 * 1024,
            duration<uint64_t> buffer_duration = seconds(120),
            duration<uint64_t> replay_duration = seconds(40),
            duration<uint64_t> connection_staleness = seconds(30),
            string codec_id = "V_MPEG4/ISO/AVC",
            string track_name = "kinesis_video",
            const unsigned char* codecPrivateData = nullptr,
            uint32_t codecPrivateDataSize = 0
    )
            : tags_(tags),
              stream_name_(stream_name)
    {
        memset(&stream_info_, 0x00, sizeof(StreamInfo));

        assert(MAX_STREAM_NAME_LEN > stream_name.size());
        strcpy(stream_info_.name, stream_name.c_str());

        stream_info_.version = STREAM_INFO_CURRENT_VERSION;
        stream_info_.retention = duration_cast<nanoseconds>(
                retention_period).count() / DEFAULT_TIME_UNIT_IN_NANOS;

        assert(MAX_ARN_LEN > kms_key_id.size());
        strcpy(stream_info_.kmsKeyId, kms_key_id.c_str());

        assert(MAX_CONTENT_TYPE_LEN > content_type.size());
        strcpy(stream_info_.streamCaps.contentType, content_type.c_str());

        stream_info_.streamCaps.maxLatency = duration_cast<nanoseconds>(
                max_latency).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        stream_info_.streamCaps.fragmentDuration = duration_cast<nanoseconds>(
                fragment_duration).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        stream_info_.streamCaps.timecodeScale = duration_cast<nanoseconds>(
                timecode_scale).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        stream_info_.streamCaps.keyFrameFragmentation = key_frame_fragmentation;
        stream_info_.streamCaps.frameTimecodes = frame_timecodes;
        stream_info_.streamCaps.absoluteFragmentTimes = absolute_fragment_times;
        stream_info_.streamCaps.fragmentAcks = fragment_acks;
        stream_info_.streamCaps.recoverOnError = restart_on_error;
        stream_info_.streamCaps.recalculateMetrics = recalculate_metrics;
        stream_info_.streamCaps.nalAdaptationFlags = nal_adaptation_flags;
        stream_info_.streamCaps.frameRate = frame_rate;
        stream_info_.streamCaps.avgBandwidthBps = avg_bandwidth_bps;
        stream_info_.streamCaps.bufferDuration = duration_cast<nanoseconds>(
                buffer_duration).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        stream_info_.streamCaps.replayDuration = duration_cast<nanoseconds>(
                replay_duration).count() / DEFAULT_TIME_UNIT_IN_NANOS;
        stream_info_.streamCaps.connectionStalenessDuration = duration_cast<nanoseconds>(
                connection_staleness).count() / DEFAULT_TIME_UNIT_IN_NANOS;

        assert(MKV_MAX_CODEC_ID_LEN > content_type.size());
        strcpy(stream_info_.streamCaps.codecId, codec_id.c_str());

        assert(MKV_MAX_TRACK_NAME_LEN > content_type.size());
        strcpy(stream_info_.streamCaps.trackName, track_name.c_str());

        // Set the Codec Private Data.
        // NOTE: We are not actually copying the bits
        stream_info_.streamCaps.codecPrivateDataSize = codecPrivateDataSize;
        stream_info_.streamCaps.codecPrivateData = (PBYTE) codecPrivateData;

        // Set the tags
        stream_info_.tagCount = tags_.count();
        stream_info_.tags = tags_.asPTag();
    }

    ~StreamDefinition() {
        for (size_t i = 0; i < stream_info_.tagCount; ++i) {
            Tag &tag = stream_info_.tags[i];
            free(tag.name);
            free(tag.value);
        }

        free(stream_info_.tags);
    }

    /**
     * @return A reference to the human readable stream name.
     */
    const string& getStreamName() const {
        return stream_name_;
    }

    /**
     * @return An Kinesis Video StreamInfo object
     */
    const StreamInfo& getStreamInfo() {
        return stream_info_;
    }

private:
    /**
     * Human readable name of the stream. Usually: <sensor ID>.camera_<stream_tag>
     */
    string stream_name_;

    /**
     * Map of key/value pairs to be added as tags on the Kinesis Video stream
     */
    StreamTags tags_;


    /**
     * The underlying object
     */
    StreamInfo stream_info_;

};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
