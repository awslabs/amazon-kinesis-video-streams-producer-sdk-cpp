/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <list>
#include <string>
#include <tuple>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

#include "StreamTags.h"

#define DEFAULT_TRACK_ID 1

using namespace std;
using namespace std::chrono;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * StreamTrackInfo__ shadows TrackInfo struct in mkvgen/include.
 * This is introduced to provide easier struct initialization for the client
 */
typedef struct StreamTrackInfo__ {
    const uint64_t track_id;
    const string track_name;
    const string codec_id;
    const uint8_t* cpd;
    uint32_t cpd_size;
    MKV_TRACK_INFO_TYPE track_type;
} StreamTrackInfo;

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
            const uint8_t* codecPrivateData = nullptr,
            uint32_t codecPrivateDataSize = 0,
            MKV_TRACK_INFO_TYPE track_type = MKV_TRACK_INFO_TYPE_VIDEO,
            const vector<uint8_t> segment_uuid = vector<uint8_t>(),
            const uint64_t default_track_id = DEFAULT_TRACK_ID
    );

    void addTrack(const uint64_t track_id,
                  const string &track_name,
                  const string &codec_id,
                  MKV_TRACK_INFO_TYPE track_type,
                  const uint8_t* codecPrivateData = nullptr,
                  uint32_t codecPrivateDataSize = 0);

    void setFrameOrderMode(FRAME_ORDER_MODE mode);

    ~StreamDefinition();

    /**
     * @return A reference to the human readable stream name.
     */
    const string& getStreamName() const;

    /**
     * @return A the number of tracks
     */
    const size_t getTrackCount() const;

    /**
     * @return An Kinesis Video StreamInfo object
     */
    const StreamInfo& getStreamInfo();

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
     * Vector of StreamTrackInfo that contain track metadata
     */
    vector<StreamTrackInfo> track_info_;

    /**
     * The underlying object
     */
    StreamInfo stream_info_;

    /**
     * Segment UUID bytes
     */
     uint8_t segment_uuid_[MKV_SEGMENT_UUID_LEN];
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
