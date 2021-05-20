#include "HeapTestFixture.h"

class HeapPerfTest : public HeapTestBase {
};

#define HEAP_PERF_TEST_VIEW_ITEM_COUNT              1500
#define HEAP_PERF_TEST_MULTI_VIEW_ITEM_COUNT        1500 * 1000
#define HEAP_PERF_TEST_MIN_ALLOCATION               2000
#define HEAP_PERF_TEST_MULTI_VIEW_MIN_ALLOCATION    64
#define HEAP_PERF_TEST_ALLOCATION_DELTA             50
#define HEAP_PERF_TEST_ITERATION_COUNT              1000000
#define HEAP_PERF_TEST_MULTI_VIEW_ITERATION_COUNT   10000000
#define HEAP_PERF_TEST_SIZE                         256 * 1024 * 1024

VOID randomAllocFree(PHeap pHeap, UINT32 itemCount, UINT32 iterationCount, UINT32 allocSize)
{
    PALLOCATION_HANDLE handles = (PALLOCATION_HANDLE) MEMALLOC(itemCount * SIZEOF(ALLOCATION_HANDLE));
    ASSERT_TRUE(handles != NULL);

    UINT32 i, size;

    for (i = 0; i < iterationCount; i++) {
        // Allocate min allocation size + some random delta, thus simulating real-live stream
        size = allocSize + RAND() % HEAP_PERF_TEST_ALLOCATION_DELTA;
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handles[i % itemCount]));
        EXPECT_NE(INVALID_ALLOCATION_HANDLE_VALUE, handles[i % itemCount]) << "Failed on iteration " << i << " item count " << itemCount;

        if (i >= itemCount) {
            // Free the allocation
            EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[(i - itemCount) % itemCount]));
        }
    }

}

VOID singleAllocFree(PHeap pHeap)
{
    ALLOCATION_HANDLE handle;

    for (UINT32 i = 0; i < HEAP_PERF_TEST_ITERATION_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, 10000, &handle));
        EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handle));
    }

}

///////////////////////////////////////////////////////////////
// Below are the heap perf tests which should not being
// executed on small footprint devices due to limited
// memory and tighter over commit protection.
///////////////////////////////////////////////////////////////

TEST_F(HeapPerfTest, randomAllocFreeWindowedPerf)
{
    PHeap pHeap;
    UINT64 time, endTime, duration, durationSystem;
    DOUBLE iterationDuration, iterationDurationSystem;
    UINT32 totalIteration = 10, sleepBetweenIteration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, successCount = 0, i = 0;

    for (; i < totalIteration; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap));
        time = GETTIME();
        randomAllocFree(pHeap, HEAP_PERF_TEST_VIEW_ITEM_COUNT, HEAP_PERF_TEST_ITERATION_COUNT, HEAP_PERF_TEST_MIN_ALLOCATION);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        durationSystem = endTime - time;
        iterationDurationSystem = (DOUBLE) durationSystem / HEAP_PERF_TEST_ITERATION_COUNT;
        DLOGI("System Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) durationSystem / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDurationSystem * DEFAULT_TIME_UNIT_IN_NANOS);

        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap));
        time = GETTIME();
        randomAllocFree(pHeap, HEAP_PERF_TEST_VIEW_ITEM_COUNT, HEAP_PERF_TEST_ITERATION_COUNT, HEAP_PERF_TEST_MIN_ALLOCATION);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        duration = endTime - time;
        iterationDuration = (DOUBLE) duration / HEAP_PERF_TEST_ITERATION_COUNT;
        DLOGI("Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) duration / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDuration * DEFAULT_TIME_UNIT_IN_NANOS);

        // check if we are within 20% of the system heap speed
        if ((DOUBLE) duration <= (DOUBLE) durationSystem * 1.2 && iterationDuration <= iterationDurationSystem * 1.2) {
            successCount++;
        }

        EXPECT_TRUE((DOUBLE) duration <= (DOUBLE) durationSystem * 2.0);
        EXPECT_TRUE(iterationDuration <= iterationDurationSystem * 2.0);

        THREAD_SLEEP(sleepBetweenIteration);
    }

    // we should be  within 20% of the system heap speed at least 50% of the time
    EXPECT_TRUE(successCount >= (totalIteration / 2));
}

TEST_F(HeapPerfTest, randomAllocFreeMultiStreamWindowedPerf)
{
    PHeap pHeap;
    UINT64 time, endTime, duration, durationSystem;
    DOUBLE iterationDuration, iterationDurationSystem;
    UINT32 totalIteration = 10, sleepBetweenIteration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, successCount = 0, i = 0;

    for (; i < totalIteration; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap));
        time = GETTIME();
        randomAllocFree(pHeap, HEAP_PERF_TEST_MULTI_VIEW_ITEM_COUNT, HEAP_PERF_TEST_MULTI_VIEW_ITERATION_COUNT, HEAP_PERF_TEST_MULTI_VIEW_MIN_ALLOCATION);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        durationSystem = endTime - time;
        iterationDurationSystem = (DOUBLE) durationSystem / HEAP_PERF_TEST_MULTI_VIEW_ITERATION_COUNT;
        DLOGI("System Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) durationSystem / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDurationSystem * DEFAULT_TIME_UNIT_IN_NANOS);

        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap));
        time = GETTIME();
        randomAllocFree(pHeap, HEAP_PERF_TEST_MULTI_VIEW_ITEM_COUNT, HEAP_PERF_TEST_MULTI_VIEW_ITERATION_COUNT, HEAP_PERF_TEST_MULTI_VIEW_MIN_ALLOCATION);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        duration = endTime - time;
        iterationDuration = (DOUBLE) duration / HEAP_PERF_TEST_MULTI_VIEW_ITERATION_COUNT;
        DLOGI("Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) duration / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDuration * DEFAULT_TIME_UNIT_IN_NANOS);

        // check if we are within 20% of the system heap speed
        if ((DOUBLE) duration <= (DOUBLE) durationSystem * 1.2 && iterationDuration <= iterationDurationSystem * 1.2) {
            successCount++;
        }

        EXPECT_TRUE((DOUBLE) duration <= (DOUBLE) durationSystem * 2.0);
        EXPECT_TRUE(iterationDuration <= iterationDurationSystem * 2.0);

        THREAD_SLEEP(sleepBetweenIteration);
    }

    // we should be within 20% of the system heap speed at least 50% of the time
    EXPECT_TRUE(successCount >= (totalIteration / 2));
}

TEST_F(HeapPerfTest, singleAllocFreePerf)
{
    PHeap pHeap;
    UINT64 time, endTime, duration, durationSystem;
    DOUBLE iterationDuration, iterationDurationSystem;
    UINT32 totalIteration = 10, sleepBetweenIteration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND, successCount = 0, i = 0;

    for (; i < totalIteration; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap));
        time = GETTIME();
        singleAllocFree(pHeap);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        durationSystem = endTime - time;
        iterationDurationSystem = (DOUBLE) durationSystem / HEAP_PERF_TEST_ITERATION_COUNT;
        DLOGI("System Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) durationSystem / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDurationSystem * DEFAULT_TIME_UNIT_IN_NANOS);

        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(HEAP_PERF_TEST_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap));
        time = GETTIME();
        singleAllocFree(pHeap);
        endTime = GETTIME();
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

        duration = endTime - time;
        iterationDuration = (DOUBLE) duration / HEAP_PERF_TEST_ITERATION_COUNT;
        DLOGI("Allocator perf time: %lf seconds, time per iteration: %lf nanos", (DOUBLE) duration / HUNDREDS_OF_NANOS_IN_A_SECOND, iterationDuration * DEFAULT_TIME_UNIT_IN_NANOS);

        // check if we are within 20% of the system heap speed
        if ((DOUBLE) duration <= (DOUBLE) durationSystem * 1.2 && iterationDuration <= iterationDurationSystem * 1.2) {
            successCount++;
        }

        EXPECT_TRUE((DOUBLE) duration <= (DOUBLE) durationSystem * 2.0);
        EXPECT_TRUE(iterationDuration <= iterationDurationSystem * 2.0);

        THREAD_SLEEP(sleepBetweenIteration);
    }

    // we should be within 20% of the system heap speed at least 50% of the time
    EXPECT_TRUE(successCount >= (totalIteration / 2));
}
