#include <gst/gst.h>
#include <cstring>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include "../plugin-src/gstkvssinkenumproperties.h"

using namespace std;
using namespace com::amazonaws::kinesis::video;

#ifdef __cplusplus
extern "C" {
#endif

int gstreamer_init(int, char **);

#ifdef __cplusplus
}
#endif

LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");

#define ACCESS_KEY_ENV_VAR "AWS_ACCESS_KEY_ID"
#define SECRET_KEY_ENV_VAR "AWS_SECRET_ACCESS_KEY"
#define SESSION_TOKEN_ENV_VAR "AWS_SESSION_TOKEN"
#define DEFAULT_REGION_ENV_VAR "AWS_DEFAULT_REGION"

typedef struct _CustomData {
    GstElement *pipeline, *source, *source_filter, *encoder, *filter, *video_convert, *h264parse, *kvsproducer;
    GstBus *bus;
    GMainLoop *main_loop;
    bool h264_stream_supported;
} CustomData;

static bool format_supported_by_source(GstCaps *src_caps, GstCaps *query_caps, int width, int height, int framerate) {
    gst_caps_set_simple(query_caps,
                        "width", G_TYPE_INT, width,
                        "height", G_TYPE_INT, height,
                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                        NULL);
    bool is_match = gst_caps_can_intersect(query_caps, src_caps);

    // in case the camera has fps as 10000000/333333
    if(!is_match) {
        gst_caps_set_simple(query_caps,
                            "framerate", GST_TYPE_FRACTION_RANGE, framerate, 1, framerate+1, 1,
                            NULL);
        is_match = gst_caps_can_intersect(query_caps, src_caps);
    }

    return is_match;
}

static bool resolution_supported(GstCaps *src_caps, GstCaps *query_caps_raw, GstCaps *query_caps_h264,
                                 CustomData &data, int width, int height, int framerate) {
    if (format_supported_by_source(src_caps, query_caps_h264, width, height, framerate)) {
        data.h264_stream_supported = true;
    } else if (format_supported_by_source(src_caps, query_caps_raw, width, height, framerate)) {
        data.h264_stream_supported = false;
    } else {
        return false;
    }
    return true;
}

/* This function is called when an error message is posted on the bus */
static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(data->main_loop);
}

int gstreamer_init(int argc, char *argv[]) {

    CustomData data;
    GstStateChangeReturn ret;
    bool vtenc, isOnRpi = false;
    /* init data struct */
    memset(&data, 0, sizeof(data));

    /* init GStreamer */
    gst_init(&argc, &argv);

    /* init stream format */
    int width = 0, height = 0, framerate = 30, bitrateInKBPS = 512;
    int opt;
    char *endptr;
    while ((opt = getopt(argc, argv, "w:h:f:b:")) != -1) {
        switch (opt) {
            case 'w':
                width = strtol(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    g_printerr("Invalid width value.\n");
                    return 1;
                }
                break;
            case 'h':
                height = strtol(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    g_printerr("Invalid height value.\n");
                    return 1;
                }
                break;
            case 'f':
                framerate = strtol(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    g_printerr("Invalid framerate value.\n");
                    return 1;
                }
                break;
            case 'b':
                bitrateInKBPS = strtol(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    g_printerr("Invalid bitrate value.\n");
                    return 1;
                }
                break;
            default: /* '?' */
                g_printerr("Invalid arguments\n");
                return 1;
        }
    }
    /* create the elements */

    data.source_filter = gst_element_factory_make("capsfilter", "source_filter");
    data.filter = gst_element_factory_make("capsfilter", "encoder_filter");
    data.h264parse = gst_element_factory_make("h264parse", "h264parse"); // required to enforce avc stream format
    data.kvsproducer = gst_element_factory_make("kvssink", "kvsproducer");

    // Attempt to create vtenc encoder (applemedia)
    data.encoder = gst_element_factory_make("vtenc_h264_hw", "encoder");
    if (data.encoder) {
        data.source = gst_element_factory_make("autovideosrc", "source");
        vtenc = true;
    } else {
        data.encoder = gst_element_factory_make("omxh264enc", "encoder");
        if (data.encoder) {
            isOnRpi = true;
        } else {
            // attempt x264enc
            data.encoder = gst_element_factory_make("x264enc", "encoder");
            isOnRpi = false;
        }
        data.source = gst_element_factory_make("v4l2src", "source");
        vtenc = false;
    }

    /* create an empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.pipeline || !data.source || !data.source_filter || !data.encoder || !data.filter
        || !data.kvsproducer || !data.h264parse) {
        if (!data.source_filter || !data.filter) {
            g_printerr("capsfilter  element could not be created.\n");
        }
        if (!data.encoder) {
            g_printerr("Encoder omxh264enc or vtenc_h264_hw or x264enc element could not be created.\n");
        }
        if (!data.h264parse) {
            g_printerr("h264parse elements could not be created.\n");
        }
        if (!data.kvsproducer) {
            g_printerr("kvsproducer sink element could not be created.\n");
        }
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    /* configure source */
    if (!vtenc) {
        g_object_set(G_OBJECT (data.source), "do-timestamp", TRUE, "device", "/dev/video0", NULL);
    }

    g_object_set(G_OBJECT (data.kvsproducer), "stream-name", "plugin-test-stream", "frame-timestamp", KVS_SINK_TIMESTAMP_DEFAULT, "storage-size", 512, NULL);


    /* Determine whether device supports h264 encoding and select a streaming resolution supported by the device*/
    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(data.source, GST_STATE_READY)) {
        g_printerr("Unable to set the source to ready state.\n");
        return 1;
    }

    GstPad *src_pad = gst_element_get_static_pad(data.source, "src");
    GstCaps *src_caps = gst_pad_query_caps(src_pad, NULL);
    gst_element_set_state(data.source, GST_STATE_NULL);

    GstCaps *query_caps_raw = gst_caps_new_simple("video/x-raw",
                                                  "width", G_TYPE_INT, width,
                                                  "height", G_TYPE_INT, height,
                                                  NULL);
    GstCaps *query_caps_h264 = gst_caps_new_simple("video/x-h264",
                                                   "width", G_TYPE_INT, width,
                                                   "height", G_TYPE_INT, height,
                                                   NULL);

    if (width != 0 && height != 0) {
        if (!resolution_supported(src_caps, query_caps_raw, query_caps_h264, data, width, height, framerate)) {
            g_printerr("Resolution %dx%d not supported by video source\n", width, height);
            return 1;
        }
    } else {
        vector<int> res_width = {1920, 1280, 640};
        vector<int> res_height = {1080, 720, 480};
        bool found_resolution = false;
        for (int i = 0; i < res_width.size(); i++) {
            width = res_width[i];
            height = res_height[i];
            if (resolution_supported(src_caps, query_caps_raw, query_caps_h264, data, width, height, framerate)) {
                found_resolution = true;
                break;
            }
        }
        if (!found_resolution) {
            g_printerr("Default list of resolutions (1920x1080, 1280x720, 640x480) are not supported by video source\n");
            return 1;
        }
    }

    gst_caps_unref(src_caps);
    gst_object_unref(src_pad);

    /* create the elemnents needed for the corresponding pipeline */
    if (!data.h264_stream_supported) {
        data.video_convert = gst_element_factory_make("videoconvert", "video_convert");

        if (!data.video_convert) {
            g_printerr("Not all elements could be created.\n");
            return 1;
        }
    }

    /* configure source filter */
    if (!data.h264_stream_supported) {
        gst_caps_set_simple(query_caps_raw,
                            "format", G_TYPE_STRING, "I420",
                            NULL);
        g_object_set(G_OBJECT (data.source_filter), "caps", query_caps_raw, NULL);
    } else {
        gst_caps_set_simple(query_caps_h264,
                            "stream-format", G_TYPE_STRING, "byte-stream",
                            "alignment", G_TYPE_STRING, "au",
                            NULL);
        g_object_set(G_OBJECT (data.source_filter), "caps", query_caps_h264, NULL);
    }
    gst_caps_unref(query_caps_h264);
    gst_caps_unref(query_caps_raw);

    /* configure encoder */
    if (!data.h264_stream_supported) {
        if (vtenc) {
            g_object_set(G_OBJECT (data.encoder), "allow-frame-reordering", FALSE, "realtime", TRUE,
                         "max-keyframe-interval",
                         45, "bitrate", bitrateInKBPS, NULL);
        } else if (isOnRpi) {
            g_object_set(G_OBJECT (data.encoder), "control-rate", 1, "target-bitrate", bitrateInKBPS * 10000, NULL);
        } else {
            g_object_set(G_OBJECT (data.encoder), "bframes", 0, "key-int-max", 45, "bitrate", bitrateInKBPS, NULL);
        }
    }


    /* configure encoder filter for h264 avc format */
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             "width", G_TYPE_INT, width,
                                             "height", G_TYPE_INT, height,
                                             "framerate", GST_TYPE_FRACTION, framerate, 1,
                                             NULL);
    if (!data.h264_stream_supported) {
        gst_caps_set_simple(h264_caps, "profile", G_TYPE_STRING, "baseline",
                            NULL);
    }
    g_object_set(G_OBJECT (data.filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    /* build the pipeline */
    if (!data.h264_stream_supported) {
        gst_bin_add_many(GST_BIN (data.pipeline), data.source, data.video_convert, data.source_filter,
                         data.encoder,
                         data.h264parse, data.filter,
                         data.kvsproducer, NULL);
        if (gst_element_link_many(data.source, data.video_convert, data.source_filter, data.encoder,
                                  data.h264parse,
                                  data.filter, data.kvsproducer, NULL) != TRUE) {
            g_printerr("Elements could not be linked in the pipeline (h264 supported upstream).\n");
            gst_object_unref(data.pipeline);
            return 1;
        }
    } else {
        gst_bin_add_many(GST_BIN (data.pipeline), data.source, data.h264parse, data.filter, data.kvsproducer, NULL);
        if (gst_element_link_many(data.source, data.h264parse, data.filter, data.kvsproducer, NULL) != TRUE) {
            g_printerr("Elements could not be linked in the pipeline \n");
            gst_object_unref(data.pipeline);
            return 1;
        }
    }


    /* Instruct the bus to emit signals for each received message, and connect to the error message signals */
    data.bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(data.bus);
    g_signal_connect (G_OBJECT(data.bus), "message::error", (GCallback) error_cb, &data);
    gst_object_unref(data.bus);

    /* start streaming */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        return 1;
    }

    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

    /* free resources */
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

int main(int argc, char *argv[]) {
    return gstreamer_init(argc, argv);
}
