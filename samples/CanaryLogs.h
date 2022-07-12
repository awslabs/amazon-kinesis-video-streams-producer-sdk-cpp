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

using namespace std;
using namespace std::chrono;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;


class CanaryLogs
{
    public:
    
    class CloudwatchLogsObject {
        public:
        Aws::CloudWatchLogs::CloudWatchLogsClient* pCwl;
        Aws::CloudWatchLogs::Model::CreateLogGroupRequest canaryLogGroupRequest;
        Aws::CloudWatchLogs::Model::CreateLogStreamRequest canaryLogStreamRequest;
        Aws::CloudWatchLogs::Model::PutLogEventsRequest canaryPutLogEventRequest;
        Aws::CloudWatchLogs::Model::PutLogEventsResult canaryPutLogEventresult;
        Aws::Vector<Aws::CloudWatchLogs::Model::InputLogEvent> canaryInputLogEventVec;
        Aws::String token;
        string logGroupName;
        string logStreamName;
        std::recursive_mutex mutex;
        CloudwatchLogsObject();
    };
    typedef class CloudwatchLogsObject* PCloudwatchLogsObject;

    // STATUS initializeCloudwatchLogger(PCloudwatchLogsObject pCloudwatchLogsObject);

    CanaryLogs();

    STATUS initializeCloudwatchLogger(PCloudwatchLogsObject pCloudwatchLogsObject);

    static VOID setUpLogEventVector(PCHAR logString);

    static VOID onPutLogEventResponseReceivedHandler(const Aws::CloudWatchLogs::CloudWatchLogsClient* cwClientLog,
                                            const Aws::CloudWatchLogs::Model::PutLogEventsRequest& request,
                                            const Aws::CloudWatchLogs::Model::PutLogEventsOutcome& outcome,
                                            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

    VOID canaryStreamSendLogs(PCloudwatchLogsObject pCloudwatchLogsObject);

    VOID canaryStreamSendLogSync(PCloudwatchLogsObject pCloudwatchLogsObject);

    VOID cloudWatchLogger(UINT32 level, PCHAR tag, PCHAR fmt, ...);
};


