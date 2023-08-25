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

using namespace std;
using namespace std::chrono;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

#ifdef __cplusplus
extern "C" {
#endif

int gstreamer_init(int, char **);

#ifdef __cplusplus
}
#endif

LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");

typedef enum _StreamSource {
    FILE_SOURCE,
    LIVE_SOURCE,
    RTSP_SOURCE
} StreamSource;

typedef struct _FileInfo {
    _FileInfo() :
            path(""),
            last_fragment_ts(0) {}

    string path;
    uint64_t last_fragment_ts;
} FileInfo;

typedef struct _CustomData {

    _CustomData() :
            streamSource(LIVE_SOURCE),
            h264_stream_supported(false),
            synthetic_dts(0),
            last_unpersisted_file_idx(0),
            stream_status(STATUS_SUCCESS),
            base_pts(0),
            max_frame_pts(0),
            key_frame_pts(0),
            main_loop(NULL),
            first_pts(GST_CLOCK_TIME_NONE),
            use_absolute_fragment_times(true),
            max_runtime(0)	{
        producer_start_time = chrono::duration_cast<nanoseconds>(systemCurrentTime().time_since_epoch()).count();
    }

    GMainLoop *main_loop;
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    shared_ptr<KinesisVideoStream> kinesis_video_stream;
    bool stream_started;
    bool h264_stream_supported;
    char *stream_name;
    mutex file_list_mtx;

    // list of files to upload.
    vector<FileInfo> file_list;

    // index of file in file_list that application is currently trying to upload.
    uint32_t current_file_idx;

    // index of last file in file_list that haven't been persisted.
    atomic_uint last_unpersisted_file_idx;

    // stores any error status code reported by StreamErrorCallback.
    atomic_uint stream_status;

    // Since each file's timestamp start at 0, need to add all subsequent file's timestamp to base_pts starting from the
    // second file to avoid fragment overlapping. When starting a new putMedia session, this should be set to 0.
    // Unit: ns
    uint64_t base_pts;

    // Max pts in a file. This will be added to the base_pts for the next file. When starting a new putMedia session,
    // this should be set to 0.
    // Unit: ns
    uint64_t max_frame_pts;

    // When uploading file, store the pts of frames that has flag FRAME_FLAG_KEY_FRAME. When the entire file has been uploaded,
    // key_frame_pts contains the timetamp of the last fragment in the file. key_frame_pts is then stored into last_fragment_ts
    // of the file.
    // Unit: ns
    uint64_t key_frame_pts;

    // Used in file uploading only. Assuming frame timestamp are relative. Add producer_start_time to each frame's
    // timestamp to convert them to absolute timestamp. This way fragments dont overlap after token rotation when doing
    // file uploading.
    uint64_t producer_start_time;

    volatile StreamSource streamSource;

    string rtsp_url;

    unique_ptr<Credentials> credential;

    uint64_t synthetic_dts;

    bool use_absolute_fragment_times;

    // Pts of first video frame
    uint64_t first_pts;

    // Used to determine how long the stream should run (seconds)
    // Does not apply for file uploads
    int max_runtime;
} CustomData;

// CustomData 
CustomData data_global;

static bool format_supported_by_source(GstCaps *src_caps, GstCaps *query_caps, int width, int height, int framerate) {
    gst_caps_set_simple(query_caps,
                        "width", G_TYPE_INT, width,
                        "height", G_TYPE_INT, height,
                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                        NULL);
    bool is_match = gst_caps_can_intersect(query_caps, src_caps);

    // in case the camera has fps as 10000000/333333
    if (!is_match) {
        gst_caps_set_simple(query_caps,
                            "framerate", GST_TYPE_FRACTION_RANGE, framerate, 1, framerate + 1, 1,
                            NULL);
        is_match = gst_caps_can_intersect(query_caps, src_caps);
    }

    return is_match;
}

static bool resolution_supported(GstCaps *src_caps, GstCaps *query_caps_raw, GstCaps *query_caps_h264,
                                 CustomData &data, int width, int height, int framerate) {
    if (query_caps_h264 && format_supported_by_source(src_caps, query_caps_h264, width, height, framerate)) {
        LOG_DEBUG("src supports h264")
        data.h264_stream_supported = true;
    } else if (query_caps_raw && format_supported_by_source(src_caps, query_caps_raw, width, height, framerate)) {
        LOG_DEBUG("src supports raw")
        data.h264_stream_supported = false;
    } else {
        return false;
    }
    return true;
}

/* callback when eos (End of Stream) is posted on bus */
static void eos_cb(GstElement *sink, GstMessage *message, CustomData *data) {
    if (data->streamSource == FILE_SOURCE) {
        // bookkeeping base_pts. add 1ms to avoid overlap.
        data->base_pts += +data->max_frame_pts + duration_cast<nanoseconds>(milliseconds(1)).count();
        data->max_frame_pts = 0;

        {
            std::unique_lock<std::mutex> lk(data->file_list_mtx);
            // store file's last fragment's timestamp.
            data->file_list.at(data->current_file_idx).last_fragment_ts = data->key_frame_pts;
        }
    }
    LOG_DEBUG("Terminating pipeline due to EOS");
    g_main_loop_quit(data->main_loop);
}

/* This function is called when an error message is posted on the bus */
static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(data->main_loop);
    data->stream_status = STATUS_KVS_GSTREAMER_SAMPLE_ERROR;
}

/* callback when each RTSP stream has been created */
static void pad_added_cb(GstElement *element, GstPad *pad, GstElement *target) {
    GstPad *target_sink = gst_element_get_static_pad(GST_ELEMENT(target), "sink");
    GstPadLinkReturn link_ret;
    gchar *pad_name = gst_pad_get_name(pad);
    g_print("New pad found: %s\n", pad_name);

    link_ret = gst_pad_link(pad, target_sink);

    if (link_ret == GST_PAD_LINK_OK) {
        LOG_INFO("Pad link successful");
    } else {
        LOG_INFO("Pad link failed");
    }

    gst_object_unref(target_sink);
    g_free(pad_name);
}

/* Function will wait maxruntime before closing stream */
void timer(CustomData *data) {
  THREAD_SLEEP(data->max_runtime);
  LOG_DEBUG("max runtime elapsed. exiting"); 
  g_main_loop_quit(data->main_loop);
  data->stream_status = STATUS_SUCCESS;  
}

/* Function handles sigint signal */
void sigint_handler(int sigint){
    LOG_DEBUG("SIGINT received.  Exiting graceully");
    
    if(data_global.main_loop != NULL){
        g_main_loop_quit(data_global.main_loop);
    }
    data_global.stream_status = STATUS_KVS_GSTREAMER_SAMPLE_INTERRUPTED;
}

void determine_credentials(GstElement *kvssink, CustomData *data) {

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
			"iot-thing-name", G_TYPE_STRING, data->stream_name, 
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

int gstreamer_live_source_init(int argc, char *argv[], CustomData *data, GstElement *pipeline) {

    bool vtenc = false, isOnRpi = false;

    /* init stream format */
    int width = 0, height = 0, framerate = 25, bitrateInKBPS = 512;
    // index 1 is stream name which is already processed
    for (int i = 2; i < argc; i++) {
        if (i < argc) {
            if ((0 == STRCMPI(argv[i], "-w")) ||
                (0 == STRCMPI(argv[i], "/w")) ||
                (0 == STRCMPI(argv[i], "--w"))) {
                // process the width
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &width))) {
                    return 1;
                }
            } else if ((0 == STRCMPI(argv[i], "-h")) ||
                       (0 == STRCMPI(argv[i], "/h")) ||
                       (0 == STRCMPI(argv[i], "--h"))) {
                // process the height
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &height))) {
                    return 1;
                }
            } else if ((0 == STRCMPI(argv[i], "-f")) ||
                       (0 == STRCMPI(argv[i], "/f")) ||
                       (0 == STRCMPI(argv[i], "--f"))) {
                // process the framerate
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &framerate))) {
                    return 1;
                }
            } else if ((0 == STRCMPI(argv[i], "-b")) ||
                       (0 == STRCMPI(argv[i], "/b")) ||
                       (0 == STRCMPI(argv[i], "--b"))) {
                // process the bitrate
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &bitrateInKBPS))) {
                    return 1;
                }
             } else if ((0 == STRCMPI(argv[i], "-runtime")) ||
                       (0 == STRCMPI(argv[i], "/runtime")) ||
                       (0 == STRCMPI(argv[i], "--runtime"))) {
                // process the max runtime
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &(data->max_runtime)))) {
                    return 1;
                }
            // skip the index
            }
            i++;
        } else if (0 == STRCMPI(argv[i], "-?") ||
                   0 == STRCMPI(argv[i], "--?") ||
                   0 == STRCMPI(argv[i], "--help")) {
            g_printerr("Invalid arguments\n");
            return 1;
        } else if (argv[i][0] == '/' ||
                   argv[i][0] == '-') {
            // Unknown option
            g_printerr("Invalid arguments\n");
            return 1;
        }
    }

    if ((width == 0 && height != 0) || (width != 0 && height == 0)) {
        g_printerr("Invalid resolution\n");
        return 1;
    }

    if(framerate <= 0 || bitrateInKBPS <= 0) {
       g_printerr("Invalid input arguments\n");
       return 1;
    }

    LOG_DEBUG("Streaming with live source and width: " << width << ", height: " << height << ", fps: " << framerate
                                                       << ", bitrateInKBPS" << bitrateInKBPS);

    GstElement *source_filter, *filter, *kvssink, *h264parse, *encoder, *source, *video_convert;

    /* create the elemnents */
    source_filter = gst_element_factory_make("capsfilter", "source_filter");
    if (!source_filter) {
        LOG_ERROR("Failed to create capsfilter (1)");
        return 1;
    }
    filter = gst_element_factory_make("capsfilter", "encoder_filter");
    if (!filter) {
        LOG_ERROR("Failed to create capsfilter (2)");
        return 1;
    }
    kvssink = gst_element_factory_make("kvssink", "kvssink");
    if (!kvssink) {
        LOG_ERROR("Failed to create kvssink");
        return 1;
    }
    h264parse = gst_element_factory_make("h264parse", "h264parse"); // needed to enforce avc stream format
    if (!h264parse) {
        LOG_ERROR("Failed to create h264parse");
        return 1;
    }

    // Attempt to create vtenc encoder
    encoder = gst_element_factory_make("vtenc_h264_hw", "encoder");
    if (encoder) {
        vtenc = true;
        source = gst_element_factory_make("videotestsrc", "source");
        if (source) {
            LOG_DEBUG("Using videotestsrc");
        } else {
            LOG_ERROR("Failed to create videotestsrc");
            return 1;
        }
    } else {
        // Failed creating vtenc - check pi hardware encoder
        encoder = gst_element_factory_make("omxh264enc", "encoder");
        if (encoder) {
            LOG_DEBUG("Using omxh264enc")
            isOnRpi = true;
        } else {
            // - attempt x264enc
            isOnRpi = false;
            encoder = gst_element_factory_make("x264enc", "encoder");
            if (encoder) {
                LOG_DEBUG("Using x264enc");
            } else {
                LOG_ERROR("Failed to create x264enc");
                return 1;
            }
        }
        source = gst_element_factory_make("v4l2src", "source");
        if (source) {
            LOG_DEBUG("Using v4l2src");
        } else {
            LOG_DEBUG("Failed to create v4l2src, trying ksvideosrc")
            source = gst_element_factory_make("ksvideosrc", "source");
            if (source) {
                LOG_DEBUG("Using ksvideosrc");
            } else {
                LOG_ERROR("Failed to create ksvideosrc");
                return 1;
            }
        }
        vtenc = false;
    }

    if (!pipeline || !source || !source_filter || !encoder || !filter || !kvssink || !h264parse) {
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    /* configure source */
    if (vtenc) {
        g_object_set(G_OBJECT(source), "is-live", TRUE, NULL);
    } else {
        g_object_set(G_OBJECT(source), "do-timestamp", TRUE, "device", "/dev/video0", NULL);
    }

    /* Determine whether device supports h264 encoding and select a streaming resolution supported by the device*/
    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(source, GST_STATE_READY)) {
        g_printerr("Unable to set the source to ready state.\n");
        return 1;
    }

    GstPad *srcpad = gst_element_get_static_pad(source, "src");
    GstCaps *src_caps = gst_pad_query_caps(srcpad, NULL);
    gst_element_set_state(source, GST_STATE_NULL);

    GstCaps *query_caps_raw = gst_caps_new_simple("video/x-raw",
                                                  "width", G_TYPE_INT, width,
                                                  "height", G_TYPE_INT, height,
                                                  NULL);
    GstCaps *query_caps_h264 = gst_caps_new_simple("video/x-h264",
                                                   "width", G_TYPE_INT, width,
                                                   "height", G_TYPE_INT, height,
                                                   NULL);

    if (width != 0 && height != 0) {
        if (!resolution_supported(src_caps, query_caps_raw, query_caps_h264, *data, width, height, framerate)) {
            g_printerr("Resolution %dx%d not supported by video source\n", width, height);
            return 1;
        }
    } else {
        vector<int> res_width = {640, 1280, 1920};
        vector<int> res_height = {480, 720, 1080};
        vector<int> fps = {30, 25, 20};
        bool found_resolution = false;
        for (int i = 0; i < res_width.size(); i++) {
            width = res_width[i];
            height = res_height[i];
            for (int j = 0; j < fps.size(); j++) {
                framerate = fps[j];
                if (resolution_supported(src_caps, query_caps_raw, query_caps_h264, *data, width, height, framerate)) {
                    found_resolution = true;
                    break;
                }
            }
            if (found_resolution) {
                break;
            }
        }
        if (!found_resolution) {
            g_printerr(
                    "Default list of resolutions (1920x1080, 1280x720, 640x480) are not supported by video source\n");
            return 1;
        }
    }

    gst_caps_unref(src_caps);
    gst_object_unref(srcpad);

    /* create the elemnents needed for the corresponding pipeline */
    if (!data->h264_stream_supported) {
        video_convert = gst_element_factory_make("videoconvert", "video_convert");

        if (!video_convert) {
            g_printerr("Not all elements could be created.\n");
            return 1;
        }
    }

    /* source filter */
    if (!data->h264_stream_supported) {
        gst_caps_set_simple(query_caps_raw,
                            "format", G_TYPE_STRING, "I420",
                            NULL);
        g_object_set(G_OBJECT(source_filter), "caps", query_caps_raw, NULL);
    } else {
        gst_caps_set_simple(query_caps_h264,
                            "stream-format", G_TYPE_STRING, "byte-stream",
                            "alignment", G_TYPE_STRING, "au",
                            NULL);
        g_object_set(G_OBJECT(source_filter), "caps", query_caps_h264, NULL);
    }
    gst_caps_unref(query_caps_h264);
    gst_caps_unref(query_caps_raw);

    /* configure encoder */
    if (!data->h264_stream_supported) {
        if (vtenc) {
            g_object_set(G_OBJECT(encoder), "allow-frame-reordering", FALSE, "realtime", TRUE, "max-keyframe-interval",
                         45, "bitrate", bitrateInKBPS, NULL);
        } else if (isOnRpi) {
            g_object_set(G_OBJECT(encoder), "control-rate", 2, "target-bitrate", bitrateInKBPS * 1000,
                         "periodicty-idr", 45, "inline-header", FALSE, NULL);
        } else {
            g_object_set(G_OBJECT(encoder), "bframes", 0, "key-int-max", 45, "bitrate", bitrateInKBPS, NULL);
        }
    }


    /* configure filter */
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    if (!data->h264_stream_supported) {
        gst_caps_set_simple(h264_caps, "profile", G_TYPE_STRING, "baseline",
                            NULL);
    }
    g_object_set(G_OBJECT(filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    /* configure kvssink */
    g_object_set(G_OBJECT(kvssink), "stream-name", data->stream_name, "storage-size", 128, NULL);
    determine_credentials(kvssink, data);

    /* build the pipeline */
    if (!data->h264_stream_supported) {
        gst_bin_add_many(GST_BIN(pipeline), source, video_convert, source_filter, encoder, h264parse, filter,
                         kvssink, NULL);
        LOG_DEBUG("Constructing pipeline with encoding element")
        if (!gst_element_link_many(source, video_convert, source_filter, encoder, h264parse, filter, kvssink, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }
    } else {
        LOG_DEBUG("Constructing pipeline without encoding element")
        gst_bin_add_many(GST_BIN(pipeline), source, source_filter, h264parse, filter, kvssink, NULL);
        if (!gst_element_link_many(source, source_filter, h264parse, filter, kvssink, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }
    }

    return 0;
}

int gstreamer_rtsp_source_init(int argc, char *argv[], CustomData *data, GstElement *pipeline) {
    // process runtime if provided
    if (argc == 5){
      if ((0 == STRCMPI(argv[3], "-runtime")) ||
          (0 == STRCMPI(argv[3], "/runtime")) ||
          (0 == STRCMPI(argv[3], "--runtime"))){
          // process the max runtime
          if (STATUS_FAILED(STRTOI32(argv[4], NULL, 10, &(data->max_runtime)))) {
                return 1;
	  }
      }
    }
    GstElement *filter, *kvssink, *depay, *source, *h264parse;

    filter = gst_element_factory_make("capsfilter", "filter");
    kvssink = gst_element_factory_make("kvssink", "kvssink");
    depay = gst_element_factory_make("rtph264depay", "depay");
    source = gst_element_factory_make("rtspsrc", "source");
    h264parse = gst_element_factory_make("h264parse", "h264parse");

    if (!pipeline || !source || !depay || !kvssink || !filter || !h264parse) {
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    // configure filter
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    g_object_set(G_OBJECT(filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    // configure kvssink
    g_object_set(G_OBJECT(kvssink), "stream-name", data->stream_name, "storage-size", 128, NULL);
    determine_credentials(kvssink, data);
    
    // configure rtspsrc
    g_object_set(G_OBJECT(source),
                 "location", data->rtsp_url.c_str(),
                 "short-header", true, // Necessary for target camera
                 NULL);

    g_signal_connect(source, "pad-added", G_CALLBACK(pad_added_cb), depay);

    /* build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source,
                     depay, h264parse, filter, kvssink,
                     NULL);

    /* Leave the actual source out - this will be done when the pad is added */
    if (!gst_element_link_many(depay, filter, h264parse,
                               kvssink,
                               NULL)) {

        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    return 0;
}

int gstreamer_file_source_init(CustomData *data, GstElement *pipeline) {

    GstElement *demux, *kvssink, *filesrc, *h264parse, *filter, *queue;
    string file_suffix;
    string file_path = data->file_list.at(data->current_file_idx).path;

    filter = gst_element_factory_make("capsfilter", "filter");
    kvssink = gst_element_factory_make("kvssink", "kvssink");
    filesrc = gst_element_factory_make("filesrc", "filesrc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    queue = gst_element_factory_make("queue", "queue");

    // set demux based off filetype
    file_suffix = file_path.substr(file_path.size() - 3);
    if (file_suffix.compare("mkv") == 0) {
        demux = gst_element_factory_make("matroskademux", "demux");
    } else if (file_suffix.compare("mp4") == 0) {
        demux = gst_element_factory_make("qtdemux", "demux");
    } else if (file_suffix.compare(".ts") == 0) {
        demux = gst_element_factory_make("tsdemux", "demux");
    } else {
        LOG_ERROR("File format not supported. Supported ones are mp4, mkv and ts. File suffix: " << file_suffix);
        return 1;
    }


    if (!demux || !filesrc || !h264parse || !kvssink || !pipeline || !filter) {
        g_printerr("Not all elements could be created:\n");
        return 1;
    }

    // configure filter
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    g_object_set(G_OBJECT(filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    // configure kvssink
    g_object_set(G_OBJECT(kvssink), "stream-name", data->stream_name, "streaming-type", STREAMING_TYPE_OFFLINE, "storage-size", 128, NULL);
    determine_credentials(kvssink, data);

    // configure filesrc
    g_object_set(G_OBJECT(filesrc), "location", file_path.c_str(), NULL);

    // configure demux
    g_signal_connect(demux, "pad-added", G_CALLBACK(pad_added_cb), queue);


    /* build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), demux,
                     filesrc, filter, kvssink, h264parse, queue,
                     NULL);

    if (!gst_element_link_many(filesrc, demux,
                               NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    if (!gst_element_link_many(queue, h264parse, filter, kvssink,
                               NULL)) {
        g_printerr("Video elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    return 0;
}

int gstreamer_init(int argc, char *argv[], CustomData *data) {

    /* init GStreamer */
    gst_init(&argc, &argv);

    GstElement *pipeline;
    int ret;
    GstStateChangeReturn gst_ret;

    // Reset first frame pts
    data->first_pts = GST_CLOCK_TIME_NONE;

    switch (data->streamSource) {
        case LIVE_SOURCE:
            LOG_INFO("Streaming from live source");
            pipeline = gst_pipeline_new("live-kinesis-pipeline");
            ret = gstreamer_live_source_init(argc, argv, data, pipeline);
            break;
        case RTSP_SOURCE:
            LOG_INFO("Streaming from rtsp source");
            pipeline = gst_pipeline_new("rtsp-kinesis-pipeline");
            ret = gstreamer_rtsp_source_init(argc, argv, data, pipeline);
            break;
        case FILE_SOURCE:
            LOG_INFO("Streaming from file source");
            pipeline = gst_pipeline_new("file-kinesis-pipeline");
            ret = gstreamer_file_source_init(data, pipeline);
            break;
    }

    if (ret != 0) {
        return ret;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    g_signal_connect(G_OBJECT(bus), "message::eos", G_CALLBACK(eos_cb), data);
    gst_object_unref(bus);
    /* start streaming */
    gst_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (gst_ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
	data->stream_status = STATUS_KVS_GSTREAMER_SAMPLE_ERROR; 
        return 1;
    }
    // set timer if valid runtime provided (non-positive values are ignored)
    if (data->streamSource != FILE_SOURCE && data->max_runtime > 0){
	LOG_DEBUG("Timeout is " << data->max_runtime << " seconds.");
	std::thread stream_timer(timer, data);
	stream_timer.detach();
    }
    LOG_DEBUG("before main loop");
    data->main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data->main_loop);
    LOG_DEBUG("after main loop")


    /* free resources */
    gst_bus_remove_signal_watch(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;
    return 0;
}

int main(int argc, char *argv[]) {
    PropertyConfigurator::doConfigure("../kvs_log_configuration");

    signal(SIGINT, sigint_handler);

    if (argc < 2) {
        LOG_ERROR(
                "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kvssink_gstreamer_sample_app my-stream-name -w width -h height -f framerate -b bitrateInKBPS -runtime runtimeInSeconds\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kvssink_gstreamer_sample_app my-stream-name\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kvssink_gstreamer_sample_app my-stream-name rtsp-url -runtime runtimeInSeconds\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kvssink_gstreamer_sample_app my-stream-name path/to/file1 path/to/file2 ...\n");
        return 1;
    }

    const int PUTFRAME_FAILURE_RETRY_COUNT = 3;

    char stream_name[MAX_STREAM_NAME_LEN + 1];
    int ret = 0;
    int file_retry_count = PUTFRAME_FAILURE_RETRY_COUNT;
    STATUS stream_status = STATUS_SUCCESS;

    STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    stream_name[MAX_STREAM_NAME_LEN] = '\0';
    data_global.stream_name = stream_name;

    data_global.streamSource = LIVE_SOURCE;
    if (argc > 2) {
        string third_arg = string(argv[2]);
        // config options for live source begin with -
        if (third_arg[0] != '-') {
            string prefix = third_arg.substr(0, 4);
            string suffix = third_arg.substr(third_arg.size() - 3);
            if (prefix.compare("rtsp") == 0) {
                data_global.streamSource = RTSP_SOURCE;
                data_global.rtsp_url = string(argv[2]);

            } else if (suffix.compare("mkv") == 0 ||
                       suffix.compare("mp4") == 0 ||
                       suffix.compare(".ts") == 0) {
                data_global.streamSource = FILE_SOURCE;
                // skip over stream name
                for (int i = 2; i < argc; ++i) {
                    string file_path = string(argv[i]);
                    // file path should be at least 4 char (shortest example: a.ts)
                    if (file_path.size() < 4) {
                        LOG_ERROR("Invalid file path");
                        return 1;
                    }
                    FileInfo fileInfo;
                    fileInfo.path = file_path;
                    data_global.file_list.push_back(fileInfo);
                }
            }
        }
    }

    bool do_retry = true;

    if (data_global.streamSource == FILE_SOURCE) {
        do {
            uint32_t i = data_global.last_unpersisted_file_idx.load();
            bool continue_uploading = true;

            for (; i < data_global.file_list.size() && continue_uploading; ++i) {

                data_global.current_file_idx = i;
                LOG_DEBUG("Attempt to upload file: " << data_global.file_list[i].path);

                // control will return after gstreamer_init after file eos or any GST_ERROR was put on the bus.
                if (gstreamer_init(argc, argv, &data_global) != 0) {
                    return 1;
                }

                // check if any stream error occurred.
                stream_status = data_global.stream_status.load();
                    
		if (STATUS_FAILED(stream_status)) {
                    continue_uploading = false;
		    do_retry = false;
                    if (stream_status == GST_FLOW_ERROR) {
                        LOG_ERROR("Fatal stream error occurred: " << stream_status << ". Terminating.");
                    } else if(stream_status == STATUS_KVS_GSTREAMER_SAMPLE_INTERRUPTED){
		        LOG_ERROR("File upload interrupted.  Terminating.");
		        continue_uploading = false;
		    }else { // non fatal case.  retry upload
                        LOG_ERROR("stream error occurred: " << stream_status << ". Terminating.");
                        do_retry = true;
                    }
                } else {
                    LOG_INFO("Finished sending file to kvs producer: " << data_global.file_list[i].path);
		    data_global.last_unpersisted_file_idx += 1;
                    // check if we just finished sending the last file.
                    if (i == data_global.file_list.size() - 1) {
                        LOG_INFO("All files have been persisted");
			do_retry = false;
                    }
                }
            }

            if (do_retry) {
                file_retry_count--;
                if (file_retry_count == 0) {
                    i = data_global.last_unpersisted_file_idx.load();
                    LOG_ERROR("Failed to upload file " << data_global.file_list[i].path << " after retrying. Terminating.");
                    do_retry = false;           // exit while loop
                } else {
                    // reset state
                    data_global.stream_status = STATUS_SUCCESS;
                    data_global.stream_started = false;
                }
            }
        } while (do_retry);

    } else {
        // non file uploading scenario
        if (gstreamer_init(argc, argv, &data_global) != 0) {
            return 1;
        }
	stream_status = data_global.stream_status.load();
        if (STATUS_SUCCEEDED(stream_status)) {
            LOG_INFO("Stream succeeded");
        } else if(stream_status == STATUS_KVS_GSTREAMER_SAMPLE_INTERRUPTED){
	    LOG_INFO("Stream Interrupted");
	} else {
            LOG_INFO("Stream Failed");
        }
    }

    return 0;
}

