#pragma once

#include <vector>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <iostream>

#include "com/amazonaws/kinesis/video/cproducer/Include.h"

using namespace std;

#define DEFAULT_FRAGMENT_DURATION_MILLISECONDS 2000
#define DEFAULT_CANARY_DURATION_SECONDS (12 * 60 * 60)
#define DEFAULT_BUFFER_DURATION_SECONDS 120

class CanaryConfig{

public: 
    string streamName;
    string sourceType;
    string canaryRunScenario; // continuous or intermitent
    string streamType; // real-time or offline
    string canaryLabel; // typically: longrun or shortrun
    string cpUrl;
    int fragmentSize; // [milliseconds]
    int canaryDuration; // [seconds]
    int bufferDuration; // [seconds]
    int storageSizeInBytes;
    int testVideoFps;
    bool useAggMetrics;

    CanaryConfig();
    void setEnvVarsString(string &configVar, string envVar);
    void setEnvVarsInt(int &configVar, string envVar);
    void setEnvVarsBool(bool &configVar, string envVar);
    void initConfigWithEnvVars();

};