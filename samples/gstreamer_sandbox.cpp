#include <gst/gst.h>
#include <string.h>
#include <chrono>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include <vector>
#include <stdlib.h>
#include <mutex>
#include <IotCertCredentialProvider.h>
#include "gstreamer/gstkvssink.h"
#include <thread>
#include "include.h"

using namespace log4cplus;

LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");



typedef struct _CustomData
{
    gboolean is_live;
    GstElement *pipeline;
    GMainLoop *loop;
} CustomData;

static void cb_message(GstBus *bus, GstMessage *msg, CustomData *data)
{

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug;

        gst_message_parse_error(msg, &err, &debug);
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);

        gst_element_set_state(data->pipeline, GST_STATE_READY);
        g_main_loop_quit(data->loop);
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        gst_element_set_state(data->pipeline, GST_STATE_READY);
        g_main_loop_quit(data->loop);
        break;
    case GST_MESSAGE_BUFFERING:
    {
        gint percent = 0;

        /* If the stream is live, we do not care about buffering. */
        if (data->is_live)
            break;

        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering (%3d%%)\r", percent);
        /* Wait until buffering is complete before start/resume playing */
        if (percent < 100)
            gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
        else
            gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
        /* Get a new clock */
        gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
        gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
        break;
    default:
        /* Unhandled message */
        break;
    }
}

void sleeper(CustomData *data) {
    sleep(10);
    LOG_DEBUG("Pausing...");
    gst_element_set_state(gst_bin_get_by_name(GST_BIN(data->pipeline), "source"), GST_STATE_PAUSED);
    // gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
    sleep(10);
    LOG_DEBUG("Playing...");
    gst_element_set_state(gst_bin_get_by_name(GST_BIN(data->pipeline), "source"), GST_STATE_PLAYING);
    // gst_element_set_state(data->pipeline, GST_STATE_PLAYING);


}

int main(int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstStateChangeReturn ret;
    GMainLoop *main_loop;
    CustomData data;


    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Initialize our data structure */
    memset(&data, 0, sizeof(data));

    /* Build the pipeline */
    pipeline = gst_parse_launch("autovideosrc name=source ! videoconvert ! video/x-raw,format=I420 ! x264enc bframes=0 bitrate=1000 tune=zerolatency ! video/x-h264,stream-format=avc,alignment=au,profile=baseline ! decodebin ! videoconvert ! clockoverlay ! autovideosink", NULL);
    // pipeline = gst_parse_launch("autovideosrc name=source ! videoconvert ! video/x-raw,format=I420 ! x264enc bframes=0 bitrate=512 tune=zerolatency sync-lookahead=0 ! video/x-h264,stream-format=avc,alignment=au,profile=baseline ! \
                                    kvssink stream-name=aTestStream storage-size=128", NULL);
    bus = gst_element_get_bus(pipeline);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    else if (ret == GST_STATE_CHANGE_NO_PREROLL)
    {
        data.is_live = TRUE;
    }

    main_loop = g_main_loop_new(NULL, FALSE);
    data.loop = main_loop;
    data.pipeline = pipeline;

    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(cb_message), &data);

    std::thread sleepThread(sleeper, &data);
    
    g_main_loop_run (main_loop);
    
    sleepThread.join();

    /* Free resources */
    g_main_loop_unref(main_loop);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}