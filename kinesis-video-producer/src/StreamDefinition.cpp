#include "StreamDefinition.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

StreamDefinition::StreamDefinition(
        string stream_name,
        duration <uint64_t, ratio<3600>> retention_period,
        const map<string, string> *tags,
        string kms_key_id,
        STREAMING_TYPE streaming_type,
        string content_type,
        duration <uint64_t, milli> max_latency,
        duration <uint64_t, milli> fragment_duration,
        duration <uint64_t, milli> timecode_scale,
        bool key_frame_fragmentation,
        bool frame_timecodes,
        bool absolute_fragment_times,
        bool fragment_acks,
        bool restart_on_error,
        bool recalculate_metrics,
        uint32_t nal_adaptation_flags,
        uint32_t frame_rate,
        uint32_t avg_bandwidth_bps,
        duration <uint64_t> buffer_duration,
        duration <uint64_t> replay_duration,
        duration <uint64_t> connection_staleness,
        string codec_id, string track_name,
        const uint8_t *codecPrivateData,
        uint32_t codecPrivateDataSize,
        MKV_TRACK_INFO_TYPE track_type,
        const vector<uint8_t> segment_uuid,
        const uint64_t default_track_id)
        : tags_(tags),
          stream_name_(stream_name) {
    memset(&stream_info_, 0x00, sizeof(StreamInfo));

    LOG_AND_THROW_IF(MAX_STREAM_NAME_LEN < stream_name.size(), "StreamName exceeded max length " << MAX_STREAM_NAME_LEN);
    strcpy(stream_info_.name, stream_name.c_str());

    stream_info_.version = STREAM_INFO_CURRENT_VERSION;
    stream_info_.retention = duration_cast<nanoseconds>(
            retention_period).count() / DEFAULT_TIME_UNIT_IN_NANOS;

    LOG_AND_THROW_IF(MAX_ARN_LEN < kms_key_id.size(), "KMS Key Id exceeded max length of " << MAX_ARN_LEN);
    strcpy(stream_info_.kmsKeyId, kms_key_id.c_str());

    LOG_AND_THROW_IF(MAX_CONTENT_TYPE_LEN < content_type.size(), "ContentType exceeded max length of " << MAX_CONTENT_TYPE_LEN);
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
    stream_info_.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;

    if (segment_uuid.empty()) {
        stream_info_.streamCaps.segmentUuid = NULL;
    } else {
        LOG_AND_THROW_IF(MKV_SEGMENT_UUID_LEN != segment_uuid.size(), "Invalid SegmentUUID length");
        memcpy(segment_uuid_, &segment_uuid[0], MKV_SEGMENT_UUID_LEN);
        stream_info_.streamCaps.segmentUuid = segment_uuid_;
    }

    LOG_AND_THROW_IF(MKV_MAX_CODEC_ID_LEN < codec_id.size(), "CodecId exceeded max length of " << MKV_MAX_CODEC_ID_LEN);
    LOG_AND_THROW_IF(MKV_MAX_TRACK_NAME_LEN < track_name.size(), "TrackName exceeded max length of " << MKV_MAX_TRACK_NAME_LEN);

    track_info_.push_back(StreamTrackInfo{default_track_id, track_name, codec_id, codecPrivateData, codecPrivateDataSize, track_type});

    // Set the tags
    stream_info_.tagCount = (UINT32)tags_.count();
    stream_info_.tags = tags_.asPTag();
}

void StreamDefinition::addTrack(const uint64_t track_id,
                                const string &track_name,
                                const string &codec_id,
                                MKV_TRACK_INFO_TYPE track_type,
                                const uint8_t* codecPrivateData,
                                uint32_t codecPrivateDataSize) {
    stream_info_.streamCaps.frameOrderingMode = FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE;
    track_info_.push_back(StreamTrackInfo{track_id,
                                          track_name,
                                          codec_id,
                                          codecPrivateData,
                                          codecPrivateDataSize,
                                          track_type});
}

void StreamDefinition::setFrameOrderMode(FRAME_ORDER_MODE mode) {
    stream_info_.streamCaps.frameOrderingMode = mode;
}

StreamDefinition::~StreamDefinition() {
    for (size_t i = 0; i < stream_info_.tagCount; ++i) {
        Tag &tag = stream_info_.tags[i];
        free(tag.name);
        free(tag.value);
    }

    free(stream_info_.tags);

    delete [] stream_info_.streamCaps.trackInfoList;
}

const string& StreamDefinition::getStreamName() const {
    return stream_name_;
}

const size_t StreamDefinition::getTrackCount() const {
    return track_info_.size();
}

const StreamInfo& StreamDefinition::getStreamInfo() {
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

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
