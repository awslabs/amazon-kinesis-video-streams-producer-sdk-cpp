#pragma once

#include <vector>
#include <stdlib.h>
#include <string.h>
#include <mutex>
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
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "CanaryConfig.h"
#include "CanaryLogs.h"

class CustomData
{
public:

    typedef enum _StreamSource {
    TEST_SOURCE,
    FILE_SOURCE,
    LIVE_SOURCE,
    RTSP_SOURCE
    } StreamSource;

    CanaryConfig* pCanaryConfig;

    Aws::Client::ClientConfiguration client_config;
    Aws::CloudWatch::CloudWatchClient* pCWclient;
    Aws::CloudWatch::Model::Dimension* Pdimension_per_stream;
    Aws::CloudWatch::Model::Dimension* Paggregated_dimension;

    CanaryLogs* pCanaryLogs;
    CanaryLogs::CloudwatchLogsObject* pCloudwatchLogsObject;

    int runTill;
    int sleepTimeStamp;
    bool onFirstFrame;

    GMainLoop* main_loop;
    unique_ptr<KinesisVideoProducer> kinesis_video_producer;
    shared_ptr<KinesisVideoStream> kinesis_video_stream;
    bool stream_started;
    bool h264_stream_supported;
    char* stream_name;

    map<uint64_t, uint64_t> *timeOfNextKeyFrame;
    UINT64 lastKeyFrameTime;
    UINT64 curKeyFrameTime;

    double timeCounter;
    double totalPutFrameErrorCount;
    double totalErrorAckCount;

    // stores any error status code reported by StreamErrorCallback.
    atomic_uint stream_status;

    // Used in file uploading only. Assuming frame timestamp are relative. Add producer_start_time to each frame's
    // timestamp to convert them to absolute timestamp. This way fragments dont overlap after token rotation when doing
    // file uploading.
    uint64_t producer_start_time; // [nanoSeconds]
    uint64_t start_time;  // [nanoSeconds]
    volatile StreamSource streamSource;

    string rtsp_url;

    unique_ptr<Credentials> credential;

    uint64_t synthetic_dts;

    bool use_absolute_fragment_times;

    // Pts of first video frame
    uint64_t first_pts;

    CustomData();
};