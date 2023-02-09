#include "KvsSinkStreamCallbackProvider.h"
#include <string.h>
#include <chrono>
#include <aws/core/Aws.h>
#include <aws/monitoring/CloudWatchClient.h>
#include <aws/monitoring/model/PutMetricDataRequest.h>
#include <aws/logs/CloudWatchLogsClient.h>

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

STATUS
KvsSinkStreamCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                                            STREAM_HANDLE stream_handle,
                                                            UINT64 time_since_last_buffering_ack) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamConnectionStale callback for stream handle " << stream_handle << ". Time since last buffering ack in 100ns: " << time_since_last_buffering_ack);
    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamErrorReportHandler(UINT64 custom_data,
                                                        STREAM_HANDLE stream_handle,
                                                        UPLOAD_HANDLE upload_handle,
                                                        UINT64 errored_timecode,
                                                        STATUS status_code) {
    LOG_ERROR("Reported stream error. Errored timecode: " << errored_timecode << " Status: 0x" << std::hex << status_code);
    auto customDataObj = reinterpret_cast<KvsSinkCustomData*>(custom_data);

    // ignore if the sdk can recover from the error
    if (!IS_RECOVERABLE_ERROR(status_code)) {
        customDataObj->stream_status = status_code;
    }

    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::droppedFrameReportHandler(UINT64 custom_data,
                                                         STREAM_HANDLE stream_handle,
                                                         UINT64 dropped_frame_timecode) {
    UNUSED_PARAM(custom_data);
    LOG_WARN("Reported droppedFrame callback for stream handle " << stream_handle << ". Dropped frame timecode in 100ns: " << dropped_frame_timecode);
    return STATUS_SUCCESS; // continue streaming
}

STATUS
KvsSinkStreamCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data,
                                                            STREAM_HANDLE stream_handle,
                                                            UINT64 current_buffer_duration) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamLatencyPressure callback for stream handle " << stream_handle << ". Current buffer duration in 100ns: " << current_buffer_duration);
    return STATUS_SUCCESS;
}

STATUS
KvsSinkStreamCallbackProvider::streamClosedHandler(UINT64 custom_data,
                                                   STREAM_HANDLE stream_handle,
                                                   UPLOAD_HANDLE upload_handle) {
    UNUSED_PARAM(custom_data);
    LOG_DEBUG("Reported streamClosed callback for stream handle " << stream_handle << ". Upload handle " << upload_handle);
    return STATUS_SUCCESS;
}

VOID pushMetric(std::string metricName, double metricValue, Aws::CloudWatch::Model::StandardUnit unit, Aws::CloudWatch::Model::MetricDatum datum,
                Aws::CloudWatch::Model::Dimension *dimension, Aws::CloudWatch::Model::PutMetricDataRequest &cwRequest)
{
    datum.SetMetricName(metricName);
    datum.AddDimensions(*dimension);
    datum.SetValue(metricValue);
    datum.SetUnit(unit);

    // Pushes back the data array, can include no more than 20 metrics per call
    cwRequest.AddMetricData(datum);
}

VOID onPutMetricDataResponseReceivedHandler(const Aws::CloudWatch::CloudWatchClient* cwClient,
                                            const Aws::CloudWatch::Model::PutMetricDataRequest& request,
                                            const Aws::CloudWatch::Model::PutMetricDataOutcome& outcome,
                                            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    if (!outcome.IsSuccess()) {
        LOG_ERROR("Failed to put sample metric data: " << outcome.GetError().GetMessage().c_str());
    } else {
        LOG_DEBUG("Successfully put sample metric data");
    }
}

STATUS
KvsSinkStreamCallbackProvider::fragmentAckHandler(UINT64 custom_data,
                                                  STREAM_HANDLE stream_handle,
                                                  UPLOAD_HANDLE upload_handle,
                                                  PFragmentAck pFragmentAck) {
    auto customDataObj = reinterpret_cast<KvsSinkCustomData*>(custom_data);
//    std::string canaryLabel = "DEFAULT_CANARY_LABEL";
//    Aws::CloudWatch::Model::Dimension aggregated_dimension;
//    aggregated_dimension.SetName("ProducerCppCanaryType");
//    aggregated_dimension.SetValue(canaryLabel);
//
//    Aws::CloudWatch::Model::Dimension DimensionPerStream;
//    DimensionPerStream.SetName("ProducerCppCanaryStreamName");
//    DimensionPerStream.SetValue(customDataObj->kvsSink->stream_name);

    if(customDataObj != NULL && customDataObj->kvsSink != NULL && pFragmentAck != NULL && pFragmentAck->ackType == FRAGMENT_ACK_TYPE_PERSISTED) {
        LOG_DEBUG("PersistedAck, timestamp " << pFragmentAck->timestamp);
        Aws::CloudWatch::Model::MetricDatum persistedAckLatencyDatum;
        Aws::CloudWatch::Model::PutMetricDataRequest cwRequest;
        cwRequest.SetNamespace("YourStreamName");

        auto currentTimestamp = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        pushMetric("PersistedAckLatency", customDataObj->kvsSink->max_latency_seconds, Aws::CloudWatch::Model::StandardUnit::Milliseconds,
                   persistedAckLatencyDatum, customDataObj->pDimensionPerStream, cwRequest);
        LOG_DEBUG("Persisted Ack Latency: " << customDataObj->kvsSink->max_latency_seconds);
        pushMetric("PersistedAckLatency", customDataObj->kvsSink->max_latency_seconds, Aws::CloudWatch::Model::StandardUnit::Milliseconds,
            persistedAckLatencyDatum, customDataObj->pAggregatedDimension, cwRequest);
        g_signal_emit(G_OBJECT(customDataObj->kvsSink), customDataObj->ackSignalId, 0, pFragmentAck->timestamp);
    }
    return STATUS_SUCCESS;
}


