#include <gst/gst.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <sstream>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int gstreamer_init(int, char **);

#ifdef __cplusplus
}
#endif

#define DEFAULT_RETRY_COUNT       3
#define DEFAULT_STREAMING_TYPE    "offline"

#define PROPERTY_PREFIX           "KVS_"
#define PROPERTY_KEY_MAX_LEN      32
#define KVS_SINK_PLUGIN_NAME      "kvssink"

#define STREAM_STATUS_OK          0
#define STREAM_STATUS_FAILED      1

#define CONTENT_TYPE_VIDEO_ONLY   0
#define CONTENT_TYPE_AUDIO_VIDEO  1

#define APP_NAME "kvs_gstreamer_file_uploader_sample"
#define LOG_INFO(fmt, ...) \
  do { fprintf(stdout, "[INFO] " APP_NAME ": " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_ERROR(fmt, ...) \
  do { fprintf(stderr, "[ERROR] " APP_NAME ": " fmt "\n", ##__VA_ARGS__); } while(0)

static const char* AVAILABLE_PROPERTIES[] = {
  PROPERTY_PREFIX "IOT_CERTIFICATE", "Use aws iot certificate to obtain credentials",
  PROPERTY_PREFIX "RETENTION_PERIOD", "Length of time stream is preserved. Unit: hours",
  PROPERTY_PREFIX "STORAGE_SIZE", "Storage Size. Unit: MB",
  NULL
};

typedef struct _CustomData {
    _CustomData(): 
        stream_status(STREAM_STATUS_OK) {}
    int stream_status;
    int content_type;
    string file_path;
    // kvssink_str contains a gstreamer pipeline syntax for kvssink plugin. All of the properties are filled
    // through environment variables.
    //
    // For example:
    //     If the program has been run with the following environment variables:
    //         KVS_STREAM_NAME=file-uploader-sample
    //         KVS_MAX_LATENCY=60
    //
    //     kvssink_str is going to be equal to the following: 
    //         kvssink stream-name=file-uploader-sample max-latency=60
    string kvssink_str;
} CustomData;

/* This function is called when an error message is posted on the bus */
static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    LOG_ERROR("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
    LOG_ERROR("Debugging information: %sn", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    data->stream_status = STREAM_STATUS_FAILED;
}

int gstreamer_init(int argc, char* argv[], CustomData *data) {
    GstElement *pipeline;
    GstMessage *msg;
    GstStateChangeReturn gst_ret;
    GError *error = NULL;
    string file_path = data->file_path;
    const char* demuxer = NULL;
    char pipeline_buf[4096];
    int ret;

    // reset state
    data->stream_status = STREAM_STATUS_OK;

    /* init GStreamer */
    LOG_INFO("Building gstreamer pipeline");
    gst_init(&argc, &argv);

    string file_suffix = file_path.substr(file_path.size() - 3);
    if (file_suffix.compare("mkv") == 0) {
        demuxer = "matroskademux";
    } else if (file_suffix.compare("mp4") == 0) {
        demuxer = "qtdemux";
    } else if (file_suffix.compare(".ts") == 0) {
        demuxer = "tsdemux";
    } else {
        LOG_ERROR("File format not supported. Supported ones are mp4, mkv and ts. File suffix: %s", file_suffix.c_str());
        return 1;
    }

    if (data->content_type == CONTENT_TYPE_VIDEO_ONLY) { // video only
        ret = snprintf(pipeline_buf, sizeof(pipeline_buf),
            "filesrc location=%s ! %s ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! %s",
            file_path.c_str(), demuxer, data->kvssink_str.c_str()
        );
    } else { // audio-video
        ret = snprintf(pipeline_buf, sizeof(pipeline_buf),
            "filesrc location=%s ! %s name=demuxer "
            "demuxer. ! queue ! h264parse ! video/x-h264,stream-format=avc,alignment=au ! %s name=sink "
            "demuxer. ! queue ! aacparse ! audio/mpeg,stream-format=raw ! sink.",
            file_path.c_str(), demuxer, data->kvssink_str.c_str()
        );
    }
    if (ret < 0) {
        LOG_ERROR("Pipeline is too long");
        return ret;
    }

    pipeline = gst_parse_launch(pipeline_buf, &error);
    if (error != NULL) {
        LOG_ERROR("Failed to construct pipeline: %s", error->message);
        g_clear_error(&error);
        return 1;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    gst_object_unref(bus);

    /* start streaming */
    LOG_INFO("Streaming from file source\n");
    gst_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (gst_ret == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("Unable to set the pipeline to the playing state.");
        gst_object_unref(pipeline);
        return 1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* free resources */
    if (msg != NULL) {
        gst_message_unref(msg);
    }
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

string build_kvssink_str(string stream_name, unsigned long long file_start_time) {
    const char** property;
    stringstream ss;
    const char *key_raw, *value;
    int prefix_len = strlen(PROPERTY_PREFIX);
    char key[PROPERTY_KEY_MAX_LEN + 1];
    char *ch;

    ss << KVS_SINK_PLUGIN_NAME
       << " stream-name=" << stream_name
       << " streaming-type=" DEFAULT_STREAMING_TYPE  
       << " file-start-time=" << file_start_time;
    for (property = AVAILABLE_PROPERTIES; *property != NULL; property += 2) {
        key_raw = property[0];
        value = getenv(key_raw);
        if (value != NULL) {
            LOG_INFO("Found a property. Key: %s Value: %s", key_raw, value);
            
            // Remove property prefix and convert it into proper gstreamer syntax
            strncpy(key, key_raw + prefix_len, PROPERTY_KEY_MAX_LEN);
            for (ch = key; *ch != '\0'; ch++) {
                if (*ch == '_') {
                    *ch = '-';
                } else {
                    *ch = *ch - 'A' + 'a';
                }
            }
            ss << ' ' << key << '=' << value;
        }
    }

    return ss.str();
}

void print_usage(char* program_path) {
    char padding[PROPERTY_KEY_MAX_LEN + 1];
    const char **property;
    int spaces;
    memset(padding, ' ', PROPERTY_KEY_MAX_LEN + 1);

    printf(
        "USAGE\n"
        "    %1$s <stream_name> <file_path> <start_time> [video-only|audio-video]\n\n"
        "EXAMPLE\n"
        "    With AWS Credentials\n"
        "        AWS_ACCESS_KEY_ID=<access_key> AWS_SECRET_ACCESS_KEY=<secret_key> %1$s stream-sample video.mp4 0\n\n"
        "    With AWS IOT Credentials\n"
        "        IOT_CERTIFICATE=\"iot-certificate,endpoint=endpoint,cert-path=/path/to/certificate,key-path=/path/to/private/key,ca-path=/path/to/ca-cert,role-aliases=role-aliases\" \\\n"
        "            %1$s stream-sample video.mp4 0\n\n"
        "ARGUMENTS\n"
        "    stream_name     Name of the destination stream\n"
        "    file_path       A path to a file that you want to upload\n"
        "    start_time      Epoch time that the file starts in kinesis video stream. If zero, current time is used. Unit: Seconds\n"
        "    content_type    Optional. Either \"video-only\" or \"audio-video\"\n\n"
        "AVAILABLE PROPERTIES\n"
        "    AWS_DEFAULT_REGION              AWS Region\n"
        "    AWS_ACCESS_KEY_ID               AWS Access Key\n"
        "    AWS_SECRET_ACCESS_KEY           AWS Secret Key\n"
        "    AWS_SESSION_TOKEN               AWS Session Token\n", program_path);

    for (property = AVAILABLE_PROPERTIES; *property != NULL; property += 2) {
        spaces = PROPERTY_KEY_MAX_LEN - strlen(property[0]);
        if (spaces < 0) {
            spaces = 0;
        }
        padding[spaces] = '\0';
        printf("    %s%s%s\n", property[0], padding, property[1]);
        padding[spaces] = ' ';
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    CustomData data;
    int ret = 0;
    int retry_count = DEFAULT_RETRY_COUNT;
    int stream_status = STREAM_STATUS_OK;
    bool do_retry = true;
    char *ptr;
    FILE *file;

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 1;
    }
    string stream_name = argv[1];
    data.file_path = argv[2];
    // checking if fopen returns null seems to be the most portable way to check
    // file doesn't exist
    if ((file = fopen(data.file_path.c_str(), "r")) == NULL) {
        LOG_ERROR("File not found. Path: %s", data.file_path.c_str());
        return 1;
    }
    fclose(file);

    unsigned long long file_start_time = strtoull(argv[3], &ptr, 10);
    if (*ptr != '\0') {
        LOG_ERROR("File start time is not a valid number");
        return 1;
    }
    data.kvssink_str = build_kvssink_str(stream_name, file_start_time);
    LOG_INFO("Built kvssink pipeline string: \"%s\"", data.kvssink_str.c_str());
    data.content_type = CONTENT_TYPE_VIDEO_ONLY;

    if (argc > 4) {
        if (strcmp(argv[4], "audio-video") == 0) {
            LOG_INFO("Uploading audio and video");
            data.content_type = CONTENT_TYPE_AUDIO_VIDEO;
        } else if (strcmp(argv[4], "video-only") == 0) {
            LOG_INFO("Uploading video only");
        } else {
            LOG_INFO("Unrecognized upload type. Default to video-only");
        }
    } else {
        LOG_INFO("No upload type specified. Default to video-only");
    }

    do {
        LOG_INFO("Attempt to upload file: %s", data.file_path.c_str());

        // control will return after gstreamer_init after file eos or any GST_ERROR was put on the bus.
        ret = gstreamer_init(argc, argv, &data);
        if (ret != 0) {
            LOG_ERROR(
                "Failed to initialize gstreamer pipeline. Have you set GST_PLUGIN_PATH properly?\n\n"
                "     For example: export GST_PLUGIN_PATH=<YourSdkFolderPath>/build:$GST_PLUGIN_PATH");
            do_retry = false;
        } else if (data.stream_status == STREAM_STATUS_OK) {
            LOG_INFO("Persisted successfully. File: %s", data.file_path.c_str());
            do_retry = false;
        } else if (retry_count == 0) {
            LOG_ERROR("Failed to persist %s even after retrying", data.file_path.c_str());
            do_retry = false;
        } else {
            LOG_INFO("Failed to persist %s, retrying...", data.file_path.c_str());
        }
    } while(do_retry);

    return 0;
}
