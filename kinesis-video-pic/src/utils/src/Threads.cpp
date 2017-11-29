#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Stub thread library functions
//
INLINE TID stubGetThreadId()
{
    return (TID) 0;
}

INLINE STATUS stubGetThreadName(TID thread, PCHAR name, UINT32 len)
{
    UNUSED_PARAM(thread);
    UNUSED_PARAM(name);
    UNUSED_PARAM(len);
    return STATUS_SUCCESS;
}

getTId globalGetThreadId = stubGetThreadId;
getTName globalGetThreadName = stubGetThreadName;

#else

INLINE STATUS defaultGetThreadName(TID thread, PCHAR name, UINT32 len)
{
    UINT32 retValue;

    if (NULL == name) {
        return STATUS_NULL_ARG;
    }

    if (len < MAX_THREAD_NAME) {
        return STATUS_INVALID_ARG;
    }

#if defined __APPLE__ && __MACH__ || __GLIBC__ && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 12
    retValue = pthread_getname_np((pthread_t) thread, name, len);
#elif defined ANDROID_BUILD
    // This will return the current thread name on Android
    retValue = prctl(PR_GET_NAME, (UINT64) name, 0, 0, 0);
#else
    // TODO need to handle case when other platform use old verison GLIBC and don't support prctl
    retValue = prctl(PR_GET_NAME, (UINT64) name, 0, 0, 0);
#endif

    return (0 == retValue) ? STATUS_SUCCESS : STATUS_INVALID_OPERATION;
}

INLINE TID defaultGetThreadId()
{
    return (TID) pthread_self();
}

getTId globalGetThreadId = defaultGetThreadId;
getTName globalGetThreadName = defaultGetThreadName;

#endif
