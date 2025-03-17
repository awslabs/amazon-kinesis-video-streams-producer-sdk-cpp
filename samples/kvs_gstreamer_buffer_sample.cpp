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


#define KVS_GST_TEST_SOURCE_NAME "test-source"
#define KVS_GST_DEVICE_SOURCE_NAME "device-source"

#define KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS 8
#define KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS 30


#define FILE_BUFFER_TIME_SECONDS 8



typedef struct _CustomData {

    _CustomData():
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

    string rtsp_url;

    unique_ptr<Credentials> credential;

    uint64_t synthetic_dts;

    bool use_absolute_fragment_times;

    // Pts of first video frame
    uint64_t first_pts;

    SingleList* pGoPList;
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
    if ((!IS_RETRIABLE_ERROR(status_code) && !IS_RECOVERABLE_ERROR(status_code))) {
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
    LOG_DEBUG("Reporting fragment ack received. Ack timecode " << pFragmentAck->timestamp);
    return STATUS_SUCCESS;
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;



atomic<bool> eventTriggered(false);
std::atomic<bool> terminated(FALSE);
std::condition_variable cv;

void sigint_handler(int sigint) {
    LOG_DEBUG("SIGINT received. Exiting...");
    terminated = TRUE;
    cv.notify_all();
    // if(main_loop != NULL) {
    //     g_main_loop_quit(main_loop);
    // }
}

typedef enum _StreamSource {
    TEST_SOURCE,
    DEVICE_SOURCE
} StreamSource;

typedef struct _Gop{
    Frame* pIFrame;
    PSingleList pPFrames; // P-frames associated with this GoP
} Gop;

void cameraEventScheduler() {
    std::mutex cv_m;
    std::unique_lock<std::mutex> lck(cv_m);
    
    while(!terminated) {

        DLOGI("Waiting to set eventTriggered to true.");
        // Wait for some time before triggering an event, exit early if app was terminated.
        if(cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PAUSED_INTERVAL_SECONDS)) != std::cv_status::timeout) {
            // break;
            return;
        }
        DLOGI("Setting eventTriggered to true.");
        eventTriggered = true;

        if(cv.wait_for(lck, std::chrono::seconds(KVS_INTERMITTENT_PLAYING_INTERVAL_SECONDS)) != std::cv_status::timeout) {
            // break;
            return;
        }
        DLOGI("Setting eventTriggered to false.");
        eventTriggered = false;
        DLOGI("Set eventTriggered to false.");

    }
}











static void eos_cb(GstElement *sink, CustomData *data) {
    // bookkeeping base_pts. add 1ms to avoid overlap.
    data->base_pts += + data->max_frame_pts + duration_cast<nanoseconds>(milliseconds(1)).count();
    data->max_frame_pts = 0;

    LOG_DEBUG("Terminating pipeline due to EOS");
    g_main_loop_quit(data->main_loop);
}

void create_and_allocate_kinesis_video_frame(Frame *frame, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags,
    void *data, size_t len) {
frame->flags = flags;
frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
// set duration to 0 due to potential high spew from rtsp streams
frame->duration = 0;
frame->size = static_cast<UINT32>(len);

frame->frameData = (uint8_t*) malloc(frame->size);
memcpy(frame->frameData, data, frame->size);

frame->trackId = DEFAULT_TRACK_ID;
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

    struct timeval tv;
    gettimeofday(&tv, NULL);
    guint64 now_ms = (guint64)tv.tv_sec * 1000 + (tv.tv_usec / 1000);

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


        if (!GST_BUFFER_DTS_IS_VALID(buffer)) {
            data->synthetic_dts += DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
            buffer->dts = data->synthetic_dts;
        } else {
            data->synthetic_dts = buffer->dts;
        }

        if (data->use_absolute_fragment_times) {
            if (data->first_pts == GST_CLOCK_TIME_NONE) {
                data->first_pts = buffer->pts;
            }
            buffer->pts += data->producer_start_time - data->first_pts;
        }

        if (!gst_buffer_map(buffer, &info, GST_MAP_READ)){
            goto CleanUp;
        }

        PSingleList pGopList = (PSingleList) data->pGoPList;


        if(!eventTriggered) {

            if(CHECK_FRAME_FLAG_KEY_FRAME(kinesis_video_flags)) {
                // DLOGE("Keyframe detected, creating new GoP object");
    
                // Make a new GoP object.
                Gop* newGop = (Gop *)calloc(1, sizeof(Gop));
                
                // Make a new Frame object.
                newGop->pIFrame = (Frame *) calloc(1, sizeof(Frame));

                create_and_allocate_kinesis_video_frame(newGop->pIFrame, std::chrono::nanoseconds(buffer->pts), std::chrono::nanoseconds(buffer->pts), kinesis_video_flags, info.data, info.size);
    
                singleListCreate(&(newGop->pPFrames));

                singleListInsertItemTail(pGopList, (UINT64) newGop);

                // Now let's check if there are GoP objects whose frames are all older than FILE_BUFFER_TIME_SECONDS.
                // If so, remove them from the list and delete the frame file.
                // Can uncomment the "isKeyFrame" to reduce frequency of checks, but buffer might end up being 1 GoP longer than necessary.
                // if(isKeyFrame) {
                    PSingleListNode pNode = NULL;
                    singleListGetHeadNode(pGopList, &pNode);
                    PSingleListNode pNextNode = pNode->pNext;

                    // DLOGE("Checking for frames to delete...");

                    while(pNode != NULL && pNextNode != NULL) {
                        // DLOGE("Checking frame...");

                        Gop* pGop = (Gop *) pNode->data;
                        Gop* pNextGop = (Gop *) pNextNode->data;

                        // If the next GoP's start timestamp is within the FILE_BUFFER_TIME_SECONDS, then we can delete this GoP.
                        // This logic allows for FILE_BUFFER_TIME_SECONDS to be exceeded to ensure we don't hold fewer frames than FILE_BUFFER_TIME_SECONDS
                        // in the cases where the FILE_BUFFER_TIME_SECONDS threshold lays in the middle of a GoP; we do not want to discard such a GoP.
                        if((now_ms - pNextGop->pIFrame->presentationTs / HUNDREDS_OF_NANOS_IN_A_MILLISECOND) >= (FILE_BUFFER_TIME_SECONDS * 1000)) {
                            DLOGE("Deleting a buffered GoP...");

                            singleListFree(pGop->pPFrames); // Free the P-Frames.
                            free(pGop->pIFrame); // Free the I-Frame.
                            free(pGop); // DeleteNode does not free the node data, so free the GoP.
                            singleListDeleteNode(pGopList, pNode); // Remove the node.
                            pNode = NULL;

                        } else {
                            DLOGE("Oldest frame does not exceed the interval");
                            break;
                        }
                        pNode = pNextNode;
                        pNextNode = pNextNode->pNext;
                    }
                // }





            } else {
                // DLOGE("Not a keyframe, not creating new GoP object");
    
                // Add the timestamp to the P-Frames list of the latest GoP object.
                PSingleListNode pTailNode = NULL;
    
                singleListGetTailNode(pGopList, &pTailNode);

                if(pTailNode != NULL) {
                    // Make a new Frame object.
                    Frame* pPFrame = (Frame *) calloc(1, sizeof(Frame));
                    create_and_allocate_kinesis_video_frame(pPFrame, std::chrono::nanoseconds(buffer->pts), std::chrono::nanoseconds(buffer->pts), kinesis_video_flags, info.data, info.size);

                    singleListInsertItemTail(((Gop *)pTailNode->data)->pPFrames, (UINT64) pPFrame);
                }
            }
        } else {
            // DLOGW("Event triggered, sending buffered frames...");
            // Handle event: send buffered frames, send live frame.

            // Send buffered frames.
            if (pGopList->pHead != NULL) {
                DLOGW("Sending buffered frames...");
                for (auto it = pGopList->pHead; it != NULL; it = it->pNext) {
                    Gop* pGop = (Gop *) it->data;
                    DLOGW("Putting buffered frame with ts: %lu", pGop->pIFrame->presentationTs);
                    data->kinesis_video_stream->putFrame(*pGop->pIFrame);
    
                    free(pGop->pIFrame->frameData);
                    // Free the I-Frame.
                    free(pGop->pIFrame);

                    for (auto it2 = pGop->pPFrames->pHead; it2 != NULL; it2 = it2->pNext) {
                        Frame* pPFrame = (Frame *) it2->data;
                        DLOGW("Putting buffered frame with ts: %lu", pPFrame->presentationTs);
                        data->kinesis_video_stream->putFrame(*pPFrame);
                        free(pPFrame->frameData);
                    }

                    // Free the PFrames list (will also free the Frame nodes).
                    singleListFree(pGop->pPFrames);

                }
                // Clear the GoP list (will also free the GoP nodes).
                singleListClear(pGopList, TRUE);
                DLOGW("Done sending buffered frames...");

            }

            // DLOGW("PTS: %lu", buffer->pts);
            // DLOGW("DTS: %lu", buffer->dts);
            // Send the live frame.
            DLOGW("Putting live frame with ts: %lu", buffer->pts);
            put_frame(data->kinesis_video_stream, info.data, info.size, std::chrono::nanoseconds(buffer->pts), std::chrono::nanoseconds(buffer->pts), kinesis_video_flags);

        }

        // DLOGW("PTS: %lu", buffer->pts);
        // DLOGW("DTS: %lu", buffer->dts);

        // put_frame(data->kinesis_video_stream, info.data, info.size, std::chrono::nanoseconds(buffer->pts),
        //     std::chrono::nanoseconds(buffer->dts), kinesis_video_flags);
        


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

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    // GMainLoop *loop = (GMainLoop *) ((CustomData *)data)->main_loop;
    // GstElement *pipeline = (GstElement *) ((CustomData *)data)->pipeline;

    // switch(GST_MESSAGE_TYPE(msg)) {
    //     case GST_MESSAGE_EOS: {
    //         LOG_DEBUG("[KVS sample] Received EOS message");
    //         cv.notify_all();
    //         break;
    //     }

    //     case GST_MESSAGE_ERROR: {
    //         gchar  *debug;
    //         GError *error;

    //         gst_message_parse_error(msg, &error, &debug);
    //         g_free(debug);

    //         LOG_ERROR("[KVS sample] GStreamer error: " << error->message);
    //         g_error_free(error);

    //         g_main_loop_quit(loop);
    //         break;
    //     }

    //     default: {
    //         break;
    //     }
    // }

    return TRUE;
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

    data->kinesis_video_producer = KinesisVideoProducer::createSync(std::move(device_info_provider),
                                                                    std::move(client_callback_provider),
                                                                    std::move(stream_callback_provider),
                                                                    std::move(credential_provider),
                                                                    API_CALL_CACHE_TYPE_ALL,
                                                                    defaultRegionStr);

    LOG_DEBUG("Client is ready");
}

void kinesis_video_stream_init(CustomData *data) {
    /* create a test stream */
    map<string, string> tags;
    char tag_name[MAX_TAG_NAME_LEN];
    char tag_val[MAX_TAG_VALUE_LEN];
    SNPRINTF(tag_name, MAX_TAG_NAME_LEN, "piTag");
    SNPRINTF(tag_val, MAX_TAG_VALUE_LEN, "piValue");

    STREAMING_TYPE streaming_type = DEFAULT_STREAMING_TYPE;
    data->use_absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;

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
        true,
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
    data->kinesis_video_stream = data->kinesis_video_producer->createStreamSync(std::move(stream_definition));

    // reset state
    data->stream_status = STATUS_SUCCESS;
    data->stream_started = false;

    LOG_DEBUG("Stream is ready");
}


int gstreamer_live_source_init(int argc, char* argv[], CustomData *data, GstElement *pipeline) {

    GstElement *source, *clock_overlay, *video_convert, *source_filter, *encoder, *sink_filter, *appsink;

    GstCaps *source_caps, *sink_caps;

    GstBus *bus;
    guint bus_watch_id;

    StreamSource source_type;

     /* Parse input arguments */

    //  DLOGE("Parsing input arguments...");
    //  // Check for invalid argument count, get stream name.
    //  if(argc > 3) {
    //      LOG_ERROR("[KVS sample] Invalid argument count, too many arguments.");
    //      LOG_INFO("[KVS sample] Usage: " << argv[0] << " <streamName (optional)> <testsrc or devicesrc (optional)>");
    //      return -1;
    //  } else if(argc > 1) {
    //      STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    //  }
 
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
 
                  
    GstElement* parse = gst_element_factory_make("h264parse", "parse");
 
     /* sink filter */
     sink_filter = gst_element_factory_make("capsfilter", "sink-filter");
     sink_caps = gst_caps_new_simple("video/x-h264",
                                     "alignment", G_TYPE_STRING, "au",
                                     "stream-format", G_TYPE_STRING, "avc",
                                     NULL);
     g_object_set(G_OBJECT(sink_filter), "caps", sink_caps, NULL);
     gst_caps_unref(sink_caps);
 
 
     appsink = gst_element_factory_make("appsink", "appsink");
 
     if (!pipeline || !source || !clock_overlay || !video_convert || 
         !source_filter || !encoder || !sink_filter || !parse || !appsink)
     {
         LOG_ERROR("[KVS sample] Not all GStreamer elements could be created.");
         return -1;
     }
 
     g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
     g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);

     
    DLOGE("GStreamer elements created successfully.");
    DLOGE("Creating GStreamer bus...");
    // Add a message handler.
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    DLOGE("Created bus.");
    bus_watch_id = gst_bus_add_watch(bus, bus_call, &data);
    DLOGE("Added bus watch.");
    gst_object_unref(bus);

    DLOGE("Adding elements to pipeline...");
    // Add elements into the pipeline.
    gst_bin_add_many(GST_BIN(pipeline),
                 source, clock_overlay, video_convert,
                 source_filter, encoder, sink_filter, parse, appsink,
                 NULL);


    // Link the elements together.

    if (!gst_element_link_many(source, clock_overlay, video_convert,
        source_filter, encoder, sink_filter, parse, appsink, NULL)) {
        LOG_ERROR("Could not link main chain");
        gst_object_unref(pipeline);
        return -1;
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


    LOG_INFO("Streaming from live source");
    pipeline = gst_pipeline_new("live-kinesis-pipeline");
    ret = gstreamer_live_source_init(argc, argv, data, pipeline);

    if (ret != 0){
        return ret;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    // GstBus *bus = gst_element_get_bus(pipeline);
    // gst_bus_add_signal_watch(bus);
    // g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    // gst_object_unref(bus);

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
    // gst_bus_remove_signal_watch(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;
    return 0;
}

int main(int argc, char* argv[]) {
    PropertyConfigurator::doConfigure("../kvs_log_configuration");

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

    data.pGoPList = NULL;
    singleListCreate(&data.pGoPList);


    /* init Kinesis Video */
    try{
        kinesis_video_init(&data);
        kinesis_video_stream_init(&data);
    } catch (runtime_error &err) {
        LOG_ERROR("Failed to initialize kinesis video with an exception: " << err.what());
        return 1;
    }

    bool do_retry = true;

    std::thread cameraEventSchedulerThread(cameraEventScheduler);

    if (gstreamer_init(argc, argv, &data) != 0) {
        LOG_ERROR("Failed to initialize gstreamer");
        return 1;
    }

    cameraEventSchedulerThread.join();

    if (STATUS_SUCCEEDED(stream_status)) {
        // if stream_status is success after eos, send out remaining frames.
        data.kinesis_video_stream->stopSync();
    } else {
        data.kinesis_video_stream->stop();
    }

    // CleanUp
    data.kinesis_video_producer->freeStream(data.kinesis_video_stream);

    return 0;
}
