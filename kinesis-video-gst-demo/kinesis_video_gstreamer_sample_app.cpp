#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string.h>
#include <chrono>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include <vector>
#include <stdlib.h>
#include <mutex>
#include <IotCertCredentialProvider.h>

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
#define DEFAULT_FRAME_DURATION_MS 1
#define DEFAULT_CREDENTIAL_ROTATION_SECONDS 3600
#define DEFAULT_CREDENTIAL_EXPIRATION_SECONDS 180

typedef enum _StreamSource {
    FILE_SOURCE,
    LIVE_SOURCE,
    RTSP_SOURCE
} StreamSource;

typedef struct _FileInfo {
    _FileInfo():
            path(""),
            last_fragment_ts(0) {}
    string path;
    uint64_t last_fragment_ts;
} FileInfo;

typedef struct _CustomData {

    _CustomData():
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
            use_absolute_fragment_times(true) {
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
} CustomData;

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
    UINT64 custom_data_;
public:
    SampleStreamCallbackProvider(UINT64 custom_data) : custom_data_(custom_data) {}

    UINT64 getCallbackCustomData() override {
        return custom_data_;
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

    FragmentAckReceivedFunc getFragmentAckReceivedCallback() override {
        return fragmentAckReceivedHandler;
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

    static STATUS
    fragmentAckReceivedHandler( UINT64 custom_data, STREAM_HANDLE stream_handle,
                                UPLOAD_HANDLE upload_handle, PFragmentAck pFragmentAck);
};

class SampleCredentialProvider : public StaticCredentialProvider {
    // Test rotation period is 40 second for the grace period.
    const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(DEFAULT_CREDENTIAL_ROTATION_SECONDS);
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
        // Set the storage size to 128mb
        device_info.storageInfo.storageSize = 128 * 1024 * 1024;
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
    CustomData *data = reinterpret_cast<CustomData *>(custom_data);
    bool terminate_pipeline = false;

    // Terminate pipeline if error is not retriable or if error is retriable but we are streaming file.
    // When streaming file, we choose to terminate the pipeline on error because the easiest way to recover
    // is to stream the file from the beginning again.
    // In realtime streaming, retriable error can be handled underneath. Otherwise terminate pipeline
    // and store error status if error is fatal.
    if ((IS_RETRIABLE_ERROR(status_code) && data->streamSource == FILE_SOURCE) ||
        (!IS_RETRIABLE_ERROR(status_code) && !IS_RECOVERABLE_ERROR(status_code))) {
        data->stream_status = status_code;
        terminate_pipeline = true;
    }

    if (terminate_pipeline && data->main_loop != NULL) {
        LOG_WARN("Terminating pipeline due to unrecoverable stream error: " << status_code);
        g_main_loop_quit(data->main_loop);
    }

    return STATUS_SUCCESS;
}

STATUS
SampleStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                        UINT64 dropped_frame_timecode) {
    LOG_WARN("Reporting dropped frame. Frame timecode " << dropped_frame_timecode);
    return STATUS_SUCCESS;
}

STATUS
SampleStreamCallbackProvider::fragmentAckReceivedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                                         UPLOAD_HANDLE upload_handle, PFragmentAck pFragmentAck) {
    CustomData *data = reinterpret_cast<CustomData *>(custom_data);
    if (data->streamSource == FILE_SOURCE && pFragmentAck->ackType == FRAGMENT_ACK_TYPE_PERSISTED) {
        std::unique_lock<std::mutex> lk(data->file_list_mtx);
        uint32_t last_unpersisted_file_idx = data->last_unpersisted_file_idx.load();
        uint64_t last_frag_ts = data->file_list.at(last_unpersisted_file_idx).last_fragment_ts /
                                duration_cast<nanoseconds>(milliseconds(DEFAULT_TIMECODE_SCALE_MILLISECONDS)).count();
        if (last_frag_ts != 0 && last_frag_ts == pFragmentAck->timestamp) {
            data->last_unpersisted_file_idx = last_unpersisted_file_idx + 1;
            LOG_INFO("Successfully persisted file " << data->file_list.at(last_unpersisted_file_idx).path);
        }
    }
    LOG_DEBUG("Reporting fragment ack received. Ack timecode " << pFragmentAck->timestamp);
    return STATUS_SUCCESS;
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;

static void eos_cb(GstElement *sink, CustomData *data) {
    // bookkeeping base_pts. add 1ms to avoid overlap.
    data->base_pts += + data->max_frame_pts + duration_cast<nanoseconds>(milliseconds(1)).count();
    data->max_frame_pts = 0;

    {
        std::unique_lock<std::mutex> lk(data->file_list_mtx);
        // store file's last fragment's timestamp.
        data->file_list.at(data->current_file_idx).last_fragment_ts = data->key_frame_pts;
    }

    LOG_DEBUG("Terminating pipeline due to EOS");
    g_main_loop_quit(data->main_loop);
}

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

bool put_frame(shared_ptr<KinesisVideoStream> kinesis_video_stream, void *data, size_t len, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, data, len);
    return kinesis_video_stream->putFrame(frame);
}

static GstFlowReturn on_new_sample(GstElement *sink, CustomData *data) {
    GstBuffer *buffer;
    bool isDroppable, isHeader, delta;
    size_t buffer_size;
    GstFlowReturn ret = GST_FLOW_OK;
    STATUS curr_stream_status = data->stream_status.load();
    GstSample *sample = nullptr;
    GstMapInfo info;

    if (STATUS_FAILED(curr_stream_status)) {
        LOG_ERROR("Received stream error: " << curr_stream_status);
        ret = GST_FLOW_ERROR;
        goto CleanUp;
    }

    info.data = nullptr;
    sample = gst_app_sink_pull_sample(GST_APP_SINK (sink));

    // capture cpd at the first frame
    if (!data->stream_started) {
        data->stream_started = true;
        GstCaps* gstcaps  = (GstCaps*) gst_sample_get_caps(sample);
        GstStructure * gststructforcaps = gst_caps_get_structure(gstcaps, 0);
        const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
        gchar *cpd = gst_value_serialize(gstStreamFormat);
        data->kinesis_video_stream->start(std::string(cpd));
        g_free(cpd);
    }

    buffer = gst_sample_get_buffer(sample);
    isHeader = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_HEADER);
    isDroppable = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) ||
                  GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
                  (GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
                  (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                  // drop if buffer contains header only and has invalid timestamp
                  (isHeader && (!GST_BUFFER_PTS_IS_VALID(buffer) || !GST_BUFFER_DTS_IS_VALID(buffer)));

    if (!isDroppable) {

        delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

        FRAME_FLAGS kinesis_video_flags = delta ? FRAME_FLAG_NONE : FRAME_FLAG_KEY_FRAME;

        // Always synthesize dts for file sources because file sources dont have meaningful dts.
        // For some rtsp sources the dts is invalid, therefore synthesize.
        if (data->streamSource == FILE_SOURCE || !GST_BUFFER_DTS_IS_VALID(buffer)) {
            data->synthetic_dts += DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
            buffer->dts = data->synthetic_dts;
        } else if (GST_BUFFER_DTS_IS_VALID(buffer)) {
            data->synthetic_dts = buffer->dts;
        }

        if (data->streamSource == FILE_SOURCE) {
            data->max_frame_pts = MAX(data->max_frame_pts, buffer->pts);

            // make sure the timestamp is continuous across multiple files.
            buffer->pts += data->base_pts + data->producer_start_time;

            if (CHECK_FRAME_FLAG_KEY_FRAME(kinesis_video_flags)) {
                data->key_frame_pts = buffer->pts;
            }
        } else if (data->use_absolute_fragment_times) {
            if (data->first_pts == GST_CLOCK_TIME_NONE) {
                data->first_pts = buffer->pts;
            }
            buffer->pts += data->producer_start_time - data->first_pts;
        }

        if (!gst_buffer_map(buffer, &info, GST_MAP_READ)){
            goto CleanUp;
        }

        put_frame(data->kinesis_video_stream, info.data, info.size, std::chrono::nanoseconds(buffer->pts),
                               std::chrono::nanoseconds(buffer->dts), kinesis_video_flags);
    }

CleanUp:

    if (info.data != nullptr) {
        gst_buffer_unmap(buffer, &info);
    }

    if (sample != nullptr) {
        gst_sample_unref(sample);
    }

    return ret;
}

static bool format_supported_by_source(GstCaps *src_caps, GstCaps *query_caps, int width, int height, int framerate) {
    gst_caps_set_simple(query_caps,
                        "width", G_TYPE_INT, width,
                        "height", G_TYPE_INT, height,
                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                        NULL);
    bool is_match = gst_caps_can_intersect(query_caps, src_caps);

    // in case the camera has fps as 10000000/333333
    if(!is_match) {
        gst_caps_set_simple(query_caps,
                            "framerate", GST_TYPE_FRACTION_RANGE, framerate, 1, framerate+1, 1,
                            NULL);
        is_match = gst_caps_can_intersect(query_caps, src_caps);
    }

    return is_match;
}

static bool resolution_supported(GstCaps *src_caps, GstCaps *query_caps_raw, GstCaps *query_caps_h264,
                                 CustomData &data, int width, int height, int framerate) {
    if (query_caps_h264 && format_supported_by_source(src_caps, query_caps_h264, width, height, framerate)) {
        data.h264_stream_supported = true;
    } else if (query_caps_raw && format_supported_by_source(src_caps, query_caps_raw, width, height, framerate)) {
        data.h264_stream_supported = false;
    } else {
        return false;
    }
    return true;
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
    unique_ptr<StreamCallbackProvider> stream_callback_provider(new SampleStreamCallbackProvider(
            reinterpret_cast<UINT64>(data)));

    char const *accessKey;
    char const *secretKey;
    char const *sessionToken;
    char const *defaultRegion;
    string defaultRegionStr;
    string sessionTokenStr;

    char const *iot_get_credential_endpoint;
    char const *cert_path;
    char const *private_key_path;
    char const *role_alias;
    char const *ca_cert_path;

    unique_ptr<CredentialProvider> credential_provider;

    if (nullptr == (defaultRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
        defaultRegionStr = DEFAULT_AWS_REGION;
    } else {
        defaultRegionStr = string(defaultRegion);
    }
    LOG_INFO("Using region: " << defaultRegionStr);

    if (nullptr != (accessKey = getenv(ACCESS_KEY_ENV_VAR)) &&
        nullptr != (secretKey = getenv(SECRET_KEY_ENV_VAR))) {

        LOG_INFO("Using aws credentials for Kinesis Video Streams");
        if (nullptr != (sessionToken = getenv(SESSION_TOKEN_ENV_VAR))) {
            LOG_INFO("Session token detected.");
            sessionTokenStr = string(sessionToken);
        } else {
            LOG_INFO("No session token was detected.");
            sessionTokenStr = "";
        }

        data->credential.reset(new Credentials(string(accessKey),
                                               string(secretKey),
                                               sessionTokenStr,
                                               std::chrono::seconds(DEFAULT_CREDENTIAL_EXPIRATION_SECONDS)));
        credential_provider.reset(new SampleCredentialProvider(*data->credential.get()));

    } else if (nullptr != (iot_get_credential_endpoint = getenv("IOT_GET_CREDENTIAL_ENDPOINT")) &&
               nullptr != (cert_path = getenv("CERT_PATH")) &&
               nullptr != (private_key_path = getenv("PRIVATE_KEY_PATH")) &&
               nullptr != (role_alias = getenv("ROLE_ALIAS")) &&
               nullptr != (ca_cert_path = getenv("CA_CERT_PATH"))) {
        LOG_INFO("Using IoT credentials for Kinesis Video Streams");
        credential_provider.reset(new IotCertCredentialProvider(iot_get_credential_endpoint,
                                                                cert_path,
                                                                private_key_path,
                                                                role_alias,
                                                                ca_cert_path,
                                                                data->stream_name));

    } else {
        LOG_AND_THROW("No valid credential method was found");
    }

    data->kinesis_video_producer = KinesisVideoProducer::createSync(move(device_info_provider),
                                                                    move(client_callback_provider),
                                                                    move(stream_callback_provider),
                                                                    move(credential_provider),
                                                                    defaultRegionStr);

    LOG_DEBUG("Client is ready");
}

void kinesis_video_stream_init(CustomData *data) {
    /* create a test stream */
    map<string, string> tags;
    char tag_name[MAX_TAG_NAME_LEN];
    char tag_val[MAX_TAG_VALUE_LEN];
    SPRINTF(tag_name, "piTag");
    SPRINTF(tag_val, "piValue");

    STREAMING_TYPE streaming_type = DEFAULT_STREAMING_TYPE;
    data->use_absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;

    if (data->streamSource == FILE_SOURCE) {
        streaming_type = STREAMING_TYPE_OFFLINE;
        data->use_absolute_fragment_times = true;
    }

    unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(
        data->stream_name,
        hours(DEFAULT_RETENTION_PERIOD_HOURS),
        &tags,
        DEFAULT_KMS_KEY_ID,
        streaming_type,
        DEFAULT_CONTENT_TYPE,
        duration_cast<milliseconds> (seconds(DEFAULT_MAX_LATENCY_SECONDS)),
        milliseconds(DEFAULT_FRAGMENT_DURATION_MILLISECONDS),
        milliseconds(DEFAULT_TIMECODE_SCALE_MILLISECONDS),
        DEFAULT_KEY_FRAME_FRAGMENTATION,
        DEFAULT_FRAME_TIMECODES,
        data->use_absolute_fragment_times,
        DEFAULT_FRAGMENT_ACKS,
        DEFAULT_RESTART_ON_ERROR,
        DEFAULT_RECALCULATE_METRICS,
        0,
        DEFAULT_STREAM_FRAMERATE,
        DEFAULT_AVG_BANDWIDTH_BPS,
        seconds(DEFAULT_BUFFER_DURATION_SECONDS),
        seconds(DEFAULT_REPLAY_DURATION_SECONDS),
        seconds(DEFAULT_CONNECTION_STALENESS_SECONDS),
        DEFAULT_CODEC_ID,
        DEFAULT_TRACKNAME,
        nullptr,
        0));
    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(move(stream_definition));

    // reset state
    data->stream_status = STATUS_SUCCESS;
    data->stream_started = false;

    // since we are starting new putMedia, timestamp need not be padded.
    if (data->streamSource == FILE_SOURCE) {
        data->base_pts = 0;
        data->max_frame_pts = 0;
    }

    LOG_DEBUG("Stream is ready");
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

int gstreamer_live_source_init(int argc, char* argv[], CustomData *data, GstElement *pipeline) {

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
            }
            else if ((0 == STRCMPI(argv[i], "-h")) ||
                     (0 == STRCMPI(argv[i], "/h")) ||
                     (0 == STRCMPI(argv[i], "--h"))) {
                // process the width
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &height))) {
                    return 1;
                }
            }
            else if ((0 == STRCMPI(argv[i], "-f")) ||
                     (0 == STRCMPI(argv[i], "/f")) ||
                     (0 == STRCMPI(argv[i], "--f"))) {
                // process the width
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &framerate))) {
                    return 1;
                }
            }
            else if ((0 == STRCMPI(argv[i], "-b")) ||
                     (0 == STRCMPI(argv[i], "/b")) ||
                     (0 == STRCMPI(argv[i], "--b"))) {
                // process the width
                if (STATUS_FAILED(STRTOI32(argv[i + 1], NULL, 10, &bitrateInKBPS))) {
                    return 1;
                }
            }
            // skip the index
            i++;
        }
        else if (0 == STRCMPI(argv[i], "-?") ||
                 0 == STRCMPI(argv[i], "--?") ||
                 0 == STRCMPI(argv[i], "--help")) {
            g_printerr("Invalid arguments\n");
            return 1;
        }
        else if (argv[i][0] == '/' ||
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

    LOG_DEBUG("Streaming with live source and width: " << width << ", height: " << height << ", fps: " << framerate << ", bitrateInKBPS" << bitrateInKBPS);

    GstElement *source_filter, *filter, *appsink, *h264parse, *encoder, *source, *video_convert;

    /* create the elemnents */
    /*
       gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=I420,width=1280,height=720,framerate=15/1 ! x264enc pass=quant bframes=0 ! video/x-h264,profile=baseline,format=I420,width=1280,height=720,framerate=15/1 ! matroskamux ! filesink location=test.mkv
     */
    source_filter = gst_element_factory_make("capsfilter", "source_filter");
    filter = gst_element_factory_make("capsfilter", "encoder_filter");
    appsink = gst_element_factory_make("appsink", "appsink");
    h264parse = gst_element_factory_make("h264parse", "h264parse"); // needed to enforce avc stream format

    // Attempt to create vtenc encoder
    encoder = gst_element_factory_make("vtenc_h264_hw", "encoder");
    if (encoder) {
        source = gst_element_factory_make("autovideosrc", "source");
        vtenc = true;
    } else {
        // Failed creating vtenc - check pi hardware encoder
        encoder = gst_element_factory_make("omxh264enc", "encoder");
        if (encoder) {
            isOnRpi = true;
        } else {
            // - attempt x264enc
            encoder = gst_element_factory_make("x264enc", "encoder");
            isOnRpi = false;
        }
        source = gst_element_factory_make("v4l2src", "source");
        if (!source) {
            source = gst_element_factory_make("ksvideosrc", "source");
        }
        vtenc = false;
    }

    if (!pipeline || !source || !source_filter || !encoder || !filter || !appsink || !h264parse) {
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    /* configure source */
    if (!vtenc) {
        g_object_set(G_OBJECT (source), "do-timestamp", TRUE, "device", "/dev/video0", NULL);
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
            g_printerr("Default list of resolutions (1920x1080, 1280x720, 640x480) are not supported by video source\n");
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
        g_object_set(G_OBJECT (source_filter), "caps", query_caps_raw, NULL);
    } else {
        gst_caps_set_simple(query_caps_h264,
                            "stream-format", G_TYPE_STRING, "byte-stream",
                            "alignment", G_TYPE_STRING, "au",
                            NULL);
        g_object_set(G_OBJECT (source_filter), "caps", query_caps_h264, NULL);
    }
    gst_caps_unref(query_caps_h264);
    gst_caps_unref(query_caps_raw);

    /* configure encoder */
    if (!data->h264_stream_supported){
        if (vtenc) {
            g_object_set(G_OBJECT (encoder), "allow-frame-reordering", FALSE, "realtime", TRUE, "max-keyframe-interval",
                         45, "bitrate", bitrateInKBPS, NULL);
        } else if (isOnRpi) {
            g_object_set(G_OBJECT (encoder), "control-rate", 2, "target-bitrate", bitrateInKBPS*1000,
                         "periodicty-idr", 45, "inline-header", FALSE, NULL);
        } else {
            g_object_set(G_OBJECT (encoder), "bframes", 0, "key-int-max", 45, "bitrate", bitrateInKBPS, NULL);
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
    g_object_set(G_OBJECT (filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    /* configure appsink */
    g_object_set(G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);

    /* build the pipeline */
    if (!data->h264_stream_supported) {
        gst_bin_add_many(GST_BIN (pipeline), source, video_convert, source_filter, encoder, h264parse, filter,
                         appsink, NULL);
        if (!gst_element_link_many(source, video_convert, source_filter, encoder, h264parse, filter, appsink, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }
    } else {
        gst_bin_add_many(GST_BIN (pipeline), source, source_filter, h264parse, filter, appsink, NULL);
        if (!gst_element_link_many(source, source_filter, h264parse, filter, appsink, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }
    }

    return 0;
}

int gstreamer_rtsp_source_init(CustomData *data, GstElement *pipeline) {

    GstElement *filter, *appsink, *depay, *source, *h264parse;

    filter = gst_element_factory_make("capsfilter", "filter");
    appsink = gst_element_factory_make("appsink", "appsink");
    depay = gst_element_factory_make("rtph264depay", "depay");
    source = gst_element_factory_make("rtspsrc", "source");
    h264parse = gst_element_factory_make("h264parse", "h264parse");

    if (!pipeline || !source || !depay || !appsink || !filter || !h264parse) {
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    // configure filter
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    g_object_set(G_OBJECT (filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    // configure appsink
    g_object_set(G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);

    // configure rtspsrc
    g_object_set(G_OBJECT (source),
                 "location", data->rtsp_url.c_str(),
                 "short-header", true, // Necessary for target camera
                 NULL);

    g_signal_connect(source, "pad-added", G_CALLBACK(pad_added_cb), depay);

    /* build the pipeline */
    gst_bin_add_many(GST_BIN (pipeline), source,
                     depay, h264parse, filter, appsink,
                     NULL);

    /* Leave the actual source out - this will be done when the pad is added */
    if (!gst_element_link_many(depay, filter, h264parse,
                               appsink,
                               NULL)) {

        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    return 0;
}

int gstreamer_file_source_init(CustomData *data, GstElement *pipeline) {

    GstElement *demux, *appsink, *filesrc, *h264parse, *filter, *queue;
    string file_suffix;
    string file_path = data->file_list.at(data->current_file_idx).path;

    filter = gst_element_factory_make("capsfilter", "filter");
    appsink = gst_element_factory_make("appsink", "appsink");
    filesrc = gst_element_factory_make("filesrc", "filesrc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    queue = gst_element_factory_make("queue", "queue");

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

    if (!demux || !filesrc || !h264parse || !appsink || !pipeline || !filter) {
        g_printerr("Not all elements could be created:\n");
        return 1;
    }

    // configure filter
    GstCaps *h264_caps = gst_caps_new_simple("video/x-h264",
                                             "stream-format", G_TYPE_STRING, "avc",
                                             "alignment", G_TYPE_STRING, "au",
                                             NULL);
    g_object_set(G_OBJECT (filter), "caps", h264_caps, NULL);
    gst_caps_unref(h264_caps);

    // configure appsink
    g_object_set(G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);
    g_signal_connect(appsink, "eos", G_CALLBACK(eos_cb), data);

    // configure filesrc
    g_object_set(G_OBJECT (filesrc), "location", file_path.c_str(), NULL);

    // configure demux
    g_signal_connect(demux, "pad-added", G_CALLBACK(pad_added_cb), queue);


    /* build the pipeline */
    gst_bin_add_many(GST_BIN (pipeline), demux,
                     filesrc, filter, appsink, h264parse, queue,
                     NULL);

    if (!gst_element_link_many(filesrc, demux,
                               NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    if (!gst_element_link_many(queue, h264parse, filter, appsink,
                               NULL)) {
        g_printerr("Video elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    return 0;
}


int gstreamer_init(int argc, char* argv[], CustomData *data) {

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
            ret = gstreamer_rtsp_source_init(data, pipeline);
            break;
        case FILE_SOURCE:
            LOG_INFO("Streaming from file source");
            pipeline = gst_pipeline_new("file-kinesis-pipeline");
            ret = gstreamer_file_source_init(data, pipeline);
            break;
    }

    if (ret != 0){
        return ret;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    gst_object_unref(bus);

    /* start streaming */
    gst_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (gst_ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    data->main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data->main_loop);

    /* free resources */
    gst_bus_remove_signal_watch(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;
    return 0;
}

int main(int argc, char* argv[]) {
    PropertyConfigurator::doConfigure("kvs_log_configuration");

    if (argc < 2) {
        LOG_ERROR(
                "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_app my-stream-name -w width -h height -f framerate -b bitrateInKBPS\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_app my-stream-name\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_app my-stream-name rtsp-url\n \
           or AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_sample_app my-stream-name path/to/file1 path/to/file2 ...\n");
        return 1;
    }

    const int PUTFRAME_FAILURE_RETRY_COUNT = 3;

    CustomData data;
    char stream_name[MAX_STREAM_NAME_LEN + 1];
    int ret = 0;
    int file_retry_count = PUTFRAME_FAILURE_RETRY_COUNT;
    STATUS stream_status = STATUS_SUCCESS;

    STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    stream_name[MAX_STREAM_NAME_LEN] = '\0';
    data.stream_name = stream_name;

    data.streamSource = LIVE_SOURCE;
    if (argc > 2) {
        string third_arg = string(argv[2]);
        // config options for live source begin with -
        if (third_arg[0] != '-') {
            string prefix = third_arg.substr(0, 4);
            string suffix = third_arg.substr(third_arg.size() - 3);
            if (prefix.compare("rtsp") == 0) {
                data.streamSource = RTSP_SOURCE;
                data.rtsp_url = string(argv[2]);

            } else if (suffix.compare("mkv") == 0 ||
                       suffix.compare("mp4") == 0 ||
                       suffix.compare(".ts") == 0) {
                data.streamSource = FILE_SOURCE;
                // skip over stream name
                for(int i = 2; i < argc; ++i) {
                    string file_path = string(argv[i]);
                    // file path should be at least 4 char (shortest example: a.ts)
                    if (file_path.size() < 4) {
                        LOG_ERROR("Invalid file path");
                        return 1;
                    }
                    FileInfo fileInfo;
                    fileInfo.path = file_path;
                    data.file_list.push_back(fileInfo);
                }
            }
        }
    }

    /* init Kinesis Video */
    try{
        kinesis_video_init(&data);
        kinesis_video_stream_init(&data);
    } catch (runtime_error &err) {
        LOG_ERROR("Failed to initialize kinesis video with an exception: " << err.what());
        return 1;
    }

    bool do_retry = true;

    if (data.streamSource == FILE_SOURCE) {
        do {
            uint32_t i = data.last_unpersisted_file_idx.load();
            bool continue_uploading = true;

            for(; i < data.file_list.size() && continue_uploading; ++i) {

                data.current_file_idx = i;
                LOG_DEBUG("Attempt to upload file: " << data.file_list[i].path);

                // control will return after gstreamer_init after file eos or any GST_ERROR was put on the bus.
                gstreamer_init(argc, argv, &data);

                // check if any stream error occurred.
                stream_status = data.stream_status.load();

                if (STATUS_FAILED(stream_status)) {
                    continue_uploading = false;
                    // fatal cases
                    if (!IS_RETRIABLE_ERROR(stream_status)) {
                        LOG_ERROR("Fatal stream error occurred: " << stream_status << ". Terminating.");
                        do_retry = false;
                    } else {
                        do_retry = true;
                    }
                    data.kinesis_video_stream->stop();
                } else {
                    LOG_INFO("Finished sending file to kvs producer: " << data.file_list[i].path);
                    // check if we just finished sending the last file.
                    if (i == data.file_list.size() - 1) {
                        // stop sync will send out remaining frames. If stopSync
                        // succeeds then everything is done, otherwise do retry
                        if (data.kinesis_video_stream->stopSync()) {
                            LOG_INFO("All files have been persisted");
                            do_retry = false;
                        } else {
                            do_retry = true;
                        }
                    }
                }
            }

            if (do_retry) {
                file_retry_count--;
                if (file_retry_count == 0) {
                    i = data.last_unpersisted_file_idx.load();
                    LOG_ERROR("Failed to upload file " << data.file_list[i].path << " after retrying. Terminating.");
                    data.kinesis_video_stream->stop();
                    do_retry = false;           // exit while loop
                } else {
                    // flush out buffers
                    data.kinesis_video_stream->resetStream();
                    // reset state
                    data.stream_status = STATUS_SUCCESS;
                    data.stream_started = false;
                }
            }
        } while(do_retry);

    } else {
        // non file uploading scenario
        gstreamer_init(argc, argv, &data);
        if (STATUS_SUCCEEDED(stream_status)) {
            // if stream_status is success after eos, send out remaining frames.
            data.kinesis_video_stream->stopSync();
        } else {
            data.kinesis_video_stream->stop();
        }
    }

    // CleanUp
    data.kinesis_video_producer->freeStream(data.kinesis_video_stream);

    return 0;
}
