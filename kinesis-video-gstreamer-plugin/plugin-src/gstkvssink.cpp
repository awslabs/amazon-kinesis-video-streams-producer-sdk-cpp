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

/**
 * SECTION:element-kvs
 *
 * GStrteamer plugin for AWS KVS service
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 *   gst-launch-1.0
 *     autovideosrc
 *   ! videoconvert
 *   ! video/x-raw,format=I420,width=1280,height=720,framerate=30/1
 *   ! vtenc_h264_hw allow-frame-reordering=FALSE realtime=TRUE max-keyframe-interval=45 bitrate=512
 *   ! h264parse
 *   ! video/x-h264,stream-format=avc, alignment=au,width=1280,height=720,framerate=30/1
 *   ! kvssink stream-name="plugin-stream" max-latency=30
 * ]|
 * </refsect2>
 */



#include "gstkvssink.h"
#include <chrono>
#include <Logger.h>
#include "KvsSinkRotatingCredentialProvider.h"
#include "KvsSinkStaticCredentialProvider.h"
#include "KvsSinkStreamCallbackProvider.h"
#include "KvsSinkClientCallbackProvider.h"
#include "KvsSinkDeviceInfoProvider.h"
#include "StreamLatencyStateMachine.h"
#include "ConnectionStaleStateMachine.h"

using namespace std;
using namespace log4cplus;

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
                                                                     GST_PAD_SINK,
                                                                     GST_PAD_ALWAYS,
                                                                     GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_kvs_sink_debug);

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

// default starting delay between reties when trying to create and start kvs stream.
#define DEFAULT_INITIAL_RETRY_DELAY_MS 400
// default number of retries when trying to create and start kvs stream.
#define DEFAULT_TOTAL_RETRY 4

/* KvsSink signals and args */
enum {
    /* FILL ME */
            SIGNAL_HANDOFF,
    SIGNAL_PREROLL_HANDOFF,
    LAST_SIGNAL
};

#define DEFAULT_SYNC FALSE

#define DEFAULT_STATE_ERROR KVS_SINK_STATE_ERROR_NONE
#define DEFAULT_SILENT TRUE
#define DEFAULT_DUMP FALSE
#define DEFAULT_SIGNAL_HANDOFFS FALSE
#define DEFAULT_LAST_MESSAGE NULL
#define DEFAULT_CAN_ACTIVATE_PUSH TRUE
#define DEFAULT_CAN_ACTIVATE_PULL FALSE
#define DEFAULT_NUM_BUFFERS (-1)

#define DEFAULT_STREAM_NAME "DEFAULT_STREAM"
#define DEFAULT_RETENTION_PERIOD_HOURS 2
#define DEFAULT_KMS_KEY_ID ""
#define DEFAULT_STREAMING_TYPE STREAMING_TYPE_REALTIME
#define DEFAULT_CONTENT_TYPE "video/h264"
#define DEFAULT_MAX_LATENCY_SECONDS 60
#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES TRUE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_STREAM_FRAMERATE 25
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
#define DEFAULT_BUFFER_DURATION_SECONDS 180
#define DEFAULT_REPLAY_DURATION_SECONDS 40
#define DEFAULT_CONNECTION_STALENESS_SECONDS 60
#define DEFAULT_CODEC_ID "V_MPEG4/ISO/AVC"
#define DEFAULT_TRACKNAME "kinesis_video"
#define DEFAULT_ACCESS_KEY "access_key"
#define DEFAULT_SECRET_KEY "secret_key"
#define DEFAULT_REGION "us-west-2"
#define DEFAULT_FRAME_DATA_SIZE_BYTE 512 * 1024
#define DEFAULT_ROTATION_PERIOD_SECONDS 2400
#define DEFAULT_LOG_FILE_PATH "./kvs_log_configuration"
#define DEFAULT_FRAME_TIMESTAMP KVS_SINK_TIMESTAMP_DEFAULT
// default size is 2250MB
#define DEFRAGMENTATION_FACTOR 1.2
#define ESTIMATED_AVG_BUFFER_SIZE_BYTE 3 * 1024 // for 720p 25 fps stream
#define DEFAULT_STORAGE_SIZE_MB \
        static_cast<UINT64>(ESTIMATED_AVG_BUFFER_SIZE_BYTE) * \
        DEFAULT_STREAM_FRAMERATE * \
        DEFAULT_BUFFER_DURATION_SECONDS * \
        DEFRAGMENTATION_FACTOR / (1024 * 1024)
#define DEFAULT_CREDENTIAL_FILE_PATH ".kvs/credential"

#define FRAME_DATA_ALLOCATION_MULTIPLIER 1.5

enum {
    PROP_0,
    PROP_SILENT,
    PROP_DUMP,
    PROP_SIGNAL_HANDOFFS,
    PROP_LAST_MESSAGE,
    PROP_CAN_ACTIVATE_PUSH,
    PROP_CAN_ACTIVATE_PULL,
    PROP_NUM_BUFFERS,
    PROP_LAST,
    PROP_STREAM_NAME,
    PROP_RETENTION_PERIOD,
    PROP_STREAMING_TYPE,
    PROP_CONTENT_TYPE,
    PROP_MAX_LATENCY,
    PROP_FRAGMENT_DURATION,
    PROP_TIMECODE_SCALE,
    PROP_KEY_FRAME_FRAGMENTATION,
    PROP_FRAME_TIMECODES,
    PROP_ABSOLUTE_FRAGMENT_TIMES,
    PROP_FRAGMENT_ACKS,
    PROP_RESTART_ON_ERROR,
    PROP_RECALCULATE_METRICS,
    PROP_NAL_ADAPTATION_FLAGS,
    PROP_FRAMERATE,
    PROP_AVG_BANDWIDTH_BPS,
    PROP_BUFFER_DURATION,
    PROP_REPLAY_DURATION,
    PROP_CONNECTION_STALENESS,
    PROP_CODEC_ID,
    PROP_TRACK_NAME,
    PROP_ACCESS_KEY,
    PROP_SECRET_KEY,
    PROP_AWS_REGION,
    PROP_ROTATION_PERIOD,
    PROP_LOG_CONFIG_PATH,
    PROP_FRAME_TIMESTAMP,
    PROP_STORAGE_SIZE,
    PROP_CREDENTIAL_FILE_PATH,
};

#define GST_TYPE_KVS_SINK_FRAME_TIMESTAMP_TYPE (gst_kvs_sink_frame_timestamp_type_get_type())
static GType
gst_kvs_sink_frame_timestamp_type_get_type (void)
{
    static GType kvssink_frame_timestamp_type_type = 0;
    static const GEnumValue kvssink_frame_timestamp_type[] = {
            {KVS_SINK_TIMESTAMP_PTS_ONLY, "Set dts equal to pts for every frame sent to kvs", "pts-only"},
            {KVS_SINK_TIMESTAMP_DTS_ONLY, "Set pts equal to dts for every frame sent to kvs", "dts-only"},
            {KVS_SINK_TIMESTAMP_DEFAULT, "Try to use both pts and dts. If one is not available, then use the other", "default-timestamp"},
            {0, NULL, NULL},
    };

    if (!kvssink_frame_timestamp_type_type) {
        kvssink_frame_timestamp_type_type =
                g_enum_register_static ("GstKvsSinkFrameTimestampType", kvssink_frame_timestamp_type);
    }
    return kvssink_frame_timestamp_type_type;
}

#define GST_TYPE_KVS_SINK_STREAMING_TYPE (gst_kvs_sink_streaming_type_get_type())
static GType
gst_kvs_sink_streaming_type_get_type (void)
{
    static GType kvssink_streaming_type_type = 0;
    static const GEnumValue kvssink_streaming_type[] = {
            {STREAMING_TYPE_REALTIME, "streaming type realtime", "streaming type realtime"},
            {STREAMING_TYPE_NEAR_REALTIME, "streaming type near realtime", "streaming type near realtime"},
            {STREAMING_TYPE_OFFLINE, "streaming type offline", "streaming type offline"},
            {0, NULL, NULL},
    };

    if (!kvssink_streaming_type_type) {
        kvssink_streaming_type_type =
                g_enum_register_static ("GstKvsSinkStreamingType", kvssink_streaming_type);
    }
    return kvssink_streaming_type_type;
}

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_kvs_sink_debug, "kvssink", 0, "KVS sink plug-in");

#define gst_kvs_sink_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE (GstKvsSink, gst_kvs_sink, GST_TYPE_BASE_SINK, _do_init);

static void gst_kvs_sink_set_property(GObject *object, guint prop_id,
                                         const GValue *value, GParamSpec *pspec);

static void gst_kvs_sink_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec);

static void gst_kvs_sink_finalize(GObject *obj);

static GstStateChangeReturn gst_kvs_sink_change_state(GstElement *element,
                                                         GstStateChange transition);

static GstFlowReturn gst_kvs_sink_preroll(GstBaseSink *bsink,
                                             GstBuffer *buffer);

static GstFlowReturn gst_kvs_sink_render(GstBaseSink *bsink,
                                            GstBuffer *buffer);

static gboolean gst_kvs_sink_event(GstBaseSink *bsink, GstEvent *event);
static gboolean gst_kvs_sink_query(GstBaseSink *bsink, GstQuery *query);

static gboolean gst_kvs_sink_start (GstBaseSink * bsink);
static gboolean gst_kvs_sink_stop (GstBaseSink * bsink);

static GstFlowReturn gst_kvs_sink_prepare(GstBaseSink *bsink,
                                          GstBuffer *buffer);

static guint gst_kvs_sink_signals[LAST_SIGNAL] = {0};

static GParamSpec *pspec_last_message = NULL;

CallbackStateMachine::_CallbackStateMachine(shared_ptr<CustomData> data, STREAM_HANDLE stream_handle) {
    stream_latency_state_machine = make_shared<StreamLatencyStateMachine>(data, stream_handle);
    connection_stale_state_machine = make_shared<ConnectionStaleStateMachine>(data, stream_handle);
}


void kinesis_video_producer_init(GstKvsSink *sink) {
    auto data = sink->data;
    unique_ptr<DeviceInfoProvider> device_info_provider = make_unique<KvsSinkDeviceInfoProvider>(data);
    unique_ptr<ClientCallbackProvider> client_callback_provider = make_unique<KvsSinkClientCallbackProvider>();
    unique_ptr<StreamCallbackProvider> stream_callback_provider = make_unique<KvsSinkStreamCallbackProvider>(data);

    char const *access_key;
    char const *secret_key;
    char const *default_region;
    string access_key_str;
    string secret_key_str;
    string region_str;
    bool credential_is_static = true;

    if (0 == strcmp(sink->access_key, DEFAULT_ACCESS_KEY)) { // Check if static credential is available in plugin property.
        if (nullptr == (access_key = getenv(ACCESS_KEY_ENV_VAR))
            || nullptr == (secret_key = getenv(SECRET_KEY_ENV_VAR))) { // Check if static credential is available in env var.
            credential_is_static = false; // No static credential available.
            access_key_str = "";
            secret_key_str = "";
        } else {
            access_key_str = string(access_key);
            secret_key_str = string(secret_key);
        }

    } else {
        access_key_str = string(sink->access_key);
        secret_key_str = string(sink->secret_key);
    }

    if (nullptr == (default_region = getenv(DEFAULT_REGION_ENV_VAR))) {
        region_str = string(sink->aws_region);
    } else {
        region_str = string(default_region); // Use env var if both property and env var are available.
    }

    unique_ptr<CredentialProvider> credential_provider;
    if (credential_is_static) {
        sink->credentials_ = make_unique<Credentials>(access_key_str,
                                                      secret_key_str,
                                                      "",
                                                      std::chrono::seconds(DEFAULT_ROTATION_PERIOD_SECONDS));
        credential_provider = make_unique<KvsSinkStaticCredentialProvider>(*sink->credentials_, sink->rotation_period);
    } else {
        credential_provider = make_unique<KvsSinkRotatingCredentialProvider>(data);
    }

    data->kinesis_video_producer = KinesisVideoProducer::createSync(move(device_info_provider),
                                                                    move(client_callback_provider),
                                                                    move(stream_callback_provider),
                                                                    move(credential_provider),
                                                                    region_str);
}

void create_kinesis_video_stream(GstKvsSink *sink) {
    auto data = sink->data;
    auto stream_definition = make_unique<StreamDefinition>(sink->stream_name,
                                                           hours(sink->retention_period_hours),
                                                           nullptr,
                                                           sink->kms_key_id,
                                                           sink->streaming_type,
                                                           sink->content_type,
                                                           duration_cast<milliseconds> (seconds(sink->max_latency_seconds)),
                                                           milliseconds(sink->fragment_duration_miliseconds),
                                                           milliseconds(sink->timecode_scale_milliseconds),
                                                           sink->key_frame_fragmentation,//Construct a fragment at each key frame
                                                           sink->frame_timecodes,//Use provided frame timecode
                                                           sink->absolute_fragment_times,//Relative timecode
                                                           sink->fragment_acks,//Ack on fragment is enabled
                                                           sink->restart_on_error,//SDK will restart when error happens
                                                           sink->recalculate_metrics,//recalculate_metrics
                                                           0,
                                                           sink->framerate,
                                                           sink->avg_bandwidth_bps,
                                                           seconds(sink->buffer_duration_seconds),
                                                           seconds(sink->replay_duration_seconds),
                                                           seconds(sink->connection_staleness_seconds),
                                                           sink->codec_id,
                                                           sink->track_name,
                                                           nullptr,
                                                           0);
    auto kinesis_video_stream = data->kinesis_video_producer->createStreamSync(move(stream_definition));
    auto stream_handle = *(kinesis_video_stream->getStreamHandle());
    data->kinesis_video_stream_map.put(stream_handle, kinesis_video_stream);
    shared_ptr<CallbackStateMachine> callbackStateMachine = make_shared<CallbackStateMachine>(data, stream_handle);
    data->callback_state_machine_map.put(stream_handle, callbackStateMachine);
    cout << "Stream is ready" << endl;
}

bool kinesis_video_stream_init(GstKvsSink *sink, string &err_msg) {
    bool ret = true;
    int retry_count = DEFAULT_TOTAL_RETRY;
    int retry_delay = DEFAULT_INITIAL_RETRY_DELAY_MS;
    bool do_retry = true;

    while(do_retry) {
        try {
            LOG_INFO("try creating stream");
            create_kinesis_video_stream(sink);
            break;
        } catch (runtime_error &err) {
            if (--retry_count == 0) {
                ostringstream oss;
                oss << "Failed to create stream. Error: " << err.what();
                err_msg = oss.str();
                ret = false;
                do_retry = false;
            } else {
                this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
                // double the delay period
                retry_delay = retry_delay << 1;
            }
        }
    }

    return ret;
}

bool kinesis_video_stream_start(GstKvsSink *sink, string &err_msg) {
    auto data = sink->data;
    bool ret = true;
    int retry_count = DEFAULT_TOTAL_RETRY;
    int retry_delay = DEFAULT_INITIAL_RETRY_DELAY_MS;
    for (auto stream : data->kinesis_video_stream_map.getMap()) {
        bool do_retry = true;
        while(do_retry) {
            LOG_INFO("try starting stream. Stream handle: " << stream.first);
            if (stream.second->start(data->cpd)) {
                break;
            } else {
                if (--retry_count == 0) {
                    ostringstream oss;
                    oss << "Failed to start kvs stream. Stream handle: " << stream.first;
                    err_msg = oss.str();
                    ret = false;
                    do_retry = false;
                } else {
                    this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
                    // double the delay period
                    retry_delay = retry_delay << 1;
                }
            }
        }
    }

    return ret;
}

static void
gst_kvs_sink_class_init(GstKvsSinkClass *klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstBaseSinkClass *gstbase_sink_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);
    gstbase_sink_class = GST_BASE_SINK_CLASS (klass);

    gobject_class->set_property = gst_kvs_sink_set_property;
    gobject_class->get_property = gst_kvs_sink_get_property;
    gobject_class->finalize = gst_kvs_sink_finalize;

    pspec_last_message = g_param_spec_string("last-message", "Last Message",
                                             "The message describing current status", DEFAULT_LAST_MESSAGE,
                                             (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(gobject_class, PROP_LAST_MESSAGE,
                                    pspec_last_message);
    g_object_class_install_property(gobject_class, PROP_SIGNAL_HANDOFFS,
                                    g_param_spec_boolean("signal-handoffs", "Signal handoffs",
                                                         "Send a signal before unreffing the buffer",
                                                         DEFAULT_SIGNAL_HANDOFFS,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent",
                                                         "Don't produce last_message events", DEFAULT_SILENT,
                                                         (GParamFlags) (G_PARAM_READWRITE | GST_PARAM_MUTABLE_PLAYING |
                                                                        G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_DUMP,
                                    g_param_spec_boolean("dump", "Dump", "Dump buffer contents to stdout",
                                                         DEFAULT_DUMP,
                                                         (GParamFlags) (G_PARAM_READWRITE | GST_PARAM_MUTABLE_PLAYING |
                                                                        G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_CAN_ACTIVATE_PUSH,
                                    g_param_spec_boolean("can-activate-push", "Can activate push",
                                                         "Can activate in push mode", DEFAULT_CAN_ACTIVATE_PUSH,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_CAN_ACTIVATE_PULL,
                                    g_param_spec_boolean("can-activate-pull", "Can activate pull",
                                                         "Can activate in pull mode", DEFAULT_CAN_ACTIVATE_PULL,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_NUM_BUFFERS,
                                    g_param_spec_int("num-buffers", "num-buffers",
                                                     "Number of buffers to accept going EOS", -1, G_MAXINT,
                                                     DEFAULT_NUM_BUFFERS,
                                                     (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STREAM_NAME,
                                     g_param_spec_string ("stream-name", "Stream Name",
                                                          "Name of the destination stream", DEFAULT_STREAM_NAME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_RETENTION_PERIOD,
                                     g_param_spec_uint ("retention-period", "Retention Period",
                                                       "Length of time stream is preserved. Unit: hours", 0, G_MAXUINT, DEFAULT_RETENTION_PERIOD_HOURS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STREAMING_TYPE,
                                     g_param_spec_enum ("streaming-type", "Streaming Type",
                                                        "Streaming type", GST_TYPE_KVS_SINK_STREAMING_TYPE, DEFAULT_STREAMING_TYPE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
                                     g_param_spec_string ("content-type", "Content Type",
                                                          "content type", DEFAULT_CONTENT_TYPE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_MAX_LATENCY,
                                     g_param_spec_uint ("max-latency", "Max Latency",
                                                       "Max Latency. Unit: seconds", 0, G_MAXUINT, DEFAULT_MAX_LATENCY_SECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FRAGMENT_DURATION,
                                     g_param_spec_uint ("fragment-duration", "Fragment Duration",
                                                       "Fragment Duration. Unit: miliseconds", 0, G_MAXUINT, DEFAULT_FRAGMENT_DURATION_MILLISECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_TIMECODE_SCALE,
                                     g_param_spec_uint ("timecode-scale", "Timecode Scale",
                                                       "Timecode Scale. Unit: milliseconds", 0, G_MAXUINT, DEFAULT_TIMECODE_SCALE_MILLISECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_KEY_FRAME_FRAGMENTATION,
                                     g_param_spec_boolean ("key-frame-fragmentation", "Do key frame fragmentation",
                                                           "If true, generate new fragment on each keyframe, otherwise generate new fragment on first keyframe after fragment-duration has passed.", DEFAULT_KEY_FRAME_FRAGMENTATION,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FRAME_TIMECODES,
                                     g_param_spec_boolean ("frame-timecodes", "Do frame timecodes",
                                                           "Do frame timecodes", DEFAULT_FRAME_TIMECODES,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_ABSOLUTE_FRAGMENT_TIMES,
                                     g_param_spec_boolean ("absolute-fragment-times", "Use absolute fragment time",
                                                           "Use absolute fragment time", DEFAULT_ABSOLUTE_FRAGMENT_TIMES,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FRAGMENT_ACKS,
                                     g_param_spec_boolean ("fragment-acks", "Do fragment acks",
                                                           "Do fragment acks", DEFAULT_FRAGMENT_ACKS,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_RESTART_ON_ERROR,
                                     g_param_spec_boolean ("restart-on-error", "Do restart on error",
                                                           "Do restart on error", DEFAULT_RESTART_ON_ERROR,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_RECALCULATE_METRICS,
                                     g_param_spec_boolean ("recalculate-metrics", "Do recalculate metrics",
                                                           "Do recalculate metrics", DEFAULT_RECALCULATE_METRICS,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FRAMERATE,
                                     g_param_spec_uint ("framerate", "Framerate",
                                                        "Framerate", 0, G_MAXUINT, DEFAULT_STREAM_FRAMERATE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_AVG_BANDWIDTH_BPS,
                                     g_param_spec_uint ("avg-bandwidth-bps", "Average bandwidth bps",
                                                       "Average bandwidth bps", 0, G_MAXUINT, DEFAULT_AVG_BANDWIDTH_BPS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_BUFFER_DURATION,
                                     g_param_spec_uint ("buffer-duration", "Buffer duration",
                                                       "Buffer duration. Unit: seconds", 0, G_MAXUINT, DEFAULT_BUFFER_DURATION_SECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_REPLAY_DURATION,
                                     g_param_spec_uint ("replay-duration", "Replay duration",
                                                       "Replay duration. Unit: seconds", 0, G_MAXUINT, DEFAULT_REPLAY_DURATION_SECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CONNECTION_STALENESS,
                                     g_param_spec_uint ("connection-staleness", "Connection staleness",
                                                       "Connection staleness. Unit: seconds", 0, G_MAXUINT, DEFAULT_CONNECTION_STALENESS_SECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CODEC_ID,
                                     g_param_spec_string ("codec-id", "Codec ID",
                                                          "Codec ID", DEFAULT_CODEC_ID, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_TRACK_NAME,
                                     g_param_spec_string ("track-name", "Track name",
                                                          "Track name", DEFAULT_TRACKNAME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_ACCESS_KEY,
                                     g_param_spec_string ("access-key", "Access Key",
                                                          "AWS Access Key", DEFAULT_ACCESS_KEY, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SECRET_KEY,
                                     g_param_spec_string ("secret-key", "Secret Key",
                                                          "AWS Secret Key", DEFAULT_SECRET_KEY, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_AWS_REGION,
                                     g_param_spec_string ("aws-region", "AWS Region",
                                                          "AWS Region", DEFAULT_REGION, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_ROTATION_PERIOD,
                                     g_param_spec_uint ("rotation-period", "Rotation Period",
                                                        "Rotation Period. Unit: seconds", 0, G_MAXUINT, DEFAULT_ROTATION_PERIOD_SECONDS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_LOG_CONFIG_PATH,
                                     g_param_spec_string ("log-config", "Log Configuration",
                                                        "Log Configuration Path", DEFAULT_LOG_FILE_PATH, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FRAME_TIMESTAMP,
                                     g_param_spec_enum ("frame-timestamp", "Frame Timestamp",
                                                        "Frame Timestamp", GST_TYPE_KVS_SINK_FRAME_TIMESTAMP_TYPE, DEFAULT_FRAME_TIMESTAMP, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STORAGE_SIZE,
                                     g_param_spec_uint ("storage-size", "Storage Size",
                                                        "Storage Size. Unit: MB", 0, G_MAXUINT, DEFAULT_STORAGE_SIZE_MB, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CREDENTIAL_FILE_PATH,
                                     g_param_spec_string ("credential-path", "Credential File Path",
                                                          "Credential File Path", DEFAULT_CREDENTIAL_FILE_PATH, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));


    /**
     * GstKvsSink::handoff:
     * @kvssink: the kvssink instance
     * @buffer: the buffer that just has been received
     * @pad: the pad that received it
     *
     * This signal gets emitted before unreffing the buffer.
     */
    gst_kvs_sink_signals[SIGNAL_HANDOFF] =
            g_signal_new("handoff", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                         G_STRUCT_OFFSET (GstKvsSinkClass, handoff), NULL, NULL,
                         g_cclosure_marshal_generic, G_TYPE_NONE, 2,
                         GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE, GST_TYPE_PAD);

    /**
     * GstKvsSink::preroll-handoff:
     * @kvssink: the kvssink instance
     * @buffer: the buffer that just has been received
     * @pad: the pad that received it
     *
     * This signal gets emitted before unreffing the buffer.
     */
    gst_kvs_sink_signals[SIGNAL_PREROLL_HANDOFF] =
            g_signal_new("preroll-handoff", G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstKvsSinkClass, preroll_handoff),
                         NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 2,
                         GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE, GST_TYPE_PAD);

    gst_element_class_set_static_metadata(gstelement_class,
                                          "KVS Sink",
                                          "Sink/Video/Network",
                                          "GStreamer AWS KVS plugin",
                                          "AWS KVS <kinesis-video-support@amazon.com>");

    //gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_template));

    gstelement_class->change_state =
            GST_DEBUG_FUNCPTR (gst_kvs_sink_change_state);

    gstbase_sink_class->event = GST_DEBUG_FUNCPTR (gst_kvs_sink_event);
    gstbase_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_kvs_sink_preroll);
    gstbase_sink_class->render = GST_DEBUG_FUNCPTR (gst_kvs_sink_render);
    gstbase_sink_class->query = GST_DEBUG_FUNCPTR (gst_kvs_sink_query);
    gstbase_sink_class->start = GST_DEBUG_FUNCPTR (gst_kvs_sink_start);
    gstbase_sink_class->stop = GST_DEBUG_FUNCPTR (gst_kvs_sink_stop);
    gstbase_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_kvs_sink_prepare);
}

static void  gst_kvs_sink_init(GstKvsSink *kvssink) {

    kvssink->silent = DEFAULT_SILENT;
    kvssink->dump = DEFAULT_DUMP;
    kvssink->last_message = g_strdup(DEFAULT_LAST_MESSAGE);
    kvssink->signal_handoffs = DEFAULT_SIGNAL_HANDOFFS;
    kvssink->num_buffers = DEFAULT_NUM_BUFFERS;

    kvssink->stream_name = g_strdup (DEFAULT_STREAM_NAME);
    kvssink->retention_period_hours = DEFAULT_RETENTION_PERIOD_HOURS;
    kvssink->kms_key_id = g_strdup (DEFAULT_KMS_KEY_ID);
    kvssink->streaming_type = DEFAULT_STREAMING_TYPE;
    kvssink->content_type = g_strdup (DEFAULT_CONTENT_TYPE);
    kvssink->max_latency_seconds = DEFAULT_MAX_LATENCY_SECONDS;
    kvssink->fragment_duration_miliseconds = DEFAULT_FRAGMENT_DURATION_MILLISECONDS;
    kvssink->timecode_scale_milliseconds = DEFAULT_TIMECODE_SCALE_MILLISECONDS;
    kvssink->key_frame_fragmentation = DEFAULT_KEY_FRAME_FRAGMENTATION;
    kvssink->frame_timecodes = DEFAULT_FRAME_TIMECODES;
    kvssink->absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;
    kvssink->fragment_acks = DEFAULT_FRAGMENT_ACKS;
    kvssink->restart_on_error = DEFAULT_RESTART_ON_ERROR;
    kvssink->recalculate_metrics = DEFAULT_RECALCULATE_METRICS;
    kvssink->framerate = DEFAULT_STREAM_FRAMERATE;
    kvssink->avg_bandwidth_bps = DEFAULT_AVG_BANDWIDTH_BPS;
    kvssink->buffer_duration_seconds = DEFAULT_BUFFER_DURATION_SECONDS;
    kvssink->replay_duration_seconds = DEFAULT_REPLAY_DURATION_SECONDS;
    kvssink->connection_staleness_seconds = DEFAULT_CONNECTION_STALENESS_SECONDS;
    kvssink->codec_id = g_strdup (DEFAULT_CODEC_ID);
    kvssink->track_name = g_strdup (DEFAULT_TRACKNAME);
    kvssink->access_key = g_strdup (DEFAULT_ACCESS_KEY);
    kvssink->secret_key = g_strdup (DEFAULT_SECRET_KEY);
    kvssink->aws_region = g_strdup (DEFAULT_REGION);
    kvssink->frame_data = new uint8_t[DEFAULT_FRAME_DATA_SIZE_BYTE];
    kvssink->current_frame_data_size = DEFAULT_FRAME_DATA_SIZE_BYTE;
    kvssink->rotation_period = DEFAULT_ROTATION_PERIOD_SECONDS;
    kvssink->log_config_path = g_strdup (DEFAULT_LOG_FILE_PATH);
    kvssink->frame_timestamp = DEFAULT_FRAME_TIMESTAMP;
    kvssink->storage_size = DEFAULT_STORAGE_SIZE_MB;
    kvssink->credential_file_path = g_strdup (DEFAULT_CREDENTIAL_FILE_PATH);

    gst_base_sink_set_sync(GST_BASE_SINK (kvssink), DEFAULT_SYNC);

    kvssink->data = make_shared<CustomData>();
    LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");
    LOG_CONFIGURE_STDOUT("DEBUG")

}

static void
gst_kvs_sink_finalize(GObject *object) {
    GstKvsSink *sink = GST_KVS_SINK (object);

    g_free(sink->stream_name);
    g_free(sink->content_type);
    g_free(sink->codec_id);
    g_free(sink->track_name);
    g_free(sink->secret_key);
    g_free(sink->access_key);
    delete [] sink->frame_data;
    G_OBJECT_CLASS (parent_class)->finalize(object);
}

static void gst_kvs_sink_set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec) {
    GstKvsSink *sink;

    sink = GST_KVS_SINK (object);

    switch (prop_id) {
        case PROP_SILENT:
            sink->silent = g_value_get_boolean(value);
            break;
        case PROP_DUMP:
            sink->dump = g_value_get_boolean(value);
            break;
        case PROP_SIGNAL_HANDOFFS:
            sink->signal_handoffs = g_value_get_boolean(value);
            break;
        case PROP_CAN_ACTIVATE_PUSH:
            GST_BASE_SINK (sink)->can_activate_push = g_value_get_boolean(value);
            break;
        case PROP_CAN_ACTIVATE_PULL:
            GST_BASE_SINK (sink)->can_activate_pull = g_value_get_boolean(value);
            break;
        case PROP_NUM_BUFFERS:
            sink->num_buffers = g_value_get_int(value);
            break;
        case PROP_STREAM_NAME:
            g_free(sink->stream_name);
            sink->stream_name = g_strdup (g_value_get_string (value));
            break;
        case PROP_RETENTION_PERIOD:
            sink->retention_period_hours = g_value_get_uint (value);
            break;
        case PROP_STREAMING_TYPE:
            sink->streaming_type = (STREAMING_TYPE) g_value_get_enum (value);
            break;
        case PROP_CONTENT_TYPE:
            g_free(sink->content_type);
            sink->content_type = g_strdup (g_value_get_string (value));
            break;
        case PROP_MAX_LATENCY:
            sink->max_latency_seconds = g_value_get_uint (value);
            break;
        case PROP_FRAGMENT_DURATION:
            sink->fragment_duration_miliseconds = g_value_get_uint (value);
            break;
        case PROP_TIMECODE_SCALE:
            sink->timecode_scale_milliseconds = g_value_get_uint (value);
            break;
        case PROP_KEY_FRAME_FRAGMENTATION:
            sink->key_frame_fragmentation = g_value_get_boolean (value);
            break;
        case PROP_FRAME_TIMECODES:
            sink->frame_timecodes = g_value_get_boolean (value);
            break;
        case PROP_ABSOLUTE_FRAGMENT_TIMES:
            sink->absolute_fragment_times = g_value_get_boolean (value);
            break;
        case PROP_FRAGMENT_ACKS:
            sink->fragment_acks = g_value_get_boolean (value);
            break;
        case PROP_RESTART_ON_ERROR:
            sink->restart_on_error = g_value_get_boolean (value);
            break;
        case PROP_RECALCULATE_METRICS:
            sink->recalculate_metrics = g_value_get_boolean (value);
            break;
        case PROP_AVG_BANDWIDTH_BPS:
            sink->avg_bandwidth_bps = g_value_get_uint (value);
            break;
        case PROP_BUFFER_DURATION:
            sink->buffer_duration_seconds = g_value_get_uint (value);
            break;
        case PROP_REPLAY_DURATION:
            sink->replay_duration_seconds = g_value_get_uint (value);
            break;
        case PROP_CONNECTION_STALENESS:
            sink->connection_staleness_seconds = g_value_get_uint (value);
            break;
        case PROP_CODEC_ID:
            g_free(sink->codec_id);
            sink->codec_id = g_strdup (g_value_get_string (value));
            break;
        case PROP_TRACK_NAME:
            g_free(sink->track_name);
            sink->track_name = g_strdup (g_value_get_string (value));
            break;
        case PROP_ACCESS_KEY:
            g_free(sink->access_key);
            sink->access_key = g_strdup (g_value_get_string (value));
            break;
        case PROP_SECRET_KEY:
            g_free(sink->secret_key);
            sink->secret_key = g_strdup (g_value_get_string (value));
            break;
        case PROP_AWS_REGION:
            g_free(sink->aws_region);
            sink->aws_region = g_strdup (g_value_get_string (value));
            break;
        case PROP_ROTATION_PERIOD:
            sink->rotation_period = g_value_get_uint (value);
            break;
        case PROP_LOG_CONFIG_PATH:
            sink->log_config_path = g_strdup (g_value_get_string (value));
            break;
        case PROP_FRAMERATE:
            sink->framerate = g_value_get_uint (value);
            break;
        case PROP_FRAME_TIMESTAMP:
            sink->frame_timestamp = (GstKvsSinkFrameTimestamp) g_value_get_enum (value);
            break;
        case PROP_STORAGE_SIZE:
            sink->storage_size = g_value_get_uint (value);
            break;
        case PROP_CREDENTIAL_FILE_PATH:
            sink->credential_file_path = g_strdup (g_value_get_string (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void gst_kvs_sink_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec) {
    GstKvsSink *sink;

    sink = GST_KVS_SINK (object);

    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean(value, sink->silent);
            break;
        case PROP_DUMP:
            g_value_set_boolean(value, sink->dump);
            break;
        case PROP_SIGNAL_HANDOFFS:
            g_value_set_boolean(value, sink->signal_handoffs);
            break;
        case PROP_LAST_MESSAGE:
            GST_OBJECT_LOCK (sink);
            g_value_set_string(value, sink->last_message);
            GST_OBJECT_UNLOCK (sink);
            break;
        case PROP_CAN_ACTIVATE_PUSH:
            g_value_set_boolean(value, GST_BASE_SINK (sink)->can_activate_push);
            break;
        case PROP_CAN_ACTIVATE_PULL:
            g_value_set_boolean(value, GST_BASE_SINK (sink)->can_activate_pull);
            break;
        case PROP_NUM_BUFFERS:
            g_value_set_uint(value, sink->num_buffers);
            break;
        case PROP_STREAM_NAME:
            g_value_set_string (value, sink->stream_name);
            break;
        case PROP_RETENTION_PERIOD:
            g_value_set_uint (value, sink->retention_period_hours);
            break;
        case PROP_STREAMING_TYPE:
            g_value_set_enum (value, sink->streaming_type);
            break;
        case PROP_CONTENT_TYPE:
            g_value_set_string (value, sink->content_type);
            break;
        case PROP_MAX_LATENCY:
            g_value_set_uint (value, sink->max_latency_seconds);
            break;
        case PROP_FRAGMENT_DURATION:
            g_value_set_uint (value, sink->fragment_duration_miliseconds);
            break;
        case PROP_TIMECODE_SCALE:
            g_value_set_uint (value, sink->timecode_scale_milliseconds);
            break;
        case PROP_KEY_FRAME_FRAGMENTATION:
            g_value_set_boolean (value, sink->key_frame_fragmentation);
            break;
        case PROP_FRAME_TIMECODES:
            g_value_set_boolean (value, sink->frame_timecodes);
            break;
        case PROP_ABSOLUTE_FRAGMENT_TIMES:
            g_value_set_boolean (value, sink->absolute_fragment_times);
            break;
        case PROP_FRAGMENT_ACKS:
            g_value_set_boolean (value, sink->fragment_acks);
            break;
        case PROP_RESTART_ON_ERROR:
            g_value_set_boolean (value, sink->restart_on_error);
            break;
        case PROP_RECALCULATE_METRICS:
            g_value_set_boolean (value, sink->recalculate_metrics);
            break;
        case PROP_AVG_BANDWIDTH_BPS:
            g_value_set_uint (value, sink->avg_bandwidth_bps);
            break;
        case PROP_BUFFER_DURATION:
            g_value_set_uint (value, sink->buffer_duration_seconds);
            break;
        case PROP_REPLAY_DURATION:
            g_value_set_uint (value, sink->replay_duration_seconds);
            break;
        case PROP_CONNECTION_STALENESS:
            g_value_set_uint (value, sink->connection_staleness_seconds);
            break;
        case PROP_CODEC_ID:
            g_value_set_string (value, sink->codec_id);
            break;
        case PROP_TRACK_NAME:
            g_value_set_string (value, sink->track_name);
            break;
        case PROP_ACCESS_KEY:
            g_value_set_string (value, sink->access_key);
            break;
        case PROP_SECRET_KEY:
            g_value_set_string (value, sink->secret_key);
            break;
        case PROP_AWS_REGION:
            g_value_set_string (value, sink->aws_region);
            break;
        case PROP_ROTATION_PERIOD:
            g_value_set_uint (value, sink->rotation_period);
            break;
        case PROP_LOG_CONFIG_PATH:
            g_value_set_string (value, sink->log_config_path);
            break;
        case PROP_FRAMERATE:
            g_value_set_uint (value, sink->framerate);
            break;
        case PROP_FRAME_TIMESTAMP:
            g_value_set_enum (value, sink->frame_timestamp);
            break;
        case PROP_STORAGE_SIZE:
            g_value_set_uint (value, sink->storage_size);
            break;
        case PROP_CREDENTIAL_FILE_PATH:
            g_value_set_string (value, sink->credential_file_path);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_kvs_sink_notify_last_message(GstKvsSink *sink) {
    g_object_notify_by_pspec((GObject *) sink, pspec_last_message);
}

static gboolean gst_kvs_sink_event(GstBaseSink *bsink, GstEvent *event) {
    GstKvsSink *sink = GST_KVS_SINK (bsink);
    auto data = sink->data;

    if (!sink->silent) {
        const GstStructure *s;
        const gchar *tstr;
        gchar *sstr;

        GST_OBJECT_LOCK (sink);
        g_free(sink->last_message);

        if (GST_EVENT_TYPE (event) == GST_EVENT_SINK_MESSAGE) {
            GstMessage *msg;
            const GstStructure *structure;

            gst_event_parse_sink_message(event, &msg);
            structure = gst_message_get_structure(msg);
            sstr = gst_structure_to_string(structure);
            sink->last_message =
                    g_strdup_printf("message ******* (%s:%s) M (type: %d, %s) %p",
                                    GST_DEBUG_PAD_NAME (GST_BASE_SINK_CAST(sink)->sinkpad),
                                    GST_MESSAGE_TYPE (msg), sstr, msg);
            gst_message_unref(msg);
        } else {
            tstr = gst_event_type_get_name(GST_EVENT_TYPE (event));

            if ((s = gst_event_get_structure(event))) {
                sstr = gst_structure_to_string(s);
            } else {
                sstr = g_strdup("");
            }

            sink->last_message =
                    g_strdup_printf("event   ******* (%s:%s) E (type: %s (%d), %s) %p",
                                    GST_DEBUG_PAD_NAME (GST_BASE_SINK_CAST(sink)->sinkpad),
                                    tstr, GST_EVENT_TYPE (event), sstr, event);
        }
        g_free(sstr);
        GST_OBJECT_UNLOCK (sink);

        gst_kvs_sink_notify_last_message(sink);
    }

    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
        GstCaps *gstcaps;
        gst_event_parse_caps(event, &gstcaps);
        GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);
        if (data->cpd.empty() && gst_structure_has_field(gststructforcaps, "codec_data")) {
            const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
            gchar *cpd = gst_value_serialize(gstStreamFormat);
            data->cpd = std::string(cpd);
        }

        GST_INFO ("structure is %" GST_PTR_FORMAT, gststructforcaps);
    }

    return GST_BASE_SINK_CLASS (parent_class)->event(bsink, event);
}

static GstFlowReturn gst_kvs_sink_preroll(GstBaseSink *bsink, GstBuffer *buffer) {
    GstKvsSink *sink = GST_KVS_SINK (bsink);

    if (sink->num_buffers_left == 0)
        goto eos;

    if (!sink->silent) {
        GST_OBJECT_LOCK (sink);
        g_free(sink->last_message);

        sink->last_message = g_strdup_printf("preroll   ******* ");
        GST_OBJECT_UNLOCK (sink);

        gst_kvs_sink_notify_last_message(sink);
    }
    if (sink->signal_handoffs) {
        g_signal_emit(sink,
                      gst_kvs_sink_signals[SIGNAL_PREROLL_HANDOFF], 0, buffer,
                      bsink->sinkpad);
    }
    return GST_FLOW_OK;

    /* ERRORS */
    eos:
    {
        GST_DEBUG_OBJECT (sink, "Reached End Of Stream (EOS)");
        return GST_FLOW_EOS;
    }
}

void recreate_kvs_stream(GstKvsSink *sink) {
    auto data = sink->data;

    {
        std::lock_guard<std::mutex> lk(data->stream_recreation_status_mtx);
        data->stream_recreation_status = STREAM_RECREATION_STATUS_IN_PROGRESS;
    }

    string err_msg;
    // retry forever until success
    bool succeeded;
    do {
        succeeded = kinesis_video_stream_init(sink, err_msg) && kinesis_video_stream_start(sink, err_msg);
    } while (!succeeded);

    std::lock_guard<std::mutex> lk(data->stream_recreation_status_mtx);
    data->stream_recreation_status = STREAM_RECREATION_STATUS_NONE;
}

/*
 * This method is called right before render for every frame. i.e. right before put_frame is called
 * for every frame.
 */
static GstFlowReturn gst_kvs_sink_prepare(GstBaseSink *bsink, GstBuffer *buffer) {

    GstKvsSink *sink = GST_KVS_SINK_CAST (bsink);
    auto data = sink->data;
    string err_msg;
    GstFlowReturn ret = GST_FLOW_OK;
    bool recreate_stream = false;

    {
        std::lock_guard<std::mutex> lk(data->closing_stream_handles_queue_mtx);
        // During streaming, if any error is received in getStreamErrorReportCallback that require a stream to be freed and
        // recreated, its stream handle is pushed into closing_stream_handles_queue by getStreamErrorReportCallback.
        if (!data->closing_stream_handles_queue.empty()) {
            do {
                STREAM_HANDLE handle = data->closing_stream_handles_queue.front();
                data->closing_stream_handles_queue.pop();
                auto stream = data->kinesis_video_stream_map.get(handle);
                if (stream == nullptr) {
                    continue;
                }
                recreate_stream = true;
                data->kinesis_video_producer->freeStream(stream);
                data->kinesis_video_stream_map.remove(handle);
                data->callback_state_machine_map.remove(handle);
            } while (!data->closing_stream_handles_queue.empty());
        }
    }

    if (recreate_stream) {
        // Use another thread to create and start new stream so that we dont block gstreamer pipeline.
        // The process could take some time because recreate_kvs_stream would retry a few times.
        std::thread worker(recreate_kvs_stream, sink);
        worker.detach();
    }

CleanUp:

    return ret;
}

void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *frame_data, size_t len) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(frame_data);
    frame->index = 0;
}

bool
put_frame(shared_ptr<KinesisVideoStream> kinesis_video_stream, void *frame_data, size_t len, const nanoseconds &pts,
          const nanoseconds &dts, FRAME_FLAGS flags) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, frame_data, len);
    return kinesis_video_stream->putFrame(frame);
}

static GstFlowReturn gst_kvs_sink_render(GstBaseSink *bsink, GstBuffer *buf) {
    GstKvsSink *sink = GST_KVS_SINK_CAST (bsink);
    auto data = sink->data;
    string err_msg;
    GstFlowReturn ret = GST_FLOW_OK;

    if (sink->dump) {
        GstMapInfo info;

        if (gst_buffer_map(buf, &info, GST_MAP_READ)) {
            gst_util_dump_mem(info.data, info.size);
            gst_buffer_unmap(buf, &info);
        }
    }

    bool isDroppable =  GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_CORRUPTED) ||
                        GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DECODE_ONLY);

    /*
     * IN_PROGRESS:     In progress of creating new kvs stream. Thus there is no kvs stream available. Therefore skip current
     *                  frame.
     * NONE:            Do nothing.
     */
    {
        std::lock_guard<std::mutex> lk(data->stream_recreation_status_mtx);
        switch (data->stream_recreation_status) {
            case STREAM_RECREATION_STATUS_IN_PROGRESS:
                isDroppable = true;
            case STREAM_RECREATION_STATUS_NONE:
                break;
        }
    }

    if (!isDroppable) {
        bool isHeader = GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_HEADER);
        // drop if buffer contains header only and has invalid timestamp
        if (!(isHeader && (!GST_BUFFER_PTS_IS_VALID(buf) || !GST_BUFFER_DTS_IS_VALID(buf)))) {

            switch (sink->frame_timestamp) {
                case KVS_SINK_TIMESTAMP_PTS_ONLY:
                    if (!GST_BUFFER_PTS_IS_VALID(buf)) {
                        GST_ELEMENT_ERROR (sink, STREAM, FAILED, (NULL),
                                           ("frame-timestamp is KVS_SINK_TIMESTAMP_PTS_ONLY but frame contains invalid pts"));
                        ret = GST_FLOW_ERROR;
                        goto CleanUp;
                    }
                    buf->dts = buf->pts;
                    break;
                case KVS_SINK_TIMESTAMP_DTS_ONLY:
                    if (!GST_BUFFER_DTS_IS_VALID(buf)) {
                        GST_ELEMENT_ERROR (sink, STREAM, FAILED, (NULL),
                                           ("frame-timestamp is KVS_SINK_TIMESTAMP_DTS_ONLY but frame contains invalid dts"));
                        ret = GST_FLOW_ERROR;
                        goto CleanUp;
                    }
                    buf->pts = buf->dts;
                    break;
                case KVS_SINK_TIMESTAMP_DEFAULT:
                    if (GST_BUFFER_PTS_IS_VALID(buf) && !GST_BUFFER_DTS_IS_VALID(buf)) {
                        buf->dts = buf->pts;
                    }
                    else if (!GST_BUFFER_PTS_IS_VALID(buf) && GST_BUFFER_DTS_IS_VALID(buf)){
                        buf->pts = buf->dts;
                    }
            }

            size_t buffer_size = gst_buffer_get_size(buf);
            if (buffer_size > sink->current_frame_data_size) {
                delete [] sink->frame_data;
                sink->current_frame_data_size = buffer_size * FRAME_DATA_ALLOCATION_MULTIPLIER;
                sink->frame_data = new uint8_t[sink->current_frame_data_size];
            }
            gst_buffer_extract(buf, 0, sink->frame_data, buffer_size);

            bool delta = GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);
            FRAME_FLAGS kinesis_video_flags;
            if(!delta) {
                buf->pts = buf->dts;
                kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
            } else {
                kinesis_video_flags = FRAME_FLAG_NONE;
            }

            for (auto stream : data->kinesis_video_stream_map.getMap()) {
                try {
                    if (!put_frame(stream.second, sink->frame_data, buffer_size,
                                   std::chrono::nanoseconds(buf->pts),
                                   std::chrono::nanoseconds(buf->dts), kinesis_video_flags)) {
                        g_printerr("Failed to put frame into Kinesis Video Stream buffer\n");
                    }
                } catch (runtime_error &err) {
                    err_msg = err.what();
                    GST_ELEMENT_ERROR (sink, STREAM, FAILED, (NULL),
                                       ("put_frame threw an exception. %s", err_msg.c_str()));
                    ret = GST_FLOW_ERROR;
                    goto CleanUp;
                }
            }

            if (sink->num_buffers_left == 0) {
                GST_DEBUG_OBJECT (sink, "Reached End Of Stream (EOS)");
                ret = GST_FLOW_EOS;
                goto CleanUp;
            }
        }
    }

CleanUp:

    return ret;
}

static gboolean gst_kvs_sink_query(GstBaseSink *bsink, GstQuery *query) {
    gboolean ret;

    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_SEEKING: {
            GstFormat fmt;

            /* we don't supporting seeking */
            gst_query_parse_seeking(query, &fmt, NULL, NULL, NULL);
            gst_query_set_seeking(query, fmt, FALSE, 0, -1);
            ret = TRUE;
            break;
        }
        default:
            ret = GST_BASE_SINK_CLASS (parent_class)->query(bsink, query);
            break;
    }

    return ret;
}

static GstStateChangeReturn  gst_kvs_sink_change_state(GstElement *element, GstStateChange transition) {
    GstStateChangeReturn ret;
    GstKvsSink *sink = GST_KVS_SINK (element);
    auto data = sink->data;
    string err_msg;

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            sink->num_buffers_left = sink->num_buffers;
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            if (!data->stream_created){
                data->stream_created = true;

                if (data->cpd.empty()) {
                    GST_ELEMENT_ERROR (sink, CORE, STATE_CHANGE, (NULL),
                                       ("Can't start kvs stream. Codec private data was not found in GST_CAPS event"));
                    ret = GST_STATE_CHANGE_FAILURE;
                    goto CleanUp;
                }

                if (false == kinesis_video_stream_start(sink, err_msg)) {
                    GST_ELEMENT_ERROR (sink, LIBRARY, INIT, (NULL),
                                       ("%s", err_msg.c_str()));
                    ret = GST_STATE_CHANGE_FAILURE;
                    goto CleanUp;
                }
            }
            break;
        default:
            break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_OBJECT_LOCK (sink);
            g_free(sink->last_message);
            sink->last_message = NULL;
            GST_OBJECT_UNLOCK (sink);
            break;
        default:
            break;
    }

CleanUp:

    return ret;
}

static gboolean gst_kvs_sink_start (GstBaseSink * bsink)
{
    GstKvsSink *sink = GST_KVS_SINK (bsink);
    auto data = sink->data;
    data->kvsSink = sink;
    bool ret = true;

    initialize();
    PropertyConfigurator::doConfigure(sink->log_config_path);

    string err_msg;
    try {
        kinesis_video_producer_init(sink);
    } catch (runtime_error &err) {
        ostringstream oss;
        oss << "Failed to init kvs producer. Error: " << err.what();
        err_msg = oss.str();
        ret = false;
        GST_ELEMENT_ERROR (sink, LIBRARY, INIT, (NULL), ("%s", err_msg.c_str()));
        goto CleanUp;
    }

    if (false == kinesis_video_stream_init(sink, err_msg)) {
        GST_ELEMENT_ERROR (sink, LIBRARY, INIT, (NULL), ("%s", err_msg.c_str()));
        ret = false;
        goto CleanUp;
    }

CleanUp:

    return ret;
}

static gboolean gst_kvs_sink_stop (GstBaseSink * bsink)
{
    return TRUE;
}

GST_DEBUG_CATEGORY (kvs_debug);

static gboolean plugin_init(GstPlugin *plugin) {

    if (!gst_element_register(plugin, "kvssink", GST_RANK_PRIMARY + 10, GST_TYPE_KVS_SINK)) {
        return FALSE;
    }

    GST_DEBUG_CATEGORY_INIT(kvs_debug, "kvs", 0, "KVS sink elements");
    return TRUE;
}

#define PACKAGE "kvssinkpackage"
GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        kvssink,
        "GStreamer AWS KVS plugin",
        plugin_init,
        "1.0",
        "Proprietary",
        "GStreamer",
        "http://gstreamer.net/"
)
