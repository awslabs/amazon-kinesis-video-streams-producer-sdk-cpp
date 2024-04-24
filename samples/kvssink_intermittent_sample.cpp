#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include <gst/gst.h>
#include <glib.h>

#include "gstreamer/gstkvssink.h"

using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

/* Modify these values to change start/stop interval. */
#define KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS 20
#define KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS 40

#define KVS_GST_TEST_SOURCE_NAME "test-source"
#define KVS_GST_DEVICE_SOURCE_NAME "device-source"


LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");

GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
std::atomic<bool> terminated(FALSE);
std::condition_variable cv;

typedef enum _StreamSource {
    TEST_SOURCE,
    DEVICE_SOURCE
} StreamSource;

typedef struct _CustomData {
    _CustomData() :
            main_loop(NULL),
            pipeline(NULL) {}

    GMainLoop *main_loop;
    GstElement *pipeline;
} CustomData;

void sigint_handler(int sigint) {
    LOG_DEBUG("SIGINT received. Exiting...");
    terminated = TRUE;
    cv.notify_all();
    if(main_loop != NULL) {
        g_main_loop_quit(main_loop);
    }
}

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) ((CustomData *)data)->main_loop;
    GstElement *pipeline = (GstElement *) ((CustomData *)data)->pipeline;

    switch(GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS: {
            LOG_DEBUG("[KVS sample] Received EOS message");
            cv.notify_all();
            break;
        }

        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            LOG_ERROR("[KVS sample] GStreamer error: " << error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }

        default: {
            break;
        }
    }

    return TRUE;
}

void determine_aws_credentials(GstElement *kvssink, char* streamName) {
    char const *iot_credential_endpoint;
    char const *cert_path;
    char const *private_key_path;
    char const *role_alias;
    char const *ca_cert_path;
    char const *credential_path;
    if(nullptr != (iot_credential_endpoint = GETENV("IOT_GET_CREDENTIAL_ENDPOINT")) &&
        nullptr != (cert_path = GETENV("CERT_PATH")) &&
        nullptr != (private_key_path = GETENV("PRIVATE_KEY_PATH")) &&
        nullptr != (role_alias = GETENV("ROLE_ALIAS")) &&
        nullptr != (ca_cert_path = GETENV("CA_CERT_PATH"))) {
    LOG_DEBUG("[KVS sample] Using IoT credentials.");
    // Set the IoT Credentials if provided in envvar.
    GstStructure *iot_credentials =  gst_structure_new(
            "iot-certificate",
            "iot-thing-name", G_TYPE_STRING, streamName, 
            "endpoint", G_TYPE_STRING, iot_credential_endpoint,
            "cert-path", G_TYPE_STRING, cert_path,
            "key-path", G_TYPE_STRING, private_key_path,
            "ca-path", G_TYPE_STRING, ca_cert_path,
            "role-aliases", G_TYPE_STRING, role_alias, NULL);
    
    g_object_set(G_OBJECT(kvssink), "iot-certificate", iot_credentials, NULL);
        gst_structure_free(iot_credentials);
    // kvssink will search for long term credentials in envvar automatically so no need to include here
    // if no long credentials or IoT credentials provided will look for credential file as last resort.
    } else if(nullptr != (credential_path = GETENV("AWS_CREDENTIAL_PATH"))) {
        LOG_DEBUG("[KVS sample] Using AWS_CREDENTIAL_PATH long term credentials.");
        g_object_set(G_OBJECT(kvssink), "credential-path", credential_path, NULL);
    } else {
        LOG_DEBUG("[KVS sample] Using credentials set by AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY env vars.");
    }
}

// This function handles the intermittent starting and stopping of the stream in a loop.
void stopStartLoop(GstElement *pipeline, GstElement *source) {
    std::mutex cv_m;
    std::unique_lock<std::mutex> lck(cv_m);

    while(!terminated) {
        // Using cv.wait_for to break sleep early upon signal interrupt.
        if(cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS)) != std::cv_status::timeout) {
            break;
        }

        LOG_INFO("[KVS sample] Stopping stream to KVS for " << KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS << " seconds");

        // EOS event pushes frames buffered by the h264enc element down to kvssink.
        GstEvent* eos = gst_event_new_eos();
        gst_element_send_event(pipeline, eos);

        // Wait for the EOS event to return from kvssink to the bus which means all elements are done handling the EOS.
        // We don't want to flush until the EOS is done to ensure all frames buffered in the pipeline have been processed.
        cv.wait(lck);

        // Set videotestsrc to paused state because it does not stop producing frames upon EOS,
        // and the frames are not cleared upon flushing.
        if(STRCMPI(GST_ELEMENT_NAME(source), KVS_GST_TEST_SOURCE_NAME) == 0) {
            gst_element_set_state(source, GST_STATE_PAUSED);
        }

        // Flushing to remove EOS status.
        GstEvent* flush_start = gst_event_new_flush_start();
        gst_element_send_event(pipeline, flush_start);

        // Using cv.wait_for to break sleep early upon signal interrupt. Checking for termination again before waiting.
        if(terminated || cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS)) != std::cv_status::timeout) {
            break;
        }

        LOG_INFO("[KVS sample] Starting stream to KVS for " << KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS << " seconds");
        
        // Stop the flush now that we are resuming streaming.
        GstEvent* flush_stop = gst_event_new_flush_stop(true);
        gst_element_send_event(pipeline, flush_stop);

        // Set videotestsrc back to playing state.
        if(STRCMPI(GST_ELEMENT_NAME(source), KVS_GST_TEST_SOURCE_NAME) == 0) {
            gst_element_set_state(source, GST_STATE_PLAYING);
        }
    }
    LOG_DEBUG("[KVS sample] Exited stopStartLoop");
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    CustomData customData;
    GstElement *pipeline, *source, *clock_overlay, *video_convert, *source_filter, *encoder, *sink_filter, *kvssink;
    GstCaps *source_caps, *sink_caps;
    GstBus *bus;
    guint bus_watch_id;
    StreamSource source_type;
    char stream_name[MAX_STREAM_NAME_LEN + 1] = {0};

    gst_init(&argc, &argv);


    /* Parse input arguments */

    // Check for invalid argument count, get stream name.
    if(argc > 3) {
        LOG_ERROR("[KVS sample] Invalid argument count, too many arguments.");
        LOG_INFO("[KVS sample] Usage: " << argv[0] << " <streamName (optional)> <testsrc or devicesrc (optional)>");
        return -1;
    } else if(argc > 1) {
        STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    }

    // Get source type.
    if(argc > 2) {
        if(0 == STRCMPI(argv[2], "testsrc")) {
            LOG_INFO("[KVS sample] Using test source (videotestsrc)");
            source_type = TEST_SOURCE;
        } else if(0 == STRCMPI(argv[2], "devicesrc")) {
            LOG_INFO("[KVS sample] Using device source (autovideosrc)");
            source_type = DEVICE_SOURCE;
        } else {
            LOG_ERROR("[KVS sample] Invalid source type");
            LOG_INFO("[KVS sample] Usage: " << argv[0] << " <streamName (optional)> <testsrc or devicesrc(optional)>");
            return -1;
        }
    } else {
        LOG_ERROR("[KVS sample] No source specified, defualting to test source (videotestsrc)");
        source_type = TEST_SOURCE;
    }


    /* Create GStreamer elements */

    pipeline = gst_pipeline_new("kvs-pipeline");

    /* source */
    if(source_type == TEST_SOURCE) {
        source = gst_element_factory_make("videotestsrc", KVS_GST_TEST_SOURCE_NAME);
        g_object_set(G_OBJECT(source),
                     "is-live", TRUE,
                     "pattern", 18,
                     "background-color", 0xff003181,
                     "foreground-color", 0xffff9900, NULL);
    } else if(source_type == DEVICE_SOURCE) {
        source = gst_element_factory_make("autovideosrc", KVS_GST_DEVICE_SOURCE_NAME);
    }

    /* clock overlay */
    clock_overlay = gst_element_factory_make("clockoverlay", "clock_overlay");
    g_object_set(G_OBJECT(clock_overlay),"time-format", "%a %B %d, %Y %I:%M:%S %p", NULL);

    /* video convert */
    video_convert = gst_element_factory_make("videoconvert", "video_convert");

    /* source filter */
    source_filter = gst_element_factory_make("capsfilter", "source-filter");
    source_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);
    g_object_set(G_OBJECT(source_filter), "caps", source_caps, NULL);
    gst_caps_unref(source_caps);

    /* encoder */
    encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(G_OBJECT(encoder),
                 "bframes", 0,
                 "key-int-max", 120, NULL);

    /* sink filter */
    sink_filter = gst_element_factory_make("capsfilter", "sink-filter");
    sink_caps = gst_caps_new_simple("video/x-h264",
                                    "stream-format", G_TYPE_STRING, "avc",
                                    "alignment", G_TYPE_STRING, "au",
                                    NULL);
    g_object_set(G_OBJECT(sink_filter), "caps", sink_caps, NULL);
    gst_caps_unref(sink_caps);

    /* kvssink */
    kvssink = gst_element_factory_make("kvssink", "kvssink");
    if (IS_EMPTY_STRING(stream_name)) {
        LOG_INFO("No stream name specified, using default kvssink stream name.")
    } else {
        g_object_set(G_OBJECT(kvssink), "stream-name", stream_name, NULL);
    }
    determine_aws_credentials(kvssink, stream_name);
    

    /* Check that GStreamer elements were all successfully created */

    if(!kvssink) {
        LOG_ERROR("[KVS sample] Failed to create kvssink element");
        return -1;
    }

    if(!pipeline || !source || !clock_overlay || !video_convert || !source_filter || !encoder || !sink_filter) {
        LOG_ERROR("[KVS sample] Not all GStreamer elements could be created.");
        return -1;
    }

    // Populate data struct.
    customData.main_loop = main_loop;
    customData.pipeline = pipeline;

    // Add a message handler.
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, &customData);
    gst_object_unref(bus);

    // Add elements into the pipeline.
    gst_bin_add_many(GST_BIN(pipeline),
                        source, clock_overlay, video_convert, source_filter, encoder, sink_filter, kvssink, NULL);

    // Link the elements together.
    if(!gst_element_link_many(source, clock_overlay, video_convert, source_filter, encoder, sink_filter, kvssink, NULL)) {
        LOG_ERROR("[KVS sample] Elements could not be linked");
        gst_object_unref(pipeline);
        return -1;
    }

    // Set the pipeline to playing state.
    LOG_INFO("[KVS sample] Starting stream to KVS for " << KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS << " seconds");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Start the stop/start thread for intermittent streaming.
    std::thread stopStartThread(stopStartLoop, pipeline, source);
    
    LOG_INFO("[KVS sample] Starting GStreamer main loop");
    g_main_loop_run(main_loop);

    stopStartThread.join();

    // Application terminated, cleanup.
    LOG_INFO("[KVS sample] Streaming terminated, cleaning up");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(main_loop);

    return 0;
}
