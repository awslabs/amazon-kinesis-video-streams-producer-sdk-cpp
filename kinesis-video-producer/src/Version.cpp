#include "Version.h"

#define AWS_SDK_KVS_PRODUCER_VERSION_STRING "1.7.8"

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include <sys/utsname.h>
#endif
#include <sstream>
#include <iostream>

using namespace std;

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    const string getProducerSDKVersion() {
        return AWS_SDK_KVS_PRODUCER_VERSION_STRING;
    }

    const string getCompilerVersion() {
#define xstr(s) str(s)
#define str(s) #s
#if defined(_MSC_VER)
        return "MSVC/" xstr(_MSC_VER);
#elif defined(__clang__)
        return "Clang/" xstr(__clang_major__) "."  xstr(__clang_minor__) "." xstr(__clang_patchlevel__);
#elif defined(__GNUC__)
        return "GCC/" xstr(__GNUC__) "."  xstr(__GNUC_MINOR__) "." xstr(__GNUC_PATCHLEVEL__);
#else
      return "UnknownCompiler";
#endif
#undef str
#undef xstr
    }

    const string getOSVersion() {
        stringstream ss;
#if defined _WIN32 || defined _WIN64

        // With the release of Windows 8.1, the behavior of the GetVersionEx API has changed in the value it will
        // return for the operating system version can be overriden by the application manifest.
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724451%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
        //
        // However, we are going to use the API to get the version.
        //

        ss << "Windows/";
        OSVERSIONINFO version_info;
        ZeroMemory(&version_info, sizeof(OSVERSIONINFO));
        version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (!GetVersionEx(&version_info)) {
            ss << "UnknownVersion";
        } else {
            ss << version_info.dwMajorVersion << "." << version_info.dwMinorVersion << "." << version_info.dwBuildNumber;
        }
#else
        utsname name;
        int32_t success = uname(&name);
        if (success >= 0) {
            ss << name.sysname << "/" << name.release;
        } else {
            ss << "non-windows/unknown";
        }
#endif
        return ss.str();
    }


    const string getPlatformName() {
        string platform;
#if defined _WIN32 || defined _WIN64
        SYSTEM_INFO sysInfo;
        ZeroMemory(&sysInfo, sizeof(SYSTEM_INFO));
        GetSystemInfo(&sysInfo);

        switch (sysInfo.wProcessorArchitecture)
        {
            case 0x09:
                platform = "AMD64";
                break;
            case 0x06:
                platform = "IA64";
                break;
            case 0x00:
                platform = "x86";
                break;
            default:
                platform = "unknownArch";
                break;
        }
#else
        utsname name;
        int32_t success = uname(&name);
        if (success >= 0) {
            platform = name.machine;
        } else {
            platform = "unknownArch";
        }
#endif
        return platform;
    }
}
}
}
}
