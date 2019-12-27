#include "Include_i.h"

getTime globalGetTime = defaultGetTime;
getMonotonicTime globalGetMonotonicTime = defaultGetMonotonicTime;

STATUS generateTimestampStr(UINT64 timestamp, PCHAR formatStr, PCHAR pDestBuffer, UINT32 destBufferLen,
                            PUINT32 pFormattedStrLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    time_t timestampSeconds;
    UINT32 formattedStrLen;
    CHK(pDestBuffer != NULL, STATUS_NULL_ARG);
    CHK(STRNLEN(formatStr, MAX_TIMESTAMP_FORMAT_STR_LEN + 1) <= MAX_TIMESTAMP_FORMAT_STR_LEN,
        STATUS_MAX_TIMESTAMP_FORMAT_STR_LEN_EXCEEDED);

    timestampSeconds = timestamp / HUNDREDS_OF_NANOS_IN_A_SECOND;
    formattedStrLen = 0;
    *pFormattedStrLen = 0;

    formattedStrLen = (UINT32) STRFTIME(pDestBuffer, destBufferLen, formatStr, GMTIME(&timestampSeconds));
    CHK(formattedStrLen != 0, STATUS_STRFTIME_FALIED);

    pDestBuffer[formattedStrLen] = '\0';
    *pFormattedStrLen = formattedStrLen;

CleanUp:

    return retStatus;
}

UINT64 defaultGetMonotonicTime()
{
#if defined __linux__ || defined __MACH__ || defined __CYGWIN__
    struct timespec nowTime;
    clock_gettime(CLOCK_MONOTONIC, &nowTime);

    // The precision needs to be on a 100th nanosecond resolution
    return (UINT64)nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64)nowTime.tv_nsec / DEFAULT_TIME_UNIT_IN_NANOS;
#elif defined _WIN32 || defined _WIN64
    #define POW10_9                 1000000000

    LARGE_INTEGER pf, pc;
    QueryPerformanceFrequency(&pf);
    QueryPerformanceCounter(&pc);

    int tv_sec = pc.QuadPart / pf.QuadPart;
    int tv_nsec = (int) (((pc.QuadPart % pf.QuadPart) * POW10_9 + (pf.QuadPart >> 1)) / pf.QuadPart);
    if (tv_nsec >= POW10_9) {
        tv_sec++;
        tv_nsec -= POW10_9;
    }
    return ((UINT64)tv_sec)*1000*1000*1000 + tv_nsec;
#else
    return defaultGetTime();
#endif
}

UINT64 defaultGetTime()
{
#if defined _WIN32 || defined _WIN64
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    return ((((UINT64)fileTime.dwHighDateTime << 32) | fileTime.dwLowDateTime) - TIME_DIFF_UNIX_WINDOWS_TIME);
#elif defined __MACH__ || defined __CYGWIN__
    struct timeval nowTime;
    if (0 != gettimeofday(&nowTime, NULL)) {
        return 0;
    }

    return (UINT64)nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64)nowTime.tv_usec * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
#else
    struct timespec nowTime;
    clock_gettime(CLOCK_REALTIME, &nowTime);

    // The precision needs to be on a 100th nanosecond resolution
    return (UINT64)nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64)nowTime.tv_nsec / DEFAULT_TIME_UNIT_IN_NANOS;
#endif
}
