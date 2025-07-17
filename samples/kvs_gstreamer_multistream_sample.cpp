#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string.h>
#include <chrono>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;
using namespace std::chrono;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

// Camera configuration structure
struct CameraConfig {
    std::string rtsp_url;
    std::string name;
    std::string codec;
};

// Simple JSON parser for cameras.json
std::vector<CameraConfig> parseCamerasJson(const std::string& filename) {
    std::vector<CameraConfig> cameras;
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open cameras.json file: " << filename);
        return cameras;
    }
    
    std::string line;
    CameraConfig current_camera;
    bool in_object = false;
    
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line == "{") {
            in_object = true;
            current_camera = CameraConfig();
        } else if (line == "}" || line == "},") {
            if (in_object && !current_camera.rtsp_url.empty()) {
                cameras.push_back(current_camera);
            }
            in_object = false;
        } else if (in_object && line.find("\"rtsp_url\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.rfind("\"");
            if (start < end) {
                current_camera.rtsp_url = line.substr(start, end - start);
            }
        } else if (in_object && line.find("\"name\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.rfind("\"");
            if (start < end) {
                current_camera.name = line.substr(start, end - start);
            }
        } else if (in_object && line.find("\"codec\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.rfind("\"");
            if (start < end) {
                current_camera.codec = line.substr(start, end - start);
            }
        }
    }
    
    file.close();
    return cameras;
}

#ifdef __cplusplus
extern "C" {
#endif

int gstreamer_init(int, char **);

#ifdef __cplusplus
}
#endif

LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");

/*
 * https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers
 */
#define MAX_URL_LENGTH 65536
#define DEFAULT_RETENTION_PERIOD_HOURS 2
#define DEFAULT_KMS_KEY_ID ""
#define DEFAULT_STREAMING_TYPE STREAMING_TYPE_REALTIME
#define DEFAULT_CONTENT_TYPE "video/h264"
#define DEFAULT_MAX_LATENCY_SECONDS 60
#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES TRUE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_STREAM_FRAMERATE 25
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
#define DEFAULT_BUFFER_DURATION_SECONDS 120
#define DEFAULT_REPLAY_DURATION_SECONDS 40
#define DEFAULT_CONNECTION_STALENESS_SECONDS 60
#define DEFAULT_CODEC_ID "V_MPEG4/ISO/AVC"
#define DEFAULT_TRACKNAME "kinesis_video"
#define APP_SINK_BASE_NAME "appsink"
#define DEFAULT_BUFFER_SIZE (1 * 1024 * 1024)
#define DEFAULT_STORAGE_SIZE (128 * 1024 * 1024)
#define DEFAULT_ROTATION_TIME_SECONDS 3600

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class SampleClientCallbackProvider : public ClientCallbackProvider {
    public:

        UINT64 getCallbackCustomData() override {
            return reinterpret_cast<UINT64> (this);
        }

        StorageOverflowPressureFunc getStorageOverflowPressureCallback() override {
            return storageOverflowPressure;
        }

        static STATUS storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes);
    };

    class SampleStreamCallbackProvider : public StreamCallbackProvider {
    public:

        UINT64 getCallbackCustomData() override {
            return reinterpret_cast<UINT64> (this);
        }

        StreamConnectionStaleFunc getStreamConnectionStaleCallback() override {
            return streamConnectionStaleHandler;
        };

        StreamErrorReportFunc getStreamErrorReportCallback() override {
            return streamErrorReportHandler;
        };

        DroppedFrameReportFunc getDroppedFrameReportCallback() override {
            return droppedFrameReportHandler;
        };

    private:
        static STATUS
        streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                     UINT64 last_buffering_ack);

        static STATUS
        streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 errored_timecode,
                                 STATUS status_code);

        static STATUS
        droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                  UINT64 dropped_frame_timecode);
    };

    class SampleCredentialProvider : public StaticCredentialProvider {
        // Test rotation period is 40 second for the grace period.
        const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(DEFAULT_ROTATION_TIME_SECONDS);
    public:
        SampleCredentialProvider(const Credentials &credentials) :
                StaticCredentialProvider(credentials) {}

        void updateCredentials(Credentials &credentials) override {
            // Copy the stored creds forward
            credentials = credentials_;

            // Update only the expiration
            auto now_time = std::chrono::duration_cast<std::chrono::seconds>(
                    systemCurrentTime().time_since_epoch());
            auto expiration_seconds = now_time + ROTATION_PERIOD;
            credentials.setExpiration(std::chrono::seconds(expiration_seconds.count()));
            LOG_INFO("New credentials expiration is " << credentials.getExpiration().count());
        }
    };

    class SampleDeviceInfoProvider : public DefaultDeviceInfoProvider {
    public:
        device_info_t getDeviceInfo() override {
            auto device_info = DefaultDeviceInfoProvider::getDeviceInfo();
            // Set the storage size to 64MB
            device_info.storageInfo.storageSize = DEFAULT_STORAGE_SIZE;
            return device_info;
        }
    };

    STATUS
    SampleClientCallbackProvider::storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes) {
        UNUSED_PARAM(custom_handle);
        LOG_WARN("Reporting storage overflow. Bytes remaining " << remaining_bytes);
        return STATUS_SUCCESS;
    }

    STATUS SampleStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                                      STREAM_HANDLE stream_handle,
                                                                      UINT64 last_buffering_ack) {
        LOG_WARN("Reporting stream stale. Last ACK received " << last_buffering_ack);
        return STATUS_SUCCESS;
    }

    STATUS
    SampleStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                           UPLOAD_HANDLE upload_handle, UINT64 errored_timecode, STATUS status_code) {
        LOG_ERROR("Reporting stream error. Errored timecode: " << errored_timecode << " Status: "
                                                               << status_code);
        return STATUS_SUCCESS;
    }

    STATUS
    SampleStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                            UINT64 dropped_frame_timecode) {
        LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
        return STATUS_SUCCESS;
    }

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;

unique_ptr<Credentials> credentials_;

typedef struct _CustomData {
    GMainLoop *main_loop;
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    map<string, shared_ptr<KinesisVideoStream>> kinesis_video_stream_handles;
    vector<GstElement *> pipelines;
    map<string, bool> stream_started;
    map<string, uint8_t*> frame_data_map;
    map<string, UINT32> frame_data_size_map;
    // Pts of first frame
    map<string, uint64_t> first_pts_map;
    map<string, uint64_t> producer_start_time_map;
    // Metadata tracking
    map<string, CameraConfig> camera_configs;      // Store camera config by appsink name
    map<string, uint64_t> last_metadata_time;      // Track when metadata was last added (in seconds)
} CustomData;

void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *data, size_t len) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    // set duration to 0 due to potential high spew from rtsp streams
    frame->duration = 0;
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(data);
    frame->trackId = DEFAULT_TRACK_ID;
}

bool put_frame(shared_ptr<KinesisVideoStream> kinesis_video_stream, void *data, size_t len, const nanoseconds &pts,
               const nanoseconds &dts, FRAME_FLAGS flags) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, data, len);
    return kinesis_video_stream->putFrame(frame);
}

static GstFlowReturn on_new_sample(GstElement *sink, CustomData *data) {
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK (sink));
    gchar *g_stream_handle_key = gst_element_get_name(sink);
    string stream_handle_key = string(g_stream_handle_key);
    GstCaps *gstcaps = (GstCaps *) gst_sample_get_caps(sample);
    GST_LOG("caps are %" GST_PTR_FORMAT, gstcaps);
    bool isHeader, isDroppable;

    GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);

    if (!data->stream_started[stream_handle_key]) {
        data->stream_started[stream_handle_key] = true;
        const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
        gchar *cpd = gst_value_serialize(gstStreamFormat);

        auto kvs_stream = data->kinesis_video_stream_handles[stream_handle_key];
        kvs_stream->start(std::string(cpd));
        g_free(cpd);
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    size_t buffer_size = gst_buffer_get_size(buffer);

    isHeader = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_HEADER);
    isDroppable = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) ||
                  GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
                  (GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
                  (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                  // drop if buffer contains header only and has invalid timestamp
                  (isHeader && (!GST_BUFFER_PTS_IS_VALID(buffer) || !GST_BUFFER_DTS_IS_VALID(buffer)));

    if (!isDroppable) {
        UINT32 frame_data_size = data->frame_data_size_map[stream_handle_key];
        if (frame_data_size < buffer_size) {
            frame_data_size = frame_data_size * 2;
            delete [] data->frame_data_map[stream_handle_key];
            data->frame_data_size_map[stream_handle_key] = frame_data_size;
            data->frame_data_map[stream_handle_key] = new uint8_t[frame_data_size];
        }

        gst_buffer_extract(buffer, 0, data->frame_data_map[stream_handle_key], buffer_size);

        bool delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        FRAME_FLAGS kinesis_video_flags;

        if (!delta) {
            kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
        } else {
            kinesis_video_flags = FRAME_FLAG_NONE;
        }

        if (!data->first_pts_map[stream_handle_key]) {
            data->first_pts_map[stream_handle_key] = buffer->pts;
            data->producer_start_time_map[stream_handle_key] = chrono::duration_cast<nanoseconds>(systemCurrentTime().time_since_epoch()).count();
        }

        buffer->pts += data->producer_start_time_map[stream_handle_key] - data->first_pts_map[stream_handle_key];

        if (false == put_frame(data->kinesis_video_stream_handles[stream_handle_key], data->frame_data_map[stream_handle_key], buffer_size, std::chrono::nanoseconds(buffer->pts),
                               std::chrono::nanoseconds(buffer->dts), kinesis_video_flags)) {
            GST_WARNING("Dropped frame");
        }
        
        // Add AWS_KINESISVIDEO_IMAGE_GENERATION metadata every 5 seconds
        auto current_time = std::chrono::system_clock::now();
        auto time_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(current_time.time_since_epoch()).count();
        
        if (time_since_epoch - data->last_metadata_time[stream_handle_key] >= 5) {
            auto kvs_stream = data->kinesis_video_stream_handles[stream_handle_key];
            bool metadata_result = kvs_stream->putFragmentMetadata("AWS_KINESISVIDEO_IMAGE_GENERATION", "", false);
            
            if (metadata_result) {
                LOG_DEBUG("Added AWS_KINESISVIDEO_IMAGE_GENERATION metadata for camera: " << data->camera_configs[stream_handle_key].name);
                data->last_metadata_time[stream_handle_key] = time_since_epoch;
            } else {
                LOG_WARN("Failed to add AWS_KINESISVIDEO_IMAGE_GENERATION metadata for camera: " << data->camera_configs[stream_handle_key].name);
            }
        }
    }

    gst_sample_unref(sample);

    return GST_FLOW_OK;
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

void kinesis_video_init(CustomData *data) {
    unique_ptr<DeviceInfoProvider> device_info_provider(new SampleDeviceInfoProvider());
    unique_ptr<ClientCallbackProvider> client_callback_provider(new SampleClientCallbackProvider());
    unique_ptr<StreamCallbackProvider> stream_callback_provider(new SampleStreamCallbackProvider());

    char const *accessKey;
    char const *secretKey;
    char const *sessionToken;
    char const *defaultRegion;
    string defaultRegionStr;
    string sessionTokenStr;
    if (nullptr == (accessKey = getenv(ACCESS_KEY_ENV_VAR))) {
        accessKey = "";
    }

    if (nullptr == (secretKey = getenv(SECRET_KEY_ENV_VAR))) {
        secretKey = "";
    }

    if (nullptr == (sessionToken = getenv(SESSION_TOKEN_ENV_VAR))) {
        sessionTokenStr = "";
    } else {
        sessionTokenStr = string(sessionToken);
    }

    if (nullptr == (defaultRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
        defaultRegionStr = DEFAULT_AWS_REGION;
    } else {
        defaultRegionStr = string(defaultRegion);
    }

    credentials_.reset(new Credentials(string(accessKey),
                                       string(secretKey),
                                       sessionTokenStr,
                                       std::chrono::seconds(180)));
    unique_ptr<CredentialProvider> credential_provider(new SampleCredentialProvider(*credentials_.get()));

    data->kinesis_video_producer = KinesisVideoProducer::createSync(std::move(device_info_provider),
                                                                    std::move(client_callback_provider),
                                                                    std::move(stream_callback_provider),
                                                                    std::move(credential_provider),
                                                                    API_CALL_CACHE_TYPE_ALL,
                                                                    defaultRegionStr);

    LOG_DEBUG("Client is ready");
}

void kinesis_stream_init(string stream_name, CustomData *data, string stream_handle_key, string codec = "h264") {
    /* create a test stream */
    string content_type = (codec == "h265") ? "video/h265" : "video/h264";
    string codec_id = (codec == "h265") ? "V_MPEGH/ISO/HEVC" : "V_MPEG4/ISO/AVC";
    
    unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(
        stream_name.c_str(),
        hours(DEFAULT_RETENTION_PERIOD_HOURS),
        nullptr,
        DEFAULT_KMS_KEY_ID,
        DEFAULT_STREAMING_TYPE,
        content_type.c_str(),
        duration_cast<milliseconds> (seconds(DEFAULT_MAX_LATENCY_SECONDS)),
        milliseconds(DEFAULT_FRAGMENT_DURATION_MILLISECONDS),
        milliseconds(DEFAULT_TIMECODE_SCALE_MILLISECONDS),
        DEFAULT_KEY_FRAME_FRAGMENTATION,
        DEFAULT_FRAME_TIMECODES,
        DEFAULT_ABSOLUTE_FRAGMENT_TIMES,
        DEFAULT_FRAGMENT_ACKS,
        DEFAULT_RESTART_ON_ERROR,
        DEFAULT_RECALCULATE_METRICS,
        true,
        NAL_ADAPTATION_FLAG_NONE,
        DEFAULT_STREAM_FRAMERATE,
        DEFAULT_AVG_BANDWIDTH_BPS,
        seconds(DEFAULT_BUFFER_DURATION_SECONDS),
        seconds(DEFAULT_REPLAY_DURATION_SECONDS),
        seconds(DEFAULT_CONNECTION_STALENESS_SECONDS),
        codec_id.c_str(),
        DEFAULT_TRACKNAME,
        nullptr,
        0));
    auto kvs_stream = data->kinesis_video_producer->createStreamSync(std::move(stream_definition));
    data->kinesis_video_stream_handles[stream_handle_key] = kvs_stream;
    data->frame_data_size_map[stream_handle_key] = DEFAULT_BUFFER_SIZE;
    data->frame_data_map[stream_handle_key] = new uint8_t[DEFAULT_BUFFER_SIZE];
    LOG_DEBUG("Stream is ready: " << stream_name);
}

/* callback when each RTSP stream has been created */
static void cb_rtsp_pad_created(GstElement *element, GstPad *pad, gpointer data) {
    gchar *pad_name = gst_pad_get_name(pad);
    GstElement *other = reinterpret_cast<GstElement *>(data);
    g_print("New RTSP source found: %s\n", pad_name);
    if (gst_element_link(element, other)) {
        g_print("Source linked.\n");
    } else {
        g_print("Source link FAILED\n");
    }
    g_free(pad_name);
}

int gstreamer_init(int argc, char *argv[]) {
    PropertyConfigurator::doConfigure("../kvs_log_configuration");

    if (argc < 2) {
        LOG_ERROR(
                "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_multistream_app cameras.json\n" <<
                "cameras.json: JSON file containing camera configurations with rtsp_url, name, and codec fields");

        return 1;
    }

    CustomData data;
    GstStateChangeReturn ret;

    /* init data struct */
    memset(&data, 0, sizeof(data));

    data.kinesis_video_stream_handles = map<string, shared_ptr<KinesisVideoStream>>();
    data.stream_started = map<string, bool>();
    data.pipelines = vector<GstElement *>();
    data.frame_data_map = map<string, uint8_t*>();
    data.frame_data_size_map = map<string, UINT32>();
    data.first_pts_map = map<string, uint64_t>();
    data.producer_start_time_map = map<string, uint64_t>();
    data.camera_configs = map<string, CameraConfig>();
    data.last_metadata_time = map<string, uint64_t>();

    /* init GStreamer */
    gst_init(&argc, &argv);

    /* Parse cameras.json file */
    std::vector<CameraConfig> cameras = parseCamerasJson(argv[1]);
    
    if (cameras.empty()) {
        LOG_ERROR("No cameras found in JSON file");
        return 1;
    }
    
    LOG_INFO("Found " << cameras.size() << " cameras in configuration");

    /* init Kinesis Video */
    string base_appsink_name(APP_SINK_BASE_NAME);
    kinesis_video_init(&data);
    
    for (size_t i = 0; i < cameras.size(); ++i) {
        const CameraConfig& camera = cameras[i];
        // Use camera name from JSON, replacing spaces and special characters with underscores
        string stream_name = camera.name;
        std::replace(stream_name.begin(), stream_name.end(), ' ', '_');
        std::replace(stream_name.begin(), stream_name.end(), '.', '_');
        std::replace(stream_name.begin(), stream_name.end(), '-', '_');
        
        string appsink_name = base_appsink_name + std::to_string(i);
        
        LOG_INFO("Setting up camera: " << camera.name << " with codec: " << camera.codec);
        
        kinesis_stream_init(stream_name, &data, appsink_name, camera.codec);
        
        // Store camera configuration for metadata context
        data.camera_configs[appsink_name] = camera;
        data.last_metadata_time[appsink_name] = 0;  // Initialize to 0 for immediate first metadata
        
        GstElement *appsink, *depay, *source, *filter, *pipeline;

        appsink = gst_element_factory_make("appsink", appsink_name.c_str());
        
        // Select appropriate depayloader based on codec
        if (camera.codec == "h264") {
            depay = gst_element_factory_make("rtph264depay", "depay");
        } else if (camera.codec == "h265") {
            depay = gst_element_factory_make("rtph265depay", "depay");
        } else {
            LOG_ERROR("Unsupported codec: " << camera.codec << " for camera: " << camera.name);
            continue;
        }
        source = gst_element_factory_make("rtspsrc", "source");
        filter = gst_element_factory_make("capsfilter", "encoder_filter");
        
        // Set caps based on codec
        GstCaps *caps;
        if (camera.codec == "h264") {
            caps = gst_caps_new_simple("video/x-h264",
                                     "stream-format", G_TYPE_STRING, "avc",
                                     "alignment", G_TYPE_STRING, "au",
                                     NULL);
        } else { // h265
            caps = gst_caps_new_simple("video/x-h265",
                                     "stream-format", G_TYPE_STRING, "hvc1",
                                     "alignment", G_TYPE_STRING, "au",
                                     NULL);
        }
        
        g_object_set(G_OBJECT (filter), "caps", caps, NULL);
        gst_caps_unref(caps);
        pipeline = gst_pipeline_new("rtsp-kinesis-pipeline");

        if (!pipeline || !source || !depay  || !appsink) {
            g_printerr("Not all elements could be created for camera %s:\n", camera.name.c_str());
            if (!pipeline) g_printerr("\tCore pipeline\n");
            if (!source) g_printerr("\trtspsrc (gst-plugins-good)\n");
            if (!depay) g_printerr("\trtph264depay/rtph265depay (gst-plugins-good)\n");
            if (!appsink) g_printerr("\tappsink (gst-plugins-base)\n");
            continue;
        }

        g_object_set(G_OBJECT (source),
                     "location", camera.rtsp_url.c_str(),
                     "short-header", true, // Necessary for target camera
                     NULL);


        /* configure appsink */
        g_object_set(G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
        g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), &data);
        LOG_DEBUG("appsink configured for camera: " << camera.name);

        g_signal_connect(source, "pad-added", G_CALLBACK(cb_rtsp_pad_created), depay);

        /* build the pipeline */
        gst_bin_add_many(GST_BIN (pipeline), source,
                         depay, filter, appsink,
                         NULL);

        /* Leave the actual source out - this will be done when the pad is added */
        if (!gst_element_link_many(depay, filter,
                                   appsink,
                                   NULL)) {

            g_printerr("Elements could not be linked for camera: %s\n", camera.name.c_str());
            gst_object_unref(pipeline);
            continue;
        }

        data.pipelines.push_back(pipeline);
    }

    for (GstElement *pipeline : data.pipelines) {
        ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("Unable to set the pipeline to the playing state.\n");
            goto CleanUp;
        }
    }

    LOG_DEBUG("Pipeline playing");

    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

CleanUp:
    for (GstElement *pipeline : data.pipelines) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    for (auto frame_data : data.frame_data_map) {
        delete [] frame_data.second;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    return gstreamer_init(argc, argv);
}
