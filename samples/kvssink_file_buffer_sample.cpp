#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsink.h>

#include <Logger.h>


#include <gst/gst.h>
#include <gst/app/app.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>


#include "gstreamer/gstkvssink.h"

using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

#define KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS 20
#define KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS 40


#define FILE_BUFFER_TIME_SECONDS 5

#define KVS_GST_TEST_SOURCE_NAME "test-source"
#define KVS_GST_DEVICE_SOURCE_NAME "device-source"


LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");

GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
std::atomic<bool> terminated(FALSE);
std::condition_variable cv;

static guint frame_count = 0;  // Counter for frame numbering


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

typedef struct _Gop{
    guint64 startTs_ms; // Start timestamp of this GoP (I-frame timestamp)
    PSingleList pPFramesTsList_ms; // P-frames associated with this GoP
} Gop;

void sigint_handler(int sigint) {
    LOG_DEBUG("SIGINT received. Exiting...");
    terminated = TRUE;
    cv.notify_all();
    if(main_loop != NULL) {
        g_main_loop_quit(main_loop);
    }
}

// 1. GStreamer pipeline writing to file.
// 2. GStreamer pipeline writing file to KVS.
// 3. GStreamer pipeline writing live footage to KVS.

// 1 should stop as soon as 3 starts capturing frames.
// 2 should start as soon as 1 stops and the written file is complete.
// 2 will stop on its own once the file has been sent to KVS.

// Once we are done live streaming, stop 3.
// 1 should start as soon as 3 stops.

// Repeat this process until terminated.






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









static gboolean ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return FALSE;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    return (lenstr >= lensuffix) && (strcmp(str + lenstr - lensuffix, suffix) == 0);
}

// Compare function for sorting by the numeric timestamp in "frame_XXXXXXXX.h264"
static gint compare_filenames(gconstpointer a, gconstpointer b){
    const char *fa = *(const char **) a;
    const char *fb = *(const char **) b;

    // DLOGD("Comparing %s with %s\n", fa + 13, fb + 13);

    // Extract numeric timestamps
    guint64 ta = g_ascii_strtoull(fa + 13, NULL, 10);
    guint64 tb = g_ascii_strtoull(fb + 13, NULL, 10);

    // DLOGD("Comparing %s (%" G_GUINT64_FORMAT ") with %s (%" G_GUINT64_FORMAT ")\n", fa, ta, fb, tb);

    if (ta < tb) {
        return -1;  // 'a' is smaller => goes first
    } else if (ta > tb) {
        return +1;  // 'a' is bigger => goes after
    } else {
        return 0;   // equal
    }
}




void sendBufferedFrames() {
    // Build a pipeline: appsrc -> h264parse -> kvssink
    GError *error = NULL;
    // GstElement *pipeline = gst_parse_launch(
    //     "appsrc name=framesrc is-live=true do-timestamp=false "
    //     "! h264parse "
    //     "! kvssink stream-name=test "
    //     "       use-original-pts=1",
    //     &error);

    DLOGE("Creating pipeline for kvssink...\n");

    GstElement *pipeline = gst_parse_launch(
        "appsrc name=framesrc is-live=false do-timestamp=false "
        "! h264parse"
        "! video/x-h264, stream-format=avc, alignment=au "
        "! kvssink stream-name=test use-original-pts=1",
        &error
    );

    if (error) {
        g_printerr("Error constructing pipeline: %s\n", error->message);
        g_error_free(error);
        // return 1;
    }

    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "framesrc");
    if (!appsrc) {
        g_printerr("Couldn't find appsrc\n");
        // return 1;
    }

    DLOGE("Setting to playing...\n");
    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    DLOGE("Set to play.\n");


    // 1) Collect the list of *.h264 files in "frames" directory
    GPtrArray *files = g_ptr_array_new_with_free_func(g_free);
    DIR *dir = opendir("frames");
    if (!dir) {
        g_printerr("Failed to open frames directory: %s\n", strerror(errno));
        // return 1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ends_with(ent->d_name, ".h264") && strncmp(ent->d_name, "frame_", 6) == 0) {
            // build a path "frames/filename"
            gchar *fullpath = g_strdup_printf("frames/%s", ent->d_name);
            g_ptr_array_add(files, fullpath);
        }
    }
    closedir(dir);

    // 2) Sort the array by numeric timestamp from the filename
    g_ptr_array_sort(files, (GCompareFunc) compare_filenames);

    // After g_ptr_array_sort(...)
    for (guint i = 0; i < files->len; i++) {
        const gchar *pathname = (const gchar*) g_ptr_array_index(files, i);
        const gchar *base = g_path_get_basename(pathname);
        guint64 val = g_ascii_strtoull(base + 6, NULL, 10);
        // g_print("Sorted i=%d: %s => %" G_GUINT64_FORMAT "\n", i, base, val);
    }


    // 3) For each file, parse out the timestamp, read data, create a buffer, set PTS
    for (guint i = 0; i < files->len; i++) {
        const gchar *pathname = (const gchar *) g_ptr_array_index(files, i);

        // Parse out the numeric portion from "frames/frame_1678889990123.h264"
        // We only want "1678889990123" => convert to GStreamer PTS in nanoseconds
        const gchar *basename = g_path_get_basename(pathname); // e.g. "frame_1678889990123.h264"
        guint64 fileTimestampMs = g_ascii_strtoull(basename + 6, NULL, 10);
        // Convert from milliseconds to nanoseconds
        GstClockTime bufferPts = (GstClockTime) (fileTimestampMs * GST_MSECOND);

        // read raw data
        FILE *f = fopen(pathname, "rb");
        if (!f) {
            g_printerr("Failed to open file '%s': %s\n", pathname, strerror(errno));
            continue;
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        guint8 *data = (guint8 *) g_malloc(sz);
        if (fread(data, 1, sz, f) != (size_t)sz) {
            g_printerr("Error reading file %s\n", pathname);
            fclose(f);
            g_free(data);
            continue;
        }
        fclose(f);

        // 4) Create a GstBuffer with this data
        GstBuffer *buffer = gst_buffer_new_allocate(NULL, sz, NULL);
        gst_buffer_fill(buffer, 0, data, sz);
        g_free(data);

        // 5) Set the original PTS from the filename
        GST_BUFFER_PTS(buffer) = bufferPts;
        // DTS can match PTS if you do not have separate decode times:
        GST_BUFFER_DTS(buffer) = bufferPts;

        // DLOGE("Pushing buffer with PTS: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(bufferPts));

        // 6) Push the buffer into appsrc
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (ret != GST_FLOW_OK) {
            g_printerr("appsrc push buffer returned %d for file %s\n", ret, pathname);
            break; // or continue
        }
    }

    // 7) Signal EOS - done sending frames
    gst_app_src_end_of_stream(GST_APP_SRC(appsrc));

    // 8) Wait until the pipeline goes EOS or ERROR
    {
        GstBus *bus = gst_element_get_bus(pipeline);
        gboolean done = FALSE;
        while (!done) {
            GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED));

            if (!msg) break;

            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_EOS:
                g_print("Pipeline reached EOS, all frames done\n");
                done = TRUE;
                break;
            case GST_MESSAGE_ERROR: {
                GError *err = NULL;
                gchar *dbg;
                gst_message_parse_error(msg, &err, &dbg);
                g_printerr("Error: %s\n", err->message);
                g_error_free(err);
                g_free(dbg);
                done = TRUE;
                break;
            }
            case GST_MESSAGE_STATE_CHANGED:
                // ignore, or debug print
                break;
            default:
                break;
            }
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    // 9) Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsrc);
    gst_object_unref(pipeline);

    g_ptr_array_free(files, TRUE);
}












// This function handles the intermittent starting and stopping of the stream in a loop.
void stopStartLoop(GstElement *pipeline, GstElement *source) {
    std::mutex cv_m;
    std::unique_lock<std::mutex> lck(cv_m);

    // while(!terminated) {
        // Using cv.wait_for to break sleep early upon signal interrupt.
        if(cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS)) != std::cv_status::timeout) {
            // break;
            return;
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



        sendBufferedFrames();



        

        // Flushing to remove EOS status.
        // GstEvent* flush_start = gst_event_new_flush_start();
        // gst_element_send_event(pipeline, flush_start);

    //     // Using cv.wait_for to break sleep early upon signal interrupt. Checking for termination again before waiting.
    //     if(terminated || cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS)) != std::cv_status::timeout) {
    //         break;
    //     }

    //     LOG_INFO("[KVS sample] Starting stream to KVS for " << KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS << " seconds");
        
    //     // Stop the flush now that we are resuming streaming.
    //     GstEvent* flush_stop = gst_event_new_flush_stop(true);
    //     gst_element_send_event(pipeline, flush_stop);

    //     // Set videotestsrc back to playing state.
    //     if(STRCMPI(GST_ELEMENT_NAME(source), KVS_GST_TEST_SOURCE_NAME) == 0) {
    //         gst_element_set_state(source, GST_STATE_PLAYING);
    //     }
    // }
    LOG_DEBUG("[KVS sample] Exited stopStartLoop");
}





/* Callback function that processes incoming frames */
static GstFlowReturn on_new_sample(GstElement *sink, gpointer user_data) {
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    gchar filename[256];

    // Pull sample from appsink
    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    // Get the current epoch time in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    guint64 now_ms = (guint64)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    bool isKeyFrame;

    PSingleList pGopList = (PSingleList) user_data;

    
    // Use that timestamp in the filename
    snprintf(filename, sizeof(filename), "frames/frame_%" G_GUINT64_FORMAT ".h264", now_ms);

    // 

    // Get buffer from sample
    buffer = gst_sample_get_buffer(sample);
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        isKeyFrame = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        FILE *file = fopen(filename, "wb");
        if (file) {
            fwrite(map.data, 1, map.size, file);
            fclose(file);
            // g_print("Saved frame to: %s\n", filename);

            // If on a keyframe, make a new GoP object and add it to the list.
            if(isKeyFrame) {
                // DLOGE("Keyframe detected, creating new GoP object");

                // Make a new GoP object.
                Gop* newGop = (Gop *)calloc(1, sizeof(Gop));
                newGop->startTs_ms = now_ms;
                
                singleListCreate(&(newGop->pPFramesTsList_ms));
                singleListInsertItemTail(pGopList, (UINT64) newGop);
            } else {
                // DLOGE("Not a keyframe, not creating new GoP object");

                // Add the timestamp to the P-Frames list of the latest GoP object.
                PSingleListNode pTailNode = NULL;

                singleListGetTailNode(pGopList, &pTailNode);

                singleListInsertItemTail(((Gop *)pTailNode->data)->pPFramesTsList_ms, now_ms);

            }
        } else {
            g_printerr("Failed to open file for writing: %s\n", filename);
        }

        gst_buffer_unmap(buffer, &map);

    }

    // Now let's check if there are GoP objects whose frames are all older than FILE_BUFFER_TIME_SECONDS.
    // If so, remove them from the list and delete the frame file.
    // Can uncomment the "isKeyFrame" to reduce frequency of checks, but buffer might end up being 1 GoP longer than necessary.
    // if(isKeyFrame) {
        PSingleListNode pNode = NULL;
        singleListGetHeadNode(pGopList, &pNode);
        PSingleListNode pNextNode = pNode->pNext;

        DLOGE("Checking for frames to delete...");

        while(pNode != NULL && pNextNode != NULL) {
            DLOGE("Checking frame...");

            Gop* pGop = (Gop *) pNode->data;
            Gop* pNextGop = (Gop *) pNextNode->data;

            // If the next GoP's start timestamp is within the FILE_BUFFER_TIME_SECONDS, then we can delete this GoP.
            // This logic allows for FILE_BUFFER_TIME_SECONDS to be exceeded to ensure we don't hold fewer frames than FILE_BUFFER_TIME_SECONDS
            // in the cases where the FILE_BUFFER_TIME_SECONDS threshold lays in the middle of a GoP; we do not want to discard such a GoP.
            if((now_ms - pNextGop->startTs_ms) >= (FILE_BUFFER_TIME_SECONDS * 1000)) {
                DLOGE("Deleting a frame file...");

                // Delete the frame file.
                memset(filename, 0, sizeof(filename));
                snprintf(filename, sizeof(filename), "frames/frame_%" G_GUINT64_FORMAT ".h264", pGop->startTs_ms);
                remove(filename);

                singleListFree(pGop->pPFramesTsList_ms);
                singleListDeleteNode(pGopList, pNode);
                free(pGop);
                pNode = NULL;

            } else {
                DLOGE("Oldest frame does not exceed the interval");
                break;
            }
            pNode = pNextNode;
            pNextNode = pNextNode->pNext;
        }
    // }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}








int main(int argc, char *argv[])
{
    DLOGE("main");

    signal(SIGINT, sigint_handler);

    CustomData customData;
    GstElement *pipeline, *source, *clock_overlay, *video_convert, *source_filter, *encoder, *sink_filter, *kvssink, *appsink;
    GstCaps *source_caps, *sink_caps;
    GstBus *bus;
    guint bus_watch_id;
    StreamSource source_type;
    char stream_name[MAX_STREAM_NAME_LEN + 1] = {0};

    DLOGE("Initializing GStreamer...");

    gst_init(&argc, &argv);





    PSingleList pGopList = NULL;
    singleListCreate(&pGopList);




    // Empty the buffer directory
    DIR *dir = opendir("frames");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ends_with(ent->d_name, ".h264") && strncmp(ent->d_name, "frame_", 6) == 0) {
                gchar *fullpath = g_strdup_printf("frames/%s", ent->d_name);
                remove(fullpath);
                g_free(fullpath);
            }
        }
        closedir(dir);
    } else {
        g_printerr("Failed to open frames directory: %s\n", strerror(errno));
    }




    /* Parse input arguments */

    DLOGE("Parsing input arguments...");
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

    DLOGE("Create GStreamer elements");
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
                                    "stream-format", G_TYPE_STRING, "byte-stream",
                                    "alignment", G_TYPE_STRING, "au",
                                    NULL);
    g_object_set(G_OBJECT(sink_filter), "caps", sink_caps, NULL);
    gst_caps_unref(sink_caps);

    appsink = gst_element_factory_make("appsink", "appsink");

    // /* kvssink */
    // kvssink = gst_element_factory_make("kvssink", "kvssink");
    // if (IS_EMPTY_STRING(stream_name)) {
    //     LOG_INFO("No stream name specified, using default kvssink stream name.")
    // } else {
    //     g_object_set(G_OBJECT(kvssink), "stream-name", stream_name, NULL);
    // }
    // determine_aws_credentials(kvssink, stream_name);

    /* Check that GStreamer elements were all successfully created */

    // if(!kvssink) {
    //     LOG_ERROR("[KVS sample] Failed to create kvssink element");
    //     return -1;
    // }

    if(!pipeline || !source || !clock_overlay || !video_convert || !source_filter || !encoder || !sink_filter || !appsink){
        LOG_ERROR("[KVS sample] Not all GStreamer elements could be created.");
        return -1;
    }

    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), pGopList);

    // Populate data struct.
    customData.main_loop = main_loop;
    customData.pipeline = pipeline;

    DLOGE("GStreamer elements created successfully.");
    DLOGE("Creating GStreamer bus...");
    // Add a message handler.
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    DLOGE("Created bus.");
    bus_watch_id = gst_bus_add_watch(bus, bus_call, &customData);
    DLOGE("Added bus watch.");
    gst_object_unref(bus);

    DLOGE("Adding elements to pipeline...");
    // Add elements into the pipeline.
    gst_bin_add_many(GST_BIN(pipeline),
                        // source, clock_overlay, video_convert, source_filter, encoder, sink_filter, kvssink, NULL);
                        source, clock_overlay, video_convert, source_filter, encoder, sink_filter, appsink, NULL);

    // Link the elements together.
    // if(!gst_element_link_many(source, clock_overlay, video_convert, source_filter, encoder, sink_filter, kvssink, NULL)) {
    if(!gst_element_link_many(source, clock_overlay, video_convert, source_filter, encoder, sink_filter, appsink, NULL)) {
        DLOGE("[KVS sample] Elements could not be linked");
        gst_object_unref(pipeline);
        return -1;
    }

    // Set the pipeline to playing state.
    DLOGE("[KVS sample] Starting stream to KVS for %lu seconds", KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Start the stop/start thread for intermittent streaming.
    std::thread stopStartThread(stopStartLoop, pipeline, source);
    
    DLOGE("[KVS sample] Starting GStreamer main loop");
    g_main_loop_run(main_loop);

    stopStartThread.join();

    // Application terminated, cleanup.
    DLOGE("[KVS sample] Streaming terminated, cleaning up");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(main_loop);

    return 0;
}