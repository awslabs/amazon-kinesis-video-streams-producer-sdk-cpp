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
#include <IotCertCredentialProvider.h>
#include "Util/KvsSinkUtil.h"

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace std;
using namespace std::chrono;
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
#define DEFAULT_MAX_LATENCY_SECONDS 60
#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES TRUE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_ALLOW_CREATE_STREAM TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_DISABLE_BUFFER_CLIPPING FALSE
#define DEFAULT_USE_ORIGINAL_PTS FALSE
#define DEFAULT_ENABLE_METRICS FALSE
#define DEFAULT_STREAM_FRAMERATE 25
#define DEFAULT_STREAM_FRAMERATE_HIGH_DENSITY 100
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
#define DEFAULT_BUFFER_DURATION_SECONDS 120
#define DEFAULT_REPLAY_DURATION_SECONDS 40
#define DEFAULT_CONNECTION_STALENESS_SECONDS 60
#define DEFAULT_CODEC_ID_H264 "V_MPEG4/ISO/AVC"
#define DEFAULT_CODEC_ID_H265 "V_MPEGH/ISO/HEVC"
#define DEFAULT_TRACKNAME "kinesis_video"
#define DEFAULT_ACCESS_KEY "access_key"
#define DEFAULT_SECRET_KEY "secret_key"
#define DEFAULT_SESSION_TOKEN "session_token"
#define DEFAULT_REGION "us-west-2"
#define DEFAULT_ROTATION_PERIOD_SECONDS 3600
#define DEFAULT_LOG_FILE_PATH "../kvs_log_configuration"
#define DEFAULT_STORAGE_SIZE_MB 128
#define DEFAULT_STOP_STREAM_TIMEOUT_SEC 120
#define DEFAULT_SERVICE_CONNECTION_TIMEOUT_SEC 5
#define DEFAULT_SERVICE_COMPLETION_TIMEOUT_SEC 10
#define DEFAULT_IOT_CONNECTION_TIMEOUT_SEC 3
#define DEFAULT_IOT_COMPLETION_TIMEOUT_SEC 5
#define DEFAULT_CREDENTIAL_FILE_PATH ".kvs/credential"
#define DEFAULT_FRAME_DURATION_MS 2

#define KVS_ADD_METADATA_G_STRUCT_NAME "kvs-add-metadata"
#define KVS_ADD_METADATA_NAME "name"
#define KVS_ADD_METADATA_VALUE "value"
#define KVS_ADD_METADATA_PERSISTENT "persist"
#define KVS_CLIENT_USER_AGENT_NAME "AWS-SDK-KVS-CPP-CLIENT"

#define DEFAULT_AUDIO_TRACK_NAME "audio"
#define DEFAULT_AUDIO_CODEC_ID_AAC "A_AAC"
#define DEFAULT_AUDIO_CODEC_ID_PCM "A_MS/ACM"
#define KVS_SINK_DEFAULT_TRACKID 1
#define KVS_SINK_DEFAULT_AUDIO_TRACKID 2

#define GSTREAMER_MEDIA_TYPE_H265       "video/x-h265"
#define GSTREAMER_MEDIA_TYPE_H264       "video/x-h264"
#define GSTREAMER_MEDIA_TYPE_AAC        "audio/mpeg"
#define GSTREAMER_MEDIA_TYPE_MULAW      "audio/x-mulaw"
#define GSTREAMER_MEDIA_TYPE_ALAW       "audio/x-alaw"

#define MAX_GSTREAMER_MEDIA_TYPE_LEN    16

namespace KvsSinkSignals {
    guint err_signal_id;
    guint ack_signal_id;
    guint metric_signal_id;
};

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
    PROP_SESSION_TOKEN,
    PROP_AWS_REGION,
    PROP_ROTATION_PERIOD,
    PROP_LOG_CONFIG_PATH,
    PROP_STORAGE_SIZE,
    PROP_STOP_STREAM_TIMEOUT,
    PROP_SERVICE_CONNECTION_TIMEOUT,
    PROP_SERVICE_COMPLETION_TIMEOUT,
    PROP_CREDENTIAL_FILE_PATH,
    PROP_IOT_CERTIFICATE,
    PROP_STREAM_TAGS,
    PROP_FILE_START_TIME,
    PROP_DISABLE_BUFFER_CLIPPING,
    PROP_USE_ORIGINAL_PTS,
    PROP_GET_METRICS,
    PROP_ALLOW_CREATE_STREAM,
    PROP_USER_AGENT_NAME
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
                                         "audio/mpeg, mpegversion = (int) { 2, 4 }, stream-format = (string) raw, channels = (int) [ 1, MAX ], rate = (int) [ 1, MAX ] ; " \
                                         "audio/x-alaw, channels = (int) { 1, 2 }, rate = (int) [ 8000, 192000 ] ; " \
                                         "audio/x-mulaw, channels = (int) { 1, 2 }, rate = (int) [ 8000, 192000 ] ; "
                                 )
        );

static GstStaticPadTemplate videosink_templ =
        GST_STATIC_PAD_TEMPLATE ("video_%u",
                                 GST_PAD_SINK,
                                 GST_PAD_REQUEST,
                                 GST_STATIC_CAPS (
                                         "video/x-h264, stream-format = (string) avc, alignment = (string) au, width = (int) [ 16, MAX ], height = (int) [ 16, MAX ] ; " \
                                         "video/x-h265, alignment = (string) au, width = (int) [ 16, MAX ], height = (int) [ 16, MAX ] ;"
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

/* Request pad callback */
static GstPad* gst_kvs_sink_request_new_pad (GstElement *element, GstPadTemplate *templ,
                                             const gchar* name, const GstCaps *caps);
static void gst_kvs_sink_release_pad (GstElement *element, GstPad *pad);

void closed(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle) {
    LOG_INFO("Closed connection with stream handle "<<stream_handle<<" and upload handle "<<upload_handle);
}
void kinesis_video_producer_init(GstKvsSink *kvssink)
{
    auto data = kvssink->data;
    unique_ptr<DeviceInfoProvider> device_info_provider(new KvsSinkDeviceInfoProvider(kvssink->storage_size,
                                                        kvssink->stop_stream_timeout,
                                                        kvssink->service_connection_timeout,
                                                        kvssink->service_completion_timeout));
    unique_ptr<ClientCallbackProvider> client_callback_provider(new KvsSinkClientCallbackProvider());
    unique_ptr<StreamCallbackProvider> stream_callback_provider(new KvsSinkStreamCallbackProvider(data));

    kvssink->data->kvs_sink = kvssink;

    char const *access_key;
    char const *secret_key;
    char const *session_token;
    char const *default_region;
    char const *control_plane_uri;
    string access_key_str;
    string secret_key_str;
    string session_token_str;
    string region_str;
    string control_plane_uri_str = "";
    bool credential_is_static = true;

    // This needs to happen after we've read in ALL of the properties
    if (!kvssink->disable_buffer_clipping) {
        gst_collect_pads_set_clip_function(kvssink->collect,
                                           GST_DEBUG_FUNCPTR(gst_collect_pads_clip_running_time), kvssink);
    }

    kvssink->data->kvs_sink = kvssink;

    if (0 == strcmp(kvssink->access_key, DEFAULT_ACCESS_KEY)) { // if no static credential is available in plugin property.
        if (nullptr == (access_key = getenv(ACCESS_KEY_ENV_VAR))
            || nullptr == (secret_key = getenv(SECRET_KEY_ENV_VAR))) { // if no static credential is available in env var.
            credential_is_static = false; // No static credential available.
            access_key_str = "";
            secret_key_str = "";
        } else {
            access_key_str = string(access_key);
            secret_key_str = string(secret_key);
        }

    } else {
        access_key_str = string(kvssink->access_key);
        secret_key_str = string(kvssink->secret_key);
    }

    // Handle session token seperately, since this is optional with long term credentials
    if (0 == strcmp(kvssink->session_token, DEFAULT_SESSION_TOKEN)) {
        session_token_str = "";
        if (nullptr != (session_token = getenv(SESSION_TOKEN_ENV_VAR))) {
            LOG_INFO("Setting session token from env for " << kvssink->stream_name);
            session_token_str = string(session_token);
        }
    } else {
        LOG_INFO("Setting session token from config for " << kvssink->stream_name);
        session_token_str = string(kvssink->session_token);
    }

    if (nullptr == (default_region = getenv(DEFAULT_REGION_ENV_VAR))) {
        region_str = string(kvssink->aws_region);
    } else {
        region_str = string(default_region); // Use env var if both property and env var are available.
    }

    unique_ptr<CredentialProvider> credential_provider;

    if (kvssink->iot_certificate) {
        LOG_INFO("Using iot credential provider within KVS sink for " << kvssink->stream_name);
        std::map<std::string, std::string> iot_cert_params;
        uint64_t iot_connection_timeout = DEFAULT_IOT_CONNECTION_TIMEOUT_SEC * HUNDREDS_OF_NANOS_IN_A_SECOND;
        uint64_t iot_completion_timeout = DEFAULT_IOT_COMPLETION_TIMEOUT_SEC * HUNDREDS_OF_NANOS_IN_A_SECOND;
        if (!kvs_sink_util::parseIotCredentialGstructure(kvssink->iot_certificate, iot_cert_params)){
            LOG_AND_THROW("Failed to parse Iot credentials for " << kvssink->stream_name);
        }
        std::map<std::string, std::string>::iterator it = iot_cert_params.find(IOT_THING_NAME);
        if (it == iot_cert_params.end()) {
            iot_cert_params.insert( std::pair<std::string,std::string>(IOT_THING_NAME, kvssink->stream_name) );
        }

        if (!iot_cert_params[IOT_CONNECTION_TIMEOUT].empty()) {
            iot_connection_timeout = std::stoull(iot_cert_params[IOT_CONNECTION_TIMEOUT]) * HUNDREDS_OF_NANOS_IN_A_SECOND;
        }

        if (!iot_cert_params[IOT_COMPLETION_TIMEOUT].empty()) {
            iot_completion_timeout = std::stoull(iot_cert_params[IOT_COMPLETION_TIMEOUT]) * HUNDREDS_OF_NANOS_IN_A_SECOND;
        }

        credential_provider.reset(new IotCertCredentialProvider(iot_cert_params[IOT_GET_CREDENTIAL_ENDPOINT],
                                                                iot_cert_params[CERTIFICATE_PATH],
                                                                iot_cert_params[PRIVATE_KEY_PATH],
                                                                iot_cert_params[ROLE_ALIASES],
                                                                iot_cert_params[CA_CERT_PATH],
                                                                iot_cert_params[IOT_THING_NAME],
                                                                iot_connection_timeout,
                                                                iot_completion_timeout));
    } else if (credential_is_static) {
        kvssink->credentials_.reset(new Credentials(access_key_str,
                                                    secret_key_str,
                                                    session_token_str,
                                                    std::chrono::seconds(DEFAULT_ROTATION_PERIOD_SECONDS)));
        credential_provider.reset(new StaticCredentialProvider(*kvssink->credentials_));
    } else {
        credential_provider.reset(new RotatingCredentialProvider(kvssink->credential_file_path));
    }

    // Handle env for providing CP URL
    if (nullptr != (control_plane_uri = getenv(CONTROL_PLANE_URI_ENV_VAR))) {
        LOG_INFO("Getting URL from env for " << kvssink->stream_name);
        control_plane_uri_str = string(control_plane_uri);
    }
    LOG_INFO("User agent string: " << kvssink->user_agent);
    data->kinesis_video_producer = KinesisVideoProducer::createSync(std::move(device_info_provider),
                                                                    std::move(client_callback_provider),
                                                                    std::move(stream_callback_provider),
                                                                    std::move(credential_provider),
                                                                    API_CALL_CACHE_TYPE_ALL,
                                                                    region_str,
                                                                    control_plane_uri_str,
                                                                    kvssink->user_agent);
}

void create_kinesis_video_stream(GstKvsSink *kvssink) {
    auto data = kvssink->data;

    map<string, string> *p_stream_tags = nullptr;
    map<string, string> stream_tags;
    if (kvssink->stream_tags) {
        gboolean ret;
        ret = kvs_sink_util::gstructToMap(kvssink->stream_tags, &stream_tags);
        if (!ret) {
            LOG_WARN("Failed to parse stream tags for " << kvssink->stream_name);
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
        data->pts_base = (uint64_t) duration_cast<nanoseconds>(seconds(kvssink->file_start_time)).count();
    }

    switch (data->media_type) {
        case AUDIO_VIDEO:
            kvssink->framerate = MAX(kvssink->framerate, DEFAULT_STREAM_FRAMERATE_HIGH_DENSITY);
            break;
        case AUDIO_ONLY:
            g_free(kvssink->codec_id);
            kvssink->codec_id = kvssink->audio_codec_id;
            g_free(kvssink->track_name);
            kvssink->track_name = g_strdup(DEFAULT_AUDIO_TRACK_NAME);
            kvssink->track_info_type = MKV_TRACK_INFO_TYPE_AUDIO;
            kvssink->key_frame_fragmentation = FALSE;
            kvssink->framerate = MAX(kvssink->framerate, DEFAULT_STREAM_FRAMERATE_HIGH_DENSITY);
            break;
        case VIDEO_ONLY:
            // no-op because default setup is for video only
            break;
    }

    unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(kvssink->stream_name,
            hours(kvssink->retention_period_hours),
            p_stream_tags,
            kvssink->kms_key_id,
            kvssink->streaming_type,
            kvssink->content_type,
            duration_cast<milliseconds> (seconds(kvssink->max_latency_seconds)),
            milliseconds(kvssink->fragment_duration_miliseconds),
            milliseconds(kvssink->timecode_scale_milliseconds),
            kvssink->key_frame_fragmentation,// Construct a fragment at each key frame
            kvssink->frame_timecodes,// Use provided frame timecode
            kvssink->absolute_fragment_times,// Relative timecode
            kvssink->fragment_acks,// Ack on fragment is enabled
            kvssink->restart_on_error,// SDK will restart when error happens
            kvssink->recalculate_metrics,// recalculate_metrics
            kvssink->allow_create_stream, // allow stream creation if stream does not exist
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
            KVS_SINK_DEFAULT_TRACKID));

    if (data->media_type == AUDIO_VIDEO) {
        stream_definition->addTrack(KVS_SINK_DEFAULT_AUDIO_TRACKID, DEFAULT_AUDIO_TRACK_NAME, kvssink->audio_codec_id, MKV_TRACK_INFO_TYPE_AUDIO);
        // Need to reorder frames to avoid fragment overlap error.
        stream_definition->setFrameOrderMode(FRAME_ORDERING_MODE_MULTI_TRACK_AV_COMPARE_PTS_ONE_MS_COMPENSATE_EOFR);
    }

    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(std::move(stream_definition));
    data->frame_count = 0;
    cout << "Stream is ready" << endl;
}

bool kinesis_video_stream_init(GstKvsSink *kvssink, string &err_msg) {
    bool ret = true;
    int retry_count = DEFAULT_TOTAL_RETRY;
    int retry_delay = DEFAULT_INITIAL_RETRY_DELAY_MS;
    bool do_retry = true;
    while(do_retry) {
        try {
            LOG_INFO("Try creating stream for " << kvssink->stream_name);
            // stream is freed when createStreamSync fails
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

static void
gst_kvs_sink_class_init(GstKvsSinkClass *klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstKvsSinkClass *basesink_class = (GstKvsSinkClass *) klass;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);

    gobject_class->set_property = gst_kvs_sink_set_property;
    gobject_class->get_property = gst_kvs_sink_get_property;
    gobject_class->finalize = gst_kvs_sink_finalize;

    g_object_class_install_property (gobject_class, PROP_STREAM_NAME,
                                     g_param_spec_string ("stream-name", "Stream Name",
                                                          "Name of the destination stream", DEFAULT_STREAM_NAME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_USER_AGENT_NAME,
                                     g_param_spec_string ("user-agent", "Custom user agent name",
                                                          "Name of the user agent", KVS_CLIENT_USER_AGENT_NAME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_RETENTION_PERIOD,
                                     g_param_spec_uint ("retention-period", "Retention Period",
                                                       "Length of time stream is preserved. Unit: hours", 0, G_MAXUINT, DEFAULT_RETENTION_PERIOD_HOURS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STREAMING_TYPE,
                                     g_param_spec_enum ("streaming-type", "Streaming Type",
                                                        "Streaming type", GST_TYPE_KVS_SINK_STREAMING_TYPE, DEFAULT_STREAMING_TYPE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
                                     g_param_spec_string ("content-type", "Content Type",
                                                          "content type", MKV_H264_CONTENT_TYPE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

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
                                                          "Codec ID", DEFAULT_CODEC_ID_H264, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_TRACK_NAME,
                                     g_param_spec_string ("track-name", "Track name",
                                                          "Track name", DEFAULT_TRACKNAME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_ACCESS_KEY,
                                     g_param_spec_string ("access-key", "Access Key",
                                                          "AWS Access Key", DEFAULT_ACCESS_KEY, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SECRET_KEY,
                                     g_param_spec_string ("secret-key", "Secret Key",
                                                          "AWS Secret Key", DEFAULT_SECRET_KEY, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SESSION_TOKEN,
                                     g_param_spec_string ("session-token", "Session token",
                                                          "AWS Session token", DEFAULT_SESSION_TOKEN, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

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

    g_object_class_install_property (gobject_class, PROP_STOP_STREAM_TIMEOUT,
                                     g_param_spec_uint ("stop-stream-timeout", "Stop stream timeout",
                                                        "Stop stream timeout: seconds", 0, G_MAXUINT, DEFAULT_STOP_STREAM_TIMEOUT_SEC, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SERVICE_CONNECTION_TIMEOUT,
                                     g_param_spec_uint ("connection-timeout", "Service call connection timeout",
                                                        "Service call connection timeout: seconds", 0, G_MAXUINT, DEFAULT_SERVICE_CONNECTION_TIMEOUT_SEC, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SERVICE_COMPLETION_TIMEOUT,
                                     g_param_spec_uint ("completion-timeout", "Service call completion timeout",
                                                        "Service call completion timeout: seconds. Should be more than connection timeout. If it isnt, SDK will override with defaults", 0, G_MAXUINT, DEFAULT_SERVICE_COMPLETION_TIMEOUT_SEC, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

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
                                     g_param_spec_uint64 ("file-start-time", "File Start Time",
                                                        "Epoch time that the file starts in kinesis video stream. By default, current time is used. Unit: Seconds",
                                                         0, G_MAXULONG, 0, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_DISABLE_BUFFER_CLIPPING,
                                     g_param_spec_boolean ("disable-buffer-clipping", "Disable Buffer Clipping",
                                                           "Set to true only if your src/mux elements produce GST_CLOCK_TIME_NONE for segment start times.  It is non-standard behavior to set this to true, only use if there are known issues with your src/mux segment start/stop times.", DEFAULT_DISABLE_BUFFER_CLIPPING,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_USE_ORIGINAL_PTS,
                                     g_param_spec_boolean ("use-original-pts", "Use Original PTS",
                                                           "Set to true only if you want to use the original presentation time stamp on the buffer and that timestamp is expected to be a valid epoch value in nanoseconds. Most encoders will not have a valid PTS", DEFAULT_USE_ORIGINAL_PTS,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_GET_METRICS,
                                     g_param_spec_boolean ("get-kvs-metrics", "Get client and stream level metrics on every key frame",
                                                           "Set to true if you want to read on the producer streamMetrics and clientMetrics object every key frame. Disabled by default", DEFAULT_ENABLE_METRICS,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_ALLOW_CREATE_STREAM,
                                     g_param_spec_boolean ("allow-create-stream", "Allow creating stream if stream does not exist",
                                                           "Set to true if allowing create stream call, false otherwise", DEFAULT_ALLOW_CREATE_STREAM,
                                                           (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

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

    KvsSinkSignals::err_signal_id = g_signal_new("stream-error", G_TYPE_FROM_CLASS(gobject_class), (GSignalFlags)(G_SIGNAL_RUN_LAST), G_STRUCT_OFFSET (GstKvsSinkClass, sink_stream_error), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT64);
    KvsSinkSignals::ack_signal_id = g_signal_new("fragment-ack", G_TYPE_FROM_CLASS(gobject_class),
                                               (GSignalFlags)(G_SIGNAL_ACTION), G_STRUCT_OFFSET (GstKvsSinkClass, sink_fragment_ack),
                                               NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
    KvsSinkSignals::metric_signal_id = g_signal_new("stream-client-metric", G_TYPE_FROM_CLASS(gobject_class),
                                               (GSignalFlags)(G_SIGNAL_ACTION), G_STRUCT_OFFSET (GstKvsSinkClass, sink_stream_metric),
                                               NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
gst_kvs_sink_init(GstKvsSink *kvssink) {
    kvssink->collect = gst_collect_pads_new();
    gst_collect_pads_set_buffer_function (kvssink->collect,
                                          GST_DEBUG_FUNCPTR (gst_kvs_sink_handle_buffer), kvssink);
    gst_collect_pads_set_event_function (kvssink->collect,
                                         GST_DEBUG_FUNCPTR (gst_kvs_sink_handle_sink_event), kvssink);

    kvssink->num_streams = 0;
    kvssink->num_audio_streams = 0;
    kvssink->num_video_streams = 0;

    // Stream definition
    kvssink->stream_name = g_strdup (DEFAULT_STREAM_NAME);
    kvssink->user_agent = g_strdup(KVS_CLIENT_USER_AGENT_NAME "/" KVSSINK_USER_AGENT_POSTFIX_VERSION);
    kvssink->retention_period_hours = DEFAULT_RETENTION_PERIOD_HOURS;
    kvssink->kms_key_id = g_strdup (DEFAULT_KMS_KEY_ID);
    kvssink->streaming_type = DEFAULT_STREAMING_TYPE;
    kvssink->max_latency_seconds = DEFAULT_MAX_LATENCY_SECONDS;
    kvssink->fragment_duration_miliseconds = DEFAULT_FRAGMENT_DURATION_MILLISECONDS;
    kvssink->timecode_scale_milliseconds = DEFAULT_TIMECODE_SCALE_MILLISECONDS;
    kvssink->key_frame_fragmentation = DEFAULT_KEY_FRAME_FRAGMENTATION;
    kvssink->frame_timecodes = DEFAULT_FRAME_TIMECODES;
    kvssink->absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;
    kvssink->fragment_acks = DEFAULT_FRAGMENT_ACKS;
    kvssink->restart_on_error = DEFAULT_RESTART_ON_ERROR;
    kvssink->recalculate_metrics = DEFAULT_RECALCULATE_METRICS;
    kvssink->allow_create_stream = DEFAULT_ALLOW_CREATE_STREAM;
    kvssink->framerate = DEFAULT_STREAM_FRAMERATE;
    kvssink->avg_bandwidth_bps = DEFAULT_AVG_BANDWIDTH_BPS;
    kvssink->buffer_duration_seconds = DEFAULT_BUFFER_DURATION_SECONDS;
    kvssink->replay_duration_seconds = DEFAULT_REPLAY_DURATION_SECONDS;
    kvssink->connection_staleness_seconds = DEFAULT_CONNECTION_STALENESS_SECONDS;
    kvssink->disable_buffer_clipping = DEFAULT_DISABLE_BUFFER_CLIPPING;
    kvssink->codec_id = g_strdup (DEFAULT_CODEC_ID_H264);
    kvssink->track_name = g_strdup (DEFAULT_TRACKNAME);
    kvssink->access_key = g_strdup (DEFAULT_ACCESS_KEY);
    kvssink->secret_key = g_strdup (DEFAULT_SECRET_KEY);
    kvssink->session_token = g_strdup(DEFAULT_SESSION_TOKEN);
    kvssink->aws_region = g_strdup (DEFAULT_REGION);
    kvssink->rotation_period = DEFAULT_ROTATION_PERIOD_SECONDS;
    kvssink->log_config_path = g_strdup (DEFAULT_LOG_FILE_PATH);
    kvssink->storage_size = DEFAULT_STORAGE_SIZE_MB;
    kvssink->stop_stream_timeout = DEFAULT_STOP_STREAM_TIMEOUT_SEC;
    kvssink->service_connection_timeout = DEFAULT_SERVICE_CONNECTION_TIMEOUT_SEC;
    kvssink->service_completion_timeout = DEFAULT_SERVICE_COMPLETION_TIMEOUT_SEC;
    kvssink->credential_file_path = g_strdup (DEFAULT_CREDENTIAL_FILE_PATH);
    kvssink->file_start_time = (uint64_t) chrono::duration_cast<seconds>(
            systemCurrentTime().time_since_epoch()).count();
    kvssink->track_info_type = MKV_TRACK_INFO_TYPE_VIDEO;
    kvssink->audio_codec_id = g_strdup (DEFAULT_AUDIO_CODEC_ID_AAC);

    kvssink->data = make_shared<KvsSinkCustomData>();
    kvssink->data->err_signal_id = KvsSinkSignals::err_signal_id;
    kvssink->data->ack_signal_id = KvsSinkSignals::ack_signal_id;
    kvssink->data->metric_signal_id = KvsSinkSignals::metric_signal_id;

    // Mark plugin as sink
    GST_OBJECT_FLAG_SET (kvssink, GST_ELEMENT_FLAG_SINK);

    LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");
    LOG_CONFIGURE_STDOUT("DEBUG");
}

static void
gst_kvs_sink_finalize(GObject *object) {
    GstKvsSink *kvssink = GST_KVS_SINK (object);
    auto data = kvssink->data;

    gst_object_unref(kvssink->collect);
    g_free(kvssink->stream_name);
    g_free(kvssink->user_agent);
    g_free(kvssink->content_type);
    g_free(kvssink->codec_id);
    g_free(kvssink->track_name);
    g_free(kvssink->secret_key);
    g_free(kvssink->access_key);
    g_free(kvssink->session_token);
    g_free(kvssink->aws_region);
    g_free(kvssink->audio_codec_id);
    g_free(kvssink->kms_key_id);
    g_free(kvssink->log_config_path);
    g_free(kvssink->credential_file_path);

    if (kvssink->iot_certificate) {
        gst_structure_free (kvssink->iot_certificate);
    }
    if (kvssink->stream_tags) {
        gst_structure_free (kvssink->stream_tags);
    }
    if (data->kinesis_video_producer) {
        data->kinesis_video_producer.reset();
    }
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
        case PROP_USER_AGENT_NAME:
            g_free(kvssink->user_agent);
            kvssink->user_agent = g_strdup (g_value_get_string (value));
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
        case PROP_SESSION_TOKEN:
            g_free(kvssink->session_token);
            kvssink->session_token = g_strdup (g_value_get_string (value));
            break;
        case PROP_AWS_REGION:
            g_free(kvssink->aws_region);
            kvssink->aws_region = g_strdup (g_value_get_string (value));
            break;
        case PROP_ROTATION_PERIOD:
            kvssink->rotation_period = g_value_get_uint (value);
            break;
        case PROP_LOG_CONFIG_PATH:
            g_free(kvssink->log_config_path);
            kvssink->log_config_path = g_strdup (g_value_get_string (value));
            break;
        case PROP_FRAMERATE:
            kvssink->framerate = g_value_get_uint (value);
            break;
        case PROP_STORAGE_SIZE:
            kvssink->storage_size = g_value_get_uint (value);
            break;
        case PROP_STOP_STREAM_TIMEOUT:
            kvssink->stop_stream_timeout = g_value_get_uint (value);
            break;

        case PROP_SERVICE_CONNECTION_TIMEOUT:
            kvssink->service_connection_timeout = g_value_get_uint (value);
            break;

        case PROP_SERVICE_COMPLETION_TIMEOUT:
            kvssink->service_completion_timeout = g_value_get_uint (value);
            break;

        case PROP_CREDENTIAL_FILE_PATH:
            g_free(kvssink->credential_file_path);
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
            kvssink->file_start_time = g_value_get_uint64 (value);
            break;
        case PROP_DISABLE_BUFFER_CLIPPING:
            kvssink->disable_buffer_clipping = g_value_get_boolean(value);
            break;
        case PROP_USE_ORIGINAL_PTS:
            kvssink->data->use_original_pts = g_value_get_boolean(value);
            break;
        case PROP_GET_METRICS:
            kvssink->data->get_metrics = g_value_get_boolean(value);
            break;
        case PROP_ALLOW_CREATE_STREAM:
            kvssink->allow_create_stream = g_value_get_boolean(value);
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
        case PROP_USER_AGENT_NAME:
            g_value_set_string (value, kvssink->user_agent);
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
        case PROP_SESSION_TOKEN:
            g_value_set_string (value, kvssink->session_token);
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
        case PROP_STOP_STREAM_TIMEOUT:
            g_value_set_uint (value, kvssink->stop_stream_timeout);
            break;

        case PROP_SERVICE_CONNECTION_TIMEOUT:
            g_value_set_uint (value, kvssink->service_connection_timeout);
            break;

        case PROP_SERVICE_COMPLETION_TIMEOUT:
            g_value_set_uint (value, kvssink->service_completion_timeout);
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
            g_value_set_uint64 (value, kvssink->file_start_time);
            break;
        case PROP_DISABLE_BUFFER_CLIPPING:
            g_value_set_boolean (value, kvssink->disable_buffer_clipping);
            break;
        case PROP_USE_ORIGINAL_PTS:
            g_value_set_boolean (value, kvssink->data->use_original_pts);
            break;
        case PROP_GET_METRICS:
            g_value_set_boolean (value, kvssink->data->get_metrics);
            break;
        case PROP_ALLOW_CREATE_STREAM:
            g_value_set_boolean (value, kvssink->allow_create_stream);
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

    gint samplerate = 0, channels = 0;
    const gchar *media_type;

    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_CAPS: {
            gst_event_parse_caps(event, &gstcaps);
            GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);
            GST_INFO ("structure is %" GST_PTR_FORMAT, gststructforcaps);
            media_type = gst_structure_get_name (gststructforcaps);

            if (!strcmp (media_type, GSTREAMER_MEDIA_TYPE_ALAW) || !strcmp (media_type, GSTREAMER_MEDIA_TYPE_MULAW)) {
                guint8 codec_private_data[KVS_PCM_CPD_SIZE_BYTE];
                KVS_PCM_FORMAT_CODE format = KVS_PCM_FORMAT_CODE_MULAW;

                gst_structure_get_int (gststructforcaps, "rate", &samplerate);
                gst_structure_get_int (gststructforcaps, "channels", &channels);

                if (samplerate == 0 || channels == 0) {
                    GST_ERROR_OBJECT (kvssink, "Missing channels/samplerate on caps");
                    ret = FALSE;
                    goto CleanUp;
                }

                if (!strcmp (media_type, GSTREAMER_MEDIA_TYPE_ALAW)) {
                    format = KVS_PCM_FORMAT_CODE_ALAW;
                } else {
                    format = KVS_PCM_FORMAT_CODE_MULAW;
                }

                if (mkvgenGeneratePcmCpd(format, (UINT32) samplerate, (UINT16) channels, (PBYTE) codec_private_data, KVS_PCM_CPD_SIZE_BYTE)) {
                    GST_ERROR_OBJECT (kvssink, "Failed to generate pcm cpd");
                    ret = FALSE;
                    goto CleanUp;
                }

                // Send cpd to kinesis video stream
                ret = data->kinesis_video_stream->start(codec_private_data, KVS_PCM_CPD_SIZE_BYTE, track_id);

            } else if (data->track_cpd_received.count(track_id) == 0 && gst_structure_has_field(gststructforcaps, "codec_data")) {
                const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
                gchar *cpd = gst_value_serialize(gstStreamFormat);
                string cpd_str = string(cpd);
                data->track_cpd_received.insert(track_id);
                g_free(cpd);

                // Send cpd to kinesis video stream
                ret = data->kinesis_video_stream->start(cpd_str, track_id);
            }

            gst_event_unref (event);
            event = NULL;

            if (!ret) {
                GST_ELEMENT_ERROR(kvssink, STREAM, FAILED, (NULL), ("Failed to start stream"));
            }

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

            LOG_INFO("received kvs-add-metadata event for " << kvssink->stream_name);
            if (NULL == gst_structure_get_string(structure, KVS_ADD_METADATA_NAME) ||
                NULL == gst_structure_get_string(structure, KVS_ADD_METADATA_VALUE) ||
                !gst_structure_get_boolean(structure, KVS_ADD_METADATA_PERSISTENT, &persistent)) {

                LOG_WARN("Event structure contains invalid field: " << std::string(gst_structure_to_string (structure)) << " for " << kvssink->stream_name);
                goto CleanUp;
            }

            metadata_name = std::string(gst_structure_get_string(structure, KVS_ADD_METADATA_NAME));
            metadata_value = std::string(gst_structure_get_string(structure, KVS_ADD_METADATA_VALUE));
            is_persist = persistent;

            bool result = data->kinesis_video_stream->putFragmentMetadata(metadata_name, metadata_value, is_persist);
            if (!result) {
                LOG_WARN("Failed to putFragmentMetadata. name: " << metadata_name << ", value: " << metadata_value << ", persistent: " << is_persist << " for " << kvssink->stream_name);
            }
            gst_event_unref (event);
            event = NULL;
            break;
        }
        case GST_EVENT_EOS: {
            LOG_INFO("EOS Event received in sink for " << kvssink->stream_name);
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

void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *frame_data, size_t len, uint64_t track_id, uint32_t index) {
    frame->flags = flags;
    frame->index = index;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 0; // with audio, frame can get as close as 0.01ms
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(frame_data);
    frame->trackId = static_cast<UINT64>(track_id);
}

bool put_frame(shared_ptr<KvsSinkCustomData> data, void *frame_data, size_t len, const nanoseconds &pts,
          const nanoseconds &dts, FRAME_FLAGS flags, uint64_t track_id, uint32_t index) {

    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, frame_data, len, track_id, index);
    bool ret = data->kinesis_video_stream->putFrame(frame);
    if (data->get_metrics && ret) {
        if (CHECK_FRAME_FLAG_KEY_FRAME(flags)  || data->on_first_frame) {
            KvsSinkMetric *kvs_sink_metric = new KvsSinkMetric();
            kvs_sink_metric->stream_metrics = data->kinesis_video_stream->getMetrics();
            kvs_sink_metric->client_metrics = data->kinesis_video_producer->getMetrics();
            kvs_sink_metric->frame_pts = frame.presentationTs;
            kvs_sink_metric->on_first_frame = data->on_first_frame;
            data->on_first_frame = false;
            g_signal_emit(G_OBJECT(data->kvs_sink), data->metric_signal_id, 0, kvs_sink_metric);
            delete kvs_sink_metric;
        }
    }
    return ret;
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
    bool delta;
    uint64_t track_id;
    FRAME_FLAGS kinesis_video_flags = FRAME_FLAG_NONE;
    GstMapInfo info;

    info.data = NULL;
    // eos reached
    if (buf == NULL && track_data == NULL) {
        LOG_INFO("Received event for " << kvssink->stream_name);
        // Need this check in case pipeline is already being set to NULL and
        // stream  is being or/already stopped. Although stopSync() is an idempotent call,
        // we want to avoid an extra call. It is not possible for this callback to be invoked
        // after stopSync() since we stop collecting on pads before invoking. But having this
        // check anyways in case it happens
        if (!data->streamingStopped.load()) {
            data->kinesis_video_stream->stopSync();
            data->streamingStopped.store(true);
            LOG_INFO("Sending eos for " << kvssink->stream_name);
        }

        // send out eos message to gstreamer bus
        message = gst_message_new_eos (GST_OBJECT_CAST (kvssink));
        gst_element_post_message (GST_ELEMENT_CAST (kvssink), message);

        ret = GST_FLOW_EOS;
        goto CleanUp;
    }

    if (STATUS_FAILED(stream_status)) {
        // in offline case, we cant tell the pipeline to restream the file again in case of network outage.
        // therefore error out and let higher level application do the retry.
        if (IS_OFFLINE_STREAMING_MODE(kvssink->streaming_type) || !IS_RETRIABLE_ERROR(stream_status)) {
            // fatal cases
            GST_ELEMENT_ERROR (kvssink, STREAM, FAILED, (NULL),
                           ("[%s] Stream error occurred. Status: 0x%08x", kvssink->stream_name, stream_status));
            ret = GST_FLOW_ERROR;
            goto CleanUp;
        } else {
            // resetStream, note that this will flush out producer buffer
            data->kinesis_video_stream->resetStream();
            // reset state
            data->stream_status = STATUS_SUCCESS;
        }
    }

    if (buf != NULL) {
        isDroppable =   GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_CORRUPTED) ||
                        GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DECODE_ONLY) ||
                        (GST_BUFFER_FLAGS(buf) == GST_BUFFER_FLAG_DISCONT) ||
                        (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                        // drop if buffer contains header and has invalid timestamp
                        (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_HEADER) && (!GST_BUFFER_PTS_IS_VALID(buf) || !GST_BUFFER_DTS_IS_VALID(buf)));
        if (isDroppable) {
            LOG_DEBUG("Dropping frame with flag: " << GST_BUFFER_FLAGS(buf) << " for " << kvssink->stream_name);
            goto CleanUp;
        }

        // In offline mode, if user specifies a file_start_time, the stream will be configured to use absolute
        // timestamp. Therefore in here we add the file_start_time to frame pts to create absolute timestamp.
        // If user did not specify file_start_time, file_start_time will be 0 and has no effect.
        if (IS_OFFLINE_STREAMING_MODE(kvssink->streaming_type)) {
            if (!data->use_original_pts) {
                buf->dts = 0; // if offline mode, i.e. streaming a file, the dts from gstreamer is undefined.
                buf->pts += data->pts_base;
            } else {
                buf->pts = buf->dts;
            }
        } else if (!GST_BUFFER_DTS_IS_VALID(buf)) {
            buf->dts = data->last_dts + DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
        }

        data->last_dts = buf->dts;
        track_id = kvs_sink_track_data->track_id;

        if (!gst_buffer_map(buf, &info, GST_MAP_READ)){
            goto CleanUp;
        }

        delta = GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);

        switch (data->media_type) {
            case AUDIO_ONLY:
            case VIDEO_ONLY:
                if (!delta) {
                    kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
                }
                break;
            case AUDIO_VIDEO:
                if (!delta && kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_VIDEO) {
                    if (data->first_video_frame) {
                        data->first_video_frame = false;
                    }
                    kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
                }
                break;
        }
        if (!IS_OFFLINE_STREAMING_MODE(kvssink->streaming_type)) {
            if (data->first_pts == GST_CLOCK_TIME_NONE) {
                data->first_pts = buf->pts;
            }

            if (data->producer_start_time == GST_CLOCK_TIME_NONE) {
                data->producer_start_time = (uint64_t) chrono::duration_cast<nanoseconds>(
                        systemCurrentTime().time_since_epoch()).count();
            }

            if (!data->use_original_pts) {
                buf->pts += data->producer_start_time - data->first_pts;
            } else {
                buf->pts = buf->dts;
            }
        }

        put_frame(kvssink->data, info.data, info.size,
                  std::chrono::nanoseconds(buf->pts),
                  std::chrono::nanoseconds(buf->dts), kinesis_video_flags, track_id, data->frame_count);
        data->frame_count++;
    } else {
        LOG_WARN("GStreamer buffer is invalid for " << kvssink->stream_name);
    }

CleanUp:
    if (info.data != NULL) {
        gst_buffer_unmap(buf, &info);
    }

    if (buf != NULL) {
        gst_buffer_unref (buf);
    }

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
init_track_data(GstKvsSink *kvssink) {
    GSList *walk;
    GstCaps *caps;
    gchar *video_content_type = NULL, *audio_content_type = NULL;
    const gchar *media_type;

    for (walk = kvssink->collect->data; walk != NULL; walk = g_slist_next (walk)) {
        GstKvsSinkTrackData *kvs_sink_track_data = (GstKvsSinkTrackData *) walk->data;

        if (kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_VIDEO) {

            if (kvssink->data->media_type == AUDIO_VIDEO) {
                kvs_sink_track_data->track_id = KVS_SINK_DEFAULT_TRACKID;
            }

            GstCollectData *collect_data = (GstCollectData *) walk->data;

            // extract media type from GstCaps to check whether it's h264 or h265
            caps = gst_pad_get_allowed_caps(collect_data->pad);
            media_type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
            if (strncmp(media_type, GSTREAMER_MEDIA_TYPE_H264, MAX_GSTREAMER_MEDIA_TYPE_LEN) == 0) {
                // default codec id is for h264 video.
                video_content_type = g_strdup(MKV_H264_CONTENT_TYPE);
            } else if (strncmp(media_type, GSTREAMER_MEDIA_TYPE_H265, MAX_GSTREAMER_MEDIA_TYPE_LEN) == 0) {
                g_free(kvssink->codec_id);
                kvssink->codec_id = g_strdup(DEFAULT_CODEC_ID_H265);
                video_content_type = g_strdup(MKV_H265_CONTENT_TYPE);
            } else {
                // no-op, should result in a caps negotiation error before getting here.
                LOG_AND_THROW("Error, media type " << media_type << "not accepted by kvssink" << " for " << kvssink->stream_name);
            }
            gst_caps_unref(caps);

        } else if (kvs_sink_track_data->track_type == MKV_TRACK_INFO_TYPE_AUDIO) {

            if (kvssink->data->media_type == AUDIO_VIDEO) {
                kvs_sink_track_data->track_id = KVS_SINK_DEFAULT_AUDIO_TRACKID;
            }

            GstCollectData *collect_data = (GstCollectData *) walk->data;

            // extract media type from GstCaps to check whether it's h264 or h265
            caps = gst_pad_get_allowed_caps(collect_data->pad);
            media_type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
            if (strncmp(media_type, GSTREAMER_MEDIA_TYPE_AAC, MAX_GSTREAMER_MEDIA_TYPE_LEN) == 0) {
                // default codec id is for aac audio.
                audio_content_type = g_strdup(MKV_AAC_CONTENT_TYPE);
            } else if (strncmp(media_type, GSTREAMER_MEDIA_TYPE_ALAW, MAX_GSTREAMER_MEDIA_TYPE_LEN) == 0) {
                g_free(kvssink->audio_codec_id);
                kvssink->audio_codec_id = g_strdup(DEFAULT_AUDIO_CODEC_ID_PCM);
                audio_content_type = g_strdup(MKV_ALAW_CONTENT_TYPE);
            } else if (strncmp(media_type, GSTREAMER_MEDIA_TYPE_MULAW, MAX_GSTREAMER_MEDIA_TYPE_LEN) == 0) {
                g_free(kvssink->audio_codec_id);
                kvssink->audio_codec_id = g_strdup(DEFAULT_AUDIO_CODEC_ID_PCM);
                audio_content_type = g_strdup(MKV_MULAW_CONTENT_TYPE);
            } else {
                // no-op, should result in a caps negotiation error before getting here.
                LOG_AND_THROW("Error, media type " << media_type << "not accepted by kvssink");
            }
            gst_caps_unref(caps);
        }
    }

    switch (kvssink->data->media_type) {
        case AUDIO_VIDEO:
            if (video_content_type != NULL && audio_content_type != NULL) {
                kvssink->content_type = g_strjoin(",", video_content_type, audio_content_type, NULL);
            }
            break;
        case AUDIO_ONLY:
            if (audio_content_type != NULL) {
                kvssink->content_type = g_strdup(audio_content_type);
            }
            break;
        case VIDEO_ONLY:
            if (video_content_type != NULL) {
                kvssink->content_type = g_strdup(video_content_type);
            }
            break;
    }

    g_free(video_content_type);
    g_free(audio_content_type);
}

static GstStateChangeReturn
gst_kvs_sink_change_state(GstElement *element, GstStateChange transition) {
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstKvsSink *kvssink = GST_KVS_SINK (element);
    auto data = kvssink->data;
    string err_msg = "";
    ostringstream oss;

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (kvssink->log_config_path != NULL) {
                log4cplus::initialize();
                log4cplus::PropertyConfigurator::doConfigure(kvssink->log_config_path);
                LOG_INFO("Logger config being used: " << kvssink->log_config_path);
            } else {
                LOG_INFO("Logger already configured...skipping");
            }

            try {
                kinesis_video_producer_init(kvssink);
                init_track_data(kvssink);
                kvssink->data->first_pts = GST_CLOCK_TIME_NONE;
                kvssink->data->producer_start_time = GST_CLOCK_TIME_NONE;

            } catch (runtime_error &err) {
                oss << "Failed to init kvs producer. Error: " << err.what();
                err_msg = oss.str();
                ret = GST_STATE_CHANGE_FAILURE;
                goto CleanUp;
            }

            if (!kinesis_video_stream_init(kvssink, err_msg)) {
                ret = GST_STATE_CHANGE_FAILURE;
                goto CleanUp;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_collect_pads_start (kvssink->collect);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            LOG_INFO("Stopping kvssink for " << kvssink->stream_name);
            gst_collect_pads_stop (kvssink->collect);

            // Need this check in case an EOS was received in the buffer handler and
            // stream was already stopped. Although stopSync() is an idempotent call,
            // we want to avoid an extra call
            if (!data->streamingStopped.load()) {
                data->kinesis_video_stream->stopSync();
                data->streamingStopped.store(true);
            } else {
                LOG_INFO("Streaming already stopped for " << kvssink->stream_name);
            }
            LOG_INFO("Stopped kvssink for " << kvssink->stream_name);
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            LOG_INFO("Pipeline state changed to NULL in kvssink");
            break;
        default:
            break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state(element, transition);

CleanUp:

    if (ret != GST_STATE_CHANGE_SUCCESS) {
        GST_ELEMENT_ERROR (kvssink, LIBRARY, INIT, (NULL), ("%s", err_msg.c_str()));
    }

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