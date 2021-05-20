/**
 * Kinesis Video Producer IoT based Credential Provider for libWebSockets
 */
#define LOG_CLASS "LwsIotCredentialProvider"
#include "../Include_i.h"

STATUS createLwsIotCredentialProvider(PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath, PCHAR caCertPath, PCHAR roleAlias,
                                      PCHAR thingName, PAwsCredentialProvider* ppCredentialProvider)
{
    return createLwsIotCredentialProviderWithTime(iotGetCredentialEndpoint, certPath, privateKeyPath, caCertPath, roleAlias, thingName,
                                                  commonDefaultGetCurrentTimeFunc, 0, ppCredentialProvider);
}

STATUS createLwsIotCredentialProviderWithTime(PCHAR iotGetCredentialEndpoint, PCHAR certPath, PCHAR privateKeyPath, PCHAR caCertPath, PCHAR roleAlias,
                                              PCHAR thingName, GetCurrentTimeFunc getCurrentTimeFn, UINT64 customData,
                                              PAwsCredentialProvider* ppCredentialProvider)
{
    return createIotCredentialProviderWithTime(iotGetCredentialEndpoint, certPath, privateKeyPath, caCertPath, roleAlias, thingName, getCurrentTimeFn,
                                               customData, blockingLwsCall, ppCredentialProvider);
}
