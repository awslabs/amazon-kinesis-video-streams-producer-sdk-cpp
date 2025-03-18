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

using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

LOGGER_TAG("com.amazonaws.kinesis.video.gstreamer");


// Modify these parameters to configure the buffer and event streaming behavior.
#define CAMERA_EVENT_LIVE_STREAM_DURATION_SECONDS 30 // Duration of live streaming to KVS upon an event.
#define CAMERA_EVENT_COOLDOWN_SECONDS             90 // How long for the event scheduler to wait between triggering events.
#define STREAM_BUFFER_DURATION_SECONDS            30 // How long to store buffered GoPs for. It is guaranteed that the buffer will be at least this long,
                                                     // but the buffer may be longer to contain the I-frame associated with buffered P-frames.
                                                    
// Stream definition parameters.
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

#define DEFAULT_STREAM_NAME "DEFAULT_STREAM_NAME"
#define KVS_GST_TEST_SOURCE_NAME "test-source"
#define KVS_GST_DEVICE_SOURCE_NAME "device-source"


/* *************************** */
/* Global Variable Definitions */
/* *************************** */

std::atomic<bool> appTerminated(FALSE);
std::condition_variable appTerminationCv;

std::atomic<bool> cameraEventTriggered(false);

GMainLoop *main_loop; // Declared globally to simplify sigint_handler logic.

// Input source types.
typedef enum _StreamSource {
    TEST_SOURCE, // Will use videotestsrc GStreamer element.
    DEVICE_SOURCE // Will use autovideosrc GStreamer element.
} StreamSource;

// Group of Pictures (GoP) structure containing an I-frame and a list of P-Frames.
typedef struct _Gop{
    Frame* pIFrame;
    PSingleList pPFrames; // P-frames associated with this GoP
} Gop;

// Application data to pass to GStreamer and Producer callbacks.
typedef struct _CustomData {
    _CustomData():
            synthetic_dts(0),
            stream_status(STATUS_SUCCESS),
            first_pts(GST_CLOCK_TIME_NONE),
            use_absolute_fragment_times(true) {
        producer_start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(systemCurrentTime().time_since_epoch()).count();
    }

    std::unique_ptr<Credentials> credential;
    std::unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    std::shared_ptr<KinesisVideoStream> kinesis_video_stream;
    bool stream_started;
    char *stream_name;

    // stores any error status code reported by StreamErrorCallback.
    std::atomic_uint stream_status;

    // Assuming frame timestamp are relative. Add producer_start_time to each frame's
    // timestamp to convert them to absolute timestamp. This way fragments dont overlap after token rotation.
    uint64_t producer_start_time;

    uint64_t synthetic_dts;

    bool use_absolute_fragment_times;

    // Pts of first video frame
    uint64_t first_pts;

    // The memory buffer for the stream.
    SingleList* pGopList;
} CustomData;



/* *************************** */
/* Global Function Definitions */
/* *************************** */

void sigint_handler(int sigint) {
    LOG_INFO("SIGINT received. Terminating application.");
    appTerminated = TRUE;
    appTerminationCv.notify_all();
    if (main_loop != NULL) {
        g_main_loop_quit(main_loop);
    }
}

// Triggers camera events which kicks-off sending buffered frames to and live-streaming to KVS.
void cameraEventScheduler() {
    std::mutex cv_m;
    std::unique_lock<std::mutex> lck(cv_m);
    
    while(!appTerminated) {

        LOG_INFO("Waiting " << CAMERA_EVENT_COOLDOWN_SECONDS << " seconds before triggering a camera event.");

        // Wait for the specified time before triggering an event, exit early if app was terminated.
        if(appTerminationCv.wait_for(lck, std::chrono::seconds(CAMERA_EVENT_COOLDOWN_SECONDS)) != std::cv_status::timeout) {
            return;
        }
        cameraEventTriggered = true;

        LOG_INFO("Camera event triggered, will send all frames currenlty in the buffer and live-stream new frames for " <<
                    CAMERA_EVENT_LIVE_STREAM_DURATION_SECONDS << " seconds.");
        
        // Wait for the specified time before ending the event to stop live-streaming to KVS.
        if(appTerminationCv.wait_for(lck, std::chrono::seconds(CAMERA_EVENT_LIVE_STREAM_DURATION_SECONDS)) != std::cv_status::timeout) {
            return;
        }
        cameraEventTriggered = false;

    }
}

// Use this function when writing frames to the memory buffer. We need to allocate memory for and copy the frames received by
// appsink because the frame is freed after its new-sample appsink callback invocation.
void create_and_allocate_kinesis_video_frame(Frame *frame, const std::chrono::nanoseconds &pts, const std::chrono::nanoseconds &dts, FRAME_FLAGS flags,
                                                void *data, size_t len) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 0;
    frame->size = static_cast<UINT32>(len);
    frame->frameData = (uint8_t*) malloc(frame->size);
    frame->trackId = DEFAULT_TRACK_ID;

    memcpy(frame->frameData, data, frame->size);
}

void create_kinesis_video_frame(Frame *frame, const std::chrono::nanoseconds &pts, const std::chrono::nanoseconds &dts, FRAME_FLAGS flags,
                                void *data, size_t len) {
    frame->flags = flags;
    frame->decodingTs = static_cast<UINT64>(dts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->presentationTs = static_cast<UINT64>(pts.count()) / DEFAULT_TIME_UNIT_IN_NANOS;
    frame->duration = 0;
    frame->size = static_cast<UINT32>(len);
    frame->frameData = reinterpret_cast<PBYTE>(data);
    frame->trackId = DEFAULT_TRACK_ID;
}

// Populates Frame struct and it sends to KVS Producer.
bool put_frame(std::shared_ptr<KinesisVideoStream> kinesis_video_stream, void *data, size_t len, const std::chrono::nanoseconds &pts, const std::chrono::nanoseconds &dts, FRAME_FLAGS flags) {
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, data, len);
    return kinesis_video_stream->putFrame(frame);
}

// Callback fired when appsink receives a new frame.
static GstFlowReturn on_new_sample(GstElement *sink, CustomData *data) {
    GstBuffer *gst_buffer; // Buffer associated with the sample received by appsink (not to be confused with the GoP buffer).
    bool isDroppable, isHeader, delta;
    GstFlowReturn ret = GST_FLOW_OK;
    STATUS curr_stream_status = data->stream_status.load();
    GstSample *sample = nullptr;
    GstMapInfo info;
    uint64_t now_ms;

    // Get current time.
    now_ms = (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(systemCurrentTime().time_since_epoch()).count();

    if (STATUS_FAILED(curr_stream_status)) {
        LOG_ERROR("Received stream error: " << curr_stream_status);
        ret = GST_FLOW_ERROR;
        goto CleanUp;
    }

    info.data = nullptr;
    sample = gst_app_sink_pull_sample(GST_APP_SINK (sink));

    // Capture cpd at the first frame.
    if (!data->stream_started) {
        data->stream_started = true;
        GstCaps* gstcaps  = (GstCaps*) gst_sample_get_caps(sample);
        GstStructure * gststructforcaps = gst_caps_get_structure(gstcaps, 0);
        const GValue *gstStreamFormat = gst_structure_get_value(gststructforcaps, "codec_data");
        gchar *cpd = gst_value_serialize(gstStreamFormat);
        data->kinesis_video_stream->start(std::string(cpd));
        g_free(cpd);
    }

    gst_buffer = gst_sample_get_buffer(sample);
    isHeader = GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_HEADER);
    isDroppable = GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_CORRUPTED) ||
                  GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
                  (GST_BUFFER_FLAGS(gst_buffer) == GST_BUFFER_FLAG_DISCONT) ||
                  (GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                  // drop if gst_buffer contains header only and has invalid timestamp
                  (isHeader && (!GST_BUFFER_PTS_IS_VALID(gst_buffer) || !GST_BUFFER_DTS_IS_VALID(gst_buffer)));

    if (!isDroppable) {
        delta = GST_BUFFER_FLAG_IS_SET(gst_buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        FRAME_FLAGS kinesis_video_flags = delta ? FRAME_FLAG_NONE : FRAME_FLAG_KEY_FRAME;

        if (!GST_BUFFER_DTS_IS_VALID(gst_buffer)) {
            data->synthetic_dts += DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
            gst_buffer->dts = data->synthetic_dts;
        } else {
            data->synthetic_dts = gst_buffer->dts;
        }

        if (data->use_absolute_fragment_times) {
            if (data->first_pts == GST_CLOCK_TIME_NONE) {
                data->first_pts = gst_buffer->pts;
            }
            gst_buffer->pts += data->producer_start_time - data->first_pts;
        }

        if (!gst_buffer_map(gst_buffer, &info, GST_MAP_READ)){
            goto CleanUp;
        }

        PSingleList pGopList = (PSingleList) data->pGopList;


        // If there currently is no camera event, write frames to buffer.
        if(!cameraEventTriggered) {

            // A key frame indicates we need to allocate a new GoP onto the buffer. We will
            // also check for old GoPs to be removed from the buffer.
            if(CHECK_FRAME_FLAG_KEY_FRAME(kinesis_video_flags)) {

                // Make a new GoP object.
                Gop* newGop = (Gop *)calloc(1, sizeof(Gop));
                
                // Allocate a new Frame object.
                newGop->pIFrame = (Frame *) calloc(1, sizeof(Frame));

                // Copy the frame sample buffer data to the new Frame allocation since it will be freed by GStreamer after we process it.
                create_and_allocate_kinesis_video_frame(newGop->pIFrame, std::chrono::nanoseconds(gst_buffer->pts), std::chrono::nanoseconds(gst_buffer->pts), kinesis_video_flags, info.data, info.size);
                singleListCreate(&(newGop->pPFrames));
                singleListInsertItemTail(pGopList, (UINT64) newGop);


                // Now let's check if there are GoP objects whose frames are all older than STREAM_BUFFER_DURATION_SECONDS.
                // If so, remove them from the list and delete the frame file.
                PSingleListNode pNode = NULL;
                singleListGetHeadNode(pGopList, &pNode);
                while(pNode != NULL && pNode->pNext != NULL) { // Iterate through the GoPs.
                    PSingleListNode pNextNode = pNode->pNext;
                    Gop* pGop = (Gop *) pNode->data;
                    Gop* pNextGop = (Gop *) pNextNode->data;

                    // If the next GoP's start timestamp is within the STREAM_BUFFER_DURATION_SECONDS, then we can delete this GoP. This logic allows for
                    // the buffer duration to be exceeded to ensure we don't hold fewer frames than the duration in the cases where the duration
                    // threshold lays on a P-frame in the middle of a GoP -- we do not want to discard such a GoP.
                    if((now_ms - pNextGop->pIFrame->presentationTs / HUNDREDS_OF_NANOS_IN_A_MILLISECOND) >= (STREAM_BUFFER_DURATION_SECONDS * 1000)) {
                        // The next GoP lays outside of or directly on the buffer duration. We can free the current GoP.
                        singleListFree(pGop->pPFrames); // Free the P-Frames.
                        free(pGop->pIFrame); // Free the I-Frame.
                        free(pGop); // DeleteNode does not free the node data, so free the GoP.
                        singleListDeleteNode(pGopList, pNode); // Remove the node.
                        pNode = NULL;
                    } else {
                        // The next GoP does not meet nor exceed the STREAM_BUFFER_DURATION_SECONDS interval.
                        // Stop traversing the list.
                        break;
                    }
                    pNode = pNextNode;
                    pNextNode = pNextNode->pNext;
                }
            } else { // Not a keyframe.

                // Add the frame to the P-Frames list of the latest GoP object.
                PSingleListNode pTailNode = NULL;
    
                singleListGetTailNode(pGopList, &pTailNode);

                if(pTailNode != NULL) {
                    // Allocate a new Frame object.
                    Frame* pPFrame = (Frame *) calloc(1, sizeof(Frame));

                    // Copy the frame sample buffer data to the new Frame allocation since it will be freed by GStreamer after we process it.
                    create_and_allocate_kinesis_video_frame(pPFrame, std::chrono::nanoseconds(gst_buffer->pts), std::chrono::nanoseconds(gst_buffer->pts), kinesis_video_flags, info.data, info.size);
                    singleListInsertItemTail(((Gop *)pTailNode->data)->pPFrames, (UINT64) pPFrame);
                }
            }

        } else { // Camera event is currently triggered.

            // Handle event: send buffered frames, send live frame.

            // Send buffered frames if the buffer list is populated.
            if (pGopList->pHead != NULL) {
                LOG_INFO("Sending buffered frames...");
                for (auto it = pGopList->pHead; it != NULL; it = it->pNext) {
                    Gop* pGop = (Gop *) it->data;
                    data->kinesis_video_stream->putFrame(*pGop->pIFrame);
    
                    // PutFrame synchonously makes a copy of the frame data in the Producer, free our buffer's frame's data.
                    free(pGop->pIFrame->frameData);
                    free(pGop->pIFrame);

                    for (auto it2 = pGop->pPFrames->pHead; it2 != NULL; it2 = it2->pNext) {
                        Frame* pPFrame = (Frame *) it2->data;
                        LOG_DEBUG("Putting buffered frame with ts: " << pPFrame->presentationTs);
                        data->kinesis_video_stream->putFrame(*pPFrame);
                        free(pPFrame->frameData);
                        // pPFrame will be freed when we free the pPFrames list.
                    }

                    // Free the PFrames list (will also free the pPFrame nodes).
                    singleListFree(pGop->pPFrames);

                }
                // Clear the GoP list (will also free the pGoP nodes).
                singleListClear(pGopList, TRUE);
                LOG_INFO("Done sending buffered frames...");

            }
            // Now that we cleared the buffer list, it will no longer be populated for this camera event, will jump straight to the below
            // putFrame call to send live frames until the event is over.

            // Send the live frame.
            LOG_DEBUG("Putting live frame with ts: " << gst_buffer->pts);
            put_frame(data->kinesis_video_stream, info.data, info.size, std::chrono::nanoseconds(gst_buffer->pts), std::chrono::nanoseconds(gst_buffer->pts), kinesis_video_flags);

        }
    }

CleanUp:

    if (info.data != nullptr) {
        gst_buffer_unmap(gst_buffer, &info);
    }

    if (sample != nullptr) {
        gst_sample_unref(sample);
    }

    return ret;
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    switch(GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS: {
            LOG_DEBUG("[KVS sample] Received EOS message");
            if (main_loop != NULL) {
                g_main_loop_quit(main_loop);
            }
            break;
        }

        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            LOG_ERROR("[KVS sample] GStreamer error: " << error->message);
            g_error_free(error);

            if (main_loop != NULL) {
                g_main_loop_quit(main_loop);
            }
            break;
        }

        default: {
            break;
        }
    }

    return TRUE;
}

// KVS Producer callbacks
namespace com { namespace amazonaws { namespace kinesis { namespace video {
    class SampleClientCallbackProvider;
    class SampleStreamCallbackProvider;
    class SampleCredentialProvider;
    class SampleDeviceInfoProvider;
}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com

// Initializes KVS Producer client with the callbacks.
void kinesis_video_init(CustomData *data);

void kinesis_video_stream_init(CustomData *data) {
    STREAMING_TYPE streaming_type = DEFAULT_STREAMING_TYPE;
    data->use_absolute_fragment_times = DEFAULT_ABSOLUTE_FRAGMENT_TIMES;

    std::unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(
        data->stream_name,
        std::chrono::hours(DEFAULT_RETENTION_PERIOD_HOURS),
        NULL,
        DEFAULT_KMS_KEY_ID,
        streaming_type,
        DEFAULT_CONTENT_TYPE,
        std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::seconds(DEFAULT_MAX_LATENCY_SECONDS)),
        std::chrono::milliseconds(DEFAULT_FRAGMENT_DURATION_MILLISECONDS),
        std::chrono::milliseconds(DEFAULT_TIMECODE_SCALE_MILLISECONDS),
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
        std::chrono::seconds(DEFAULT_BUFFER_DURATION_SECONDS),
        std::chrono::seconds(DEFAULT_REPLAY_DURATION_SECONDS),
        std::chrono::seconds(DEFAULT_CONNECTION_STALENESS_SECONDS),
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

    GstElement *source, *clock_overlay, *video_convert, *source_filter, *encoder, *parser, *sink_filter, *appsink;
    GstCaps *source_caps, *sink_caps;
    StreamSource source_type;

 
     // Parse program argument for source type.
     if(argc > 2) {
         if(0 == STRCMPI(argv[2], "testsrc")) {
             LOG_INFO("[KVS sample] Using test source (videotestsrc)");
             source_type = TEST_SOURCE;
         } else if(0 == STRCMPI(argv[2], "devicesrc")) {
             LOG_INFO("[KVS sample] Using device source (autovideosrc)");
             source_type = DEVICE_SOURCE;
         } else {
             LOG_ERROR("[KVS sample] Invalid source type...\n"
                << std::string(55, ' ') << "Usage: " << std::string(8, ' ') << argv[0] << " <streamName (optional)> <testsrc or devicesrc (optional)>\n"
                << std::string(55, ' ') << "Example Usage: " << argv[0] << " myStreamName testsrc");
            return -1;
             return -1;
         }
     } else {
         LOG_ERROR("[KVS sample] No source specified, defualting to test source (videotestsrc)");
         source_type = TEST_SOURCE;
     }
 

     /* Create GStreamer elements */
  
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
                  "tune", 0x00000004,
                  "key-int-max", 120, NULL);     
 
    /* h.264 parser */ 
    parser = gst_element_factory_make("h264parse", "parser");
 
    /* sink caps filter */
    sink_filter = gst_element_factory_make("capsfilter", "sink-filter");
    sink_caps = gst_caps_new_simple("video/x-h264",
                                    "alignment", G_TYPE_STRING, "au",
                                    "stream-format", G_TYPE_STRING, "avc",
                                    NULL);
    g_object_set(G_OBJECT(sink_filter), "caps", sink_caps, NULL);
    gst_caps_unref(sink_caps);

    /* appsink */
    appsink = gst_element_factory_make("appsink", "appsink");
    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);


    if (!pipeline || !source || !clock_overlay || !video_convert || 
        !source_filter || !encoder || !sink_filter || !parser || !appsink)
    {
        LOG_ERROR("[KVS sample] Not all GStreamer elements could be created.");
        return -1;
    }

    // Add elements into the pipeline.
    gst_bin_add_many(GST_BIN(pipeline),
                 source, clock_overlay, video_convert,
                 source_filter, encoder, sink_filter, parser, appsink,
                 NULL);

    // Link the elements together.
    if (!gst_element_link_many(source, clock_overlay, video_convert,
        source_filter, encoder, sink_filter, parser, appsink, NULL)) {
        LOG_ERROR("Elements could not be linked");
        gst_object_unref(pipeline);
        return -1;
    }

    return 0;
}

int init_and_run_gstreamer_pipeline(int argc, char* argv[], CustomData *data) {

    int ret;
    GstElement *pipeline;
    GstStateChangeReturn gst_ret;
    GstBus *bus;
    guint bus_watch_id;

    // Initialize GStreamer.
    gst_init(&argc, &argv);
        
    // Reset first frame pts.
    data->first_pts = GST_CLOCK_TIME_NONE;

    LOG_INFO("Streaming from live source");
    pipeline = gst_pipeline_new("live-kinesis-pipeline");
    ret = gstreamer_live_source_init(argc, argv, data, pipeline);

    if (ret != 0){
        return ret;
    }

    // Add a bus message handler.
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, &data);
    gst_object_unref(bus);

    // Start streaming.
    gst_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (gst_ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // Notify waiting threads we are done streaming.
    appTerminated = true;
    appTerminationCv.notify_all();

    // Free resources.
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(main_loop);
    main_loop = NULL;
    return 0;
}


/* **** */
/* Main */
/* **** */
int main(int argc, char* argv[]) {
    signal(SIGINT, sigint_handler);
    main_loop = NULL;

    PropertyConfigurator::doConfigure("../kvs_log_configuration");

    CustomData data;
    char stream_name[MAX_STREAM_NAME_LEN + 1];
    int ret = 0;
    int maxArgCount = 3;
    STATUS stream_status = STATUS_SUCCESS;

    // Validate there are no extra program arguments.
    if (argc > maxArgCount) {
        LOG_ERROR("[KVS sample] Invalid input. Program arguments count ("
            << argc << ") exceeds max supported count (" << maxArgCount << ")...\n"
            << std::string(55, ' ') << "Usage: " << std::string(8, ' ') << argv[0] << " <streamName (optional)> <testsrc or devicesrc (optional)>\n"
            << std::string(55, ' ') << "Example Usage: " << argv[0] << " myStreamName testsrc");
        return -1;
    }

    // Parse the stream name from program argument.
    if (argc > 1) {
        LOG_INFO("Stream name specified: " << argv[1]);
        STRNCPY(stream_name, argv[1], MAX_STREAM_NAME_LEN);
    } else {
        LOG_INFO("No stream name specified, using default stream name: " << DEFAULT_STREAM_NAME);
        STRNCPY(stream_name, DEFAULT_STREAM_NAME, MAX_STREAM_NAME_LEN);
    }
    stream_name[MAX_STREAM_NAME_LEN] = '\0';
    data.stream_name = stream_name;

    // Initialize the GoP buffer linked list.
    data.pGopList = NULL;
    singleListCreate(&data.pGopList);

    // Initialize Kinesis Video Client and Stream constructs.
    try{
        kinesis_video_init(&data);
        kinesis_video_stream_init(&data);
    } catch (std::runtime_error &err) {
        LOG_ERROR("Failed to initialize Kinesis Video with an exception: " << err.what());
        return 1;
    }

    // This thread will trigger events during streaming, prompting the buffer to be sent to
    // KVS and for live streaming to KVS to begin.
    std::thread cameraEventSchedulerThread(cameraEventScheduler);

    // Initialize GStreamer, construct pipeline, and begin streaming.
    if (init_and_run_gstreamer_pipeline(argc, argv, &data) != 0) {
        LOG_ERROR("Failed to initialize gstreamer");
        return 1;
    }
    
    // End of stream. The GMainLoop has exited.

    cameraEventSchedulerThread.join();

    if (STATUS_SUCCEEDED(stream_status)) {
        // If stream_status is success after eos, send out remaining frames.
        data.kinesis_video_stream->stopSync();
    } else {
        data.kinesis_video_stream->stop();
    }

    // CleanUp
    data.kinesis_video_producer->freeStream(data.kinesis_video_stream);

    return 0;
}




/* ********************** */
/* Additional Definitions */
/* ********************** */

/* KVS Producer callbacks */
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
    
        if (terminate_pipeline && main_loop != NULL) {
            LOG_WARN("Terminating pipeline due to unrecoverable stream error: " << status_code);
            g_main_loop_quit(main_loop);
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


/* Initializes KVS Producer client with the callbacks. */
void kinesis_video_init(CustomData *data) {
    std::unique_ptr<DeviceInfoProvider> device_info_provider(new SampleDeviceInfoProvider());
    std::unique_ptr<ClientCallbackProvider> client_callback_provider(new SampleClientCallbackProvider());
    std::unique_ptr<StreamCallbackProvider> stream_callback_provider(new SampleStreamCallbackProvider(
            reinterpret_cast<UINT64>(data)));

    char const *accessKey;
    char const *secretKey;
    char const *sessionToken;
    char const *defaultRegion;
    std::string defaultRegionStr;
    std::string sessionTokenStr;

    char const *iot_get_credential_endpoint;
    char const *cert_path;
    char const *private_key_path;
    char const *role_alias;
    char const *ca_cert_path;

    std::unique_ptr<CredentialProvider> credential_provider;

    if (nullptr == (defaultRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
        defaultRegionStr = DEFAULT_AWS_REGION;
    } else {
        defaultRegionStr = std::string(defaultRegion);
    }
    LOG_INFO("Using region: " << defaultRegionStr);

    if (nullptr != (accessKey = getenv(ACCESS_KEY_ENV_VAR)) &&
        nullptr != (secretKey = getenv(SECRET_KEY_ENV_VAR))) {

        LOG_INFO("Using aws credentials for Kinesis Video Streams");
        if (nullptr != (sessionToken = getenv(SESSION_TOKEN_ENV_VAR))) {
            LOG_INFO("Session token detected.");
            sessionTokenStr = std::string(sessionToken);
        } else {
            LOG_INFO("No session token was detected.");
            sessionTokenStr = "";
        }

        data->credential.reset(new Credentials(std::string(accessKey),
                                                std::string(secretKey),
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
