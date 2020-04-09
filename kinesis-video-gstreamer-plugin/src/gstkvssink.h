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
#include <map>

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


typedef struct _GstKvsSink GstKvsSink;
typedef struct _GstKvsSinkClass GstKvsSinkClass;

typedef struct _CustomData CustomData;

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
    guint                       framerate;
    guint                       avg_bandwidth_bps;
    guint                       buffer_duration_seconds;
    guint                       replay_duration_seconds;
    guint                       connection_staleness_seconds;
    gchar                       *codec_id;
    gchar                       *track_name;
    gchar                       *access_key;
    gchar                       *secret_key;
    gchar                       *aws_region;
    guint                       rotation_period;
    gchar                       *log_config_path;
    guint                       storage_size;
    gchar                       *credential_file_path;
    GstStructure                *iot_certificate;
    GstStructure                *stream_tags;
    guint64                     file_start_time;
    MKV_TRACK_INFO_TYPE         track_info_type;
    gchar                       *audio_codec_id;


    guint                       num_streams;
    guint                       num_audio_streams;
    guint                       num_video_streams;

    unique_ptr<Credentials> credentials_;
    shared_ptr<CustomData> data;
};

struct _GstKvsSinkClass {
    GstElementClass parent_class;
};

GType gst_kvs_sink_get_type (void);

G_END_DECLS

typedef struct _CustomData {

    _CustomData():
            stream_status(STATUS_SUCCESS),
            last_dts(0),
            pts_base(0),
            media_type(VIDEO_ONLY),
            first_video_frame(true),
            frame_count(0),
            first_pts(GST_CLOCK_TIME_NONE),
            producer_start_time(GST_CLOCK_TIME_NONE),
            frame_put(0),
            terminate_putFrame_monitor(false) {}
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    shared_ptr<KinesisVideoStream> kinesis_video_stream;

    map<uint64_t, string> track_cpd;
    GstKvsSink *kvsSink;
    MediaType media_type;
    bool first_video_frame;
    uint32_t frame_count;
    atomic_uint32_t frame_put;
    atomic_bool terminate_putFrame_monitor;
    std::thread worker;

    atomic_uint stream_status;

    uint64_t last_dts;
    uint64_t pts_base;
    uint64_t first_pts;
    uint64_t producer_start_time;
} CustomData;

#endif /* __GST_KVS_SINK_H__ */
