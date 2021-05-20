/**
 * Trace profiler internal functionality
 */
#ifndef __TRACE_PROFILER_INCLUDE_I_H__
#define __TRACE_PROFILER_INCLUDE_I_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

/**
 * Including the headers
 */
#include "com/amazonaws/kinesis/video/trace/Include.h"

/**
 * Minimum number of traces we should be able to store
 */
#define MIN_TRACE_NUM 100

/**
 * Max trace name including the NULL terminator
 */
#define MAX_TRACE_NAME 32

/**
 * Max number of chars to represent decimal unsigned 64 bit 0 to 18,446,744,073,709,551,615 plus a null terminator
 */
#define MAX_DECIMAL_UINT64_CHARS 21

/**
 * Aiv formatting delimiter
 */
#define AIV_FORMAT_DELIMITER      ','
#define AIV_FORMAT_LINE_DELIMITER '\n'
#define AIV_TRACE_TYPE_NAME       "trace"

/**
 * Trace object declaration
 */
typedef struct {
    /**
     * Trace creator thread id. This is the thread ID of the initial trace creator and not the consequent reporter.
     */
    TID threadId;

    /**
     * Null terminated thread name.
     */
    CHAR threadName[MAX_THREAD_NAME + 1];

    /**
     * Trace level which will be either included or excluded based upon the perf trace level
     */
    TRACE_LEVEL traceLevel;

    /**
     * Trace name - user specified.
     */
    CHAR traceName[MAX_TRACE_NAME + 1];

    /**
     * Current trace count
     */
    UINT32 traceCount;

    /**
     * Timestamp of the beginning of the trace in epoch
     */
    UINT64 start;

    /**
     * Duration of the trace
     */
    UINT64 duration;
} Trace, *PTrace;

//////////////////////////////////////////////////////////////////////
// Main Profiler object
//////////////////////////////////////////////////////////////////////
/**
 * Trace profiler object.
 *
 * IMPORTANT!!! This is a variable length object.
 */
typedef struct {
    /**
     * Current trace level
     */
    TRACE_LEVEL traceLevel;

    /**
     * Stored behavior flags
     */
    TRACE_PROFILER_BEHAVIOR_FLAGS behaviorFlags;

    /**
     * Next available trace
     */
    PTrace nextTrace;

    /**
     * End of the trace buffer - needed for fast pointer operations
     * instead of calculating the offsets and sizes
     */
    PVOID traceBufferEnd;

    /**
     * The current trace number - wrapping will not affect this
     */
    UINT32 traceCount;

    /**
     * Max number of traces that can be stored
     */
    UINT32 traceBufferLength;

    /**
     * Tracing function pointers
     */
    TraceStartFunc traceStartFn;
    TraceStopFunc traceStopFn;

    /**
     * Tracing function pointers
     */
    MUTEX traceLock;

    /**
     * Trace buffer. It's a ring buffer. IMPORTANT! This will point to the end of the main structure
     */
    PTrace traceBuffer;
} TraceProfiler, *PTraceProfiler;

/**
 * Conversion definitions
 */
#ifndef TRACE_PROFILER_HANDLE_TO_POINTER
#define TRACE_PROFILER_HANDLE_TO_POINTER(h) (IS_VALID_TRACE_PROFILER_HANDLE(h) ? (PTraceProfiler)(h) : NULL)
#endif

#ifndef TRACE_HANDLE_TO_POINTER
#define TRACE_HANDLE_TO_POINTER(h) ((PTrace)(h))
#endif

STATUS getAivFormattedTraceBuffer(PTraceProfiler pTraceProfiler, PCHAR* ppBuffer, PUINT32 pBufferSize, UINT32 traceCount, PTrace pCurTrace);
STATUS traceStartInternalWorker(TRACE_PROFILER_HANDLE traceProfilerHandle, PCHAR traceName, TRACE_LEVEL traceLevel, PTRACE_HANDLE pTraceHandle,
                                TID threadId, PCHAR threadName, UINT64 currentTime);
STATUS traceStopInternalWorker(TRACE_PROFILER_HANDLE traceProfilerHandle, TRACE_HANDLE traceHandle, UINT64 currentTime);
/**
 * We define minimal trace profiler buffer size including auxiliary array of structures
 */
#define MIN_TRACE_PROFILER_BUFFER_SIZE (SIZEOF(TraceProfiler) + MIN_TRACE_NUM * SIZEOF(Trace))

/**
 * Actual trace functions definitions
 */
DEFINE_TRACE_START(traceStartInternal);
DEFINE_TRACE_STOP(traceStopInternal);

/**
 * No-op trace functions definitions
 */
DEFINE_TRACE_START(traceStartNoop);
DEFINE_TRACE_STOP(traceStopNoop);

#ifdef __cplusplus
}
#endif

#endif // __TRACE_PROFILER_INCLUDE_I_H__
