/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <list>
#include <string>
#include <tuple>
#include <memory>
#include <functional>
#include <vector>

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
    )
            : tags_(tags),
              stream_name_(stream_name)
    {
        memset(&stream_info_, 0x00, sizeof(StreamInfo));

        assert(MAX_STREAM_NAME_LEN >= stream_name.size());
        strcpy(stream_info_.name, stream_name.c_str());

        stream_info_.version = STREAM_INFO_CURRENT_VERSION;
        stream_info_.retention = duration_cast<nanoseconds>(
                retention_period).count() / DEFAULT_TIME_UNIT_IN_NANOS;

        assert(MAX_ARN_LEN >= kms_key_id.size());
        strcpy(stream_info_.kmsKeyId, kms_key_id.c_str());

        assert(MAX_CONTENT_TYPE_LEN >= content_type.size());
        strcpy(stream_info_.streamCaps.contentType, content_type.c_str());

        stream_info_.streamCaps.streamingType = streaming_type;
        stream_info_.streamCaps.adaptive = FALSE;
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

        if (segment_uuid.empty()) {
            stream_info_.streamCaps.segmentUuid = NULL;
        } else {
            assert(MKV_SEGMENT_UUID_LEN == segment_uuid.size());
            memcpy(segment_uuid_, &segment_uuid[0], MKV_SEGMENT_UUID_LEN);
            stream_info_.streamCaps.segmentUuid = segment_uuid_;
        }

        assert(MKV_MAX_CODEC_ID_LEN >= codec_id.size());
        assert(MKV_MAX_TRACK_NAME_LEN >= track_name.size());

        track_info_.push_back(StreamTrackInfo{default_track_id, track_name, codec_id, codecPrivateData, codecPrivateDataSize, track_type});

        // Set the tags
        stream_info_.tagCount = (UINT32)tags_.count();
        stream_info_.tags = tags_.asPTag();
    }

    void addTrack(const uint64_t track_id,
                  const string &track_name,
                  const string &codec_id,
                  MKV_TRACK_INFO_TYPE track_type,
                  const uint8_t* codecPrivateData = nullptr,
                  uint32_t codecPrivateDataSize = 0) {
        track_info_.push_back(StreamTrackInfo{track_id,
                                              track_name,
                                              codec_id,
                                              codecPrivateData,
                                              codecPrivateDataSize,
                                              track_type});
    }

    ~StreamDefinition() {
        for (size_t i = 0; i < stream_info_.tagCount; ++i) {
            Tag &tag = stream_info_.tags[i];
            free(tag.name);
            free(tag.value);
        }

        free(stream_info_.tags);

        delete [] stream_info_.streamCaps.trackInfoList;
    }

    /**
     * @return A reference to the human readable stream name.
     */
    const string& getStreamName() const {
        return stream_name_;
    }

    const size_t getTrackCount() const {
        return track_info_.size();
    }

    /**
     * @return An Kinesis Video StreamInfo object
     */
    const StreamInfo& getStreamInfo() {
        stream_info_.streamCaps.trackInfoCount = static_cast<UINT32>(track_info_.size());
        stream_info_.streamCaps.trackInfoList = new TrackInfo[track_info_.size()];
        memset(stream_info_.streamCaps.trackInfoList, 0, sizeof(TrackInfo) * track_info_.size());
        for (size_t i = 0; i < track_info_.size(); ++i) {
            TrackInfo &trackInfo = stream_info_.streamCaps.trackInfoList[i];
            trackInfo.trackId = track_info_[i].track_id;
            trackInfo.trackType = track_info_[i].track_type;

            strncpy(trackInfo.trackName, track_info_[i].track_name.c_str(), MKV_MAX_TRACK_NAME_LEN + 1);
            trackInfo.trackName[MKV_MAX_TRACK_NAME_LEN] = '\0';

            strncpy(trackInfo.codecId, track_info_[i].codec_id.c_str(), MKV_MAX_CODEC_ID_LEN + 1);
            trackInfo.codecId[MKV_MAX_CODEC_ID_LEN] = '\0';

            // Set the Codec Private Data.
            // NOTE: We are not actually copying the bits
            trackInfo.codecPrivateData = const_cast<PBYTE>(track_info_[i].cpd);
            trackInfo.codecPrivateDataSize = (UINT32) track_info_[i].cpd_size;
        }

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
