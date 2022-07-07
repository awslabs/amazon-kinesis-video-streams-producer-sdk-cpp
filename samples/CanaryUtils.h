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

//#include "CanaryCallbackProvider.h"

using namespace std;
using namespace std::chrono;
using namespace com::amazonaws::kinesis::video;
using namespace log4cplus;

std::mutex gLogLock; // Protect access to gCloudwatchLogsObject

typedef struct __CloudwatchLogsObject CloudwatchLogsObject;
struct __CloudwatchLogsObject {
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
};
typedef struct __CloudwatchLogsObject* PCloudwatchLogsObject;


// STATUS initializeCloudwatchLogger(PCloudwatchLogsObject pCloudwatchLogsObject);

PCloudwatchLogsObject gCloudwatchLogsObject = NULL;


VOID setUpLogEventVector(PCHAR logString)
{
    std::lock_guard<std::mutex> lock(gLogLock);
    if(gCloudwatchLogsObject != NULL) {
        std::unique_lock<std::recursive_mutex> lock(gCloudwatchLogsObject->mutex);
        Aws::String awsCwString((Aws::String) logString);
        auto logEvent =
            Aws::CloudWatchLogs::Model::InputLogEvent().WithMessage(awsCwString).WithTimestamp(GETTIME() / HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        gCloudwatchLogsObject->canaryInputLogEventVec.push_back(logEvent);
    }
}

VOID onPutLogEventResponseReceivedHandler(const Aws::CloudWatchLogs::CloudWatchLogsClient* cwClientLog,
                                          const Aws::CloudWatchLogs::Model::PutLogEventsRequest& request,
                                          const Aws::CloudWatchLogs::Model::PutLogEventsOutcome& outcome,
                                          const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    if (!outcome.IsSuccess()) {
        DLOGE("Failed to push logs: %s", outcome.GetError().GetMessage().c_str());
    } else {
        DLOGS("Successfully pushed logs to cloudwatch");
        gCloudwatchLogsObject->token = outcome.GetResult().GetNextSequenceToken();
    }
}

VOID canaryStreamSendLogs(PCloudwatchLogsObject pCloudwatchLogsObject)
{
    std::unique_lock<std::recursive_mutex> lock(pCloudwatchLogsObject->mutex);
    Aws::CloudWatchLogs::Model::PutLogEventsOutcome outcome;
    auto request = Aws::CloudWatchLogs::Model::PutLogEventsRequest()
                       .WithLogGroupName(pCloudwatchLogsObject->logGroupName)
                       .WithLogStreamName(pCloudwatchLogsObject->logStreamName)
                       .WithLogEvents(pCloudwatchLogsObject->canaryInputLogEventVec);
    if (pCloudwatchLogsObject->token != "") {
        request.SetSequenceToken(pCloudwatchLogsObject->token);
    }
    pCloudwatchLogsObject->pCwl->PutLogEventsAsync(request, onPutLogEventResponseReceivedHandler);
    pCloudwatchLogsObject->canaryInputLogEventVec.clear();
}

VOID canaryStreamSendLogSync(PCloudwatchLogsObject pCloudwatchLogsObject)
{
    std::unique_lock<std::recursive_mutex> lock(pCloudwatchLogsObject->mutex);
    auto request = Aws::CloudWatchLogs::Model::PutLogEventsRequest()
                       .WithLogGroupName(pCloudwatchLogsObject->logGroupName)
                       .WithLogStreamName(pCloudwatchLogsObject->logStreamName)
                       .WithLogEvents(pCloudwatchLogsObject->canaryInputLogEventVec);
    if (pCloudwatchLogsObject->token != "") {
        request.SetSequenceToken(pCloudwatchLogsObject->token);
    }
    auto outcome = pCloudwatchLogsObject->pCwl->PutLogEvents(request);
    if (!outcome.IsSuccess()) {
        DLOGE("Failed to push logs: %s", outcome.GetError().GetMessage().c_str());
    } else {
        DLOGS("Successfully pushed logs to cloudwatch");
    }
    pCloudwatchLogsObject->canaryInputLogEventVec.clear();
}

VOID cloudWatchLogger(UINT32 level, PCHAR tag, PCHAR fmt, ...)
{
    CHAR logFmtString[MAX_LOG_FORMAT_LENGTH + 1];
    CHAR cwLogFmtString[MAX_LOG_FORMAT_LENGTH + 1];
    UINT32 logLevel = GET_LOGGER_LOG_LEVEL();
    UNUSED_PARAM(tag);

    if (level >= logLevel) {
        addLogMetadata(logFmtString, (UINT32) ARRAY_SIZE(logFmtString), fmt, level);

        // Creating a copy to store the logFmtString for cloudwatch logging purpose
        va_list valist, valist_cw;
        va_start(valist_cw, fmt);
        vsnprintf(cwLogFmtString, (SIZE_T) SIZEOF(cwLogFmtString), logFmtString, valist_cw);
        va_end(valist_cw);
        va_start(valist, fmt);
        vprintf(logFmtString, valist);
        va_end(valist);
        setUpLogEventVector(cwLogFmtString);
    }
}

