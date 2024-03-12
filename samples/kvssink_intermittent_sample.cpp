#include <gst/gst.h>
#include <glib.h>

#include "gstreamer/gstkvssink.h"

using namespace std;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);

typedef enum _StreamSource {
    TEST_SOURCE,
    DEVICE_SOURCE,
    RTSP_SOURCE
} StreamSource;

void sigint_handler(int sigint){
    LOG_DEBUG("SIGINT received.  Exiting...");
    
    if(main_loop != NULL){
        g_main_loop_quit(main_loop);
    }
}

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("Received EOS\n");
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

void determine_credentials(GstElement *kvssink, G_TYPE_STRING streamName) {

    char const *iot_credential_endpoint;
    char const *cert_path;
    char const *private_key_path;
    char const *role_alias;
    char const *ca_cert_path;
    char const *credential_path;
    if (nullptr != (iot_credential_endpoint = getenv("IOT_GET_CREDENTIAL_ENDPOINT")) &&
        nullptr != (cert_path = getenv("CERT_PATH")) &&
        nullptr != (private_key_path = getenv("PRIVATE_KEY_PATH")) &&
        nullptr != (role_alias = getenv("ROLE_ALIAS")) &&
        nullptr != (ca_cert_path = getenv("CA_CERT_PATH"))) {
	// set the IoT Credentials if provided in envvar
	GstStructure *iot_credentials =  gst_structure_new(
			"iot-certificate",
			"iot-thing-name", G_TYPE_STRING, streamName, 
			"endpoint", G_TYPE_STRING, iot_credential_endpoint,
			"cert-path", G_TYPE_STRING, cert_path,
			"key-path", G_TYPE_STRING, private_key_path,
			"ca-path", G_TYPE_STRING, ca_cert_path,
			"role-aliases", G_TYPE_STRING, role_alias, NULL);
	
	g_object_set(G_OBJECT (kvssink), "iot-certificate", iot_credentials, NULL);
        gst_structure_free(iot_credentials);
    // kvssink will search for long term credentials in envvar automatically so no need to include here
    // if no long credentials or IoT credentials provided will look for credential file as last resort
    } else if(nullptr != (credential_path = getenv("AWS_CREDENTIAL_PATH"))){
        g_object_set(G_OBJECT (kvssink), "credential-path", credential_path, NULL);
    }
}


int main (int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    GstElement *pipeline, *source, *video_convert, *source_filter, *encoder, *h264parse, *kvssink;
    GstCaps *source_caps;
    GstBus *bus;
    guint bus_watch_id;
    StreamSource source_type;
    char stream_name[MAX_STREAM_NAME_LEN + 1];

    /* GStreamer Initialisation */
    gst_init (&argc, &argv);

    /* Check input arguments */
    if (argc > 3 || argc < 2) {
        g_printerr ("Usage: %s <streamName (required)> <testsrc, devicesrc, or rtspsrc (optional, default is testsrc)>\n", argv[0]);
        return -1;
    }

    STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    stream_name[MAX_STREAM_NAME_LEN] = '\0';

    if(argc > 2) {
        if(0 == STRCMPI(argv[2], "testsrc")) {
            source_type = TEST_SOURCE;
        } else if (0 == STRCMPI(argv[2], "devicesrc")) {
            source_type = DEVICE_SOURCE;
        } else if (0 == STRCMPI(argv[2], "rtspsrc")) {
            source_type = RTSP_SOURCE;
        }
    } else {
        source_type = TEST_SOURCE;
    }


    /* Create GStreamer elements */
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
    g_object_set(G_OBJECT(kvssink), "stream-name", stream_name, NULL);
    determine_credentials(kvssink, stream_name);

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
    bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
    gst_object_unref (bus);

    /* Add elements into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline),
                        source, video_convert, source_filter, encoder, h264parse, kvssink, NULL);

    /* Link the elements together */
    if (!gst_element_link_many(source, video_convert, source_filter, encoder, h264parse, kvssink, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return -1;
    }

    /* Set the pipeline to "playing" state*/
    g_print ("Playing..\n");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);


    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (main_loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (main_loop);

    return 0;
}