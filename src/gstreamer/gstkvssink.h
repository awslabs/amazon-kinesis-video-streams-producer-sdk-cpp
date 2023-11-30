// Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// Portions Copyright
/*
* GStreamer
* Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
* Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
* Copyright (C) 2017 <<user@hostname.org>>
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#ifndef __GST_KVS_SINK_H__
#define __GST_KVS_SINK_H__

#include <gst/gst.h>
#include <KinesisVideoProducer.h>
#include <string.h>
#include <mutex>
#include <atomic>
#include <gst/base/gstcollectpads.h>
#include <unordered_set>

using namespace com::amazonaws::kinesis::video;

G_BEGIN_DECLS

#define KVS_GST_VERSION AWS_SDK_KVS_PRODUCER_VERSION_STRING

#define GST_TYPE_KVS_SINK \
  (gst_kvs_sink_get_type())
#define GST_KVS_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KVS_SINK,GstKvsSink))
#define GST_KVS_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KVS_SINK,GstKvsSinkClass))
#define GST_IS_KVS_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KVS_SINK))
#define GST_IS_KVS_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KVS_SINK))
#define GST_KVS_SINK_CAST(obj) ((GstKvsSink *)obj)

#ifdef CPP_VERSION_STRING
#define KVSSINK_USER_AGENT_POSTFIX_VERSION CPP_VERSION_STRING
#else
#define KVSSINK_USER_AGENT_POSTFIX_VERSION "UNKNOWN"
#endif

typedef struct _GstKvsSink GstKvsSink;
typedef struct _GstKvsSinkClass GstKvsSinkClass;
typedef struct _KvsSinkCustomData KvsSinkCustomData;
typedef struct _KvsSinkMetric KvsSinkMetric;

/* all information needed for one track */
typedef struct _GstKvsSinkTrackData {
    GstCollectData collect;       /* we extend the CollectData */
    MKV_TRACK_INFO_TYPE track_type;
    GstKvsSink *kvssink;
    guint track_id;
} GstKvsSinkTrackData;

typedef enum _MediaType {
    AUDIO_VIDEO,
    VIDEO_ONLY,
    AUDIO_ONLY
} MediaType;

/**
 * GstKvsSink:
 *
 * The opaque #GstKvsSink data structure.
 */
struct _GstKvsSink {
    GstElement		element;

    GstCollectPads              *collect;

    // Stream definition
    gchar                       *stream_name;
    gchar                       *user_agent;
    guint                       retention_period_hours;
    gchar                       *kms_key_id;
    STREAMING_TYPE              streaming_type;
    gchar                       *content_type;
    guint                       max_latency_seconds;
    guint                       fragment_duration_miliseconds;
    guint                       timecode_scale_milliseconds;
    gboolean                    key_frame_fragmentation;
    gboolean                    frame_timecodes;
    gboolean                    absolute_fragment_times;
    gboolean                    fragment_acks;
    gboolean                    restart_on_error;
    gboolean                    recalculate_metrics;
    gboolean                    allow_create_stream;
    gboolean                    disable_buffer_clipping;
    guint                       framerate;
    guint                       avg_bandwidth_bps;
    guint                       buffer_duration_seconds;
    guint                       replay_duration_seconds;
    guint                       connection_staleness_seconds;
    gchar                       *codec_id;
    gchar                       *track_name;
    gchar                       *access_key;
    gchar                       *secret_key;
    gchar                       *session_token;
    gchar                       *aws_region;
    guint                       rotation_period;
    gchar                       *log_config_path;
    guint                       storage_size;
    guint                       stop_stream_timeout;
    guint                       service_connection_timeout;
    guint                       service_completion_timeout;
    gchar                       *credential_file_path;
    GstStructure                *iot_certificate;
    GstStructure                *stream_tags;
    guint64                     file_start_time;
    MKV_TRACK_INFO_TYPE         track_info_type;
    gchar                       *audio_codec_id;


    guint                       num_streams;
    guint                       num_audio_streams;
    guint                       num_video_streams;


    std::unique_ptr<Credentials> credentials_;
    std::shared_ptr<KvsSinkCustomData> data;
};

struct _GstKvsSinkClass {
    GstElementClass parent_class;
    void (*sink_fragment_ack)              (GstKvsSink *kvssink, gpointer user_data);
    void (*sink_stream_metric)             (GstKvsSink *kvssink, gpointer user_data);
    void (*sink_stream_error)              (GstKvsSink *kvssink, gpointer user_data);
};

GType gst_kvs_sink_get_type (void);

G_END_DECLS

struct _KvsSinkCustomData {

    _KvsSinkCustomData():
            stream_status(STATUS_SUCCESS),
            last_dts(0),
            pts_base(0),
            media_type(VIDEO_ONLY),
            first_video_frame(true),
            use_original_pts(false),
            get_metrics(false),
            on_first_frame(true),
            frame_count(0),
            first_pts(GST_CLOCK_TIME_NONE),
            producer_start_time(GST_CLOCK_TIME_NONE),
            streamingStopped(false) {}
    std::unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    std::shared_ptr<KinesisVideoStream> kinesis_video_stream;

    std::unordered_set<uint64_t> track_cpd_received;
    GstKvsSink *kvs_sink = nullptr;
    MediaType media_type;
    bool first_video_frame;
    bool use_original_pts;
    bool get_metrics;
    uint32_t frame_count;
    bool on_first_frame;
    std::atomic<bool> streamingStopped;
    uint64_t frame_pts;

    std::atomic_uint stream_status;

    uint64_t last_dts;
    uint64_t pts_base;
    uint64_t first_pts;
    uint64_t producer_start_time;
    guint err_signal_id = 0;
    guint ack_signal_id = 0;
    guint metric_signal_id = 0;
    uint64_t start_time;  // [nanoSeconds]
};

struct _KvsSinkMetric {
    _KvsSinkMetric():
        frame_pts(0),
        on_first_frame(true)
        {}
    KinesisVideoStreamMetrics stream_metrics = KinesisVideoStreamMetrics();
    KinesisVideoProducerMetrics client_metrics = KinesisVideoProducerMetrics();
    uint64_t frame_pts;
    bool on_first_frame;
};

#endif /* __GST_KVS_SINK_H__ */
