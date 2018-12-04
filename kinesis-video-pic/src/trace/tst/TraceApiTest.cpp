#include "TraceTestFixture.h"

class TraceApiTest : public TraceTestBase {
};

TEST_F(TraceApiTest, InvalidInputProfilerInitialize_NullPointer) {
    EXPECT_TRUE(STATUS_FAILED(profilerInitialize(1000000, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, NULL)));
}

TEST_F(TraceApiTest, InvalidInputProfilerInitialize_SmallBuffer) {
    TRACE_PROFILER_HANDLE handle;
    EXPECT_TRUE(STATUS_FAILED(profilerInitialize(100, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));
}

TEST_F(TraceApiTest, InvalidInputProfilerRelease_InvalidHandleRelease) {
    TRACE_PROFILER_HANDLE handle;
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(1000000, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    // release it normal
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));

    // release invalid handle - should be idempotent
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(INVALID_TRACE_PROFILER_HANDLE)));
}

TEST_F(TraceApiTest, InvalidInputSetLevel_InvalidHandleRelease) {
    TRACE_PROFILER_HANDLE handle;
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(1000000, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    // Set it normal
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_REPORT_ALWAYS)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_REPORT_ALWAYS)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_CRITICAL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_CRITICAL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_DEBUG)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_DEBUG)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_VERBOSE)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_VERBOSE)));

    // Check internal fn pointer
    PTraceProfiler pTraceProfiler = TRACE_PROFILER_HANDLE_TO_POINTER(handle);
    EXPECT_TRUE(pTraceProfiler->traceStartFn == traceStartInternal);
    EXPECT_TRUE(pTraceProfiler->traceStopFn == traceStopInternal);


    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_DISABLED)));
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_DISABLED)));

    // Check internal fn pointer
    EXPECT_TRUE(pTraceProfiler->traceStartFn == traceStartNoop);
    EXPECT_TRUE(pTraceProfiler->traceStopFn == traceStopNoop);

    // Invalid handle
    EXPECT_TRUE(STATUS_FAILED(setProfilerLevel(INVALID_TRACE_PROFILER_HANDLE, TRACE_LEVEL_INFO)));

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}


TEST_F(TraceApiTest, InvalidInputGetBuffer_InvalidInput) {
    TRACE_PROFILER_HANDLE handle;
    PCHAR pBuffer;
    UINT32 bufferSize;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(1000000, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    // Invalid handle
    EXPECT_TRUE(STATUS_FAILED(getFormattedTraceBuffer(INVALID_TRACE_PROFILER_HANDLE, &pBuffer, &bufferSize)));

    // Null input buffer
    EXPECT_TRUE(STATUS_FAILED(getFormattedTraceBuffer(handle, NULL, &bufferSize)));

    // Null input buffer size - should be fine/
    EXPECT_TRUE(STATUS_SUCCEEDED(getFormattedTraceBuffer(handle, &pBuffer, &bufferSize)));

    // Release the trace buffer
    EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(pBuffer)));

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}

TEST_F(TraceApiTest, InvalidInputFreeBuffer_NullPointer) {
    EXPECT_TRUE(STATUS_SUCCEEDED(freeTraceBuffer(NULL)));
}


TEST_F(TraceApiTest, InvalidInputSetLevel_InvalidInputTraceStartStop) {
    TRACE_PROFILER_HANDLE handle;
    TRACE_HANDLE traceHandle;

    EXPECT_TRUE(STATUS_SUCCEEDED(profilerInitialize(1000000, TRACE_LEVEL_REPORT_ALWAYS, FLAGS_USE_AIV_TRACE_PROFILER_FORMAT, &handle)));

    //
    // Set the trace levels to INFO
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_INFO)));

    // Invalid handle
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Null trace name
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, NULL, TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, NULL, TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, NULL, TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Empty trace name
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Null trace handle
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_INFO, NULL)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, NULL)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, NULL)));

    // Invalid profiler handle
    EXPECT_TRUE(STATUS_FAILED(traceStop(INVALID_TRACE_PROFILER_HANDLE, traceHandle)));

    // Invalid trace handle - should succeed
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, INVALID_TRACE_HANDLE_VALUE)));

    //
    // Disable the calls and re-try the same
    //
    EXPECT_TRUE(STATUS_SUCCEEDED(setProfilerLevel(handle, TRACE_LEVEL_DISABLED)));

    // Invalid handle
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(INVALID_TRACE_PROFILER_HANDLE, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Null trace name - succeeds because the handler is the stub handler
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, NULL, TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, NULL, TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, NULL, TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Empty trace name - succeeds because the handler is the stub handler
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_INFO, &traceHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_CRITICAL, &traceHandle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStart(handle, (PCHAR) "", TRACE_LEVEL_VERBOSE, &traceHandle)));

    // Null trace handle - fails as even with the stub handler we still need to return the trace handle
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_INFO, NULL)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_CRITICAL, NULL)));
    EXPECT_TRUE(STATUS_FAILED(traceStart(handle, TEST_TRACE_NAME, TRACE_LEVEL_VERBOSE, NULL)));

    // Invalid profiler handle
    EXPECT_TRUE(STATUS_FAILED(traceStop(INVALID_TRACE_PROFILER_HANDLE, traceHandle)));

    // Invalid trace handle - should succeed
    EXPECT_TRUE(STATUS_SUCCEEDED(traceStop(handle, INVALID_TRACE_HANDLE_VALUE)));

    // release the profiler
    EXPECT_TRUE(STATUS_SUCCEEDED(profilerRelease(handle)));
}
