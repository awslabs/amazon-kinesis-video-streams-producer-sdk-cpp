// Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License").
// You may not use this file except in compliance with the License.
// A copy of the License is located at
//
//   http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.
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
#include <gst/base/gstbasesink.h>
#include "KinesisVideoProducer.h"
#include <string.h>
#include "gstkvssinkenumproperties.h"
#include <mutex>
#include <queue>
#include <condition_variable>

using namespace com::amazonaws::kinesis::video;

G_BEGIN_DECLS

#define KVS_GST_VERSION "1.0"

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

/**
 * GstKvsSink:
 *
 * The opaque #GstKvsSink data structure.
 */
struct _GstKvsSink {
    GstBaseSink		element;

    gboolean		            silent;
    gboolean		            dump;
    gboolean		            signal_handoffs;
    gchar			            *last_message;
    gint                        num_buffers;
    gint                        num_buffers_left;
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
    uint8_t                     *frame_data;
    size_t                      current_frame_data_size;
    guint                       rotation_period;
    gchar                       *log_config_path;
    GstKvsSinkFrameTimestamp    frame_timestamp;
    guint                       storage_size;
    gchar                       *credential_file_path;

    unique_ptr<Credentials> credentials_;
    shared_ptr<CustomData> data;
};

struct _GstKvsSinkClass {
    GstBaseSinkClass parent_class;

    /* signals */
    void (*handoff) (GstElement *element, GstBuffer *buf, GstPad *pad);
    void (*preroll_handoff) (GstElement *element, GstBuffer *buf, GstPad *pad);
};

GType gst_kvs_sink_get_type (void);

G_END_DECLS

class StreamLatencyStateMachine;
class ConnectionStaleStateMachine;

typedef struct _CallbackStateMachine {
    shared_ptr<StreamLatencyStateMachine> stream_latency_state_machine;
    shared_ptr<ConnectionStaleStateMachine> connection_stale_state_machine;

    _CallbackStateMachine(shared_ptr<CustomData> data, STREAM_HANDLE stream_handle);
} CallbackStateMachine;

typedef enum _StreamRecreationStatus {
    STREAM_RECREATION_STATUS_NONE,
    STREAM_RECREATION_STATUS_IN_PROGRESS
} StreamRecreationStatus;

typedef struct _CustomData {
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    ThreadSafeMap<STREAM_HANDLE, shared_ptr<KinesisVideoStream>> kinesis_video_stream_map;
    ThreadSafeMap<STREAM_HANDLE, shared_ptr<CallbackStateMachine>> callback_state_machine_map;
    string cpd = "";
    GstKvsSink *kvsSink;
    bool stream_created = false;

    queue<STREAM_HANDLE> closing_stream_handles_queue;
    StreamRecreationStatus stream_recreation_status = STREAM_RECREATION_STATUS_NONE;
    std::mutex closing_stream_handles_queue_mtx;
    std::mutex stream_recreation_status_mtx;
} CustomData;

#define ACCESS_KEY_ENV_VAR "AWS_ACCESS_KEY_ID"
#define SECRET_KEY_ENV_VAR "AWS_SECRET_ACCESS_KEY"
#define SESSION_TOKEN_ENV_VAR "AWS_SESSION_TOKEN"
#define TOKEN_EXPIRATION_ENV_VAR "AWS_TOKEN_EXPIRATION"
#define DEFAULT_REGION_ENV_VAR "AWS_DEFAULT_REGION"

#endif /* __GST_KVS_SINK_H__ */
