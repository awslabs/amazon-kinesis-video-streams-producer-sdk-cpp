#include <gst/gst.h>
#include <cstring>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include <gst/rtsp/gstrtsptransport.h>

using namespace std;
using namespace com::amazonaws::kinesis::video;

#ifdef __cplusplus
extern "C" {
#endif

int gstreamer_init(int, char **);

#ifdef __cplusplus
}
#endif

#define ACCESS_KEY_ENV_VAR "AWS_ACCESS_KEY_ID"
#define SECRET_KEY_ENV_VAR "AWS_SECRET_ACCESS_KEY"
#define SESSION_TOKEN_ENV_VAR "AWS_SESSION_TOKEN"
#define DEFAULT_REGION_ENV_VAR "AWS_DEFAULT_REGION"
#define MAX_URL_LENGTH 65536

typedef struct _CustomData {
    GstElement *pipeline, *source, *depay, *kvsproducer, *filter, *target_for_source;
    GstBus *bus;
    GMainLoop *main_loop;
    bool h264_stream_supported;
} CustomData;


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

/* callback when each RTSP stream has been created */
static void cb_rtsp_pad_created(GstElement *element, GstPad *pad, CustomData *data) {
    gchar *pad_name = gst_pad_get_name(pad);
    g_print("New RTSP source found: %s\n", pad_name);
    if (gst_element_link_pads(data->source, pad_name, data->target_for_source, "sink")) {
        g_print("Source linked.\n");
    } else {
        g_print("Source link FAILED\n");
    }
    g_free(pad_name);

    const char *name;
    char *padCapsName;
    GstCaps *padCaps = gst_pad_get_current_caps(pad);
    GstStructure *gststructureforcaps = gst_caps_get_structure(padCaps, 0);
    GST_INFO ("structure is %"
                      GST_PTR_FORMAT, gststructureforcaps);
    name = gst_structure_get_name(gststructureforcaps);
    g_debug("name of caps struct string: %s", name);
    padCapsName = gst_caps_to_string(padCaps);
    g_debug("name of rtsp caps string: %s", padCapsName);
    if (gst_structure_has_field(gststructureforcaps, "a-framerate")) {
        gint numerator, denominator;
        gst_structure_get_fraction(gststructureforcaps, "a-framerate", &numerator, &denominator);
        g_debug("frame rate range %d / %d", numerator, denominator);
    }
    g_free(padCapsName);

}

int gstreamer_init(int argc, char *argv[]) {

    CustomData data;
    GstStateChangeReturn ret;

    char rtsp_url[MAX_URL_LENGTH];
    SNPRINTF(rtsp_url, MAX_URL_LENGTH, "%s", argv[1]);

    char stream_name[MAX_STREAM_NAME_LEN];
    SNPRINTF(stream_name, MAX_STREAM_NAME_LEN, "%s", argv[2]);

    char protocol[4];
    SNPRINTF(protocol, 4, "%s", argv[3]);

    /* init data struct */
    memset(&data, 0, sizeof(data));

    /* init GStreamer */
    gst_init(&argc, &argv);


    /* create the elements */
    // kvssink element
    data.kvsproducer = gst_element_factory_make("kvssink", "kvsproducer");

    // rtph264depay: Converts rtp data to raw h264
    data.depay = gst_element_factory_make("rtph264depay", "depay");

    // RTSP source component
    data.source = gst_element_factory_make("rtspsrc", "source");

    data.filter = gst_element_factory_make("capsfilter", "encoder_filter");
    //h264 format avc with au
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    g_object_set(G_OBJECT (data.filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    /* create an rtspsource to kvssink pipeline */
    data.pipeline = gst_pipeline_new("rtsp-kinesis-pipeline");

    if (!data.pipeline || !data.source || !data.depay || !data.filter || !data.kvsproducer) {
        if (!data.filter) {
            g_printerr("capsfilter  element could not be created.\n");
        }
        if (!data.depay) {
            g_printerr("h264depay element could not be created.\n");
        }
        if (!data.source) {
            g_printerr("h264parse elements could not be created.\n");
        }
        if (!data.kvsproducer) {
            g_printerr("kvsproducersink element could not be created.\n");
        }
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    g_object_set(G_OBJECT (data.kvsproducer), "stream-name", stream_name, "storage-size", 512, NULL);


    g_object_set(G_OBJECT (data.source),
                 "location", rtsp_url,
                 "short-header", true, // Necessary for target camera
                 "protocols", strcmp(protocol, "udp") == 0 ? GST_RTSP_LOWER_TRANS_UDP : GST_RTSP_LOWER_TRANS_TCP,
                 NULL);

    g_signal_connect(data.source, "pad-added", G_CALLBACK(cb_rtsp_pad_created), &data);

    /* build the pipeline */
    gst_bin_add_many(GST_BIN (data.pipeline), data.source,
                     data.depay, data.filter, data.kvsproducer,
                     NULL);

    if (gst_element_link_many(data.depay, data.filter,
                              data.kvsproducer,
                              NULL) != TRUE) {

        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return 1;
    }

    data.target_for_source = data.depay;


    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
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

