#include "Include_i.h"

//
// Version functions
//
#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

STATUS defaultGetPlatformName(PCHAR pResult, UINT32 len)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 requiredLen;
    SYSTEM_INFO sysInfo;
    PCHAR platform;

    CHK(pResult != NULL, STATUS_NULL_ARG);

    ZeroMemory(&sysInfo, SIZEOF(SYSTEM_INFO));
    GetSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture) {
        case 0x09:
            platform = (PCHAR) "AMD64";
            break;
        case 0x06:
            platform = (PCHAR) "IA64";
            break;
        case 0x00:
            platform = (PCHAR) "x86";
            break;
        default:
            platform = (PCHAR) "unknownArch";
            break;
    }

    requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", platform);

    CHK(requiredLen > 0 && (UINT32) requiredLen < len, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    return retStatus;
}

#pragma warning(push)
#pragma warning(disable : 4996)
STATUS defaultGetOsVersion(PCHAR pResult, UINT32 len)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 requiredLen;
    OSVERSIONINFO versionInfo;

    CHK(pResult != NULL, STATUS_NULL_ARG);

    // With the release of Windows 8.1, the behavior of the GetVersionEx API has changed in the value it will
    // return for the operating system version can be overriden by the application manifest.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724451%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    //
    // However, we are going to use the API to get the version.
    //

    ZeroMemory(&versionInfo, SIZEOF(OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = SIZEOF(OSVERSIONINFO);

    if (!GetVersionEx(&versionInfo)) {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", (PCHAR) "Windows/UnknownVersion");
    } else {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s%u.%u.%u", (PCHAR) "Windows/", versionInfo.dwMajorVersion, versionInfo.dwMinorVersion,
                               versionInfo.dwBuildNumber);
    }

    CHK(requiredLen > 0 && (UINT32) requiredLen < len, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    return retStatus;
}
#pragma warning(pop)

#else

STATUS defaultGetOsVersion(PCHAR pResult, UINT32 len)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 requiredLen;
    struct utsname name;

    CHK(pResult != NULL, STATUS_NULL_ARG);

    if (uname(&name) >= 0) {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s/%s", name.sysname, name.release);
    } else {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", (PCHAR) "non-windows/unknown");
    }

    CHK(requiredLen > 0 && requiredLen < len, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    return retStatus;
}

STATUS defaultGetPlatformName(PCHAR pResult, UINT32 len)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 requiredLen;
    struct utsname name;

    CHK(pResult != NULL, STATUS_NULL_ARG);

    if (uname(&name) >= 0) {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", name.machine);
    } else {
        requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", (PCHAR) "unknownArch");
    }

    CHK(requiredLen > 0 && requiredLen < len, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    return retStatus;
}

#endif

STATUS defaultGetCompilerInfo(PCHAR pResult, UINT32 len)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 requiredLen;

    CHK(pResult != NULL, STATUS_NULL_ARG);

#define __xstr__(s) __str__(s)
#define __str__(s)  #s
#if defined(_MSC_VER)
    requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s/%s", (PCHAR) "MSVC", (PCHAR) __xstr__(_MSC_VER));
#elif defined(__clang__)
    requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s/%s.%s.%s", (PCHAR) "Clang", (PCHAR) __xstr__(__clang_major__), (PCHAR) __xstr__(__clang_minor__),
                           (PCHAR) __xstr__(__clang_patchlevel__));
#elif defined(__GNUC__)
    requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s/%s.%s.%s", (PCHAR) "GCC", (PCHAR) __xstr__(__GNUC__), (PCHAR) __xstr__(__GNUC_MINOR__),
                           (PCHAR) __xstr__(__GNUC_PATCHLEVEL__));
#else
    requiredLen = SNPRINTF(pResult, len, (PCHAR) "%s", (PCHAR) "UnknownCompiler");
#endif
#undef __str__
#undef __xstr__

    CHK(requiredLen > 0 && (UINT32) requiredLen < len, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    return retStatus;
}

getPlatformName globalGetPlatformName = defaultGetPlatformName;
getOsVersion globalGetOsVersion = defaultGetOsVersion;
getCompilerInfo globalGetCompilerInfo = defaultGetCompilerInfo;
