#include "CustomData.h"

CustomData::CustomData()
{
    sleepTimeStamp = 0;
    totalPutFrameErrorCount = 0;
    totalErrorAckCount = 0;
    lastKeyFrameTime = 0;
    curKeyFrameTime = 0;
    onFirstFrame = true;
    streamSource = TEST_SOURCE;
    h264_stream_supported = false;
    synthetic_dts = 0;
    stream_status = STATUS_SUCCESS;
    main_loop = NULL;
    first_pts = GST_CLOCK_TIME_NONE;
    use_absolute_fragment_times = true;

    producer_start_time = chrono::duration_cast<nanoseconds>(systemCurrentTime().time_since_epoch()).count(); // [nanoSeconds]
    client_config.region = "us-west-2";
    pCWclient = nullptr;
    Pdimension_per_stream = nullptr;
    Paggregated_dimension = nullptr;
    timeOfNextKeyFrame = new map<uint64_t, uint64_t>();
    timeCounter = producer_start_time / 1000000000; // [seconds]
    // Default first intermittent run to 1 min for testing
    runTill = producer_start_time / 1000000000 / 60 + 1; // [minutes]
    pCanaryConfig = nullptr;
    pCloudwatchLogsObject = nullptr;
    pCanaryLogs = nullptr;
}