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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstkvssink.h"
#include <chrono>
#include <Logger.h>
#include <RotatingCredentialProvider.h>
#include "KvsSinkStreamCallbackProvider.h"
#include "KvsSinkClientCallbackProvider.h"
#include "KvsSinkDeviceInfoProvider.h"
#include "StreamLatencyStateMachine.h"
#include "ConnectionStaleStateMachine.h"
#include <IotCertCredentialProvider.h>
#include <RotatingStaticCredentialProvider.h>
#include "Util/KvsSinkUtil.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace std;
using namespace log4cplus;

GST_DEBUG_CATEGORY_STATIC (gst_kvs_sink_debug);
#define GST_CAT_DEFAULT gst_kvs_sink_debug

// default starting delay between reties when trying to create and start kvs stream.
#define DEFAULT_INITIAL_RETRY_DELAY_MS 400
// default number of retries when trying to create and start kvs stream.
#define DEFAULT_TOTAL_RETRY 4

#define DEFAULT_STREAM_NAME "DEFAULT_STREAM"
#define DEFAULT_RETENTION_PERIOD_HOURS 2
#define DEFAULT_KMS_KEY_ID ""
#define DEFAULT_STREAMING_TYPE STREAMING_TYPE_REALTIME
#define DEFAULT_CONTENT_TYPE_VIDEO_H264 "video/h264"
#define DEFAULT_CONTENT_TYPE_AV_H264_AAC "video/h264,audio/aac"
#define DEFAULT_CONTENT_TYPE_AUDIO "audio/aac"
#define DEFAULT_MAX_LATENCY_SECONDS 60
#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES FALSE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_STREAM_FRAMERATE 25
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
#define DEFAULT_BUFFER_DURATION_SECONDS 120
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
#define DEFAULT_FILE_START_TIME 0
#define DEFAULT_STORAGE_SIZE_MB 128
#define DEFAULT_CREDENTIAL_FILE_PATH ".kvs/credential"
#define DEFAULT_FRAME_DURATION_MS 2

#define FRAME_DATA_ALLOCATION_MULTIPLIER 1.5
#define KVS_ADD_METADATA_G_STRUCT_NAME "kvs-add-metadata"
#define KVS_ADD_METADATA_NAME "name"
#define KVS_ADD_METADATA_VALUE "value"
#define KVS_ADD_METADATA_PERSISTENT "persist"
#define KVS_CLIENT_USER_AGENT_NAME "AWS-SDK-KVS-CLIENT"

#define DEFAULT_AUDIO_TRACK_NAME "audio"
#define DEFAULT_AUDIO_CODEC_ID "A_AAC"
#define KVS_SINK_DEFAULT_TRACKID 1
#define KVS_SINK_DEFAULT_AUDIO_TRACKID 2

enum {
    PROP_0,
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
    PROP_STORAGE_SIZE,
    PROP_CREDENTIAL_FILE_PATH,
    PROP_IOT_CERTIFICATE,
    PROP_STREAM_TAGS,
    PROP_FILE_START_TIME
};

#define GST_TYPE_KVS_SINK_STREAMING_TYPE (gst_kvs_sink_streaming_type_get_type())
static GType
gst_kvs_sink_streaming_type_get_type (void)
{
    static GType kvssink_streaming_type_type = 0;
    static const GEnumValue kvssink_streaming_type[] = {
            {STREAMING_TYPE_REALTIME, "streaming type realtime", "realtime"},
            {STREAMING_TYPE_NEAR_REALTIME, "streaming type near realtime", "near-realtime"},
            {STREAMING_TYPE_OFFLINE, "streaming type offline", "offline"},
            {0, NULL, NULL},
    };

    if (!kvssink_streaming_type_type) {
        kvssink_streaming_type_type =
                g_enum_register_static ("GstKvsSinkStreamingType", kvssink_streaming_type);
    }
    return kvssink_streaming_type_type;
}

static GstStaticPadTemplate audiosink_templ =
        GST_STATIC_PAD_TEMPLATE ("audio_%u",
                                 GST_PAD_SINK,
                                 GST_PAD_REQUEST,
                                 GST_STATIC_CAPS (
                                         "audio/mpeg, mpegversion = (int) { 2, 4 }, stream-format = (string) raw, channels = (int) [ 1, MAX ], rate = (int) [ 1, MAX ] ;"
                                 )
        );

static GstStaticPadTemplate videosink_templ =
        GST_STATIC_PAD_TEMPLATE ("video_%u",
                                 GST_PAD_SINK,
                                 GST_PAD_REQUEST,
                                 GST_STATIC_CAPS (
                                         "video/x-h264, stream-format = (string) avc, alignment = (string) au, width = (int) [ 16, MAX ], height = (int) [ 16, MAX ] ;"
                                 )
        );

#define _do_init GST_DEBUG_CATEGORY_INIT (gst_kvs_sink_debug, "kvssink", 0, "KVS sink plug-in");

#define gst_kvs_sink_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE (GstKvsSink, gst_kvs_sink, GST_TYPE_ELEMENT, _do_init);


static void gst_kvs_sink_set_property(GObject *object, guint prop_id,
                                      const GValue *value, GParamSpec *pspec);

static void gst_kvs_sink_get_property(GObject *object, guint prop_id,
                                      GValue *value, GParamSpec *pspec);

static void gst_kvs_sink_finalize(GObject *obj);

static GstStateChangeReturn gst_kvs_sink_change_state(GstElement *element,
                                                      GstStateChange transition);

/* collectpad callback */
static GstFlowReturn gst_kvs_sink_handle_buffer (GstCollectPads * pads,
                                                 GstCollectData * data, GstBuffer * buf, gpointer user_data);
static gboolean gst_kvs_sink_handle_sink_event (GstCollectPads * pads,
                                                GstCollectData * data, GstEvent * event, gpointer user_data);
static gint gst_kvs_sink_collect_compare (GstCollectPads *pads,
                                          GstCollectData *data1,
                                          GstClockTime timestamp1,
                                          GstCollectData *data2,
                                          GstClockTime timestamp2,
                                          gpointer user_data);

/* Request pad callback */
static GstPad* gst_kvs_sink_request_new_pad (GstElement *element, GstPadTemplate *templ,
                                             const gchar* name, const GstCaps *caps);
static void gst_kvs_sink_release_pad (GstElement *element, GstPad *pad);


CallbackStateMachine::_CallbackStateMachine(shared_ptr<CustomData> data) {
    stream_latency_state_machine = make_shared<StreamLatencyStateMachine>(data);
    connection_stale_state_machine = make_shared<ConnectionStaleStateMachine>(data);
}


void kinesis_video_producer_init(GstKvsSink *kvssink) {
    auto data = kvssink->data;
    unique_ptr<DeviceInfoProvider> device_info_provider = make_unique<KvsSinkDeviceInfoProvider>(kvssink->storage_size);
    unique_ptr<ClientCallbackProvider> client_callback_provider = make_unique<KvsSinkClientCallbackProvider>();
    unique_ptr<StreamCallbackProvider> stream_callback_provider = make_unique<KvsSinkStreamCallbackProvider>(data);

    char const *access_key;
    char const *secret_key;
    char const *session_token;
    char const *default_region;
    string access_key_str;
    string secret_key_str;
    string session_token_str;
    string region_str;
    bool credential_is_static = true;

    if (0 == strcmp(kvssink->access_key, DEFAULT_ACCESS_KEY)) { // if no static credential is available in plugin property.
        if (nullptr == (access_key = getenv(ACCESS_KEY_ENV_VAR))
            || nullptr == (secret_key = getenv(SECRET_KEY_ENV_VAR))) { // if no static credential is available in env var.
            credential_is_static = false; // No static credential available.
            access_key_str = "";
            secret_key_str = "";
        } else {
            access_key_str = string(access_key);
            secret_key_str = string(secret_key);
            session_token_str = "";
            if (nullptr != (session_token = getenv(SESSION_TOKEN_ENV_VAR))) {
                session_token_str = string(session_token);
            }
        }

    } else {
        access_key_str = string(kvssink->access_key);
        secret_key_str = string(kvssink->secret_key);
    }

    if (nullptr == (default_region = getenv(DEFAULT_REGION_ENV_VAR))) {
        region_str = string(kvssink->aws_region);
    } else {
        region_str = string(default_region); // Use env var if both property and env var are available.
    }

    unique_ptr<CredentialProvider> credential_provider;

    if (credential_is_static) {
        kvssink->credentials_ = make_unique<Credentials>(access_key_str,
                                                         secret_key_str,
                                                         session_token_str,
                                                         std::chrono::seconds(DEFAULT_ROTATION_PERIOD_SECONDS));
        credential_provider = make_unique<RotatingStaticCredentialProvider>(*kvssink->credentials_, kvssink->rotation_period);
    } else if (kvssink->iot_certificate) {
        std::map<std::string, std::string> iot_cert_params;
        gboolean ret = kvs_sink_util::parseIotCredentialGstructure(kvssink->iot_certificate, iot_cert_params);
        g_assert_true(ret);
        credential_provider = make_unique<IotCertCredentialProvider>(iot_cert_params[IOT_GET_CREDENTIAL_ENDPOINT],
                                                                     iot_cert_params[CERTIFICATE_PATH],
                                                                     iot_cert_params[PRIVATE_KEY_PATH],
                                                                     iot_cert_params[ROLE_ALIASES],
                                                                     iot_cert_params[CA_CERT_PATH],
                                                                     kvssink->stream_name);
    } else {
        credential_provider = make_unique<RotatingCredentialProvider>(kvssink->credential_file_path);
    }

    data->kinesis_video_producer = KinesisVideoProducer::createSync(move(device_info_provider),
                                                                    move(client_callback_provider),
                                                                    move(stream_callback_provider),
                                                                    move(credential_provider),
                                                                    region_str,
                                                                    "",
                                                                    KVS_CLIENT_USER_AGENT_NAME);
}

void create_kinesis_video_stream(GstKvsSink *kvssink) {
    auto data = kvssink->data;

    map<string, string> *p_stream_tags = nullptr;
    map<string, string> stream_tags;
    if (kvssink->stream_tags) {
        gboolean ret;
        ret = kvs_sink_util::gstructToMap(kvssink->stream_tags, &stream_tags);
        if (!ret) {
            LOG_WARN("Failed to parse stream tags");
        } else {
            p_stream_tags = &stream_tags;
        }
    }

    // If doing offline mode file uploading, and the user wants to use a specific file start time,
    // then switch to use abosolute fragment time. Since we will be adding the file_start_time to the timestamp
    // of each frame to make each frame's timestamp absolute. Assuming each frame's timestamp is relative
    // (i.e. starting from 0)
    if (kvssink->streaming_type == STREAMING_TYPE_OFFLINE && kvssink->file_start_time != 0) {
        kvssink->absolute_fragment_times = TRUE;
        data->pts_base = duration_cast<nanoseconds>(seconds(kvssink->file_start_time)).count();
    }

    switch (data->media_type) {
        case AUDIO_VIDEO:
            g_free(kvssink->content_type);
            kvssink->content_type = g_strdup(DEFAULT_CONTENT_TYPE_AV_H264_AAC);
            break;
        case AUDIO_ONLY:
            g_free(kvssink->content_type);
            kvssink->content_type = g_strdup(DEFAULT_CONTENT_TYPE_AUDIO);
            g_free(kvssink->codec_id);
            kvssink->codec_id = g_strdup(DEFAULT_AUDIO_CODEC_ID);
            g_free(kvssink->track_name);
            kvssink->track_name = g_strdup(DEFAULT_AUDIO_TRACK_NAME);
            kvssink->track_info_type = MKV_TRACK_INFO_TYPE_AUDIO;
            kvssink->key_frame_fragmentation = FALSE;
            break;
        case VIDEO_ONLY:
            // no-op because default setup is for video only
            break;
    }

    auto stream_definition = make_unique<StreamDefinition>(kvssink->stream_name,
                                                           hours(kvssink->retention_period_hours),
                                                           p_stream_tags,
                                                           kvssink->kms_key_id,
                                                           kvssink->streaming_type,
                                                           kvssink->content_type,
                                                           duration_cast<milliseconds> (seconds(kvssink->max_latency_seconds)),
                                                           milliseconds(kvssink->fragment_duration_miliseconds),
                                                           milliseconds(kvssink->timecode_scale_milliseconds),
                                                           kvssink->key_frame_fragmentation,//Construct a fragment at each key frame
                                                           kvssink->frame_timecodes,//Use provided frame timecode
                                                           kvssink->absolute_fragment_times,//Relative timecode
                                                           kvssink->fragment_acks,//Ack on fragment is enabled
                                                           kvssink->restart_on_error,//SDK will restart when error happens
                                                           kvssink->recalculate_metrics,//recalculate_metrics
                                                           0,
                                                           kvssink->framerate,
                                                           kvssink->avg_bandwidth_bps,
                                                           seconds(kvssink->buffer_duration_seconds),
                                                           seconds(kvssink->replay_duration_seconds),
                                                           seconds(kvssink->connection_staleness_seconds),
                                                           kvssink->codec_id,
                                                           kvssink->track_name,
                                                           nullptr,
                                                           0,
                                                           kvssink->track_info_type,
                                                           vector<uint8_t>(),
                                                           KVS_SINK_DEFAULT_TRACKID);

    if (data->media_type == AUDIO_VIDEO) {
        stream_definition->addTrack(KVS_SINK_DEFAULT_AUDIO_TRACKID, DEFAULT_AUDIO_TRACK_NAME, DEFAULT_AUDIO_CODEC_ID, MKV_TRACK_INFO_TYPE_AUDIO);
    }

    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(move(stream_definition));
    data->callback_state_machine = make_shared<CallbackStateMachine>(data);
    cout << "Stream is ready" << endl;
}

bool kinesis_video_stream_init(GstKvsSink *kvssink, string &err_msg) {
    bool ret = true;
    int retry_count = DEFAULT_TOTAL_RETRY;
    int retry_delay = DEFAULT_INITIAL_RETRY_DELAY_MS;
    bool do_retry = true;
    while(do_retry) {
        try {
            LOG_INFO("try creating stream");
            create_kinesis_video_stream(kvssink);
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

bool kinesis_video_stream_start(shared_ptr<CustomData> data, string &cpd, uint64_t track_id) {
    bool ret = true;
    int retry_count = DEFAULT_TOTAL_RETRY;
    int retry_delay = DEFAULT_INITIAL_RETRY_DELAY_MS;
    bool do_retry = true;

    while(do_retry) {
        LOG_INFO("try starting stream.");
        if (data->kinesis_video_stream->start(cpd, track_id)) {
            do_retry = false;
        } else {
            if (--retry_count == 0) {
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

static void
gst_kvs_sink_class_init(GstKvsSinkClass *klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);

    gobject_class->set_property = gst_kvs_sink_set_property;
    gobject_class->get_property = gst_kvs_sink_get_property;
    gobject_class->finalize = gst_kvs_sink_finalize;

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
                                                          "content type", DEFAULT_CONTENT_TYPE_VIDEO_H264, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

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

    g_object_class_install_property (gobject_class, PROP_STORAGE_SIZE,
                                     g_param_spec_uint ("storage-size", "Storage Size",
                                                        "Storage Size. Unit: MB", 0, G_MAXUINT, DEFAULT_STORAGE_SIZE_MB, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CREDENTIAL_FILE_PATH,
                                     g_param_spec_string ("credential-path", "Credential File Path",
                                                          "Credential File Path", DEFAULT_CREDENTIAL_FILE_PATH, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_IOT_CERTIFICATE,
                                     g_param_spec_boxed ("iot-certificate", "Iot Certificate",
                                                         "Use aws iot certificate to obtain credentials",
                                                         GST_TYPE_STRUCTURE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STREAM_TAGS,
                                     g_param_spec_boxed ("stream-tags", "Stream Tags",
                                                         "key-value pair that you can define and assign to each stream",
                                                         GST_TYPE_STRUCTURE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FILE_START_TIME,
                                     g_param_spec_ulong ("file-start-time", "File Start Time",
                                                        "Epoch time that the file starts in kinesis video stream. By default, current time is used. Unit: Seconds",
                                                         0, G_MAXULONG, 0, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
                                          "KVS Sink",
                                          "Sink/Video/Network",
                                          "GStreamer AWS KVS plugin",
                                          "AWS KVS <kinesis-video-support@amazon.com>");

    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&audiosink_templ));
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&videosink_templ));

    gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_kvs_sink_change_state);
    gstelement_class->request_new_pad = GST_DEBUG_FUNCPTR (gst_kvs_sink_request_new_pad);
    gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_kvs_sink_release_pad);
}

static void
gst_kvs_sink_init(GstKvsSink *kvssink) {
    kvssink->collect = gst_collect_pads_new();

    gst_collect_pads_set_buffer_function (kvssink->collect,
                                          GST_DEBUG_FUNCPTR (gst_kvs_sink_handle_buffer), kvssink);
    gst_collect_pads_set_event_function (kvssink->collect,
                                         GST_DEBUG_FUNCPTR (gst_kvs_sink_handle_sink_event), kvssink);
    gst_collect_pads_set_compare_function(kvssink->collect,
                                          GST_DEBUG_FUNCPTR (gst_kvs_sink_collect_compare), kvssink);

    kvssink->num_streams = 0;
    kvssink->num_audio_streams = 0;
    kvssink->num_video_streams = 0;

    // Stream definition
    kvssink->stream_name = g_strdup (DEFAULT_STREAM_NAME);
    kvssink->retention_period_hours = DEFAULT_RETENTION_PERIOD_HOURS;
    kvssink->kms_key_id = g_strdup (DEFAULT_KMS_KEY_ID);
    kvssink->streaming_type = DEFAULT_STREAMING_TYPE;
    kvssink->content_type = g_strdup (DEFAULT_CONTENT_TYPE_VIDEO_H264);
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
    kvssink->storage_size = DEFAULT_STORAGE_SIZE_MB;
    kvssink->credential_file_path = g_strdup (DEFAULT_CREDENTIAL_FILE_PATH);
    kvssink->file_start_time = DEFAULT_FILE_START_TIME;
    kvssink->track_info_type = MKV_TRACK_INFO_TYPE_VIDEO;

    kvssink->data = make_shared<CustomData>();

    // Mark plugin as sink
    GST_OBJECT_FLAG_SET (kvssink, GST_ELEMENT_FLAG_SINK);

    LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");
    LOG_CONFIGURE_STDOUT("DEBUG")
}

static void
gst_kvs_sink_finalize(GObject *object) {
    GstKvsSink *kvssink = GST_KVS_SINK (object);
    auto data = kvssink->data;

    data->kinesis_video_producer->freeStream(data->kinesis_video_stream);

    g_free(kvssink->stream_name);
    g_free(kvssink->content_type);
    g_free(kvssink->codec_id);
    g_free(kvssink->track_name);
    g_free(kvssink->secret_key);
    g_free(kvssink->access_key);
    if (kvssink->iot_certificate) {
        gst_structure_free (kvssink->iot_certificate);
    }
    if (kvssink->stream_tags) {
        gst_structure_free (kvssink->stream_tags);
    }
    delete [] kvssink->frame_data;
    G_OBJECT_CLASS (parent_class)->finalize(object);
}

static void
gst_kvs_sink_set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec) {
    GstKvsSink *kvssink;

    kvssink = GST_KVS_SINK (object);

    switch (prop_id) {
        case PROP_STREAM_NAME:
            g_free(kvssink->stream_name);
            kvssink->stream_name = g_strdup (g_value_get_string (value));
            break;
        case PROP_RETENTION_PERIOD:
            kvssink->retention_period_hours = g_value_get_uint (value);
            break;
        case PROP_STREAMING_TYPE:
            kvssink->streaming_type = (STREAMING_TYPE) g_value_get_enum (value);
            break;
        case PROP_CONTENT_TYPE:
            g_free(kvssink->content_type);
            kvssink->content_type = g_strdup (g_value_get_string (value));
            break;
        case PROP_MAX_LATENCY:
            kvssink->max_latency_seconds = g_value_get_uint (value);
            break;
        case PROP_FRAGMENT_DURATION:
            kvssink->fragment_duration_miliseconds = g_value_get_uint (value);
            break;
        case PROP_TIMECODE_SCALE:
            kvssink->timecode_scale_milliseconds = g_value_get_uint (value);
            break;
        case PROP_KEY_FRAME_FRAGMENTATION:
            kvssink->key_frame_fragmentation = g_value_get_boolean (value);
            break;
        case PROP_FRAME_TIMECODES:
            kvssink->frame_timecodes = g_value_get_boolean (value);
            break;
        case PROP_ABSOLUTE_FRAGMENT_TIMES:
            kvssink->absolute_fragment_times = g_value_get_boolean (value);
            break;
        case PROP_FRAGMENT_ACKS:
            kvssink->fragment_acks = g_value_get_boolean (value);
            break;
        case PROP_RESTART_ON_ERROR:
            kvssink->restart_on_error = g_value_get_boolean (value);
            break;
        case PROP_RECALCULATE_METRICS:
            kvssink->recalculate_metrics = g_value_get_boolean (value);
            break;
        case PROP_AVG_BANDWIDTH_BPS:
            kvssink->avg_bandwidth_bps = g_value_get_uint (value);
            break;
        case PROP_BUFFER_DURATION:
            kvssink->buffer_duration_seconds = g_value_get_uint (value);
            break;
        case PROP_REPLAY_DURATION:
            kvssink->replay_duration_seconds = g_value_get_uint (value);
            break;
        case PROP_CONNECTION_STALENESS:
            kvssink->connection_staleness_seconds = g_value_get_uint (value);
            break;
        case PROP_CODEC_ID:
            g_free(kvssink->codec_id);
            kvssink->codec_id = g_strdup (g_value_get_string (value));
            break;
        case PROP_TRACK_NAME:
            g_free(kvssink->track_name);
            kvssink->track_name = g_strdup (g_value_get_string (value));
            break;
        case PROP_ACCESS_KEY:
            g_free(kvssink->access_key);
            kvssink->access_key = g_strdup (g_value_get_string (value));
            break;
        case PROP_SECRET_KEY:
            g_free(kvssink->secret_key);
            kvssink->secret_key = g_strdup (g_value_get_string (value));
            break;
        case PROP_AWS_REGION:
            g_free(kvssink->aws_region);
            kvssink->aws_region = g_strdup (g_value_get_string (value));
            break;
        case PROP_ROTATION_PERIOD:
            kvssink->rotation_period = g_value_get_uint (value);
            break;
        case PROP_LOG_CONFIG_PATH:
            kvssink->log_config_path = g_strdup (g_value_get_string (value));
            break;
        case PROP_FRAMERATE:
            kvssink->framerate = g_value_get_uint (value);
            break;
        case PROP_STORAGE_SIZE:
            kvssink->storage_size = g_value_get_uint (value);
            break;
        case PROP_CREDENTIAL_FILE_PATH:
            kvssink->credential_file_path = g_strdup (g_value_get_string (value));
            break;
        case PROP_IOT_CERTIFICATE: {
            const GstStructure *s = gst_value_get_structure(value);

            if (kvssink->iot_certificate) {
                gst_structure_free(kvssink->iot_certificate);
            }
            kvssink->iot_certificate = s ? gst_structure_copy(s) : NULL;
            break;
        }
        case PROP_STREAM_TAGS: {
            const GstStructure *s = gst_value_get_structure(value);

            if (kvssink->stream_tags) {
                gst_structure_free(kvssink->stream_tags);
            }
            kvssink->stream_tags = s ? gst_structure_copy(s) : NULL;
            break;
        }
        case PROP_FILE_START_TIME:
            kvssink->file_start_time = g_value_get_ulong (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_kvs_sink_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec) {
    GstKvsSink *kvssink;

    kvssink = GST_KVS_SINK (object);

    switch (prop_id) {
        case PROP_STREAM_NAME:
            g_value_set_string (value, kvssink->stream_name);
            break;
        case PROP_RETENTION_PERIOD:
            g_value_set_uint (value, kvssink->retention_period_hours);
            break;
        case PROP_STREAMING_TYPE:
            g_value_set_enum (value, kvssink->streaming_type);
            break;
        case PROP_CONTENT_TYPE:
            g_value_set_string (value, kvssink->content_type);
            break;
        case PROP_MAX_LATENCY:
            g_value_set_uint (value, kvssink->max_latency_seconds);
            break;
        case PROP_FRAGMENT_DURATION:
            g_value_set_uint (value, kvssink->fragment_duration_miliseconds);
            break;
        case PROP_TIMECODE_SCALE:
            g_value_set_uint (value, kvssink->timecode_scale_milliseconds);
            break;
        case PROP_KEY_FRAME_FRAGMENTATION:
            g_value_set_boolean (value, kvssink->key_frame_fragmentation);
            break;
        case PROP_FRAME_TIMECODES:
            g_value_set_boolean (value, kvssink->frame_timecodes);
            break;
        case PROP_ABSOLUTE_FRAGMENT_TIMES:
            g_value_set_boolean (value, kvssink->absolute_fragment_times);
            break;
        case PROP_FRAGMENT_ACKS:
            g_value_set_boolean (value, kvssink->fragment_acks);
            break;
        case PROP_RESTART_ON_ERROR:
            g_value_set_boolean (value, kvssink->restart_on_error);
            break;
        case PROP_RECALCULATE_METRICS:
            g_value_set_boolean (value, kvssink->recalculate_metrics);
            break;
        case PROP_AVG_BANDWIDTH_BPS:
            g_value_set_uint (value, kvssink->avg_bandwidth_bps);
            break;
        case PROP_BUFFER_DURATION:
            g_value_set_uint (value, kvssink->buffer_duration_seconds);
            break;
        case PROP_REPLAY_DURATION:
            g_value_set_uint (value, kvssink->replay_duration_seconds);
            break;
        case PROP_CONNECTION_STALENESS:
            g_value_set_uint (value, kvssink->connection_staleness_seconds);
            break;
        case PROP_CODEC_ID:
            g_value_set_string (value, kvssink->codec_id);
            break;
        case PROP_TRACK_NAME:
            g_value_set_string (value, kvssink->track_name);
            break;
        case PROP_ACCESS_KEY:
            g_value_set_string (value, kvssink->access_key);
            break;
        case PROP_SECRET_KEY:
            g_value_set_string (value, kvssink->secret_key);
            break;
        case PROP_AWS_REGION:
            g_value_set_string (value, kvssink->aws_region);
            break;
        case PROP_ROTATION_PERIOD:
            g_value_set_uint (value, kvssink->rotation_period);
            break;
        case PROP_LOG_CONFIG_PATH:
            g_value_set_string (value, kvssink->log_config_path);
            break;
        case PROP_FRAMERATE:
            g_value_set_uint (value, kvssink->framerate);
            break;
        case PROP_STORAGE_SIZE:
            g_value_set_uint (value, kvssink->storage_size);
            break;
        case PROP_CREDENTIAL_FILE_PATH:
            g_value_set_string (value, kvssink->credential_file_path);
            break;
        case PROP_IOT_CERTIFICATE:
            gst_value_set_structure (value, kvssink->iot_certificate);
            break;
        case PROP_STREAM_TAGS:
            gst_value_set_structure (value, kvssink->stream_tags);
            break;
        case PROP_FILE_START_TIME:
            g_value_set_ulong (value, kvssink->file_start_time);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean
gst_kvs_sink_handle_sink_event (GstCollectPads *pads,
                                GstCollectData *track_data, GstEvent * event, gpointer user_data) {
    GstKvsSink *kvssink = GST_KVS_SINK(user_data);
    auto data = kvssink->data;
    GstKvsSinkTrackData *kvs_sink_track_data = (GstKvsSinkTrackData *) track_data;
    gboolean ret = TRUE;
    GstCaps *gstcaps = NULL;
    string err_msg;
    uint64_t track_id = kvs_sink_track_data->track_id;

    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_CAPS: {
            gst_event_parse_caps(event, &gstcaps);
            GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);
            GST_INFO ("structure is %" GST_PTR_FORMAT, gststructforcaps);
            if (data->track_cpd.count(track_id) == 0 && gst_structure_has_field(gststructforcaps, "codec_data")) {
                const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
                gchar *cpd = gst_value_serialize(gstStreamFormat);
                string cpd_str = string(cpd);
                data->track_cpd[track_id] = cpd_str;
                g_free(cpd);

                // Send cpd to kinesis video stream and mark kvs stream ready to consume frames.
                if (!kinesis_video_stream_start(data, cpd_str, track_id)) {
                    GST_ELEMENT_ERROR(kvssink, STREAM, FAILED, (NULL), ("Failed to start stream"));
                    ret = false;
                    goto CleanUp;
                }
                data->stream_ready = true;
            }
            gst_event_unref (event);
            event = NULL;
            break;
        }
        case GST_EVENT_CUSTOM_DOWNSTREAM: {
            const GstStructure *structure = gst_event_get_structure(event);
            std::string metadata_name, metadata_value;
            gboolean persistent;
            bool is_persist;

            if (!gst_structure_has_name(structure, KVS_ADD_METADATA_G_STRUCT_NAME)) {
                goto CleanUp;
            }

            LOG_INFO("received kvs-add-metadata event");
            if (NULL == gst_structure_get_string(structure, KVS_ADD_METADATA_NAME) ||
                NULL == gst_structure_get_string(structure, KVS_ADD_METADATA_VALUE) ||
                FALSE == gst_structure_get_boolean(structure, KVS_ADD_METADATA_PERSISTENT, &persistent)) {

                LOG_WARN("Event structure contains invalid field: " << std::string(gst_structure_to_string (structure)));
                goto CleanUp;
            }

            metadata_name = std::string(gst_structure_get_string(structure, KVS_ADD_METADATA_NAME));
            metadata_value = std::string(gst_structure_get_string(structure, KVS_ADD_METADATA_VALUE));
            is_persist = persistent;

            bool result = data->kinesis_video_stream->putFragmentMetadata(metadata_name, metadata_value, is_persist);
            if (false == result) {
                LOG_WARN("Failed to putFragmentMetadata. name: " << metadata_name << ", value: " << metadata_value << ", persistent: " << is_persist);
            }
            gst_event_unref (event);
            event = NULL;
            break;
        }
        default:
            break;
    }

CleanUp:
    if (event != NULL) {
        gst_collect_pads_event_default(pads, track_data, event, FALSE);
    }

    return ret;
}

void recreate_kvs_stream(GstKvsSink *kvssink) {
    auto data = kvssink->data;

    data->stream_ready = false;

    string err_msg;
    // retry forever until success
    bool succeeded;
    do {
        succeeded = kinesis_video_stream_init(kvssink, err_msg);
        if (succeeded) {
            for(map<uint64_t, string>::iterator it = data->track_cpd.begin(); it != data->track_cpd.end(); it++) {
                if (!kinesis_video_stream_start(data, it->second, it->first)) {
                    LOG_DEBUG("Failed to start stream");
                    succeeded = false;
                    data->kinesis_video_producer->freeStream(data->kinesis_video_stream);
                    break;
                }
            }
        }
    } while (!succeeded);

    data->stream_ready = true;
}


void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *frame_data, size_t len, uint64_t track_id) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 0; // with audio, frame can get as close as 0.01ms
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(frame_data);
    frame->index = 0;
    frame->trackId = static_cast<UINT64>(track_id);
}

bool
put_frame(shared_ptr<KinesisVideoStream> kinesis_video_stream, void *frame_data, size_t len, const nanoseconds &pts,
          const nanoseconds &dts, FRAME_FLAGS flags, uint64_t track_id) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, frame_data, len, track_id);
    return kinesis_video_stream->putFrame(frame);
}

static GstFlowReturn
gst_kvs_sink_handle_buffer (GstCollectPads * pads,
                            GstCollectData * track_data, GstBuffer * buf, gpointer user_data) {
    GstKvsSink *kvssink = GST_KVS_SINK(user_data);
    GstFlowReturn ret = GST_FLOW_OK;
    GstKvsSinkTrackData *kvs_sink_track_data = (GstKvsSinkTrackData *) track_data;
    auto data = kvssink->data;
    string err_msg;
    bool isDroppable;
    STATUS stream_status = data->stream_status.load();
    GstMessage *message;
    bool isKey, delta;
    uint64_t track_id;
    FRAME_FLAGS kinesis_video_flags = FRAME_FLAG_NONE;
    size_t buffer_size;

    // eos reached
    if (buf == NULL || track_data == NULL) {
        data->kinesis_video_stream->stopSync();
        LOG_INFO("Sending eos");

        // send out eos message to gstreamer bus
        message = gst_message_new_eos (GST_OBJECT_CAST (kvssink));
        gst_element_post_message (GST_ELEMENT_CAST (kvssink), message);

        ret = GST_FLOW_EOS;
        goto CleanUp;
    }

    if (!data->stream_ready.load()) {
        goto CleanUp;
    }

    if (STATUS_FAILED(stream_status)) {
        if (IS_RETRIABLE_ERROR(stream_status)) {

            data->kinesis_video_producer->freeStream(data->kinesis_video_stream);

            // reset status so we dont create any more recreate stream threads. Subsequent frame will be drop because
            // stream_ready is false.
            data->stream_status = STATUS_SUCCESS;

            // Use another thread to create and start new stream so that we dont block gstreamer pipeline.
            // The process could take some time because recreate_kvs_stream would retry a few times.
            std::thread worker(recreate_kvs_stream, kvssink);
            worker.detach();
            goto CleanUp;
        }

        // received unrecoverable error
        GST_ELEMENT_ERROR (kvssink, STREAM, FAILED, (NULL),
                           ("Stream error occurred. Status: 0x%08x", stream_status));
        ret = GST_FLOW_ERROR;
        goto CleanUp;
    }

    isDroppable =   GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_CORRUPTED) ||
                    GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DECODE_ONLY) ||
                    (GST_BUFFER_FLAGS(buf) == GST_BUFFER_FLAG_DISCONT) ||
                    (!GST_BUFFER_PTS_IS_VALID(buf)); //frame with invalid pts cannot be processed.

    if (isDroppable) {
        goto CleanUp;
    }

    // In offline mode, if user specifies a file_start_time, the stream will be configured to use absolute
    // timestamp. Therefore in here we add the file_start_time to frame pts to create absolute timestamp.
    // If user did not specify file_start_time, file_start_time will be 0 and has no effect.
    if (IS_OFFLINE_STREAMING_MODE(kvssink->streaming_type)) {
        buf->dts = data->last_dts + DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
        buf->pts += data->pts_base;
    } else if (!GST_BUFFER_DTS_IS_VALID(buf)) {
        buf->dts = data->last_dts + DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
    }

    data->last_dts = buf->dts;
    track_id = kvs_sink_track_data->track_id;

    buffer_size = gst_buffer_get_size(buf);
    if (buffer_size > kvssink->current_frame_data_size) {
        delete [] kvssink->frame_data;
        kvssink->current_frame_data_size = (size_t) (buffer_size * FRAME_DATA_ALLOCATION_MULTIPLIER);
        kvssink->frame_data = new uint8_t[kvssink->current_frame_data_size];
    }
    gst_buffer_extract(buf, 0, kvssink->frame_data, buffer_size);

    delta = GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);

    switch (data->media_type) {
        case AUDIO_ONLY:
        case VIDEO_ONLY:
            if (!delta) {
                kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
            }
            break;
        case AUDIO_VIDEO:
            if(!delta && kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_VIDEO) {
                if (data->first_video_frame) {
                    data->first_video_frame = false;
                } else {
                    kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
                }
            }
            break;
    }

    try {
        if (!put_frame(data->kinesis_video_stream, kvssink->frame_data, buffer_size,
                       std::chrono::nanoseconds(buf->pts),
                       std::chrono::nanoseconds(buf->dts), kinesis_video_flags, track_id)) {
            g_printerr("Failed to put frame into Kinesis Video Stream buffer\n");
        }
    } catch (runtime_error &err) {
        err_msg = err.what();
        GST_ELEMENT_ERROR (kvssink, STREAM, FAILED, (NULL),
                           ("put_frame threw an exception. %s", err_msg.c_str()));
        ret = GST_FLOW_ERROR;
        goto CleanUp;
    }

CleanUp:
    if (buf != NULL) {
        gst_buffer_unref (buf);
    }

    return ret;
}

/**
 * Collectpad function for comparing buffers.
 * If return 0, both buffers are considered equally old.
 * If return -1, first buffer is considered older
 * If return 1, second buffer is considered older
 * Older buffer go first.
 */
static gint gst_kvs_sink_collect_compare (GstCollectPads *pads,
                                          GstCollectData *data1,
                                          GstClockTime timestamp1,
                                          GstCollectData *data2,
                                          GstClockTime timestamp2,
                                          gpointer user_data) {
    GstKvsSink *kvssink = GST_KVS_SINK(user_data);
    gint ret = 0;
    GstKvsSinkTrackData *kvs_sink_track_data1 = (GstKvsSinkTrackData *) data1;
    GstKvsSinkTrackData *kvs_sink_track_data2 = (GstKvsSinkTrackData *) data2;

    // If two buffer timestamps' mkv timecode are equal, and they are from different tracks, let video go first.
    // This prevents fragment x's last audio frame timecode being equal to fragment x+1's first video frame timecode.
    if (GST_TIME_AS_MSECONDS(timestamp1) == GST_TIME_AS_MSECONDS(timestamp2) &&
        kvs_sink_track_data1->track_type != kvs_sink_track_data2->track_type) {
        ret = kvs_sink_track_data1->track_type == MKV_TRACK_INFO_TYPE_VIDEO ? -1 : 1;
        goto CleanUp;
    }

    if (timestamp1 > timestamp2) {
        ret = 1;
    } else if (timestamp1 < timestamp2) {
        ret = -1;
    }

CleanUp:
    return ret;
}

static GstPad *
gst_kvs_sink_request_new_pad (GstElement * element, GstPadTemplate * templ,
                                    const gchar * req_name, const GstCaps * caps)
{
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
    GstKvsSink* kvssink = GST_KVS_SINK(element);

    gchar *name = NULL;
    GstPad *newpad = NULL;
    const gchar *pad_name = NULL;
    MKV_TRACK_INFO_TYPE track_type = MKV_TRACK_INFO_TYPE_VIDEO;
    gboolean locked = TRUE;
    GstKvsSinkTrackData *kvs_sink_track_data;

    if (req_name != NULL) {
        GST_WARNING_OBJECT (kvssink, "Custom pad name not supported");
    }

    // Check if the pad template is supported
    if (templ == gst_element_class_get_pad_template (klass, "audio_%u")) {
        if (kvssink->num_audio_streams == 1) {
            GST_ERROR_OBJECT (kvssink, "Can not have more than one audio stream.");
            goto CleanUp;
        }

        name = g_strdup_printf ("audio_%u", kvssink->num_audio_streams++);
        pad_name = name;
        track_type = MKV_TRACK_INFO_TYPE_AUDIO;

    } else if (templ == gst_element_class_get_pad_template (klass, "video_%u")) {
        if (kvssink->num_video_streams == 1) {
            GST_ERROR_OBJECT (kvssink, "Can not have more than one video stream.");
            goto CleanUp;
        }

        name = g_strdup_printf ("video_%u", kvssink->num_video_streams++);
        pad_name = name;
        track_type = MKV_TRACK_INFO_TYPE_VIDEO;

    } else {
        GST_WARNING_OBJECT (kvssink, "This is not our template!");
        goto CleanUp;
    }

    if (kvssink->num_video_streams > 0 && kvssink->num_audio_streams > 0) {
        kvssink->data->media_type = AUDIO_VIDEO;
    } else if (kvssink->num_video_streams > 0) {
        kvssink->data->media_type = VIDEO_ONLY;
    } else {
        kvssink->data->media_type = AUDIO_ONLY;
    }

    newpad = GST_PAD_CAST (g_object_new (GST_TYPE_PAD,
                                          "name", pad_name, "direction", templ->direction, "template", templ,
                                          NULL));

    kvs_sink_track_data = (GstKvsSinkTrackData *)
            gst_collect_pads_add_pad (kvssink->collect, GST_PAD (newpad),
                                      sizeof (GstKvsSinkTrackData),
                                      NULL, locked);
    kvs_sink_track_data->kvssink = kvssink;
    kvs_sink_track_data->track_type = track_type;
    kvs_sink_track_data->track_id = KVS_SINK_DEFAULT_TRACKID;

    if (!gst_element_add_pad (element, GST_PAD (newpad))) {
        gst_object_unref (newpad);
        newpad = NULL;
        GST_WARNING_OBJECT (kvssink, "Adding the new pad '%s' failed", pad_name);
        goto CleanUp;
    }

    kvssink->num_streams++;

    GST_INFO("Added new request pad");

CleanUp:
    g_free (name);
    return newpad;
}

static void
gst_kvs_sink_release_pad (GstElement *element, GstPad *pad) {
    GstKvsSink *kvssink = GST_KVS_SINK (GST_PAD_PARENT (pad));
    GSList *walk;

    // when a pad is released, check whether it's audio or video and keep track of the stream count
    for (walk = kvssink->collect->data; walk; walk = g_slist_next (walk)) {
        GstCollectData *c_data;
        c_data = (GstCollectData *) walk->data;

        if (c_data->pad == pad) {
            GstKvsSinkTrackData *kvs_sink_track_data;
            kvs_sink_track_data = (GstKvsSinkTrackData *) walk->data;
            if (kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_VIDEO) {
                kvssink->num_video_streams--;
            } else if (kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_AUDIO) {
                kvssink->num_audio_streams--;
            }
        }
    }

    gst_collect_pads_remove_pad (kvssink->collect, pad);
    if (gst_element_remove_pad (element, pad)) {
        kvssink->num_streams--;
    }
}

static void
assign_track_id(GstKvsSink *kvssink) {
    GSList *walk;

    if (kvssink->data->media_type == AUDIO_VIDEO) {
        for (walk = kvssink->collect->data; walk; walk = g_slist_next (walk)) {
            GstKvsSinkTrackData *kvs_sink_track_data;
            kvs_sink_track_data = (GstKvsSinkTrackData *) walk->data;

            // set up track id in kvs_sink_track_data
            kvs_sink_track_data->track_id = kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_AUDIO ?
                                            KVS_SINK_DEFAULT_AUDIO_TRACKID :
                                            KVS_SINK_DEFAULT_TRACKID;
        }
    }
}

static GstStateChangeReturn
gst_kvs_sink_change_state(GstElement *element, GstStateChange transition) {
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstKvsSink *kvssink = GST_KVS_SINK (element);
    auto data = kvssink->data;
    string err_msg = "";

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            log4cplus::initialize();
            log4cplus::PropertyConfigurator::doConfigure(kvssink->log_config_path);
            try {
                kinesis_video_producer_init(kvssink);
            } catch (runtime_error &err) {
                ostringstream oss;
                oss << "Failed to init kvs producer. Error: " << err.what();
                err_msg = oss.str();
                ret = GST_STATE_CHANGE_FAILURE;
                GST_ELEMENT_ERROR (kvssink, LIBRARY, INIT, (NULL), ("%s", err_msg.c_str()));
                goto CleanUp;
            }

            if (kvssink->data->media_type == AUDIO_ONLY) {
                GST_ELEMENT_ERROR (kvssink, STREAM, FAILED, (NULL), ("Audio only is not supported yet"));
                ret = GST_STATE_CHANGE_FAILURE;
                goto CleanUp;
            }

            assign_track_id(kvssink);

            if (false == kinesis_video_stream_init(kvssink, err_msg)) {
                GST_ELEMENT_ERROR (kvssink, LIBRARY, INIT, (NULL), ("%s", err_msg.c_str()));
                ret = GST_STATE_CHANGE_FAILURE;
                goto CleanUp;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_collect_pads_start (kvssink->collect);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_collect_pads_stop (kvssink->collect);
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
            break;
        default:
            break;
    }

CleanUp:
    return ret;
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
