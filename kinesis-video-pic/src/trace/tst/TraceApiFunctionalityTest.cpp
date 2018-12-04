#include "TraceTestFixture.h"

class TraceApiFunctionalityTest : public TraceTestBase {
};

TEST_F(TraceApiFunctionalityTest, OverflowTest_StartStop)
{
    TRACE_PROFILER_HANDLE handle;
    TRACE_HANDLE traceHandle;
    UINT32 traceCount = 200;
    UINT32 index;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(SIZEOF(TraceProfiler) + traceCount * SIZEOF(Trace), TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    //
    // Set the trace levels to INFO
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));

    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle)));
        EXPECT_TRUE(traceHandle != INVALID_TRACE_HANDLE_VALUE);
        // EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, traceHandle)));
    }

    PTraceProfiler pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    //
    // Lower priority trace
    //
    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle)));
        EXPECT_TRUE(traceHandle == INVALID_TRACE_HANDLE_VALUE);
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, traceHandle)));
    }

    // The counts shouldn't change
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}

TEST_F(TraceApiFunctionalityTest, OverflowTest_StartOnly)
{
    TRACE_PROFILER_HANDLE handle;
    TRACE_HANDLE traceHandle;
    UINT32 traceCount = 200;
    UINT32 index;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(SIZEOF(TraceProfiler) + traceCount * SIZEOF(Trace), TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    //
    // Set the trace levels to INFO
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));

    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle)));
        EXPECT_TRUE(traceHandle != INVALID_TRACE_HANDLE_VALUE);
    }

    PTraceProfiler pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    //
    // Lower priority trace
    //
    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle)));
        EXPECT_TRUE(traceHandle == INVALID_TRACE_HANDLE_VALUE);
    }

    // The counts shouldn't change
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}

TEST_F(TraceApiFunctionalityTest, OverflowTest_StartStopNoop)
{
    TRACE_PROFILER_HANDLE handle;
    TRACE_HANDLE traceHandle = INVALID_TRACE_HANDLE_VALUE;
    TRACE_HANDLE overflowHandle = INVALID_TRACE_HANDLE_VALUE;
    UINT32 traceCount = 200;
    UINT32 index;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(SIZEOF(TraceProfiler) + traceCount * SIZEOF(Trace), TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    //
    // Set the trace levels to INFO
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));

    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle)));
        EXPECT_TRUE(traceHandle != INVALID_TRACE_HANDLE_VALUE);
        if (index == 0) {
            // Store the first handle
            overflowHandle = traceHandle;
        } else {
            EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, traceHandle)));
        }
    }

    // Ensure nothing has changed if we stop overflown trace handle
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, overflowHandle)));

    PTraceProfiler pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    //
    // Lower priority trace
    //
    for (index = 0; index < traceCount + 1; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle)));
        EXPECT_TRUE(traceHandle == INVALID_TRACE_HANDLE_VALUE);
        if (index == 0) {
            // Store the first handle
            overflowHandle = traceHandle;
        } else {
            EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, traceHandle)));
        }
    }

    // Ensure nothing has changed if we stop overflown trace handle
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, overflowHandle)));

    // The counts shouldn't change
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + 1);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + 1);

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}

TEST_F(TraceApiFunctionalityTest, GetFormattedBuffer)
{
    TRACE_PROFILER_HANDLE handle;
    TRACE_HANDLE traceHandle;
    UINT32 traceCount = 200;
    UINT32 overflowCount = 50;
    UINT32 index;
    UINT32 bufferSize;
    UINT64 currentTime = 10000000000L;
    PCHAR pBuffer = NULL;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(SIZEOF(TraceProfiler) + traceCount * SIZEOF(Trace), TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    //
    // Set the trace levels to INFO
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));

    for (index = 0; index < traceCount + overflowCount; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStartInternalWorker(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle, 100 + index, (PCHAR) "ThreadName", currentTime)));
        EXPECT_TRUE(traceHandle != INVALID_TRACE_HANDLE_VALUE);
        currentTime += 200000;
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStopInternalWorker(handle, traceHandle, currentTime)));
        currentTime += 100000;

        // Get the buffer somewhere in the middle before the overflow
        if (index == traceCount / 2) {
            EXPECT_TRUE(STATUS_SUCCEEDED(getFormattedTraceBuffer(handle, &pBuffer, &bufferSize)));
            EXPECT_TRUE(pBuffer != NULL && bufferSize != 0);
            EXPECT_TRUE(0 == STRNCMP((PCHAR) "trace,Test trace name,ThreadName,100,1000000,20", pBuffer, 46));
            printf("%s\n", pBuffer);
            EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(pBuffer)));
        }
    }

    PTraceProfiler pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceCount == traceCount + overflowCount);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer + overflowCount);
    EXPECT_TRUE(STATUS_SUCCEEDED(getFormattedTraceBuffer(handle, &pBuffer, &bufferSize)));
    EXPECT_TRUE(pBuffer != NULL && bufferSize != 0);
    printf("%s\n", pBuffer);
    EXPECT_TRUE(0 == STRNCMP((PCHAR) "trace,Test trace name,ThreadName,150,1001500,20", pBuffer, 46));
    EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(pBuffer)));

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));

    // Create again for the noop operations
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(SIZEOF(TraceProfiler) + traceCount * SIZEOF(Trace), TRACE_LEVEL_INFO, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));
    pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceLevel == TRACE_LEVEL_INFO);

    //
    // Lower priority trace
    //
    for (index = 0; index < traceCount + overflowCount; index++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStartInternalWorker(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle, 100 + index, (PCHAR) "ThreadName", currentTime)));
        EXPECT_TRUE(traceHandle == INVALID_TRACE_HANDLE_VALUE);
        currentTime += 200000;
        EXPECT_TRUE(STATUS_SUCCEEDED(traceStopInternalWorker(handle, traceHandle, currentTime)));
        currentTime += 100000;

        // Get the buffer somewhere in the middle before the overflow
        if (index == traceCount / 2) {
            EXPECT_TRUE(STATUS_SUCCEEDED(getFormattedTraceBuffer(handle, &pBuffer, &bufferSize)));
            EXPECT_TRUE(pBuffer == NULL);
            EXPECT_TRUE(bufferSize == 0);
            EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(pBuffer)));
        }
    }

    EXPECT_TRUE(pTraceProfiler->traceCount == 0);
    EXPECT_TRUE(pTraceProfiler->nextTrace == pTraceProfiler->traceBuffer);
    EXPECT_TRUE(STATUS_SUCCEEDED(getFormattedTraceBuffer(handle, &pBuffer, &bufferSize)));
    EXPECT_TRUE(pBuffer == NULL);
    EXPECT_TRUE(bufferSize == 0);
    EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(pBuffer)));

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}