
#ifndef __KINESIS_VIDEO_STREAM_INFO_PROVIDER_INCLUDE_I__
#define __KINESIS_VIDEO_STREAM_INFO_PROVIDER_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Calculate defaults for the following using buffer duration
#define REPLAY_DURATION_FACTOR          0.5
#define LATENCY_PRESSURE_FACTOR         0.85
#define PRODUCER_DEFRAGMENTATION_FACTOR 0.85

#define VIDEO_ONLY_TRACK_COUNT       1
#define VIDEO_WITH_AUDIO_TRACK_COUNT 2

#define STREAM_INFO_DEFAULT_CONNECTION_STALE_DURATION (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define STREAM_INFO_DEFAULT_FRAGMENT_DURATION         (2 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define DEFAULT_AVG_BANDWIDTH                         (2 * 1024 * 1024)

#define STREAM_INFO_DEFAULT_TIMESCALE (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

// Default audio and video track names
#define DEFAULT_AUDIO_TRACK_NAME (PCHAR) "kvs_audio_track"
#define DEFAULT_VIDEO_TRACK_NAME (PCHAR) "kvs_video_track"

// Used in determining the content view items
#define DEFAULT_VIDEO_AUDIO_FRAME_RATE 120

////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////

/**
 * Sets Track Info for the Video Stream
 * @param - PTrackInfo
 * @return - STATUS code of the execution
 */
STATUS createH264VideoTrackInfo(PTrackInfo pTrackInfo);

/**
 * Sets Track Info for the Audio Stream
 * @param - PTrackInfo
 * @return - STATUS code of the execution
 */
STATUS createAacAudioTrackInfo(PTrackInfo pTrackInfo);

/**
 * Sets Stream Info for given retention, bufferDuration
 * @param - UINT64 - retention
 * @param - UINT64 - bufferDuration
 * @param - UINT32 - trackCount 1 for Video, 2 Video with Audio
 * @param - PTrackInfo
 * @param - PStreamInfo
 * @return - STATUS code of the execution
 */
STATUS setStreamInfoDefaults(STREAMING_TYPE, UINT64, UINT64, UINT32, PStreamInfo, PTrackInfo);

/**
 * Sets Stream Info for given retention, bufferDuration
 * @param - STREAMING_TYPE
 * @param - PCHAR  - stream name
 * @param - UINT64 - retention
 * @param - UINT64 - bufferDuration
 * @param - PStreamInfo
 * @return - STATUS code of the execution
 */
STATUS createVideoStreamInfo(STREAMING_TYPE, PCHAR, UINT64, UINT64, PStreamInfo*);

/**
 * Sets Stream Info for audio video stream
 * @param - STREAMING_TYPE
 * @param - PCHAR - stream name
 * @param - UINT64 - retention
 * @param - UINT64 - bufferDuration
 * @param - PStreamInfo
 * @return - STATUS code of the execution
 */
STATUS createAudioVideoStreamInfo(STREAMING_TYPE, PCHAR, UINT64, UINT64, PStreamInfo*);

#ifdef __cplusplus
}
#endif

#endif //__KINESIS_VIDEO_STREAM_INFO_PROVIDER_INCLUDE_I__
