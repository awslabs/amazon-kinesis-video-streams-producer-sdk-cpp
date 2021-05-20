/**
 * Kinesis Video Producer Version functionality
 */
#define LOG_CLASS "Version"
#include "Include_i.h"

STATUS getUserAgentString(PCHAR userAgentName, PCHAR customUserAgent, UINT32 len, PCHAR pUserAgent)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pUserAgentName;
    INT32 requiredLen;
    CHAR osVer[MAX_OS_VERSION_STRING_LEN + 1];
    CHAR platformName[MAX_PLATFORM_NAME_STRING_LEN + 1];
    CHAR compilerInfo[MAX_COMPILER_INFO_STRING_LEN + 1];

    CHK(pUserAgent != NULL, STATUS_NULL_ARG);
    CHK(len <= MAX_USER_AGENT_LEN, STATUS_INVALID_USER_AGENT_LENGTH);

    CHK_STATUS(GET_OS_VERSION(osVer, ARRAY_SIZE(osVer)));
    CHK_STATUS(GET_PLATFORM_NAME(platformName, ARRAY_SIZE(platformName)));
    CHK_STATUS(GET_COMPILER_INFO(compilerInfo, ARRAY_SIZE(compilerInfo)));

    // Both the custom agent and the postfix are optional
    if (userAgentName == NULL || userAgentName[0] == '\0') {
        pUserAgentName = USER_AGENT_NAME;
    } else {
        pUserAgentName = userAgentName;
    }

    if (customUserAgent == NULL) {
        requiredLen = SNPRINTF(pUserAgent,
                               len + 1, // account for null terminator
                               (PCHAR) "%s/%s %s %s %s", pUserAgentName, AWS_SDK_KVS_PRODUCER_VERSION_STRING, compilerInfo, osVer, platformName);
    } else {
        requiredLen = SNPRINTF(pUserAgent,
                               len + 1, // account for null terminator
                               (PCHAR) "%s/%s %s %s %s %s", pUserAgentName, AWS_SDK_KVS_PRODUCER_VERSION_STRING, compilerInfo, osVer, platformName,
                               customUserAgent);
    }

    CHK(requiredLen > 0 && (UINT32) requiredLen <= len, STATUS_BUFFER_TOO_SMALL);

CleanUp:

    return retStatus;
}
