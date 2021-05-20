/**
 * Implementation of TraceProfiler
 */

#define LOG_CLASS "TraceProfiler"
#include "Include_i.h"

//////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////
STATUS profilerInitialize(UINT32 bufferSize, TRACE_LEVEL traceLevel, TRACE_PROFILER_BEHAVIOR_FLAGS behaviorFlags,
                          PTRACE_PROFILER_HANDLE pTraceProfilerHandle)
{
    // No locking is required
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    PTraceProfiler pTraceProfiler = NULL;

    CHK(pTraceProfilerHandle != NULL, STATUS_NULL_ARG);
    CHK(bufferSize >= MIN_TRACE_PROFILER_BUFFER_SIZE, STATUS_MIN_PROFILER_BUFFER);

    // Create the possible format mask and validate against it.
    // IMPORTANT!!! We will fall-back to FLAGS_USE_AIV_TRACE_PROFILER_FORMAT if none is specified

    // Check that only single formatting is specified or none by checking whether the formats is a power of two
    CHK(CHECK_POWER_2(behaviorFlags & PROFILER_FORMAT_MASK), STATUS_INVALID_ARG);

    DLOGS("Initializing native trace profiler with buffer size %u, trace level %u flags 0x%08x", bufferSize, traceLevel, behaviorFlags);

    // Calculate the overall size
    pTraceProfiler = (PTraceProfiler) MEMCALLOC(1, bufferSize);
    CHK(pTraceProfiler != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the values and prepare the object
    pTraceProfiler->behaviorFlags = behaviorFlags;

    // Set the end pointer
    pTraceProfiler->traceBufferEnd = (PBYTE) pTraceProfiler + bufferSize;

    // Trace count reset
    pTraceProfiler->traceCount = 0;

    // Create and initialize a reentrant mutex for interlocking
    pTraceProfiler->traceLock = MUTEX_CREATE(TRUE);

    // Set the max length of the trace buffer
    pTraceProfiler->traceBufferLength = (bufferSize - SIZEOF(TraceProfiler)) / SIZEOF(Trace);

    // Set the buffer pointer immediately after the struct
    pTraceProfiler->traceBuffer = (PTrace)(pTraceProfiler + 1);
    // Set the next available trace at the start of the buffer
    pTraceProfiler->nextTrace = pTraceProfiler->traceBuffer;

    // Set the profiler trace level
    setProfilerLevel(POINTER_TO_HANDLE(pTraceProfiler), traceLevel);

    DLOGS("Trace profiler is initialized OK.");

    // Set the return pointer
    *pTraceProfilerHandle = POINTER_TO_HANDLE(pTraceProfiler);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS profilerRelease(TRACE_PROFILER_HANDLE traceProfilerHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;

    // The call is idempotent
    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_SUCCESS);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    DLOGS("Releasing trace profiler");

    // Free the lock
    MUTEX_FREE(pTraceProfiler->traceLock);

    // Free the allocated memory
    MEMFREE(pTraceProfiler);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS setProfilerLevel(TRACE_PROFILER_HANDLE traceProfilerHandle, TRACE_LEVEL traceLevel)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;

    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    // Requires locking
    MUTEX_LOCK(pTraceProfiler->traceLock);

    // Set the level first
    pTraceProfiler->traceLevel = traceLevel;

    // If the profiling is disabled then set the noop functions for faster processing
    if (pTraceProfiler->traceLevel == TRACE_LEVEL_DISABLED) {
        pTraceProfiler->traceStartFn = traceStartNoop;
        pTraceProfiler->traceStopFn = traceStopNoop;
    } else {
        pTraceProfiler->traceStartFn = traceStartInternal;
        pTraceProfiler->traceStopFn = traceStopInternal;
    }

    // Unlock before exiting
    MUTEX_UNLOCK(pTraceProfiler->traceLock);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getFormattedTraceBuffer(TRACE_PROFILER_HANDLE traceProfilerHandle, PCHAR* ppBuffer, PUINT32 pBufferSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;
    UINT32 format, numberOfTraces;
    BOOL locked = FALSE;
    PTrace pCurTrace = NULL;

    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    // Requires locking
    MUTEX_LOCK(pTraceProfiler->traceLock);
    locked = TRUE;

    CHK(ppBuffer != NULL, STATUS_NULL_ARG);

    // Set the return values first of all
    *ppBuffer = NULL;
    if (pBufferSize != NULL) {
        *pBufferSize = 0;
    }

    // Calculate the size. Need to know the number of traces - whether we had wrapped around the buffer.
    if (pTraceProfiler->traceCount < pTraceProfiler->traceBufferLength) {
        numberOfTraces = pTraceProfiler->traceCount;
        pCurTrace = pTraceProfiler->traceBuffer;
    } else {
        numberOfTraces = pTraceProfiler->traceBufferLength;
        // Check the edge case
        if ((PBYTE) pTraceProfiler->nextTrace + SIZEOF(Trace) > (PBYTE) pTraceProfiler->traceBufferEnd) {
            pCurTrace = pTraceProfiler->traceBuffer;
        } else {
            pCurTrace = pTraceProfiler->nextTrace;
        }
    }

    // The calculated size will be slightly larger as we are taking max of the thread name and trace names - fixed size.
    // IMPORTANT!!! The overall buffer size is format specific!!
    // IMPORTANT!!! We will default to AIV format if none had been specified
    format = pTraceProfiler->behaviorFlags & PROFILER_FORMAT_MASK;
    switch (format) {
        case PROFILER_FLAGS_NONE:
            // Deliberate fall-through to the AIV format
        case FLAGS_USE_AIV_TRACE_PROFILER_FORMAT:

            CHK_STATUS(getAivFormattedTraceBuffer(pTraceProfiler, ppBuffer, pBufferSize, numberOfTraces, pCurTrace));

            break;
        default:
            break;
    }

CleanUp:

    if (locked) {
        // If locked then trace profiler object is not NULL so no need to validate
        MUTEX_UNLOCK(pTraceProfiler->traceLock);
    }

    LEAVES();
    return retStatus;
}

STATUS freeTraceBuffer(PCHAR pBuffer)
{
    // No locking is required
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Idempotent call
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Free the buffer
    MEMFREE(pBuffer);

CleanUp:
    LEAVES();
    return retStatus;
}

DEFINE_TRACE_START(traceStart)
{
    // Locking is handled in the internal function
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;

    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    // Call the Internal API
    CHK_STATUS(pTraceProfiler->traceStartFn(traceProfilerHandle, traceName, traceLevel, pTraceHandle));

CleanUp:
    return retStatus;
}

DEFINE_TRACE_STOP(traceStop)
{
    // Locking is handled in the internal function
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;

    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    // Call the Internal API
    CHK_STATUS(pTraceProfiler->traceStopFn(traceProfilerHandle, traceHandle));

CleanUp:
    return retStatus;
}

//////////////////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////////////////

DEFINE_TRACE_START(traceStartInternal)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 currentTime = GETTIME();
    TID threadId = GETTID();
    CHAR threadName[MAX_THREAD_NAME];
    CHK_STATUS(GETTNAME(threadId, threadName, MAX_THREAD_NAME));

    // Null terminate just in case
    threadName[MAX_THREAD_NAME - 1] = '\0';

    // The actual processing is handled by the worker for unit test access
    CHK_STATUS(traceStartInternalWorker(traceProfilerHandle, traceName, traceLevel, pTraceHandle, threadId, threadName, currentTime));

CleanUp:
    return retStatus;
}

DEFINE_TRACE_STOP(traceStopInternal)
{
    STATUS retStatus = STATUS_SUCCESS;

    // The actual processing is handled by the worker for unit test access
    CHK_STATUS(traceStopInternalWorker(traceProfilerHandle, traceHandle, GETTIME()));

CleanUp:
    return retStatus;
}

STATUS traceStartInternalWorker(TRACE_PROFILER_HANDLE traceProfilerHandle, PCHAR traceName, TRACE_LEVEL traceLevel, PTRACE_HANDLE pTraceHandle,
                                TID threadId, PCHAR threadName, UINT64 currentTime)
{
    // Locking is handled by the caller
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;
    PTrace pTrace = NULL;
    BOOL locked = FALSE;

    CHK(traceName != NULL && pTraceHandle != NULL, STATUS_NULL_ARG);
    CHK(traceName[0] != '\0', STATUS_INVALID_ARG);

    // Validate the profiler
    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    MUTEX_LOCK(pTraceProfiler->traceLock);
    locked = TRUE;

    // See if we need to do anything due to the trace level - Early return with success
    *pTraceHandle = INVALID_TRACE_HANDLE_VALUE;
    CHK(traceLevel <= pTraceProfiler->traceLevel, STATUS_SUCCESS);

    // Get the next available trace
    pTrace = pTraceProfiler->nextTrace;

    // Setup the trace
    pTrace->duration = 0;
    pTrace->traceCount = pTraceProfiler->traceCount;
    pTrace->start = currentTime;
    pTrace->threadId = threadId;
    pTrace->traceLevel = traceLevel;
    STRNCPY(pTrace->threadName, threadName, MAX_THREAD_NAME);
    STRNCPY(pTrace->traceName, traceName, MAX_TRACE_NAME);

    // Null terminate just in case
    pTrace->traceName[MAX_TRACE_NAME] = '\0';
    pTrace->threadName[MAX_THREAD_NAME] = '\0';

    // Increment the next trace pointer and the counter
    // IMPORTANT FAILSAFE!!! From this point on nothing should fail as we had already 'allocated' the trace
    pTraceProfiler->nextTrace++;
    pTraceProfiler->traceCount++;

    // See if we need to wrap around
    if ((PBYTE) pTraceProfiler->nextTrace + SIZEOF(Trace) > (PBYTE) pTraceProfiler->traceBufferEnd) {
        // Wrap the buffer
        pTraceProfiler->nextTrace = pTraceProfiler->traceBuffer;
    }

    // Set the return value
    *pTraceHandle = POINTER_TO_HANDLE(pTrace);

CleanUp:

    if (locked) {
        MUTEX_UNLOCK(pTraceProfiler->traceLock);
    }

    return retStatus;
}

STATUS traceStopInternalWorker(TRACE_PROFILER_HANDLE traceProfilerHandle, TRACE_HANDLE traceHandle, UINT64 currentTime)
{
    // Locking is handled by the caller
    STATUS retStatus = STATUS_SUCCESS;
    PTraceProfiler pTraceProfiler = NULL;
    PTrace pTrace = NULL;
    BOOL locked = FALSE;

    // Quick check for the no-op trace. Return success
    CHK(IS_VALID_TRACE_HANDLE(traceHandle), STATUS_SUCCESS);

    // Validate the profiler
    CHK(IS_VALID_TRACE_PROFILER_HANDLE(traceProfilerHandle), STATUS_INVALID_ARG);
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(traceProfilerHandle);

    MUTEX_LOCK(pTraceProfiler->traceLock);
    locked = TRUE;

    pTrace = TRACE_HANDLE_TO_POINTER(traceHandle);

    CHK(pTraceProfiler->traceCount >= pTrace->traceCount, STATUS_INTERNAL_ERROR);

    // Check if we have wrapped around already in which case we can't do anything about it
    CHK(pTraceProfiler->traceCount - pTrace->traceCount < pTraceProfiler->traceBufferLength, STATUS_SUCCESS);

    // Set the duration
    pTrace->duration = currentTime - pTrace->start;

CleanUp:

    if (locked) {
        MUTEX_UNLOCK(pTraceProfiler->traceLock);
    }

    return retStatus;
}

//////////////////////////////////////////////////////////////////////
// Internal no-op functions. These are needed for faster/no-footprint operations
// in cas the traces are disabled.
//////////////////////////////////////////////////////////////////////

DEFINE_TRACE_START(traceStartNoop)
{
    UNUSED_PARAM(traceProfilerHandle);
    UNUSED_PARAM(traceName);
    UNUSED_PARAM(traceLevel);

    STATUS retStatus = STATUS_SUCCESS;
    CHK(pTraceHandle != NULL, STATUS_NULL_ARG);

    // Return an invalid trace handle
    *pTraceHandle = INVALID_TRACE_HANDLE_VALUE;

CleanUp:
    return retStatus;
}

DEFINE_TRACE_STOP(traceStopNoop)
{
    UNUSED_PARAM(traceProfilerHandle);
    UNUSED_PARAM(traceHandle);

    return STATUS_SUCCESS;
}

/*
 * Formatting for AIV trace profiler
 */
STATUS getAivFormattedTraceBuffer(PTraceProfiler pTraceProfiler, PCHAR* ppBuffer, PUINT32 pBufferSize, UINT32 traceCount, PTrace pCurTrace)
{
    // Locking has been handled in the public caller
    STATUS retStatus = STATUS_SUCCESS;

    UINT32 i;
    UINT32 allocationSize = 0;
    PCHAR pBuffer = NULL;
    PCHAR pCurChar;
    UINT32 size;
    UINT64 time;

    // Check if we need to do anything
    CHK(traceCount != 0, STATUS_SUCCESS);

    // Allocate the buffer taking into account the number of semicolons and new lines
    allocationSize = 6 * SIZEOF(CHAR) +           // trace type and comma = "trace,"
        MAX_TRACE_NAME * SIZEOF(CHAR) +           // Trace name
        MAX_THREAD_NAME * SIZEOF(CHAR) +          // Thread name
        MAX_DECIMAL_UINT64_CHARS * SIZEOF(CHAR) + // Thread id
        MAX_DECIMAL_UINT64_CHARS * SIZEOF(CHAR) + // Start time
        MAX_DECIMAL_UINT64_CHARS * SIZEOF(CHAR) + // Duration
        SIZEOF(CHAR);                             // New line

    pBuffer = (PCHAR) MEMCALLOC(traceCount, allocationSize);
    CHK(pBuffer != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pCurChar = pBuffer;
    for (i = 0; i < traceCount; i++) {
        // Handle the wrapping
        if ((PBYTE) pCurTrace + SIZEOF(Trace) > (PBYTE) pTraceProfiler->traceBufferEnd) {
            pCurTrace = pTraceProfiler->traceBuffer;
        }

        // Add the trace type
        STRCPY(pCurChar, AIV_TRACE_TYPE_NAME);

        // Includes the null terminator
        pCurChar += (SIZEOF(AIV_TRACE_TYPE_NAME) - 1) / SIZEOF(CHAR);

        *pCurChar = AIV_FORMAT_DELIMITER;
        pCurChar++;

        // Add the trace name
        size = MIN((UINT32) STRLEN(pCurTrace->traceName), MAX_TRACE_NAME);
        STRNCPY(pCurChar, pCurTrace->traceName, size);
        pCurChar += size;

        *pCurChar = AIV_FORMAT_DELIMITER;
        pCurChar++;

        // Add the thread name
        size = MIN((UINT32) STRLEN(pCurTrace->threadName), MAX_THREAD_NAME);
        STRNCPY(pCurChar, pCurTrace->threadName, size);
        pCurChar += size;

        *pCurChar = AIV_FORMAT_DELIMITER;
        pCurChar++;

        // Add the thread id
        CHK_STATUS(ULLTOSTR(pCurTrace->threadId, pCurChar, MAX_DECIMAL_UINT64_CHARS, 10, &size));
        pCurChar += size;

        *pCurChar = AIV_FORMAT_DELIMITER;
        pCurChar++;

        // Add the start - the AIV format is in milliseconds
        time = pCurTrace->start / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
        CHK_STATUS(ULLTOSTR(time, pCurChar, MAX_DECIMAL_UINT64_CHARS, 10, &size));
        pCurChar += size;

        *pCurChar = AIV_FORMAT_DELIMITER;
        pCurChar++;

        // Add the duration - the AIV format is in milliseconds
        time = pCurTrace->duration / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
        CHK_STATUS(ULLTOSTR(time, pCurChar, MAX_DECIMAL_UINT64_CHARS, 10, &size));
        pCurChar += size;

        *pCurChar = AIV_FORMAT_LINE_DELIMITER;
        pCurChar++;

        // Iterate the trace
        pCurTrace++;
    }

    // Set the return buffer pointer
    *ppBuffer = pBuffer;

    // Set the actual buffer size
    if (pBufferSize != NULL) {
        *pBufferSize = (UINT32)(pCurChar - pBuffer);
    }

CleanUp:

    return retStatus;
}
