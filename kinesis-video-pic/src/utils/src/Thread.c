#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Thread library functions
//
PUBLIC_API TID defaultGetThreadId()
{
    return (TID)GetCurrentThread();
}

PUBLIC_API STATUS defaultGetThreadName(TID thread, PCHAR name, UINT32 len)
{
    UNUSED_PARAM(thread);
    UNUSED_PARAM(name);
    UNUSED_PARAM(len);
    return STATUS_SUCCESS;
}

PUBLIC_API DWORD WINAPI startWrapperRoutine(LPVOID data)
{
    // Get the data
    PWindowsThreadRoutineWrapper pWrapper = (PWindowsThreadRoutineWrapper)data;
    WindowsThreadRoutineWrapper wrapper;
    CHECK(NULL != pWrapper);

    // Struct-copy to store the heap allocated wrapper as the thread routine
    // can be cancelled which would lead to memory leak.
    wrapper = *pWrapper;

    // Free the heap allocated wrapper as we have a local stack copy
    MEMFREE(pWrapper);

    UINT32 retVal = (UINT32) (UINT64) wrapper.storedStartRoutine(wrapper.storedArgs);

    return retVal;
}

PUBLIC_API STATUS defaultCreateThread(PTID pThreadId, startRoutine start, PVOID args)
{
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

PUBLIC_API STATUS defaultJoinThread(TID threadId, PVOID* retVal)
{
    STATUS retStatus = STATUS_SUCCESS;
    UNUSED_PARAM(retVal);

    CHK(WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)threadId, INFINITE), STATUS_JOIN_THREAD_FAILED);
    CloseHandle((HANDLE)threadId);

CleanUp:
    return retStatus;
}

PUBLIC_API VOID defaultThreadSleep(UINT64 time)
{
    Sleep((UINT32) (time / HUNDREDS_OF_NANOS_IN_A_MILLISECOND));
}

PUBLIC_API STATUS defaultCancelThread(TID threadId)
{
    STATUS retStatus = STATUS_SUCCESS;
    CHK(TerminateThread((HANDLE) threadId, 0), STATUS_CANCEL_THREAD_FAILED);

CleanUp:
    return retStatus;
}

#pragma warning(push)
#pragma warning(disable : 4102)
PUBLIC_API STATUS defaultDetachThread(TID threadId)
{
    STATUS retStatus = STATUS_SUCCESS;
    CloseHandle((HANDLE)threadId);

CleanUp:
    return retStatus;
}
#pragma warning(pop)

#else

PUBLIC_API STATUS defaultGetThreadName(TID thread, PCHAR name, UINT32 len)
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

PUBLIC_API TID defaultGetThreadId()
{
    return (TID) pthread_self();
}

PUBLIC_API STATUS defaultCreateThread(PTID pThreadId, startRoutine start, PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    pthread_t threadId;
    INT32 result;
    pthread_attr_t *pAttr = NULL;

    CHK(pThreadId != NULL, STATUS_NULL_ARG);

#ifdef CONSTRAINED_DEVICE
    pthread_attr_t attr;
    pAttr = &attr;
    result = pthread_attr_init(pAttr);
    CHK_ERR(result == 0, STATUS_THREAD_ATTR_INIT_FAILED, "pthread_attr_init failed with %d", result);
    result = pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE_ON_CONSTRAINED_DEVICE);
    CHK_ERR(result == 0, STATUS_THREAD_ATTR_SET_STACK_SIZE_FAILED, "pthread_attr_setstacksize failed with %d", result);
#endif

    result = pthread_create(&threadId, pAttr, start, args);
    switch (result) {
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

    if (pAttr != NULL) {
        result = pthread_attr_destroy(pAttr);
        if (result != 0) {
            DLOGW("pthread_attr_destroy failed with %u", result);
        }
    }

    return retStatus;
}

PUBLIC_API STATUS defaultJoinThread(TID threadId, PVOID* retVal)
{
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

PUBLIC_API VOID defaultThreadSleep(UINT64 time)
{
    usleep(time / HUNDREDS_OF_NANOS_IN_A_MICROSECOND);
}

/**
 * Android doesn't have a definition for pthread_cancel
 */
#ifdef ANDROID_BUILD

PUBLIC_API STATUS defaultCancelThread(TID threadId)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 cancelResult = pthread_kill((pthread_t) threadId, 0);

    switch (cancelResult) {
        case 0:
            // Successful case
            break;
        case ESRCH:
            CHK(FALSE, STATUS_THREAD_DOES_NOT_EXIST);
        default:
            // Generic error
            CHK(FALSE, STATUS_CANCEL_THREAD_FAILED);
    }

CleanUp:
    return retStatus;
}

#else

PUBLIC_API STATUS defaultCancelThread(TID threadId)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 cancelResult = pthread_cancel((pthread_t) threadId);

    switch (cancelResult) {
        case 0:
            // Successful case
            break;
        case ESRCH:
            CHK(FALSE, STATUS_THREAD_DOES_NOT_EXIST);
        default:
            // Generic error
            CHK(FALSE, STATUS_CANCEL_THREAD_FAILED);
    }

CleanUp:
    return retStatus;
}

#endif

PUBLIC_API STATUS defaultDetachThread(TID threadId)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 detachResult = pthread_detach((pthread_t) threadId);

    switch (detachResult) {
        case 0:
            // Successful case
            break;
        case ESRCH:
            CHK(FALSE, STATUS_THREAD_DOES_NOT_EXIST);
        case EINVAL:
            CHK(FALSE, STATUS_THREAD_IS_NOT_JOINABLE);
        default:
            // Generic error
            CHK(FALSE, STATUS_DETACH_THREAD_FAILED);
    }

CleanUp:
    return retStatus;
}

#endif

PUBLIC_API VOID defaultThreadSleepUntil(UINT64 time)
{
    UINT64 curTime = GETTIME();

    if (time > curTime) {
        THREAD_SLEEP(time - curTime);
    }
}

getTId globalGetThreadId = defaultGetThreadId;
getTName globalGetThreadName = defaultGetThreadName;
createThread globalCreateThread = defaultCreateThread;
threadSleep globalThreadSleep = defaultThreadSleep;
threadSleepUntil globalThreadSleepUntil = defaultThreadSleepUntil;
joinThread globalJoinThread = defaultJoinThread;
cancelThread globalCancelThread = defaultCancelThread;
detachThread globalDetachThread = defaultDetachThread;