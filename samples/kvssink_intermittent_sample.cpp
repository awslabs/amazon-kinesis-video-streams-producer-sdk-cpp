#include <gst/gst.h>
#include <glib.h>

using namespace std;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;


typedef enum _StreamSource {
    TEST_SOURCE,
    DEVICE_SOURCE,
    RTSP_SOURCE
} StreamSource;

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}


static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}



int main (int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *video_convert, *source_filter, *encoder, *h264parse, *kvssink;
    GstCaps *source_caps;

    GstBus *bus;
    guint bus_watch_id;

    StreamSource source_type;

    /* GStreamer Initialisation */
    gst_init (&argc, &argv);

    loop = g_main_loop_new (NULL, FALSE);

    /* Check input arguments */
    if (argc > 2) {
        g_printerr ("Usage: %s <testsrc, devicesrc, or rtspsrc (default is testsrc)>\n", argv[0]);
        return -1;
    }

    if(argc > 1) {
        if(0 == STRCMPI(argv[1], "tesetesrc")) {
            source_type = TEST_SOURCE;
        } else if (0 == STRCMPI(argv[1], "devicesrc")) {
            source_type = DEVICE_SOURCE;
        } else if (0 == STRCMPI(argv[1], "rtspsrc")) {
            source_type = RTSP_SOURCE;
        }
    } else {
        source_type = TEST_SOURCE;
    }

    /* Create gstreamer elements */
    pipeline = gst_pipeline_new ("kvs-pipeline");

    if(source_type == TEST_SOURCE) {
        source   = gst_element_factory_make ("videotestsrc", "test-source");
    } else if (source_type == DEVICE_SOURCE) {
        source   = gst_element_factory_make ("autovideosrc", "device-source");
    } else { // RTSP_SOURCE
    }

    video_convert = gst_element_factory_make("videoconvert", "video_convert");

    source_filter = gst_element_factory_make("capsfilter", "source-filter");
    source_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);
    g_object_set(G_OBJECT(source_filter), "caps", source_caps, NULL);

    encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(G_OBJECT(encoder), "bframes", 0, "tune", "zero-latency", NULL);

    h264parse = gst_element_factory_make("h264parse", "h264parse");

    kvssink = gst_element_factory_make("kvssink", "kvssink")
    
    if (!kvssink) {
        LOG_ERROR("Failed to create kvssink element");
        return -1;
    }

    if (!pipeline || !source || !video_convert || !source_filter || !encoder || !h264parse) {
        g_printerr ("Not all GStreamer elements could be created.\n");
        return -1;
    }

    /* Set up the pipeline */

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    /* we add all elements into the pipeline */
    /* file-source | ogg-demuxer | vorbis-decoder | converter | alsa-output */
    gst_bin_add_many (GST_BIN (pipeline),
                        source, demuxer, decoder, conv, sink, NULL);

    /* we link the elements together */
    /* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -> alsa-output */
    gst_element_link (source, demuxer);
    gst_element_link_many (decoder, conv, sink, NULL);
    g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), decoder);

    /* note that the demuxer will be linked to the decoder dynamically.
        The reason is that Ogg may contain various streams (for example
        audio and video). The source pad(s) will be created at run time,
        by the demuxer when it detects the amount and nature of streams.
        Therefore we connect a callback function which will be executed
        when the "pad-added" is emitted.*/


    /* Set the pipeline to "playing" state*/
    g_print ("Now playing: %s\n", argv[1]);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);


    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);

    return 0;
}