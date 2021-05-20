#include "HeapTestFixture.h"

class HeapApiFunctionalityTest : public HeapTestBase {
};

VOID zeroHandleArray(PALLOCATION_HANDLE handles, UINT32 count)
{
    for (UINT32 i = 0; i < count; i++) {
        handles[i] = INVALID_ALLOCATION_HANDLE_VALUE;
    }
}

VOID singleLargeAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, MIN_HEAP_SIZE, &handle)));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID singleByteAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID multipleMapUnmapByteAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    PVOID pAlloc;
    UINT64 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Try multiple times
    for (UINT32 i = 0; i < NUM_ITERATIONS; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &size)));
        EXPECT_EQ(size, 1000);
        EXPECT_TRUE(pAlloc != NULL);

        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &size)));
        EXPECT_EQ(size, 1000);
        EXPECT_TRUE(pAlloc != NULL);

        EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(pHeap, pAlloc)));
        EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(pHeap, pAlloc)));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID multipleLargeAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT64 size = MIN_HEAP_SIZE / NUM_ITERATIONS;
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i;

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle)) << "Iteration " << i;
        handles[i] = handle;
    }

    EXPECT_EQ(STATUS_SUCCESS,heapAlloc(pHeap, size, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Free the allocations as in system heap mode we will leak
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[i]));
    }

    EXPECT_EQ(STATUS_SUCCESS,heapRelease(pHeap));
}

VOID minBlockFitAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = HEAP_PACKED_SIZE(MIN_HEAP_SIZE / NUM_ITERATIONS);
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i;
    UINT64 retSize;
    PVOID pAlloc;

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        handles[i] = handle;
    }

    // Free the third block
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[2])));

    // Try to allocate a smaller blocks which would fit in the freed allocation
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size / 3, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size / 3, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Free the second block
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[1])));

    // Try to allocate slightly smaller block which would still take the entire free block space
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size - 10, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Ensure the size is correct and the entire block hasn't been used
    EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &retSize)));
    EXPECT_EQ(retSize, size - 10);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(pHeap, pAlloc)));

    // Free the first block
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[0])));

    // Try to allocate same size
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Ensure the size is correct
    EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &retSize)));
    EXPECT_EQ(retSize, size);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(pHeap, pAlloc)));

    // Free the block and try to allocate slightly smaller
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));

    // Try to allocate slightly larger
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size + 1, &handle)));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    heapDebugCheckAllocator(pHeap, TRUE);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID minBlockFitAllocResize(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = MIN_HEAP_SIZE / NUM_ITERATIONS;
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i;
    UINT64 retSize, setSize;

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        handles[i] = handle;
    }

    // Allocate a smaller block due to the de-fragmentation
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size / 2, &handles[NUM_ITERATIONS - 1]));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handles[NUM_ITERATIONS - 1]));

    // Ensure we can't allocate any more
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Ensure we can't allocate half the size
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size / 2, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Re-size the middle element to allow for allocation
    handle = handles[NUM_ITERATIONS / 2];
    setSize = size / 2 - 100;
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));

    // Make sure the handle didn't change
    EXPECT_EQ(handle, handles[NUM_ITERATIONS / 2]);

    // Check the size
    EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &retSize));
    EXPECT_EQ(setSize, retSize);

    // Ensure we can fit a small block
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, setSize, &handle));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &retSize));
    EXPECT_EQ(setSize, retSize);

    heapDebugCheckAllocator(pHeap, TRUE);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID blockCoalesceAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = HEAP_PACKED_SIZE(MIN_HEAP_SIZE / NUM_ITERATIONS);
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i;

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        handles[i] = handle;
    }

    // Try to allocate anything large
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size, &handle)));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Free the second block
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[1])));
    // Try to allocate anything large
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size + 1, &handle)));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));
    // Free the third block
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[2])));
    // Try to allocate anything large = should fit due to free block coalescense
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size + 1, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    heapDebugCheckAllocator(pHeap, TRUE);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

VOID defragmentationAlloc(PHeap pHeap)
{
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = ROUND_DOWN(MIN_HEAP_SIZE / NUM_ITERATIONS, 8);
    UINT32 smallSize = size - 8;
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i, blocks;

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (blocks = 0; blocks < NUM_ITERATIONS; blocks++) {
        // Allocate a larger and then a smaller block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, blocks % 2 == 0 ? size : smallSize, &handle)));
        if (!IS_VALID_ALLOCATION_HANDLE(handle)) {
            break;
        }

        handles[blocks] = handle;
    }

    // Free all smaller blocks
    for (i = 1; i < blocks; i += 2) {
        if (IS_VALID_ALLOCATION_HANDLE(handles[i])) {
            // Free the smaller block
            EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handles[i])));
            handles[i] = INVALID_ALLOCATION_HANDLE_VALUE;
        }
    }

    // Expect an allocation of a single large block to fail whereas smaller one would succeed
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, size, &handle)));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, smallSize, &handle)));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    heapDebugCheckAllocator(pHeap, TRUE);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, GetHeapSizeAndGetAllocSize)
{
    PHeap pHeap;
    UINT64 i, size, heapSize;
    ALLOCATION_HANDLE handle;
    ALLOCATION_HANDLE handles[10];

    // AIV heap
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    for (i = 0; i < 10; i++) {
        // Allocate a block block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
        EXPECT_EQ(1000, size);
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapGetSize(pHeap, &heapSize)));
    EXPECT_EQ(10 * (1000 + aivGetAllocationHeaderSize() + aivGetAllocationFooterSize()), heapSize);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    // System heap
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    for (i = 0; i < 10; i++) {
        // Allocate a block block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
        EXPECT_EQ(1000, size);
        handles[i] = handle;
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapGetSize(pHeap, &heapSize)));
    EXPECT_EQ(10 * (1000 + sysGetAllocationHeaderSize() + sysGetAllocationFooterSize()), heapSize);

    for (i = 0; i < 10; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[i]));
    }
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    // AIV hybrid heap
    EXPECT_EQ(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE * 2 + 100000, 50, FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP, NULL, &pHeap));
    for (i = 0; i < 10; i++) {
        // Allocate a block block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
        EXPECT_EQ(1000, size);
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    // System hybrid heap
    EXPECT_EQ(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE * 2 + 100000, 50, FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP, NULL, &pHeap));
    for (i = 0; i < 10; i++) {
        // Allocate a block block
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
        EXPECT_EQ(1000, size);

        handles[i] = handle;
    }

    for (i = 0; i < 10; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[i]));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, SingleLargeAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    singleLargeAlloc(pHeap);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    singleLargeAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, MultipleLargeAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    multipleLargeAlloc(pHeap);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    multipleLargeAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, DefragmentationAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    defragmentationAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, SingleByteAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    singleByteAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, AivHeapMinBlockFitAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    minBlockFitAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, AivHeapBlockCoallesceAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    blockCoalesceAlloc(pHeap);
}

TEST_F(HeapApiFunctionalityTest, AivHeapMinBlockFitResize)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    minBlockFitAllocResize(pHeap);
}

TEST_F(HeapApiFunctionalityTest, AivHeapUnalignedHeapLimit)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE + 1, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
#ifdef ALIGNED_MEMORY_MODEL
    EXPECT_EQ(MIN_HEAP_SIZE + 8, pHeap->heapLimit);
#else
    EXPECT_EQ(MIN_HEAP_SIZE + 1, pHeap->heapLimit);
#endif

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, AivHeapResizeEdgeCases)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = HEAP_PACKED_SIZE(MIN_HEAP_SIZE / NUM_ITERATIONS);
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i;
    UINT64 setSize, iter;
    PVOID pAlloc;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handle, &pAlloc, &setSize));
        MEMSET(pAlloc, (BYTE)i, setSize);
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
        handles[i] = handle;
    }

    // Ensure we can't allocate any more
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Re-size the middle element to allow for allocation
    handle = handles[NUM_ITERATIONS / 2];
    setSize = size + size / 2;
    EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));

    // Free the allocation
    EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[NUM_ITERATIONS / 2]));

    // Set a new allocation slightly smaller
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size - 10, &handles[NUM_ITERATIONS / 2]));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handles[NUM_ITERATIONS / 2]));

    // Re-size to larger and check the allocation
    handle = handles[NUM_ITERATIONS / 2];
    setSize = size - 1;
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));
    EXPECT_EQ(handles[NUM_ITERATIONS / 2], handle);

    // Set the existing size
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, size));
    EXPECT_EQ(handles[NUM_ITERATIONS / 2], handle);

    // Set one larger and expect a failure
    setSize = size + 1;
    EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));
    EXPECT_EQ(handles[NUM_ITERATIONS / 2], handle);

    // Free a non-adjacent allocation and retry
    EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[0]));
    EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));

    // Free an adjacent one and retry
    EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[1]));
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, setSize));
    EXPECT_NE(handles[NUM_ITERATIONS / 2], handle);

    // Resize the 3rd allocation - make sure we can't
    EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handles[2], size + 1));
    // Free the 4th
    EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handles[3]));
    // Make sure we can resize now into an adjacent space
    handle = handles[2];
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, size + 1));
    EXPECT_EQ(handles[2], handle);

    // Make a slightly smaller size
    handle = handles[4];
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, size - 10));
    EXPECT_EQ(handles[4], handle);

    // Set smaller and cause a split of the block
    handle = handles[5];
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, size - 100));
    EXPECT_EQ(handles[5], handle);

    // Set size 1
    handle = handles[6];
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, 1));
    EXPECT_EQ(handles[6], handle);

    // Make progressively smaller
    handle = handles[7];
    EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
    EXPECT_EQ(size, setSize);
    for (iter = setSize; iter > 0; iter--) {
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, iter));
        EXPECT_EQ(handles[7], handle);
    }
    EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
    EXPECT_EQ(1, setSize);

    // Make the allocation progressively larger
    handle = handles[8];
    EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
    EXPECT_EQ(size, setSize);
    for (i = 1; i <= size; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, i));
        EXPECT_EQ(handles[8], handle);
    }

    heapDebugCheckAllocator(pHeap, TRUE);
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, AivHeapResizeUpDownBottomUp)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = HEAP_PACKED_SIZE(MIN_HEAP_SIZE / NUM_ITERATIONS);
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i, j;
    UINT64 setSize;
    PVOID pAlloc;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handle, &pAlloc, &setSize));
        MEMSET(pAlloc, (BYTE)i, setSize);
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
        handles[i] = handle;
    }

    // Ensure we can't allocate any more
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Iterate over every allocation
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        for (j = size; j > 0; j--) {
            handle = handles[i];
            EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, j));
            EXPECT_EQ(handles[i], handle);
            EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
            EXPECT_EQ(j, setSize);
            EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handle, &pAlloc, &setSize));
            // Validate that the first, mid and last elements are correct
            EXPECT_EQ((BYTE)i, ((PBYTE)pAlloc)[0]);
            EXPECT_EQ((BYTE)i, ((PBYTE)pAlloc)[setSize - 1]);
            EXPECT_EQ((BYTE)i, ((PBYTE)pAlloc)[(setSize - 1) / 2]);
            EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
        }
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handles[i], size)) << "Iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[i], &setSize));
        EXPECT_EQ(size, setSize);
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        handle = handles[i];
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, 1));
        EXPECT_EQ(handles[i], handle);
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
        EXPECT_EQ(1, setSize);
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, AivHeapResizeUpDownTopDown)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT32 size = HEAP_PACKED_SIZE(MIN_HEAP_SIZE / NUM_ITERATIONS);
    ALLOCATION_HANDLE handles[NUM_ITERATIONS];
    UINT32 i, j;
    UINT64 setSize;
    PVOID pAlloc;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));

    // Set to default
    zeroHandleArray(handles, NUM_ITERATIONS);

    // Iterate until we can't allocate a large block
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        // Allocate a larger block
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handle, &pAlloc, &setSize));
        MEMSET(pAlloc, (BYTE)i, setSize);
        EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
        handles[i] = handle;
    }

    // Ensure we can't allocate any more
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, size, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Iterate over every allocation
    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        for (j = size; j > 0; j--) {
            handle = handles[NUM_ITERATIONS - 2 - i];
            EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, j));
            EXPECT_EQ(handles[NUM_ITERATIONS - 2 - i], handle);
            EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
            EXPECT_EQ(j, setSize);
            EXPECT_EQ(STATUS_SUCCESS, heapMap(pHeap, handle, &pAlloc, &setSize));
            // Validate that the first, mid and last elements are correct
            EXPECT_EQ((BYTE)(NUM_ITERATIONS - 2 - i), ((PBYTE)pAlloc)[0]);
            EXPECT_EQ((BYTE)(NUM_ITERATIONS - 2 - i), ((PBYTE)pAlloc)[setSize - 1]);
            EXPECT_EQ((BYTE)(NUM_ITERATIONS - 2 - i), ((PBYTE)pAlloc)[(setSize - 1) / 2]);
            EXPECT_EQ(STATUS_SUCCESS, heapUnmap(pHeap, pAlloc));
        }
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handles[NUM_ITERATIONS - 2 - i], size)) << "Iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handles[NUM_ITERATIONS - 2 - i], &setSize));
        EXPECT_EQ(size, setSize);
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    for (i = 0; i < NUM_ITERATIONS - 1; i++) {
        handle = handles[NUM_ITERATIONS - 2 - i];
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, 1));
        EXPECT_EQ(handles[NUM_ITERATIONS - 2 - i], handle);
        EXPECT_EQ(STATUS_SUCCESS, heapGetAllocSize(pHeap, handle, &setSize));
        EXPECT_EQ(1, setSize);
    }

    heapDebugCheckAllocator(pHeap, TRUE);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiFunctionalityTest, MultipleMapUnmapByteAlloc)
{
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    multipleMapUnmapByteAlloc(pHeap);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    multipleMapUnmapByteAlloc(pHeap);
}
