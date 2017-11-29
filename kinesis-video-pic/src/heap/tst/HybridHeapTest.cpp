#include "HeapTestFixture.h"

class HybridHeapTest : public HeapTestBase {
};

TEST_F(HybridHeapTest, hybridCreateHeap)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = 50;

    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = heapSize * ((DOUBLE)spillRatio / 100);
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Initialize
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                             spillRatio,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));

    // Validate internal calls
    EXPECT_EQ(mVramGetMaxCount, 1);
    EXPECT_EQ(mVramInitCount, 1);
    EXPECT_EQ(mVramAllocCount, 0);
    EXPECT_EQ(mVramFreeCount, 0);
    EXPECT_EQ(mVramLockCount, 0);
    EXPECT_EQ(mVramUnlockCount, 0);
    EXPECT_EQ(mVramUninitCount, 0);
    EXPECT_EQ(mDlOpenCount, 1);
    EXPECT_EQ(mDlCloseCount, 0);
    EXPECT_EQ(mDlErrorCount, 0);
    EXPECT_EQ(mDlSymCount, 7);

    DLOGI("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    // Validate internal calls
    EXPECT_EQ(mVramGetMaxCount, 1);
    EXPECT_EQ(mVramInitCount, 1);
    EXPECT_EQ(mVramAllocCount, 0);
    EXPECT_EQ(mVramFreeCount, 0);
    EXPECT_EQ(mVramLockCount, 0);
    EXPECT_EQ(mVramUnlockCount, 0);
    EXPECT_EQ(mVramUninitCount, 0);
    EXPECT_EQ(mDlOpenCount, 1);
    EXPECT_EQ(mDlCloseCount, 0);
    EXPECT_EQ(mDlErrorCount, 0);
    EXPECT_EQ(mDlSymCount, 7);

    DLOGI("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, vramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    // Validate internal calls
    EXPECT_EQ(mVramGetMaxCount, 1);
    EXPECT_EQ(mVramInitCount, 1);
    EXPECT_EQ(mVramAllocCount, numAlloc);
    EXPECT_EQ(mVramFreeCount, 0);
    EXPECT_EQ(mVramLockCount, 50); // We lock/unlock during allocations
    EXPECT_EQ(mVramUnlockCount, 50);
    EXPECT_EQ(mVramUninitCount, 0);
    EXPECT_EQ(mDlOpenCount, 1);
    EXPECT_EQ(mDlCloseCount, 0);
    EXPECT_EQ(mDlErrorCount, 0);
    EXPECT_EQ(mDlSymCount, 7);

    // Release the heap and validate internal calls
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_EQ(mVramGetMaxCount, 1);
    EXPECT_EQ(mVramInitCount, 1);
    EXPECT_EQ(mVramAllocCount, numAlloc);
    EXPECT_EQ(mVramFreeCount, 0);
    EXPECT_EQ(mVramLockCount, 50);
    EXPECT_EQ(mVramUnlockCount, 50);
    EXPECT_EQ(mVramUninitCount, 1);
    EXPECT_EQ(mDlOpenCount, 1);
    EXPECT_EQ(mDlCloseCount, 1);
    EXPECT_EQ(mDlErrorCount, 0);
    EXPECT_EQ(mDlSymCount, 7);
}

TEST_F(HybridHeapTest, hybridCreateHeapMemHeapSmall)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    // Initialize should fail as MIN_HEAP_SIZE will be smaller for the mem heap with 20% reduction
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapDlOpenCount)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Initialize with one time opening
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_EQ(mDlCloseCount, 1);
    EXPECT_EQ(mDlOpenCount, 1);

    // Initialize with reopen
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                                &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_EQ(mDlCloseCount, 2);
    EXPECT_EQ(mDlOpenCount, 3);
}

TEST_F(HybridHeapTest, hybridCreateHeapDlOpenError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlOpen = NULL;

    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));

    // Validate the on error we don't attempt to close the library as the handle is NULL
    EXPECT_EQ(mDlCloseCount, 0);
}

TEST_F(HybridHeapTest, hybridCreateHeapDlSymError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlSym = FALSE;

    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));


    // Validate the on error we close the library
    EXPECT_EQ(mDlCloseCount, 1);
}

TEST_F(HybridHeapTest, hybridCreateHeapDlCloseError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlClose = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                             50,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));

    EXPECT_TRUE(STATUS_FAILED(heapRelease(pHeap)));
    // Validate the on error we close the library
    EXPECT_EQ(mDlCloseCount, 1);
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamInitError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramInit = 1;

    // Initialize should fail as we will fail to vram init
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapGetMaxVramSmall)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramGetMax = MIN_HEAP_SIZE;

    // Initialize should fail as requested vram size will be smaller than the max vram
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamReleaseError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramUninit = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));

    EXPECT_TRUE(STATUS_FAILED(heapRelease(pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamAllocError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = 50;

    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = heapSize * ((DOUBLE)spillRatio / 100);
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramAlloc = 0;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));

    DLOGI("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    DLOGI("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_FALSE(STATUS_SUCCEEDED(heapAlloc(pHeap, vramAllocSize, &handle)));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamMapError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = 50;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = heapSize * ((DOUBLE)spillRatio / 100);
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramLock = NULL;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));

    DLOGI("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    DLOGI("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, vramAllocSize, &handle)));
        EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamUnmapError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 retSize;
    PVOID pAlloc;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = 50;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = heapSize * ((DOUBLE)spillRatio / 100);
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramUnlock = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));

    DLOGI("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    DLOGI("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        // Should produce a warning but shouldn't fail
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, vramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

        // Try mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &retSize)));

        // Try un-mapping it - should fail
        EXPECT_TRUE(STATUS_FAILED(heapUnmap(pHeap, pAlloc)));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HybridHeapTest, hybridCreateHeapVRamFreeError)
{
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 retSize;
    PVOID pAlloc;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = 50;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = heapSize * ((DOUBLE)spillRatio / 100);
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramFree = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                                &pHeap)));

    DLOGI("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
    }

    DLOGI("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        // Should produce a warning but shouldn't fail
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, vramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

        // Try mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(pHeap, handle, &pAlloc, &retSize)));

        // Try un-mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(pHeap, pAlloc)));

        // Try freeing it - should fail
        EXPECT_TRUE(STATUS_FAILED(heapFree(pHeap, handle)));
    }

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}