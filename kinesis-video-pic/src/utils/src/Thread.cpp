#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Thread library functions
//
INLINE TID defaultGetThreadId()
{
    return (TID)GetCurrentThread();
}

INLINE STATUS defaultGetThreadName(TID thread, PCHAR name, UINT32 len)
{
    UNUSED_PARAM(thread);
    UNUSED_PARAM(name);
    UNUSED_PARAM(len);
    return STATUS_SUCCESS;
}

DWORD WINAPI startWrapperRoutine(LPVOID data) {
    // Get the data
    PWindowsThreadRoutineWrapper pWrapper = (PWindowsThreadRoutineWrapper)data;
    CHECK(NULL != pWrapper);

    UINT32 retVal = (UINT32) (UINT64) pWrapper->storedStartRoutine(pWrapper->storedArgs);

    // Free the memory on exit
    MEMFREE(pWrapper);

    return retVal;
}

INLINE STATUS defaultCreateThread(PTID pThreadId, startRoutine start, PVOID args) {
    STATUS retStatus = STATUS_SUCCESS;
    HANDLE threadHandle;
    PWindowsThreadRoutineWrapper pWrapper = NULL;
    
    CHK(pThreadId != NULL, STATUS_NULL_ARG);

    // Allocate temporary wrapper and store it
    pWrapper = (PWindowsThreadRoutineWrapper)MEMALLOC(SIZEOF(WindowsThreadRoutineWrapper));
    CHK(pWrapper != NULL, STATUS_NOT_ENOUGH_MEMORY);
    pWrapper->storedArgs = args;
    pWrapper->storedStartRoutine = start;

    threadHandle = CreateThread(NULL, 0, startWrapperRoutine, pWrapper, 0, NULL);
    CHK(threadHandle != NULL, STATUS_CREATE_THREAD_FAILED);

    *pThreadId = (TID)threadHandle;

CleanUp:
    if (STATUS_FAILED(retStatus) && pWrapper != NULL) {
        MEMFREE(pWrapper);
    }

    return retStatus;
}

INLINE STATUS defaultJoinThread(TID threadId, PVOID* retVal) {
    STATUS retStatus = STATUS_SUCCESS;
    UNUSED_PARAM(retVal);

    CHK(WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)threadId, INFINITE), STATUS_JOIN_THREAD_FAILED);
    CloseHandle((HANDLE)threadId);

CleanUp:
    return retStatus;
}

INLINE VOID defaultThreadSleep(UINT64 uSeconds)
{
    Sleep((UINT32) (uSeconds / 1000));
}

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

INLINE STATUS defaultCreateThread(PTID pThreadId, startRoutine start, PVOID args) {
    STATUS retStatus = STATUS_SUCCESS;
    pthread_t threadId;
    INT32 createResult;

    CHK(pThreadId != NULL, STATUS_NULL_ARG);
    
    createResult = pthread_create(&threadId, NULL, start, args);
    switch (createResult) {
    case 0: 
        // Successful case
        break;
    case EAGAIN:
        CHK(FALSE, STATUS_THREAD_NOT_ENOUGH_RESOURCES);
    case EINVAL:
        CHK(FALSE, STATUS_THREAD_INVALID_ARG);
    case EPERM:
        CHK(FALSE, STATUS_THREAD_PERMISSIONS);
    default:
        // Generic error
        CHK(FALSE, STATUS_CREATE_THREAD_FAILED);
    }

    *pThreadId = (TID)threadId;
    
CleanUp:
    return retStatus;
}

INLINE STATUS defaultJoinThread(TID threadId, PVOID* retVal) {
    STATUS retStatus = STATUS_SUCCESS;
    INT32 joinResult = pthread_join((pthread_t) threadId, retVal);

    switch (joinResult) {
    case 0:
        // Successful case
        break;
    case EDEADLK:
        CHK(FALSE, STATUS_THREAD_DEADLOCKED);
    case EINVAL:
        CHK(FALSE, STATUS_THREAD_INVALID_ARG);
    case ESRCH:
        CHK(FALSE, STATUS_THREAD_DOES_NOT_EXIST);
    default:
        // Generic error
        CHK(FALSE, STATUS_JOIN_THREAD_FAILED);
    }

CleanUp:
    return retStatus;
}

INLINE VOID defaultThreadSleep(UINT64 uSeconds)
{
    usleep(uSeconds);
}

#endif

getTId globalGetThreadId = defaultGetThreadId;
getTName globalGetThreadName = defaultGetThreadName;
createThread globalCreateThread = defaultCreateThread;
threadSleep globalThreadSleep = defaultThreadSleep;
joinThread globalJoinThread = defaultJoinThread;
