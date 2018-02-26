#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string.h>
#include <chrono>
#include <Logger.h>
#include "KinesisVideoProducer.h"

using namespace std;
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

#define ACCESS_KEY_ENV_VAR "AWS_ACCESS_KEY_ID"
#define SECRET_KEY_ENV_VAR "AWS_SECRET_ACCESS_KEY"
#define SESSION_TOKEN_ENV_VAR "AWS_SESSION_TOKEN"
#define DEFAULT_REGION_ENV_VAR "AWS_DEFAULT_REGION"
#define MAX_URL_LENGTH 65536 // https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers

namespace com {
    namespace amazonaws {
        namespace kinesis {
            namespace video {

                class SampleClientCallbackProvider : public ClientCallbackProvider {
                public:

                    StorageOverflowPressureFunc getStorageOverflowPressureCallback() override {
                        return storageOverflowPressure;
                    }

                    static STATUS storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes);
                };

                class SampleStreamCallbackProvider : public StreamCallbackProvider {
                public:

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
                    streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 errored_timecode,
                                             STATUS status_code);

                    static STATUS
                    droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                              UINT64 dropped_frame_timecode);
                };

                class SampleCredentialProvider : public StaticCredentialProvider {
                    // Test rotation period is 40 second for the grace period.
                    const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(2400);
                public:
                    SampleCredentialProvider(const Credentials &credentials) :
                            StaticCredentialProvider(credentials) {}

                    void updateCredentials(Credentials &credentials) override {
                        // Copy the stored creds forward
                        credentials = credentials_;

                        // Update only the expiration
                        auto now_time = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch());
                        auto expiration_seconds = now_time + ROTATION_PERIOD;
                        credentials.setExpiration(std::chrono::seconds(expiration_seconds.count()));
                        LOG_INFO("New credentials expiration is " << credentials.getExpiration().count());
                    }
                };

                class SampleDeviceInfoProvider : public DefaultDeviceInfoProvider {
                public:
                    device_info_t getDeviceInfo() override {
                        auto device_info = DefaultDeviceInfoProvider::getDeviceInfo();
                        // Set the storage size to 512MB
                        device_info.storageInfo.storageSize = 512 * 1024 * 1024;
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
                                                                       UINT64 errored_timecode, STATUS status_code) {
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
    GstElement *pipeline, *source, *depay, *appsink, *filter, *target_for_source;

    GstBus *bus;
    GMainLoop *main_loop;
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    shared_ptr<KinesisVideoStream> kinesis_video_stream;
    bool stream_started;;
} CustomData;

void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *data, size_t len) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(data);
}

bool put_frame(shared_ptr<KinesisVideoStream> kinesis_video_stream, void *data, size_t len, const nanoseconds &pts,
               const nanoseconds &dts, FRAME_FLAGS flags) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, data, len);
    return kinesis_video_stream->putFrame(frame);
}

static GstFlowReturn on_new_sample(GstElement *sink, CustomData *data) {
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK (sink));

    GstCaps *gstcaps = (GstCaps *) gst_sample_get_caps(sample);
    GST_LOG("caps are %"
                    GST_PTR_FORMAT, gstcaps);

    GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);


    if (!data->stream_started) {
        data->stream_started = true;
        const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
        gchar *cpd = gst_value_serialize(gstStreamFormat);
        data->kinesis_video_stream->start(std::string(cpd));
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    size_t buffer_size = gst_buffer_get_size(buffer);
    uint8_t *frame_data = new uint8_t[buffer_size];
    gst_buffer_extract(buffer, 0, frame_data, buffer_size);

    bool delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    FRAME_FLAGS kinesis_video_flags;
   
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
         buffer->dts = buffer->pts;
    }
    else {
         buffer->pts = buffer->dts;
    }

    if (!delta) {
        kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
    } else {
        kinesis_video_flags = FRAME_FLAG_NONE;
    }
    DLOGE("kinesis_video_flags=%u pts=%llu dts=%llu  \n", kinesis_video_flags, buffer->pts, buffer->dts);

    if (false == put_frame(data->kinesis_video_stream, frame_data, buffer_size, std::chrono::nanoseconds(buffer->pts),
                           std::chrono::nanoseconds(buffer->dts), kinesis_video_flags)) {
        GST_WARNING("Dropped frame");
    }

    delete[] frame_data;
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

void kinesis_video_init(CustomData *data, char *stream_name) {
    unique_ptr<DeviceInfoProvider> device_info_provider = make_unique<SampleDeviceInfoProvider>();
    unique_ptr<ClientCallbackProvider> client_callback_provider = make_unique<SampleClientCallbackProvider>();
    unique_ptr<StreamCallbackProvider> stream_callback_provider = make_unique<SampleStreamCallbackProvider>();

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

    credentials_ = make_unique<Credentials>(string(accessKey),
                                            string(secretKey),
                                            sessionTokenStr,
                                            std::chrono::seconds(180));
    unique_ptr<CredentialProvider> credential_provider = make_unique<SampleCredentialProvider>(*credentials_.get());

    data->kinesis_video_producer = KinesisVideoProducer::createSync(move(device_info_provider),
                                                                    move(client_callback_provider),
                                                                    move(stream_callback_provider),
                                                                    move(credential_provider),
                                                                    defaultRegionStr);

    LOG_DEBUG("Client is ready");
    /* create a test stream */
    map<string, string> tags;
    char tag_name[MAX_TAG_NAME_LEN];
    char tag_val[MAX_TAG_VALUE_LEN];
    sprintf(tag_name, "piTag");
    sprintf(tag_val, "piValue");
    auto stream_definition = make_unique<StreamDefinition>(stream_name,
                                                           hours(2),
                                                           &tags,
                                                           "",
                                                           STREAMING_TYPE_REALTIME,
                                                           "video/h264",
                                                           milliseconds::zero(),
                                                           seconds(2),
                                                           milliseconds(1),
                                                           true,
                                                           true,
                                                           false,
                                                           true,
                                                           true,
                                                           true,
                                                           0,
                                                           30,
                                                           4 * 1024 * 1024,
                                                           seconds(120),
                                                           seconds(40),
                                                           seconds(30),
                                                           "V_MPEG4/ISO/AVC",
                                                           "kinesis_video",
                                                           nullptr,
                                                           0);
    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(move(stream_definition));

    LOG_DEBUG("Stream is ready");
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
}

int gstreamer_init(int argc, char *argv[]) {
    BasicConfigurator config;
    config.configure();

    if (argc < 3) {
        LOG_ERROR(
                "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_app rtsp-url my-stream-name");
        return 1;
    }

    CustomData data;
    GstStateChangeReturn ret;
    bool vtenc;

    /* init data struct */
    memset(&data, 0, sizeof(data));

    /* init GStreamer */
    gst_init(&argc, &argv);

    /* init Kinesis Video */
    char stream_name[MAX_STREAM_NAME_LEN];
    SNPRINTF(stream_name, MAX_STREAM_NAME_LEN, argv[2]);
    kinesis_video_init(&data, stream_name);

    char rtsp_url[MAX_URL_LENGTH];
    SNPRINTF(rtsp_url, MAX_STREAM_NAME_LEN, argv[1]);

    // Appsink: This is us
    data.appsink = gst_element_factory_make("appsink", "appsink");

    // rtph264depay: Converts rtp data to raw h264
    data.depay = gst_element_factory_make("rtph264depay", "depay");

    // RTSP source component
    data.source = gst_element_factory_make("rtspsrc", "source");
 
    data.filter = gst_element_factory_make("capsfilter", "encoder_filter");

    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                              NULL);
    g_object_set(G_OBJECT (data.filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);
    /* create an empty pipeline */
    data.pipeline = gst_pipeline_new("rtsp-kinesis-pipeline");

    if (!data.pipeline || !data.source || !data.depay  || !data.appsink) {
        g_printerr("Not all elements could be created:\n");
        if (!data.pipeline) g_printerr("\tCore pipeline\n");
        if (!data.source) g_printerr("\trtspsrc (gst-plugins-good)\n");
        if (!data.depay) g_printerr("\trtph264depay (gst-plugins-good)\n");
        if (!data.appsink) g_printerr("\tappsink (gst-plugins-base)\n");
        return 1;
    }
    LOG_DEBUG("Pipeline OK");
    LOG_DEBUG(rtsp_url);

    g_object_set(G_OBJECT (data.source),
                 "location", rtsp_url,
                 "short-header", true, // Necessary for target camera
                 NULL);


    /* configure appsink */
    g_object_set(G_OBJECT (data.appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(data.appsink, "new-sample", G_CALLBACK(on_new_sample), &data);
    LOG_DEBUG("appsink configured");

    g_signal_connect(data.source, "pad-added", G_CALLBACK(cb_rtsp_pad_created), &data);

    /* build the pipeline */
    gst_bin_add_many(GST_BIN (data.pipeline), data.source,
                     data.depay, data.filter, data.appsink,
                     NULL);

    LOG_DEBUG("Pipeline built, elements *NOT* yet linked");

    /* Leave the actual source out - this will be done when the pad is added */
    if (gst_element_link_many(data.depay, data.filter,
                              data.appsink,
                              NULL) != TRUE) {

        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return 1;
    }

    data.target_for_source = data.depay; // Set target for RTSP source

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
    LOG_DEBUG("Pipeline playing");

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

