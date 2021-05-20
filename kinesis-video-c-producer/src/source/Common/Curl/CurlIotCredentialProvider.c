/**
 * Kinesis Video Producer IoT based Credential Provider for libCurl
 */
#define LOG_CLASS "CurlIotCredentialProvider"
#include "../Include_i.h"

STATUS createCurlIotCredentialProvider(PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath, PCHAR caCertPath, PCHAR roleAlias,
                                       PCHAR thingName, PAwsCredentialProvider* ppCredentialProvider)
{
    return createCurlIotCredentialProviderWithTime(iotGetCredentialEndpoint, certPath, privateKeyPath, caCertPath, roleAlias, thingName,
                                                   commonDefaultGetCurrentTimeFunc, 0, ppCredentialProvider);
}

STATUS createCurlIotCredentialProviderWithTime(PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath, PCHAR caCertPath,
                                               PCHAR roleAlias, PCHAR thingName, GetCurrentTimeFunc getCurrentTimeFn, UINT64 customData,
                                               PAwsCredentialProvider* ppCredentialProvider)
{
    return createIotCredentialProviderWithTime(iotGetCredentialEndpoint, certPath, privateKeyPath, caCertPath, roleAlias, thingName, getCurrentTimeFn,
                                               customData, blockingCurlCall, ppCredentialProvider);
}
