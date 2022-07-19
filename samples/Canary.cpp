#pragma once

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
#include <unistd.h> 
#include <com/amazonaws/kinesis/video/cproducer/Include.h>
#include <aws/core/Aws.h>
#include <aws/monitoring/CloudWatchClient.h>
#include <aws/monitoring/model/PutMetricDataRequest.h>
#include <aws/logs/CloudWatchLogsClient.h>
#include <aws/logs/model/CreateLogGroupRequest.h>
#include <aws/logs/model/CreateLogStreamRequest.h>
#include <aws/logs/model/PutLogEventsRequest.h>
#include <aws/logs/model/DeleteLogStreamRequest.h>
#include <aws/logs/model/DescribeLogStreamsRequest.h>

#include "CanaryCallbackProvider.h"
#include "CanaryConfig.h"
#include "CanaryLogs.h"
#include "CustomData.h"


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
// #define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_TIMECODE_SCALE_MILLISECONDS 1
#define DEFAULT_KEY_FRAME_FRAGMENTATION TRUE
#define DEFAULT_FRAME_TIMECODES TRUE
#define DEFAULT_ABSOLUTE_FRAGMENT_TIMES TRUE
#define DEFAULT_FRAGMENT_ACKS TRUE
#define DEFAULT_RESTART_ON_ERROR TRUE
#define DEFAULT_RECALCULATE_METRICS TRUE
#define DEFAULT_AVG_BANDWIDTH_BPS (4 * 1024 * 1024)
// #define DEFAULT_BUFFER_DURATION_SECONDS 120
#define DEFAULT_REPLAY_DURATION_SECONDS 40
#define DEFAULT_CONNECTION_STALENESS_SECONDS 60
#define DEFAULT_CODEC_ID "V_MPEG4/ISO/AVC"
#define DEFAULT_TRACKNAME "kinesis_video"
#define DEFAULT_FRAME_DURATION_MS 1
#define DEFAULT_CREDENTIAL_ROTATION_SECONDS 3600
#define DEFAULT_CREDENTIAL_EXPIRATION_SECONDS 180

#define CANARY_METADATA_SIZE  (SIZEOF(INT64) + SIZEOF(UINT32) + SIZEOF(UINT32) + SIZEOF(UINT64))

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


// PCHAR getLogLevelStr(UINT32 loglevel)
// {
//     switch (loglevel) {
//         case LOG_LEVEL_VERBOSE:
//             return LOG_LEVEL_VERBOSE_STR;
//         case LOG_LEVEL_DEBUG:
//             return LOG_LEVEL_DEBUG_STR;
//         case LOG_LEVEL_INFO:
//             return LOG_LEVEL_INFO_STR;
//         case LOG_LEVEL_WARN:
//             return LOG_LEVEL_WARN_STR;
//         case LOG_LEVEL_ERROR:
//             return LOG_LEVEL_ERROR_STR;
//         case LOG_LEVEL_FATAL:
//             return LOG_LEVEL_FATAL_STR;
//         default:
//             return LOG_LEVEL_SILENT_STR;
//     }
// }

// VOID addLogMetadata(PCHAR buffer, UINT32 bufferLen, PCHAR fmt, UINT32 logLevel)
// {
//     UINT32 timeStrLen = 0;
//     // space for "yyyy-mm-dd HH:MM:SS\0" + space + null
//     CHAR timeString[MAX_TIMESTAMP_FORMAT_STR_LEN + 1 + 1];
//     STATUS retStatus = STATUS_SUCCESS;
//     UINT32 offset = 0;

// #ifdef ENABLE_LOG_THREAD_ID
//     // MAX_THREAD_ID_STR_LEN + null
//     CHAR tidString[MAX_THREAD_ID_STR_LEN + 1];
//     TID threadId = GETTID();
//     SNPRINTF(tidString, ARRAY_SIZE(tidString), "(thread-0x%" PRIx64 ")", threadId);
// #endif

//     // if something fails in getting time, still print the log, just without timestamp
//     retStatus = generateTimestampStr(globalGetTime(), "%Y-%m-%d %H:%M:%S ", timeString, (UINT32) ARRAY_SIZE(timeString), &timeStrLen);
//     if (STATUS_FAILED(retStatus)) {
//         PRINTF("Fail to get time with status code is %08x\n", retStatus);
//         timeString[0] = '\0';
//     }

//     offset = (UINT32) SNPRINTF(buffer, bufferLen, "%s%-*s ", timeString, MAX_LOG_LEVEL_STRLEN, getLogLevelStr(logLevel));
// #ifdef ENABLE_LOG_THREAD_ID
//     offset += SNPRINTF(buffer + offset, bufferLen - offset, "%s ", tidString);
// #endif
//     SNPRINTF(buffer + offset, bufferLen - offset, "%s\n", fmt);
// }

void pushMetric(string metricName, double metricValue, Aws::CloudWatch::Model::StandardUnit unit, Aws::CloudWatch::Model::MetricDatum datum, 
                Aws::CloudWatch::Model::Dimension *dimension, Aws::CloudWatch::Model::PutMetricDataRequest &cwRequest)
{
    datum.SetMetricName(metricName);
    datum.AddDimensions(*dimension);
    datum.SetValue(metricValue);
    datum.SetUnit(unit);
    cwRequest.AddMetricData(datum);
}

STATUS
SampleClientCallbackProvider::storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes) {
    UNUSED_PARAM(custom_handle);
    LOG_WARN("Reporting storage overflow. Bytes remaining " << remaining_bytes);
    return STATUS_SUCCESS;
}

STATUS
SampleStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
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
    if (pFragmentAck->ackType != FRAGMENT_ACK_TYPE_PERSISTED && pFragmentAck->ackType != FRAGMENT_ACK_TYPE_RECEIVED)
    {
        return STATUS_SUCCESS;
    }

    cout << "Map Debug: searching in map for a key of pFragmentAck->timestamp: " << pFragmentAck->timestamp << endl;
    map<uint64_t, uint64_t>::iterator iter;
    iter = data->timeOfNextKeyFrame->find(pFragmentAck->timestamp);
    if(iter == data->timeOfNextKeyFrame->end())
    {
        cout << "Map Debug: TimeOfNextKeyFrame key-value pair not present in map" << endl;
    }
    else
    {
        cout << "Map Debug: Found TimeOfNextKeyFrame key-value pair in map" << endl;
        uint64_t timeOfFragmentEndSent = data->timeOfNextKeyFrame->find(pFragmentAck->timestamp)->second;
        cout << "Map Debug: with value of " << timeOfFragmentEndSent << endl;
        cout << "Map Debug: latency: " << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - timeOfFragmentEndSent << endl;

        // When Canary sleeps, timeOfFragmentEndSent become less than currentTimeStamp, don't send those metrics
        if (timeOfFragmentEndSent > pFragmentAck->timestamp)
        {
            if (pFragmentAck->ackType == FRAGMENT_ACK_TYPE_PERSISTED)
            {
                Aws::CloudWatch::Model::MetricDatum persistedAckLatency_datum;
                Aws::CloudWatch::Model::PutMetricDataRequest cwRequest;
                cwRequest.SetNamespace("KinesisVideoSDKCanaryCPP");

                auto currentTimestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                auto persistedAckLatency = (currentTimestamp - timeOfFragmentEndSent); // [milliseconds]
                pushMetric("PersistedAckLatency", persistedAckLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, persistedAckLatency_datum, data->Pdimension_per_stream, cwRequest);
                if (data->pCanaryConfig->useAggMetrics)
                {
                    pushMetric("PersistedAckLatency", persistedAckLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, persistedAckLatency_datum, data->Paggregated_dimension, cwRequest);

                }
                auto outcome = data->pCWclient->PutMetricData(cwRequest);
                if (!outcome.IsSuccess())
                {
                    cout << "Failed to put PersistedAckLatency metric data:" << outcome.GetError().GetMessage() << endl;
                }
                else
                {
                    cout << "Successfully put PersistedAckLatency metric data" << endl;
                }
            } else if (pFragmentAck->ackType == FRAGMENT_ACK_TYPE_RECEIVED)
                {
                    Aws::CloudWatch::Model::MetricDatum recievedAckLatency_datum;
                    Aws::CloudWatch::Model::PutMetricDataRequest cwRequest;
                    cwRequest.SetNamespace("KinesisVideoSDKCanaryCPP");

                    auto currentTimestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                    auto recievedAckLatency = (currentTimestamp - timeOfFragmentEndSent); // [milliseconds]
                    pushMetric("RecievedAckLatency", recievedAckLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, recievedAckLatency_datum, data->Pdimension_per_stream, cwRequest);

                    if (data->pCanaryConfig->useAggMetrics)
                    {
                        pushMetric("RecievedAckLatency", recievedAckLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, recievedAckLatency_datum, data->Paggregated_dimension, cwRequest);
                    }

                    auto outcome = data->pCWclient->PutMetricData(cwRequest);
                    if (!outcome.IsSuccess())
                    {
                        cout << "Failed to put RecievedAckLatency metric data:" << outcome.GetError().GetMessage() << endl;
                    }
                    else
                    {
                        cout << "Successfully put RecievedAckLatency metric data" << endl;
                    }
                }
        } else
        {
            cout << "Not sending Ack Latency metric because: timeOfFragmentEndSent < pFragmentAck->timestamp" << endl;
        }
    }
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;

// add frame pts, frame index, original frame size, CRC to beginning of buffer
VOID addCanaryMetadataToFrameData(PFrame pFrame)
{
    PBYTE pCurPtr = pFrame->frameData;
    putUnalignedInt64BigEndian((PINT64) pCurPtr, pFrame->presentationTs / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    pCurPtr += SIZEOF(UINT64);
    putUnalignedInt32BigEndian((PINT32) pCurPtr, pFrame->index);
    pCurPtr += SIZEOF(UINT32);
    putUnalignedInt32BigEndian((PINT32) pCurPtr, pFrame->size);
    pCurPtr += SIZEOF(UINT32);
    putUnalignedInt32BigEndian((PINT32) pCurPtr, COMPUTE_CRC32(pFrame->frameData, pFrame->size));
}

VOID createCanaryFrameData(PFrame pFrame)
{
    UINT32 i;

    for (i = CANARY_METADATA_SIZE; i < pFrame->size; i++) {
        pFrame->frameData[i] = RAND();
    }
    addCanaryMetadataToFrameData(pFrame);
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

void updateFragmentEndTimes(UINT64 curKeyFrameTime, uint64_t &lastKeyFrameTime, map<uint64_t, uint64_t> *mapPtr)
{
        if (lastKeyFrameTime != 0)
        {
            (*mapPtr)[lastKeyFrameTime / HUNDREDS_OF_NANOS_IN_A_MILLISECOND] = curKeyFrameTime / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
            cout << "Map Debug: current time is              " << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() << endl;
            cout << "Map Debug: adding lastKeyFrameTime key: " << lastKeyFrameTime / HUNDREDS_OF_NANOS_IN_A_MILLISECOND << endl;
            cout << "Map Debug: with curKeyFrameTime value:  " << curKeyFrameTime / HUNDREDS_OF_NANOS_IN_A_MILLISECOND<< endl;
            auto iter = mapPtr->begin();
            while (iter != mapPtr->end()) {
                // clean up map: remove timestamps older than 5 min from now
                // if (iter->first < (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - (300000)))
                if (iter->first < (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - (3000000)))
                {
                    iter = mapPtr->erase(iter);
                    cout << "Map Debug: ereasing a map key-value pair" << endl;
                } else {
                    break;
                }
            }
        }
        lastKeyFrameTime = curKeyFrameTime;
}

void pushKeyFrameMetrics(Frame frame, CustomData *cusData)
{
    updateFragmentEndTimes(frame.presentationTs, cusData->lastKeyFrameTime, cusData->timeOfNextKeyFrame);
    
    Aws::CloudWatch::Model::MetricDatum metricDatum;
    Aws::CloudWatch::Model::PutMetricDataRequest cwRequest;
    cwRequest.SetNamespace("KinesisVideoSDKCanaryCPP");    

    auto stream_metrics = cusData->kinesis_video_stream->getMetrics();
    auto client_metrics = cusData->kinesis_video_stream->getProducer().getMetrics();
    auto stream_metrics_raw = stream_metrics.getRawMetrics();
    
    double frameRate = stream_metrics.getCurrentElementaryFrameRate();
    pushMetric("FrameRate", frameRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Pdimension_per_stream, cwRequest);

    double transferRate = 8 * stream_metrics.getCurrentTransferRate() / 1024; // *8 makes it bytes->bits. /1024 bits->kilobits
    pushMetric("TransferRate", transferRate, Aws::CloudWatch::Model::StandardUnit::Kilobits_Second, metricDatum, cusData->Pdimension_per_stream, cwRequest);

    double currentViewDuration = stream_metrics.getCurrentViewDuration().count();
    pushMetric("CurrentViewDuration", currentViewDuration, Aws::CloudWatch::Model::StandardUnit::Milliseconds, metricDatum, cusData->Pdimension_per_stream, cwRequest);

    double availableStoreSize = client_metrics.getContentStoreSizeSize();
    pushMetric("ContentStoreAvailableSize", availableStoreSize, Aws::CloudWatch::Model::StandardUnit::Bytes, metricDatum, cusData->Pdimension_per_stream, cwRequest);

    if (cusData->pCanaryConfig->useAggMetrics)
    {
        pushMetric("FrameRate", frameRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Paggregated_dimension, cwRequest);
        pushMetric("TransferRate", transferRate, Aws::CloudWatch::Model::StandardUnit::Kilobits_Second, metricDatum, cusData->Paggregated_dimension, cwRequest);
        pushMetric("CurrentViewDuration", currentViewDuration, Aws::CloudWatch::Model::StandardUnit::Milliseconds, metricDatum, cusData->Paggregated_dimension, cwRequest);
        pushMetric("ContentStoreAvailableSize", availableStoreSize, Aws::CloudWatch::Model::StandardUnit::Bytes, metricDatum, cusData->Paggregated_dimension, cwRequest);
    }

    // Capture error rate metrics every 60 seconds
    double duration = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - cusData->timeCounter;
    if(duration > 60)
    {
        cusData->timeCounter = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

        double newPutFrameErrors = (double)stream_metrics_raw->putFrameErrors - cusData->totalPutFrameErrorCount;
        cusData->totalPutFrameErrorCount = stream_metrics_raw->putFrameErrors;
        double putFrameErrorRate = newPutFrameErrors / (double)duration;
        pushMetric("PutFrameErrorRate", putFrameErrorRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Pdimension_per_stream, cwRequest);

        double newErrorAcks = (double)stream_metrics_raw->errorAcks - cusData->totalErrorAckCount;
        cusData->totalErrorAckCount = stream_metrics_raw->errorAcks;
        double errorAckRate = newErrorAcks / (double)duration;
        pushMetric("ErrorAckRate", errorAckRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Pdimension_per_stream, cwRequest);

        double totalNumberOfErrors = cusData->totalPutFrameErrorCount + cusData->totalErrorAckCount;
        pushMetric("TotalNumberOfErrors", totalNumberOfErrors, Aws::CloudWatch::Model::StandardUnit::Count, metricDatum, cusData->Pdimension_per_stream, cwRequest);

        if (cusData->pCanaryConfig->useAggMetrics)
        {
            pushMetric("PutFrameErrorRate", putFrameErrorRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Paggregated_dimension, cwRequest);
            pushMetric("ErrorAckRate", errorAckRate, Aws::CloudWatch::Model::StandardUnit::Count_Second, metricDatum, cusData->Paggregated_dimension, cwRequest);
            pushMetric("TotalNumberOfErrors", totalNumberOfErrors, Aws::CloudWatch::Model::StandardUnit::Count, metricDatum, cusData->Paggregated_dimension, cwRequest);
        }

        cusData->pCanaryLogs->canaryStreamSendLogs(cusData->pCloudwatchLogsObject);
    }

    // Send metrics to CW
    auto outcome = cusData->pCWclient->PutMetricData(cwRequest);
    if (!outcome.IsSuccess())
    {
        std::cout << "Failed to put sample metric data:" <<
            outcome.GetError().GetMessage() << std::endl;
    }
    else
    {
        std::cout << "Successfully put sample metric data" << std::endl;
    }

}

 void pushStartupLatencyMetric(CustomData *data)
{
    double currentTimestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    double startUpLatency = (double)(currentTimestamp - data->producer_start_time / 1000000); // [milliseconds]
    Aws::CloudWatch::Model::MetricDatum startupLatency_datum;
    Aws::CloudWatch::Model::PutMetricDataRequest cwRequest;
    cwRequest.SetNamespace("KinesisVideoSDKCanaryCPP");

    pushMetric("StartupLatency", startUpLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, startupLatency_datum, data->Pdimension_per_stream, cwRequest);
    if (data->pCanaryConfig->useAggMetrics)
    {
        pushMetric("StartupLatency", startUpLatency, Aws::CloudWatch::Model::StandardUnit::Milliseconds, startupLatency_datum, data->Paggregated_dimension, cwRequest);
    }

    auto outcome = data->pCWclient->PutMetricData(cwRequest);
    if (!outcome.IsSuccess())
    {
        std::cout << "Failed to put StartupLatency metric data:" <<
            outcome.GetError().GetMessage() << std::endl;
    }
    else
    {
        std::cout << "Successfully put StartupLatency metric data" << std::endl;
    }
}

bool put_frame(CustomData *cusData, void *data, size_t len, const nanoseconds &pts, const nanoseconds &dts, FRAME_FLAGS flags)
{
    Frame frame;
    create_kinesis_video_frame(&frame, pts, dts, flags, data, len);
    createCanaryFrameData(&frame);
    bool ret = cusData->kinesis_video_stream->putFrame(frame);

    // push stream metrics on key frames
    if (CHECK_FRAME_FLAG_KEY_FRAME(flags))
    {
        pushKeyFrameMetrics(frame, cusData);
    }

    return ret;
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
            
    int currTime;
    if (!isDroppable) {

        delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

        FRAME_FLAGS kinesis_video_flags = delta ? FRAME_FLAG_NONE : FRAME_FLAG_KEY_FRAME;

        // For some rtsp sources the dts is invalid, therefore synthesize.
        if (!GST_BUFFER_DTS_IS_VALID(buffer)) {
            data->synthetic_dts += DEFAULT_FRAME_DURATION_MS * HUNDREDS_OF_NANOS_IN_A_MILLISECOND * DEFAULT_TIME_UNIT_IN_NANOS;
            buffer->dts = data->synthetic_dts;
        } else if (GST_BUFFER_DTS_IS_VALID(buffer)) {
            data->synthetic_dts = buffer->dts;
        }

        if (data->use_absolute_fragment_times) {
            if (data->first_pts == GST_CLOCK_TIME_NONE) {
                data->producer_start_time = chrono::duration_cast<nanoseconds>(systemCurrentTime().time_since_epoch()).count();
                data->first_pts = buffer->pts;
            }
            buffer->pts += (data->producer_start_time - data->first_pts);
        }

        if (!gst_buffer_map(buffer, &info, GST_MAP_READ)){
            goto CleanUp;
        }
        if (CHECK_FRAME_FLAG_KEY_FRAME(kinesis_video_flags)) {
            data->kinesis_video_stream->putEventMetadata(STREAM_EVENT_TYPE_NOTIFICATION | STREAM_EVENT_TYPE_IMAGE_GENERATION, NULL);
        }

        bool putFrameSuccess = put_frame(data, info.data, info.size, std::chrono::nanoseconds(buffer->pts),
                               std::chrono::nanoseconds(buffer->dts), kinesis_video_flags);

        // If on first frame of stream, push startup latency metric to CW
        if(data->onFirstFrame && putFrameSuccess)
        {
            pushStartupLatencyMetric(data);
            data->onFirstFrame = false;
        }
    }
    
    // Check if we have reached Canary's stop time
    currTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    if (currTime > (data->producer_start_time / 1000000000 + data->pCanaryConfig->canaryDuration))
    {
        g_main_loop_quit(data->main_loop);
    }

    // If intermittent run, check if Canary should be paused
    if(data->pCanaryConfig->canaryRunScenario == "Intermittent" && duration_cast<minutes>(system_clock::now().time_since_epoch()).count() > data->runTill)
    {
        data->timeOfNextKeyFrame->clear();
        int sleepTime = ((rand() % 10) + 1); // [minutes]
        cout << "Intermittent sleep time is set to: " << sleepTime << " minutes" << endl;
        data->sleepTimeStamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); // [milliseconds]
        sleep(sleepTime * 60); // [seconds]
        data->onFirstFrame = true;
        int runTime = (rand() % 10) + 1; // [minutes]
        cout << "Intermittent run time is set to: " << runTime << " minutes" << endl;
        // Set runTill to a new random value 1-10 minutes into the future
        data->runTill = duration_cast<minutes>(system_clock::now().time_since_epoch()).count() + runTime; // [minutes]
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


// This function is called when an error message is posted on the bus
static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    // Print error details on the screen
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
    unique_ptr<StreamCallbackProvider> stream_callback_provider(new SampleStreamCallbackProvider(reinterpret_cast<UINT64>(data)));

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

    unique_ptr<CanaryCallbackProvider> canary_callbacks(new CanaryCallbackProvider(move(client_callback_provider),
            move(stream_callback_provider),
            move(credential_provider),
            DEFAULT_AWS_REGION,
            data->pCanaryConfig->cpUrl,
            DEFAULT_USER_AGENT_NAME,
            device_info_provider->getCustomUserAgent(),
            EMPTY_STRING,
            false,
            DEFAULT_ENDPOINT_CACHE_UPDATE_PERIOD));

    data->kinesis_video_producer = KinesisVideoProducer::createSync(move(device_info_provider),
                                                                    move(canary_callbacks));

    LOG_DEBUG("Client is ready");
}

void kinesis_video_stream_init(CustomData *data) {
    // create a test stream
    map<string, string> tags;
    char tag_name[MAX_TAG_NAME_LEN];
    char tag_val[MAX_TAG_VALUE_LEN];
    SPRINTF(tag_name, "piTag");
    SPRINTF(tag_val, "piValue");

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
        0,
        data->pCanaryConfig->testVideoFps,
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


    LOG_DEBUG("Stream is ready");
}

// callback when each RTSP stream has been created
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

int gstreamer_test_source_init(CustomData *data, GstElement *pipeline) {
    
    GstElement *appsink, *source, *video_src_filter, *h264parse, *video_filter, *h264enc, *autovidcon;

    GstCaps *caps;

    // define the elements
    source = gst_element_factory_make("videotestsrc", "source");
    autovidcon = gst_element_factory_make("autovideoconvert", "vidconv");
    h264enc = gst_element_factory_make("x264enc", "h264enc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    appsink = gst_element_factory_make("appsink", "appsink");

    // to change output video pattern to a moving ball, uncomment below
    //g_object_set(source, "pattern", 18, NULL);

    // videotestsrc must be set to "live" in order for pts and dts to be incremented
    g_object_set(source, "is-live", TRUE, NULL);

    // configure appsink
    g_object_set(G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), data);
    
    // define the elements
    // h264enc = gst_element_factory_make("vtenc_h264_hw", "h264enc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");

    // define and configure video filter, we only want the specified format to pass to the sink
    // ("caps" is short for "capabilities")
    string video_caps_string = "video/x-h264, stream-format=(string) avc, alignment=(string) au";
    video_filter = gst_element_factory_make("capsfilter", "video_filter");
    caps = gst_caps_from_string(video_caps_string.c_str());
    g_object_set(G_OBJECT (video_filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    video_caps_string = "video/x-raw, framerate=" + to_string(data->pCanaryConfig->testVideoFps) + "/1" + ", width=1440, height=1080";
    video_src_filter = gst_element_factory_make("capsfilter", "video_source_filter");
    caps = gst_caps_from_string(video_caps_string.c_str());
    g_object_set(G_OBJECT (video_src_filter), "caps", caps, NULL);
    gst_caps_unref(caps);
    
    // check if all elements were created
    if (!pipeline || !source || !video_src_filter || !appsink || !autovidcon || !h264parse || 
        !video_filter || !h264enc)
    {
        g_printerr("Not all elements could be created.\n");
        return 1;
    }

    // build the pipeline
    gst_bin_add_many(GST_BIN (pipeline), source, video_src_filter, autovidcon, h264enc,
                    h264parse, video_filter, appsink, NULL);

    // check if all elements were linked
    if (!gst_element_link_many(source, video_src_filter, autovidcon, h264enc, 
        h264parse, video_filter, appsink, NULL)) 
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    return 0;
}

int gstreamer_init(int argc, char* argv[], CustomData *data) {

    // init GStreamer
    gst_init(&argc, &argv);

    GstElement *pipeline;
    int ret;
    GstStateChangeReturn gst_ret;

    // Reset first frame pts
    data->first_pts = GST_CLOCK_TIME_NONE;

    switch (data->streamSource) {
        case CustomData::TEST_SOURCE:
            LOG_INFO("Streaming from test source");
            pipeline = gst_pipeline_new("test-kinesis-pipeline");
            ret = gstreamer_test_source_init(data, pipeline);
            break;
    }
    if (ret != 0){
        return ret;
    }

    // Instruct the bus to emit signals for each received message, and connect to the interesting signals
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    gst_object_unref(bus);

    // start streaming
    gst_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (gst_ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return 1;
    }

    data->main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data->main_loop);

    // free resources
    gst_bus_remove_signal_watch(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;
    return 0;
}

int main(int argc, char* argv[]) {
    PropertyConfigurator::doConfigure("../kvs_log_configuration");
    initializeEndianness();
    srand(time(0));
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        CanaryConfig canaryConfig;
        bool useEnvVars = true;
        if (useEnvVars)
        {
            canaryConfig.initConfigWithEnvVars();
        }
        CustomData data;
        data.pCanaryConfig = &canaryConfig;

        const int PUTFRAME_FAILURE_RETRY_COUNT = 3;

        STATUS stream_status = STATUS_SUCCESS;

        CanaryLogs canaryLogs; // TODO: consider renaming to CanaryLogger
        data.pCanaryLogs = &canaryLogs;

        Aws::CloudWatch::CloudWatchClient CWclient(data.client_config);
        data.pCWclient = &CWclient;
        STATUS retStatus = STATUS_SUCCESS;
        Aws::CloudWatchLogs::CloudWatchLogsClient CWLclient(data.client_config);
        CanaryLogs::CloudwatchLogsObject cloudwatchLogsObject;
        cloudwatchLogsObject.logGroupName = "ProducerCppSDK";
        cloudwatchLogsObject.logStreamName = data.pCanaryConfig->streamName +"-log-" + to_string(GETTIME() / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        cloudwatchLogsObject.pCwl = &CWLclient;
        if ((retStatus = canaryLogs.initializeCloudwatchLogger(&cloudwatchLogsObject)) != STATUS_SUCCESS) {
            cout << "Cloudwatch logger failed to be initialized with 0x" << retStatus << ">> error code. Fallback to file logging" << endl;
        }
        else
        {
            cout << "Cloudwatch logger initialization success" << endl;;
        }
        data.pCloudwatchLogsObject = &cloudwatchLogsObject;

        data.stream_name = const_cast<char*>(data.pCanaryConfig->streamName.c_str());

        // set the video stream source
        if (data.pCanaryConfig->sourceType == "TEST_SOURCE")
        {
            data.streamSource = CustomData::TEST_SOURCE;     
        }

        Aws::CloudWatch::Model::Dimension dimension_per_stream;
        dimension_per_stream.SetName("ProducerSDKCanaryStreamNameCPP");
        dimension_per_stream.SetValue(data.stream_name);
        data.Pdimension_per_stream = &dimension_per_stream;

        Aws::CloudWatch::Model::Dimension aggregated_dimension;
        aggregated_dimension.SetName("ProducerSDKCanaryType");
        aggregated_dimension.SetValue(canaryConfig.canaryLabel);
        data.Paggregated_dimension = &aggregated_dimension;


        // init Kinesis Video
        try{
            kinesis_video_init(&data);
            kinesis_video_stream_init(&data);
        } catch (runtime_error &err) {
            LOG_ERROR("Failed to initialize kinesis video with an exception: " << err.what());
            return 1;
        }

        bool do_retry = true;

        if (data.streamSource == CustomData::TEST_SOURCE)
        {
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
        delete (data.timeOfNextKeyFrame);
        canaryLogs.canaryStreamSendLogSync(&cloudwatchLogsObject);
        cout << "end of canary" << endl;
    }

    return 0;
}