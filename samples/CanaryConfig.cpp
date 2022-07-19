#include "CanaryConfig.h"

CanaryConfig::CanaryConfig()
{
    testVideoFps = 25;
    streamName = "DefaultStreamName";
    sourceType = "TEST_SOURCE";
    canaryRunScenario = "Continuous"; // (or intermittent)
    streamType = "REALTIME";
    canaryLabel = "DEFAULT_CANARY_LABEL"; // need to decide on a default value
    cpUrl = "";
    fragmentSize = DEFAULT_FRAGMENT_DURATION_MILLISECONDS;
    canaryDuration = DEFAULT_CANARY_DURATION_SECONDS;
    bufferDuration = DEFAULT_BUFFER_DURATION_SECONDS;
    storageSizeInBytes = 0;
    useAggMetrics = true;
}

void CanaryConfig::setEnvVarsString(string &configVar, string envVar)
{
    if (getenv(envVar.c_str()) != NULL)
    {
        configVar = getenv(envVar.c_str());
    }
}

void CanaryConfig::setEnvVarsInt(int &configVar, string envVar)
{
    if (getenv(envVar.c_str()) != NULL)
    {
        configVar = stoi(getenv(envVar.c_str()));
    }
}

void CanaryConfig::setEnvVarsBool(bool &configVar, string envVar)
{
    if (getenv(envVar.c_str()) != NULL)
    {
        if (getenv(envVar.c_str()) == "TRUE" || getenv(envVar.c_str()) == "true" || getenv(envVar.c_str()) == "True")
        {
            configVar = true;
        } else
        {
            configVar = false;
        }
    }
}

void CanaryConfig::initConfigWithEnvVars()
{
    setEnvVarsString(streamName, "CANARY_STREAM_NAME");
    //setEnvVarsString(sourceType, "CANARY_SOURCE_TYPE");
    setEnvVarsString(canaryRunScenario, "CANARY_RUN_SCENARIO");
    setEnvVarsString(streamType, "CANARY_STREAM_TYPE");
    setEnvVarsString(canaryLabel, "CANARY_LABEL");
    setEnvVarsString(cpUrl, "CANARY_CP_URL");

    setEnvVarsInt(fragmentSize, "CANARY_FRAGMENT_SIZE");
    setEnvVarsInt(canaryDuration, "CANARY_DURATION_IN_SECONDS");
    setEnvVarsInt(bufferDuration, "CANARY_BUFFER_DURATION");
    setEnvVarsInt(storageSizeInBytes, "CANARY_STORAGE_SIZE");
    setEnvVarsInt(testVideoFps, "CANARY_FPS");

    cout << "CANARY_STREAM_NAME: " << streamName << endl;
    cout << "CANARY_RUN_SCENARIO: " << canaryRunScenario << endl;
    cout << "CANARY_STREAM_TYPE: " << streamType << endl;
    cout << "CANARY_LABEL: " << canaryLabel << endl;
    cout << "CANARY_CP_URL: " << cpUrl << endl;
    cout << "CANARY_FRAGMENT_SIZE: " << fragmentSize << endl;
    cout << "CANARY_DURATION: " << canaryDuration << endl;
    cout << "CANARY_STORAGE_SIZE: " << storageSizeInBytes << endl;
    cout << "CANARY_FPS: " << testVideoFps << endl;
}