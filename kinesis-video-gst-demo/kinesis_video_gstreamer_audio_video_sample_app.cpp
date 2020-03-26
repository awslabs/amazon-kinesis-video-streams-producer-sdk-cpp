#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string.h>
#include <chrono>
#include <Logger.h>
#include "KinesisVideoProducer.h"
#include <fstream>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <IotCertCredentialProvider.h>
#include <queue>

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

#define VIDEO_DEVICE_ENV_VAR "AWS_KVS_VIDEO_DEVICE"
#define AUDIO_DEVICE_ENV_VAR "AWS_KVS_AUDIO_DEVICE"

#define DEFAULT_RETENTION_PERIOD_HOURS 2
#define DEFAULT_KMS_KEY_ID ""
#define DEFAULT_STREAMING_TYPE STREAMING_TYPE_REALTIME
#define DEFAULT_CONTENT_TYPE "video/h264,audio/aac"
#define DEFAULT_MAX_LATENCY_SECONDS 60
#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES TRUE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_STREAM_FRAMERATE 100
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
#define DEFAULT_BUFFER_DURATION_SECONDS 120
#define DEFAULT_REPLAY_DURATION_SECONDS 40
#define DEFAULT_CONNECTION_STALENESS_SECONDS 60
#define DEFAULT_CODEC_ID "V_MPEG4/ISO/AVC"
#define DEFAULT_TRACKNAME "kinesis_video"
#define APP_SINK_BASE_NAME "appsink"
#define DEFAULT_BUFFER_SIZE (1 * 1024 * 1024)
#define DEFAULT_STORAGE_SIZE (128 * 1024 * 1024)
#define DEFAULT_CREDENTIAL_ROTATION_SECONDS 3600
#define DEFAULT_CREDENTIAL_EXPIRATION_SECONDS 180
#define DEFAULT_AUDIO_VIDEO_DRIFT_TIMEOUT_SECOND 5

#define DEFAULT_VIDEO_TRACKID 1
#define DEFAULT_AUDIO_TRACK_NAME "audio"
#define DEFAULT_AUDIO_CODEC_ID "A_AAC"
#define DEFAULT_AUDIO_TRACKID 2

typedef struct _FileInfo {
    _FileInfo():
            path(""),
            last_fragment_ts(0) {}
    string path;
    uint64_t last_fragment_ts;
} FileInfo;

typedef struct _CustomData {
    _CustomData():
            first_video_frame(true),
            eos_triggered(false),
            pipeline_blocked(false),
            stream_status(STATUS_SUCCESS),
            base_pts(0),
            max_frame_pts(0),
            total_track_count(1),
            key_frame_pts(0),
            current_file_idx(0),
            last_unpersisted_file_idx(0),
            kinesis_video_producer(nullptr),
            kinesis_video_stream(nullptr),
            main_loop(NULL),
            first_pts(GST_CLOCK_TIME_NONE),
            use_absolute_fragment_times(true) {
        producer_start_time = chrono::duration_cast<nanoseconds>(systemCurrentTime().time_since_epoch()).count();
    }

    GMainLoop *main_loop;
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    shared_ptr<KinesisVideoStream> kinesis_video_stream;

    // queue elements to be linked to demuxer's sometimes pad in demux_pad_cb
    GstElement *video_queue, *audio_queue;

    mutex audio_video_sync_mtx;
    mutex file_list_mtx;
    condition_variable audio_video_sync_cv;

    char *stream_name;

    // indicate if either audio or video media pipeline is currently blocked. If so, the other pipeline line will wake up
    // the blocked one when the time is right.
    volatile bool pipeline_blocked;

    // indicate whether a video key frame has been received or not.
    volatile bool first_video_frame;

    // whether the application is configured to upload a file or live streaming.
    volatile bool uploading_file;

    // list of files to upload.
    vector<FileInfo> file_list;

    // index of file in file_list that application is currently trying to upload.
    uint32_t current_file_idx;

    // index of last file in file_list that haven't been persisted.
    atomic_uint last_unpersisted_file_idx;

    // when uploading file, whether one of audio or video pipeline has reached eos.
    atomic_bool eos_triggered;

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

    // key:     trackId
    // value:   whether application has received the first frame for trackId.
    map<int, bool> stream_started;

    // When uploading file, store the pts of frames that has flag FRAME_FLAG_KEY_FRAME. When the entire file has been uploaded,
    // key_frame_pts contains the timetamp of the last fragment in the file. key_frame_pts is then stored into last_fragment_ts
    // of the file.
    // Unit: ns
    uint64_t key_frame_pts;

    // Used in file uploading only. Assuming frame timestamp are relative. Add producer_start_time to each frame's
    // timestamp to convert them to absolute timestamp. This way fragments dont overlap after token rotation when doing
    // file uploading.
    uint64_t producer_start_time;

    unique_ptr<Credentials> credential;

    uint32_t total_track_count;

    bool use_absolute_fragment_times;

    // Pts of first frame
    uint64_t first_pts;

    GstElement *pipeline;
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
        // Test rotation period is 40 minute for the grace period.
        const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(DEFAULT_CREDENTIAL_ROTATION_SECONDS);
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
            // Set the storage size to 128MB
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
        CustomData *data = reinterpret_cast<CustomData *>(custom_data);
        bool terminate_pipeline = false;

        // Terminate pipeline if error is not retriable or if error is retriable but we are streaming file.
        // When streaming file, we choose to terminate the pipeline on error because the easiest way to recover
        // is to stream the file from the beginning again.
        // In realtime streaming, retriable error can be handled underneath. Otherwise terminate pipeline
        // and store error status if error is fatal.
        if ((IS_RETRIABLE_ERROR(status_code) && data->uploading_file) ||
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
        if (data->uploading_file && pFragmentAck->ackType == FRAGMENT_ACK_TYPE_PERSISTED) {
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

void create_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
                                void *data, size_t len, UINT64 track_id) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 0; // with audio, frame can get as close as 0.01ms
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(data);
    frame->trackId = track_id;
}

bool all_stream_started(CustomData *data) {
    bool started = true;
    if (data->stream_started.size() < data->total_track_count) {
        started = false;
    } else {
        for (map<int, bool>::iterator it = data->stream_started.begin(); it != data->stream_started.end(); ++it) {
            if (!it->second) {
                started = false;
                break;
            }
        }
    }

    return started;
}

void kinesis_video_stream_init(CustomData *data);

static GstFlowReturn on_new_sample(GstElement *sink, CustomData *data) {
    std::unique_lock<std::mutex> lk(data->audio_video_sync_mtx);
    GstSample *sample = nullptr;
    GstBuffer *buffer;
    size_t buffer_size;
    bool delta, dropFrame;
    FRAME_FLAGS kinesis_video_flags;
    uint8_t *data_buffer;
    Frame frame;
    GstFlowReturn ret = GST_FLOW_OK;
    STATUS curr_stream_status = data->stream_status.load();
    GstSegment *segment;
    GstClockTime buf_pts, buf_dts;
    gint dts_sign;
    gchar *g_stream_handle_key = gst_element_get_name(sink);
    int track_id = (string(g_stream_handle_key).back()) - '0';
    g_free(g_stream_handle_key);
    GstMapInfo info;

    info.data = nullptr;
    sample = gst_app_sink_pull_sample(GST_APP_SINK (sink));

    // extract cpd for the first frame for each track
    if (!data->stream_started[track_id]) {
        data->stream_started[track_id] = true;
        GstCaps *gstcaps = (GstCaps *) gst_sample_get_caps(sample);
        GST_LOG("caps are %" GST_PTR_FORMAT, gstcaps);
        GstStructure *gststructforcaps = gst_caps_get_structure(gstcaps, 0);
        const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
        gchar *cpd = gst_value_serialize(gstStreamFormat);
        data->kinesis_video_stream->start(std::string(cpd), track_id);
        g_free(cpd);

        // dont block waiting for cpd if pipeline state is not GST_STATE_PLAYING, otherwise it will
        // block pipeline state transition.
        if (GST_STATE(data->pipeline) != GST_STATE_PLAYING) {
            goto CleanUp;
        }

        // block pipeline until cpd for all tracks have been received. Otherwise we will get STATUS_INVALID_STREAM_STATE
        if (!all_stream_started(data)) {
            if (!data->uploading_file) {
                goto CleanUp;
            }
            data->audio_video_sync_cv.wait_for(lk, seconds(DEFAULT_AUDIO_VIDEO_DRIFT_TIMEOUT_SECOND), [data]{
                return all_stream_started(data);
            });

            if(!all_stream_started(data)) {
                LOG_ERROR("Drift between audio and video is above threshold");
                ret = GST_FLOW_ERROR;
                goto CleanUp;
            }
        } else {
            data->audio_video_sync_cv.notify_all();
        }
    }

    buffer = gst_sample_get_buffer(sample);
    buffer_size = gst_buffer_get_size(buffer);

    dropFrame =  GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) ||
                 GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
                 (GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
                 (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                 (!GST_BUFFER_PTS_IS_VALID(buffer)); //frame with invalid pts cannot be processed.
    if (dropFrame) {
        if (!GST_BUFFER_PTS_IS_VALID(buffer)) {
            LOG_WARN("Dropping frame due to invalid presentation timestamp.");
        } else {
            LOG_WARN("Dropping invalid frame.");
        }
        goto CleanUp;
    }

    // convert from segment timestamp to running time in live mode. In offline mode, dts_sign is always negative or 0,
    // causing frame to always get dropped.
    if (!data->uploading_file) {
        segment = gst_sample_get_segment(sample);
        buf_pts = gst_segment_to_running_time(segment, GST_FORMAT_TIME, buffer->pts);
        if (!GST_CLOCK_TIME_IS_VALID(buf_pts)) {
            LOG_DEBUG("Frame contains invalid PTS dropping the frame.");
            goto CleanUp;
        }

        dts_sign = gst_segment_to_running_time_full(segment, GST_FORMAT_TIME, buffer->dts, &buf_dts);
        if (!(dts_sign > 0)) {
            LOG_DEBUG("Frame contains invalid DTS dropping the frame.");
            goto CleanUp;
        }

        buffer->pts = buf_pts;
        buffer->dts = buf_dts;
    }

    delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    kinesis_video_flags = FRAME_FLAG_NONE;

    if (!delta && track_id == DEFAULT_VIDEO_TRACKID) {
        if (data->first_video_frame) {
            // start cutting fragment at second video key frame because we can have audio frames before first video key frame
            data->first_video_frame = false;
        } else {
            kinesis_video_flags = FRAME_FLAG_KEY_FRAME;
        }
    }

    if (data->uploading_file) {

        data->max_frame_pts = MAX(data->max_frame_pts, buffer->pts);

        // when reading file using gstreamer, dts is undefined.
        buffer->dts = 0;
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
    create_kinesis_video_frame(&frame, std::chrono::nanoseconds(buffer->pts), std::chrono::nanoseconds(buffer->dts),
                               kinesis_video_flags, info.data, info.size, track_id);

    data->kinesis_video_stream->putFrame(frame);

CleanUp:

    if (info.data != nullptr) {
        gst_buffer_unmap(buffer, &info);
    }

    if (sample != nullptr) {
        gst_sample_unref(sample);
    }

    return ret;
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

static void eos_cb(GstElement *sink, CustomData *data) {
    if (!data->eos_triggered.load()) {
        // Media pipeline for one track has ended. Next time eos_cb is called means the entire file has been received.
        data->eos_triggered = true;
        data->audio_video_sync_cv.notify_all();
    } else {

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
}

static gboolean demux_pad_cb(GstElement *element, GstPad *pad, CustomData *data) {
    GstPad *video_sink = gst_element_get_static_pad(GST_ELEMENT(data->video_queue), "sink");
    GstPad *audio_sink = gst_element_get_static_pad(GST_ELEMENT(data->audio_queue), "sink");

    GstPadLinkReturn link_ret;
    gboolean ret = TRUE;
    gchar *pad_name = gst_pad_get_name(pad);

    // link queue to corresponding sinks
    if (gst_pad_can_link(pad, video_sink)) {
        link_ret = gst_pad_link(pad, video_sink);
    } else {
        link_ret = gst_pad_link(pad, audio_sink);
    }
    gst_object_unref(video_sink);
    gst_object_unref(audio_sink);

    if (link_ret != GST_PAD_LINK_OK) {
        LOG_ERROR("Failed to link demuxer's pad " << string(pad_name));
        ret = FALSE;
    }
    g_free(pad_name);
    return ret;
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

    STREAMING_TYPE streaming_type = DEFAULT_STREAMING_TYPE;
    data->use_absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;

    if (data->uploading_file) {
        streaming_type = STREAMING_TYPE_OFFLINE;
        data->use_absolute_fragment_times = true;
    }

    unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(
        data->stream_name,
        hours(DEFAULT_RETENTION_PERIOD_HOURS),
        nullptr,
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
        NAL_ADAPTATION_FLAG_NONE,
        DEFAULT_STREAM_FRAMERATE,
        DEFAULT_AVG_BANDWIDTH_BPS,
        seconds(DEFAULT_BUFFER_DURATION_SECONDS),
        seconds(DEFAULT_REPLAY_DURATION_SECONDS),
        seconds(DEFAULT_CONNECTION_STALENESS_SECONDS),
        DEFAULT_CODEC_ID,
        DEFAULT_TRACKNAME,
        nullptr,
        0,
        MKV_TRACK_INFO_TYPE_VIDEO,
        vector<uint8_t>(),
        DEFAULT_VIDEO_TRACKID));

    stream_definition->addTrack(DEFAULT_AUDIO_TRACKID, DEFAULT_AUDIO_TRACK_NAME, DEFAULT_AUDIO_CODEC_ID, MKV_TRACK_INFO_TYPE_AUDIO);
    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(move(stream_definition));
    data->stream_started.clear();

    // since we are starting new putMedia, timestamp need not be padded.
    if (data->uploading_file) {
        data->base_pts = 0;
        data->max_frame_pts = 0;
    }

    // reset state
    data->stream_status = STATUS_SUCCESS;

    LOG_DEBUG("Stream is ready: " << data->stream_name);
}

int gstreamer_init(int argc, char *argv[], CustomData &data) {

    GstStateChangeReturn ret;

    // Reset first frame pts
    data.first_pts = GST_CLOCK_TIME_NONE;

    //reset state
    data.eos_triggered = false;

    /* init GStreamer */
    gst_init(&argc, &argv);

    GstElement *appsink_video, *appsink_audio, *audio_queue, *video_queue, *pipeline, *video_filter, *audio_filter;
    string video_caps_string, audio_caps_string;
    GstCaps *caps;

    video_caps_string = "video/x-h264, stream-format=(string) avc, alignment=(string) au";
    audio_caps_string = "audio/mpeg, stream-format=(string) raw";
    video_filter = gst_element_factory_make("capsfilter", "video_filter");
    caps = gst_caps_from_string(video_caps_string.c_str());
    g_object_set(G_OBJECT (video_filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    audio_filter = gst_element_factory_make("capsfilter", "audio_filter");
    caps = gst_caps_from_string(audio_caps_string.c_str());
    g_object_set(G_OBJECT (audio_filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // hardcoding appsink name and track id
    const string video_appsink_name = "appsink_" + to_string(DEFAULT_VIDEO_TRACKID);
    const string audio_appsink_name = "appsink_" + to_string(DEFAULT_AUDIO_TRACKID);

    appsink_video = gst_element_factory_make("appsink", (gchar *) video_appsink_name.c_str());
    appsink_audio = gst_element_factory_make("appsink", (gchar *) audio_appsink_name.c_str());

    /* configure appsink */
    g_object_set(G_OBJECT (appsink_video), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink_video, "new-sample", G_CALLBACK(on_new_sample), &data);
    g_signal_connect(appsink_video, "eos", G_CALLBACK(eos_cb), &data);
    g_object_set(G_OBJECT (appsink_audio), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink_audio, "new-sample", G_CALLBACK(on_new_sample), &data);
    g_signal_connect(appsink_audio, "eos", G_CALLBACK(eos_cb), &data);
    LOG_DEBUG("appsink configured");

    audio_queue = gst_element_factory_make("queue", "audio_queue");
    video_queue = gst_element_factory_make("queue", "video_queue");

    data.audio_queue = audio_queue;
    data.video_queue = video_queue;

    pipeline = gst_pipeline_new("audio-video-kinesis-pipeline");

    if (!pipeline || !appsink_video || !appsink_audio || !video_queue || !audio_queue || !video_filter || !audio_filter) {
        g_printerr("Not all elements could be created:\n");
        return 1;
    }

    if (data.uploading_file) {
        GstElement *demux, *filesrc, *h264parse, *aac_parse;
        string demuxer, file_suffix;
        string file_path = data.file_list.at(data.current_file_idx).path;

        file_suffix = file_path.substr(file_path.size() - 3);
        if (file_suffix.compare("mkv") == 0) {
            demuxer = "matroskademux";
        } else if (file_suffix.compare("mp4") == 0) {
            demuxer = "qtdemux";
        } else if (file_suffix.compare(".ts") == 0) {
            demuxer = "tsdemux";
        } else {
            LOG_ERROR("File format not supported. Supported ones are mp4, mkv and ts. File suffix: " << file_suffix);
            return 1;
        }

        filesrc = gst_element_factory_make("filesrc", "filesrc");
        g_object_set(G_OBJECT (filesrc), "location", file_path.c_str(), NULL);
        demux = gst_element_factory_make(demuxer.c_str(), "demux");
        h264parse = gst_element_factory_make("h264parse", "h264parse");
        aac_parse = gst_element_factory_make("aacparse", "aac_parse");

        if (!demux || !filesrc || !h264parse || !aac_parse) {
            g_printerr("Not all elements could be created:\n");
            return 1;
        }

        gst_bin_add_many(GST_BIN (pipeline), appsink_video, appsink_audio, filesrc, demux, h264parse, aac_parse, video_queue,
                         audio_queue, video_filter, audio_filter,
                         NULL);

        if (!gst_element_link_many(filesrc, demux,
                                   NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }

        if (!gst_element_link_many(video_queue, h264parse, video_filter, appsink_video,
                                   NULL)) {
            g_printerr("Video elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }

        if (!gst_element_link_many(audio_queue, aac_parse, audio_filter, appsink_audio,
                                   NULL)) {
            g_printerr("Audio elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }

        g_signal_connect(demux, "pad-added", G_CALLBACK(demux_pad_cb), &data);

    } else {

        GstElement *videosrc, *videoconvert, *h264enc, *video_src_filter, *h264parse;
        GstElement *audiosrc, *audioconvert, *aac_enc, *audio_resample, *audio_src_filter, *aac_parse;
        string audio_device, video_device;

        audio_device = (nullptr == getenv(AUDIO_DEVICE_ENV_VAR)) ? "" : string(getenv(AUDIO_DEVICE_ENV_VAR));
        video_device = (nullptr == getenv(VIDEO_DEVICE_ENV_VAR)) ? "" : string(getenv(VIDEO_DEVICE_ENV_VAR));

        video_caps_string = "video/x-raw, framerate=(fraction) [ 20/1, 35/1 ], format=I420";
        audio_caps_string = "audio/x-raw, rate=(int) { 16000, 24000, 48000 }";

        h264parse = gst_element_factory_make("h264parse", "h264parse");
        aac_parse = gst_element_factory_make("aacparse", "aac_parse");

#if defined _WIN32
        videosrc = gst_element_factory_make("ksvideosrc", "videosrc");
        if (!video_device.empty()) {
            LOG_INFO("Using video device " << video_device);
            // find your video device by running gst-device-monitor-1.0
            g_object_set(G_OBJECT (videosrc), "device-path", video_device.c_str(), NULL);
        } else {
            LOG_INFO("Using default video device");
        }
        h264enc = gst_element_factory_make("x264enc", "h264enc");
        g_object_set(G_OBJECT (h264enc), "bframes", 0, "key-int-max", 45, "bitrate", 512, NULL);
        gst_util_set_object_arg(G_OBJECT (h264enc), "tune", "zerolatency");
        audiosrc = gst_element_factory_make("wasapisrc", "audiosrc");
        if (!audio_device.empty()) {
            LOG_INFO("Using audio device " << audio_device);
            // find your audio device by running gst-device-monitor-1.0
            g_object_set(G_OBJECT (audiosrc), "device", audio_device.c_str(), "low-latency", TRUE, "use-audioclient3", TRUE, NULL);
        } else {
            LOG_ERROR("No audio device found. Please do export " << AUDIO_DEVICE_ENV_VAR << "=audio_device to config audio device");
            return 1;
        }

#elif defined __APPLE__
        gint audio_device_index = 0, video_device_index = 0; // default devices
        if (!audio_device.empty()) {
            stringstream ss(audio_device);
            ss >> audio_device_index;
        }
        if (!video_device.empty()) {
            stringstream ss(video_device);
            ss >> video_device_index;
        }

        LOG_INFO("Using video device " << video_device_index);
        LOG_INFO("Using audio device " << audio_device_index);

        videosrc = gst_element_factory_make("avfvideosrc", "videosrc");
        g_object_set(G_OBJECT (videosrc), "device-index", video_device_index, NULL);
        h264enc = gst_element_factory_make("vtenc_h264_hw", "h264enc");
        g_object_set(G_OBJECT (h264enc), "allow-frame-reordering", FALSE, "realtime", TRUE, "max-keyframe-interval", 45, "bitrate", 512, NULL);
        audiosrc = gst_element_factory_make("osxaudiosrc", "audiosrc");
        g_object_set(G_OBJECT (audiosrc), "device", audio_device_index, NULL);

        // mac quirk
        video_caps_string += ", width=(int) 1280, height=(int) 720";

#elif defined __linux__
        videosrc = gst_element_factory_make("v4l2src", "videosrc");
        if (!video_device.empty()) {
            LOG_INFO("Using video device " << video_device);
            // find your video device by running path/to/sdk/kinesis-video-native-build/downloads/local/bin/gst-device-monitor-1.0
            g_object_set(G_OBJECT (videosrc), "device", video_device.c_str(), NULL);
        } else {
            LOG_INFO("Using default video device");
        }

        if (nullptr != (h264enc = gst_element_factory_make("omxh264enc", "h264enc"))) {
            // setting target bitrate in omx is currently broken: https://gitlab.freedesktop.org/gstreamer/gst-omx/issues/21
            g_object_set(G_OBJECT (h264enc), "periodicty-idr", 45, "inline-header", FALSE, NULL);
        } else {
            h264enc = gst_element_factory_make("x264enc", "h264enc");
            g_object_set(G_OBJECT (h264enc), "bframes", 0, "key-int-max", 45, "bitrate", 512, NULL);
            gst_util_set_object_arg(G_OBJECT (h264enc), "tune", "zerolatency");
        }

        audiosrc = gst_element_factory_make("alsasrc", "audiosrc");
        if (!audio_device.empty()) {
            LOG_INFO("Using audio device " << audio_device);
            // find your audio recording device by running "arecord -l"
            g_object_set(G_OBJECT (audiosrc), "device", audio_device.c_str(), NULL);
        } else {
            LOG_ERROR("No audio device found. Please do export " << AUDIO_DEVICE_ENV_VAR << "=audio_device to config audio device (e.g. export AWS_KVS_AUDIO_DEVICE=hw:1,0)");
            return 1;
        }

#else
        LOG_ERROR("Cannot determine the Operating system - this sample has been tested in MacOS, Linux, Windows and Raspberry Pi");
        return 1;
#endif

        video_src_filter = gst_element_factory_make("capsfilter", "video_src_filter");
        caps = gst_caps_from_string(video_caps_string.c_str());
        g_object_set(G_OBJECT (video_src_filter), "caps", caps, NULL);
        gst_caps_unref(caps);

        audio_src_filter = gst_element_factory_make("capsfilter", "audio_src_filter");
        caps = gst_caps_from_string(audio_caps_string.c_str());
        g_object_set(G_OBJECT (audio_src_filter), "caps", caps, NULL);
        gst_caps_unref(caps);

        videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
        audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
        aac_enc = gst_element_factory_make("avenc_aac", "aac_enc");
        audio_resample = gst_element_factory_make("audioresample", "audioresample");

        if (!videosrc || !h264enc || !audiosrc || !audioconvert || !aac_enc || !videoconvert || !audio_resample ||
            !audio_src_filter || !video_src_filter || !h264parse || !aac_parse) {
            g_printerr("Not all elements could be created:\n");
            return 1;
        }

        gst_bin_add_many(GST_BIN (pipeline), appsink_video,
                         appsink_audio, videosrc, h264enc, audiosrc, audioconvert, aac_enc, videoconvert, video_queue,
                         audio_queue, video_filter, audio_filter, audio_resample, audio_src_filter, video_src_filter,
                         h264parse, aac_parse, NULL);

        if (!gst_element_link_many(videosrc, videoconvert, video_src_filter, h264enc, h264parse, video_filter, video_queue,
                                   appsink_video,
                                   NULL)) {
            g_printerr("Video elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }

        if (!gst_element_link_many(audiosrc, audio_resample, audio_queue, audioconvert, audio_src_filter, aac_enc, aac_parse, audio_filter,
                                   appsink_audio,
                                   NULL)) {
            g_printerr("Audio elements could not be linked.\n");
            gst_object_unref(pipeline);
            return 1;
        }
    }

    data.pipeline = pipeline;

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, &data);
    gst_object_unref(bus);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        goto CleanUp;
    }

    LOG_DEBUG("Pipeline playing");

    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);
    LOG_DEBUG("Pipeline finished");

CleanUp:
    /*
     * Known Issue: In live streaming mode on mac. a null pointer exception from osxaudiosrc will be thrown when the
     * next line is executed.
     */
    gst_bus_remove_signal_watch(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT (pipeline));
    g_main_loop_unref(data.main_loop);
    data.main_loop = NULL;

    return 0;
}

int main(int argc, char *argv[]) {
    PropertyConfigurator::doConfigure("kvs_log_configuration");

    if (argc < 2) {
        LOG_ERROR(
                "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_audio_video_sample_app my-stream-name /path/to/file"
                        "AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET ./kinesis_video_gstreamer_audio_video_sample_app my-stream-name");
        return 1;
    }

    const int PUTFRAME_FAILURE_RETRY_COUNT = 3;

    CustomData data;
    char stream_name[MAX_STREAM_NAME_LEN];
    string file_path;
    int file_retry_count = PUTFRAME_FAILURE_RETRY_COUNT;
    STATUS stream_status = STATUS_SUCCESS;


    /* init Kinesis Video */
    STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    stream_name[MAX_STREAM_NAME_LEN - 1] = '\0';
    data.stream_name = stream_name;
    data.total_track_count = 2;
    data.uploading_file = false;

    if (argc >= 3) {
        // skip over stream name
        for(int i = 2; i < argc; ++i) {
            file_path = string(argv[i]);
            // file path should be at least 4 char (shortest example: a.ts)
            if (file_path.size() < 4) {
                LOG_ERROR("Invalid file path");
                return 1;
            }
            FileInfo fileInfo;
            fileInfo.path = file_path;
            data.file_list.push_back(fileInfo);
        }

        data.uploading_file = true;
    }

    try {
        kinesis_video_init(&data);
        kinesis_video_stream_init(&data);
    } catch (runtime_error &err) {
        LOG_ERROR("Failed to initialize kinesis video.");
        return 0;
    }

    if (data.uploading_file) {
        bool do_retry = true;
        do {
            uint32_t i = data.last_unpersisted_file_idx.load();
            bool continue_uploading = true;

            for(; i < data.file_list.size() && continue_uploading; ++i) {

                data.current_file_idx = i;
                LOG_DEBUG("Attempt to upload file: " << data.file_list[i].path);

                // control will return after gstreamer_init after file eos or any GST_ERROR was put on the bus.
                gstreamer_init(argc, argv, data);

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
                    do_retry = false;           // exit while loop
                } else {
                    // flush out buffers
                    data.kinesis_video_stream->resetStream();
                    // reset state
                    data.stream_status = STATUS_SUCCESS;
                    data.stream_started.clear();
                }
            }
        } while(do_retry);

    } else {
        // non file uploading scenario
        gstreamer_init(argc, argv, data);
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
