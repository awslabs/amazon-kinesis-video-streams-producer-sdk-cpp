
#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <src/utils/src/Include_i.h>

//
// Default allocator functions
//
extern UINT64 gTotalUtilsMemoryUsage;
extern MUTEX gUtilityMemMutex;

INLINE PVOID instrumentedUtilMemAlloc(SIZE_T size)
{
    DLOGS("Test malloc %llu bytes", (UINT64) size);
    MUTEX_LOCK(gUtilityMemMutex);
    gTotalUtilsMemoryUsage += size;
    MUTEX_UNLOCK(gUtilityMemMutex);
    PBYTE pAlloc = (PBYTE) malloc(size + SIZEOF(SIZE_T));
    *(PSIZE_T) pAlloc = size;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE PVOID instrumentedUtilMemAlignAlloc(SIZE_T size, SIZE_T alignment)
{
    DLOGS("Test align malloc %llu bytes", (UINT64) size);
    // Just do malloc
    UNUSED_PARAM(alignment);
    return instrumentedUtilMemAlloc(size);
}

INLINE PVOID instrumentedUtilMemCalloc(SIZE_T num, SIZE_T size)
{
    SIZE_T overallSize = num * size;
    DLOGS("Test calloc %llu bytes", (UINT64) overallSize);

    MUTEX_LOCK(gUtilityMemMutex);
    gTotalUtilsMemoryUsage += overallSize;
    MUTEX_UNLOCK(gUtilityMemMutex);

    PBYTE pAlloc = (PBYTE) calloc(1, overallSize + SIZEOF(SIZE_T));
    *(PSIZE_T) pAlloc = overallSize;

    return pAlloc + SIZEOF(SIZE_T);
}

INLINE VOID instrumentedUtilMemFree(PVOID ptr)
{
    PBYTE pAlloc = (PBYTE) ptr - SIZEOF(SIZE_T);
    SIZE_T size = *(PSIZE_T) pAlloc;
    DLOGS("Test free %llu bytes", (UINT64) size);

    MUTEX_LOCK(gUtilityMemMutex);
    gTotalUtilsMemoryUsage -= size;
    MUTEX_UNLOCK(gUtilityMemMutex);

    free(pAlloc);
}

//
// Set the allocators to the instrumented equivalents
//
extern memAlloc globalMemAlloc;
extern memAlignAlloc globalMemAlignAlloc;
extern memCalloc globalMemCalloc;
extern memFree globalMemFree;

class UtilTestBase : public ::testing::Test {
  public:
    UtilTestBase(BOOL setAllocators = TRUE)
    {
        // Store the function pointers
        gTotalUtilsMemoryUsage = 0;

        allocatorsSet = setAllocators;

        if (allocatorsSet) {
            storedMemAlloc = globalMemAlloc;
            storedMemAlignAlloc = globalMemAlignAlloc;
            storedMemCalloc = globalMemCalloc;
            storedMemFree = globalMemFree;

            // Create the mutex for the synchronization for the instrumentation
            gUtilityMemMutex = MUTEX_CREATE(FALSE);

            // Save the instrumented ones
            globalMemAlloc = instrumentedUtilMemAlloc;
            globalMemAlignAlloc = instrumentedUtilMemAlignAlloc;
            globalMemCalloc = instrumentedUtilMemCalloc;
            globalMemFree = instrumentedUtilMemFree;
        }
    };

    virtual void SetUp()
    {
        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
            SET_LOGGER_LOG_LEVEL(logLevel);
        }

        DLOGI("\nSetting up test: %s\n", GetTestName());
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        // Validate the allocations cleanup
        DLOGI("Final remaining allocation size is %llu\n", gTotalUtilsMemoryUsage);

        EXPECT_EQ((UINT64) 0, gTotalUtilsMemoryUsage);

        if (allocatorsSet) {
            globalMemAlloc = storedMemAlloc;
            globalMemAlignAlloc = storedMemAlignAlloc;
            globalMemCalloc = storedMemCalloc;
            globalMemFree = storedMemFree;
            MUTEX_FREE(gUtilityMemMutex);
        }
    };

    PCHAR GetTestName()
    {
        return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

  protected:
    // Stored function pointers to reset on exit
    memAlloc storedMemAlloc;
    memAlignAlloc storedMemAlignAlloc;
    memCalloc storedMemCalloc;
    memFree storedMemFree;

    BOOL allocatorsSet;
};
