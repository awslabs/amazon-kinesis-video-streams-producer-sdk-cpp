#include "HeapTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class HybridFileHeapTest : public HeapTestBase,
                           public WithParamInterface<::std::tuple<HEAP_BEHAVIOR_FLAGS>>
{
protected:
    VOID SetUp() {
        HeapTestBase::SetUp();

        HEAP_BEHAVIOR_FLAGS primaryHeapType;
        std::tie(primaryHeapType) = GetParam();
        mHeapType = primaryHeapType | FLAGS_USE_HYBRID_FILE_HEAP;
    }

    UINT32 mHeapType;
};

TEST_P(HybridFileHeapTest, hybridFileHeapOperationsAivPrimaryHeap)
{
    const UINT32 AllocationCount = 100;
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    ALLOCATION_HANDLE handles[AllocationCount];
    UINT32 memHeapLimit;
    UINT32 fileHeapLimit;
    UINT32 fileAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;
    UINT32 i, fileHandleIndex, skip;
    CHAR filePath[MAX_PATH_LEN + 1];
    BOOL exist;
    UINT64 fileSize, allocSize, retAllocSize;
    PVOID pAlloc;

    // Split the 50% and allocate half from ram and half from file heap
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    fileHeapLimit = heapSize - memHeapLimit;
    fileAllocSize = fileHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set the invalid allocation handles
    for (i = 0; i < AllocationCount; i++) {
        handles[i] = INVALID_ALLOCATION_HANDLE_VALUE;
    }

    // Initialize
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &pHeap)));

    DLOGV("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)))  << "Failed allocating from direct heap with index: " << i;
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle)) << "Invalid direct allocation handle at index: " << i;

        // Store the handle for later processing
        handles[i] = handle;
    }

    DLOGV("Allocating from File heap");

    // Allocate from file heap
    for (i = 0, fileHandleIndex = numAlloc - 1; i < numAlloc; i++, fileHandleIndex++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, fileAllocSize, &handle))) << "Failed allocating from file heap with index: " << i;
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle)) << "Invalid file allocation handle at index: " << i;
        handles[fileHandleIndex] = handle;
    }

    // Validate the allocations
    for (i = 0; i < numAlloc; i++) {
        SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, FILE_HEAP_DEFAULT_ROOT_DIRECTORY, FPATHSEPARATOR, i + 1);
        exist = FALSE;
        EXPECT_EQ(STATUS_SUCCESS, fileExists(filePath, &exist));
        EXPECT_EQ(TRUE, exist);
        EXPECT_EQ(STATUS_SUCCESS, getFileLength(filePath, &fileSize));
        EXPECT_EQ(fileAllocSize + SIZEOF(ALLOCATION_HEADER), fileSize);
    }

    // Try to map, read, write, unmap, map again and verify
    for (i = 0; i < AllocationCount - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &retAllocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handles[i], &pAlloc, &allocSize));
        EXPECT_EQ(retAllocSize, allocSize);
        MEMSET(pAlloc, 'a', allocSize);
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));

        // Validate
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handles[i], &pAlloc, &retAllocSize));
        EXPECT_EQ(allocSize, retAllocSize);
        EXPECT_TRUE(MEMCHK(pAlloc, 'a', (SIZE_T) retAllocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
    }

    // Resize larger
    for (i = 0; i < AllocationCount - 1; i++) {
        // Increase the allocation size by 1 and validate the returned sizes
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &retAllocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handles[i], retAllocSize + 1));
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &allocSize));
        EXPECT_EQ(retAllocSize + 1, allocSize);

        // Map the allocation and ensure the content is OK
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handles[i], &pAlloc, &allocSize));
        EXPECT_EQ(retAllocSize + 1, allocSize);
        EXPECT_TRUE(MEMCHK(pAlloc, 'a', (SIZE_T) retAllocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
    }

    // Resize smaller
    for (i = 0; i < AllocationCount - 1; i++) {
        // Reduce the allocation size by 2 and validate the returned sizes
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &retAllocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handles[i], retAllocSize - 2));
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &allocSize));
        EXPECT_EQ(retAllocSize - 2, allocSize);

        // Map the allocation and ensure the content is OK
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handles[i], &pAlloc, &allocSize));
        EXPECT_EQ(retAllocSize - 2, allocSize);
        EXPECT_TRUE(MEMCHK(pAlloc, 'a', (SIZE_T) allocSize));
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
    }

    // Validate the allocations
    for (i = 0; i < numAlloc; i++) {
        SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, FILE_HEAP_DEFAULT_ROOT_DIRECTORY, FPATHSEPARATOR, i + 1);
        exist = FALSE;
        EXPECT_EQ(STATUS_SUCCESS, fileExists(filePath, &exist));
        EXPECT_EQ(TRUE, exist);
        EXPECT_EQ(STATUS_SUCCESS, getFileLength(filePath, &fileSize));

        // After the modifications, we increased the size by 1 and decreased by 2 so the diff is minus 1
        EXPECT_EQ(fileAllocSize + SIZEOF(ALLOCATION_HEADER) - 1, fileSize);
    }

    // Free odd allocations in case of AIV heap and all of the allocations in case of system heap
    skip = (mHeapType & FLAGS_USE_SYSTEM_HEAP) != HEAP_FLAGS_NONE ? 1 : 2;
    for (i = 0; i < AllocationCount; i += skip) {
        if (IS_VALID_ALLOCATION_HANDLE(handles[i])) {
            EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[i]));
        }
    }

    // Release the heap which should free the rest of the allocations
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_P(HybridFileHeapTest, hybridFileCreateHeapMemHeapSmall)
{
    PHeap pHeap;

    // Initialize should fail as MIN_HEAP_SIZE will be smaller for the mem heap with 20% reduction
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             mHeapType,
                                             NULL,
                                             &pHeap)));
}

INSTANTIATE_TEST_CASE_P(PermutatedHeapType, HybridFileHeapTest,
                        Values(FLAGS_USE_AIV_HEAP, FLAGS_USE_SYSTEM_HEAP));
