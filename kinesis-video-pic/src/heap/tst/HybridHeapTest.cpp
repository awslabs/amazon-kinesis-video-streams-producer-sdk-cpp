#include "HeapTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class HybridHeapTest : public HeapTestBase,
        public WithParamInterface<::std::tuple<HEAP_BEHAVIOR_FLAGS>>
{
protected:
    static const UINT32 AllocationCount = 100;

    VOID SetUp() {
        HeapTestBase::SetUp();

        HEAP_BEHAVIOR_FLAGS primaryHeapType;
        std::tie(primaryHeapType) = GetParam();
        mHeapType = primaryHeapType | FLAGS_USE_HYBRID_VRAM_HEAP;

        // Set the invalid allocation handles
        for (UINT32 i = 0; i < AllocationCount; i++) {
            mHandles[i] = INVALID_ALLOCATION_HANDLE_VALUE;
        }

        mHeap = NULL;
    }

    VOID freeAllocations() {
        // Release the allocations in case of the system heap usage as heap release doesn't free allocations
        if (mHeap != NULL && (mHeapType & FLAGS_USE_SYSTEM_HEAP) != HEAP_FLAGS_NONE) {
            for (UINT32 i = 0; i < AllocationCount; i++) {
                if (IS_VALID_ALLOCATION_HANDLE(mHandles[i])) {
                    EXPECT_EQ(STATUS_SUCCESS, heapFree(mHeap, mHandles[i]));
                }
            }
        }
    }

    UINT32 mHeapType;
    PHeap mHeap;
    ALLOCATION_HANDLE mHandles[AllocationCount];
};

TEST_P(HybridHeapTest, hybridCreateHeap)
{
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;

    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Initialize
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

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

    DLOGV("Allocating from RAM type %s",
          (mHeapType & FLAGS_USE_AIV_HEAP) != HEAP_FLAGS_NONE ? "AIV heap" : "System heap");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
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

    DLOGV("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, vramAllocSize, &handle)));
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

    // Free allocations before release
    freeAllocations();

    // Release the heap and validate internal calls
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
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

TEST_P(HybridHeapTest, hybridCreateHeapMemHeapSmall)
{
    // Initialize should fail as MIN_HEAP_SIZE will be smaller for the mem heap with 20% reduction
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             mHeapType,
                                             NULL,
                                             &mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapDlOpenCount)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Initialize with one time opening
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
    EXPECT_EQ(mDlCloseCount, 1);
    EXPECT_EQ(mDlOpenCount, 1);

    // Initialize with reopen
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                mHeapType | FLAGS_REOPEN_VRAM_LIBRARY,
                                                NULL,
                                                &mHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
    EXPECT_EQ(mDlCloseCount, 2);
    EXPECT_EQ(mDlOpenCount, 3);
}

TEST_P(HybridHeapTest, hybridCreateHeapDlOpenError)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlOpen = NULL;

    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             mHeapType,
                                             NULL,
                                             &mHeap)));

    // Validate the on error we don't attempt to close the library as the handle is NULL
    EXPECT_EQ(mDlCloseCount, 0);
}

TEST_P(HybridHeapTest, hybridCreateHeapDlSymError)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlSym = FALSE;

    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             mHeapType,
                                             NULL,
                                             &mHeap)));


    // Validate the on error we close the library
    EXPECT_EQ(mDlCloseCount, 1);
}

TEST_P(HybridHeapTest, hybridCreateHeapDlCloseError)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mDlClose = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    EXPECT_TRUE(STATUS_FAILED(heapRelease(mHeap)));
    // Validate the on error we close the library
    EXPECT_EQ(mDlCloseCount, 1);
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamInitError)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramInit = 1;

    // Initialize should fail as we will fail to vram init
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             mHeapType,
                                             NULL,
                                             &mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapGetMaxVramSmall)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramGetMax = MIN_HEAP_SIZE;

    // Initialize should fail as requested vram size will be smaller than the max vram
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(heapSize,
                                             50,
                                             mHeapType,
                                             NULL,
                                             &mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamReleaseError)
{
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;

    // Set to produce an error
    mVramUninit = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                50,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    EXPECT_TRUE(STATUS_FAILED(heapRelease(mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamAllocError)
{
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;

    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramAlloc = 0;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    DLOGV("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
    }

    DLOGV("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_FALSE(STATUS_SUCCEEDED(heapAlloc(mHeap, vramAllocSize, &handle)));
        mHandles[numAlloc - 1 + i] = handle;
    }

    freeAllocations();
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamMapError)
{
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramLock = NULL;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    DLOGV("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
    }

    DLOGV("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_TRUE(STATUS_FAILED(heapAlloc(mHeap, vramAllocSize, &handle)));
        EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[numAlloc - 1 + i] = handle;
    }

    freeAllocations();
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamUnmapError)
{
    ALLOCATION_HANDLE handle;
    UINT64 retSize;
    PVOID pAlloc;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramUnlock = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    DLOGV("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
    }

    DLOGV("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        // Should produce a warning but shouldn't fail
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, vramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

        // Try mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(mHeap, handle, &pAlloc, &retSize)));

        // Try un-mapping it - should fail
        EXPECT_TRUE(STATUS_FAILED(heapUnmap(mHeap, pAlloc)));

        mHandles[numAlloc - 1 + i] = handle;
    }

    freeAllocations();
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
}

TEST_P(HybridHeapTest, hybridCreateHeapVRamFreeError)
{
    ALLOCATION_HANDLE handle;
    UINT64 retSize;
    PVOID pAlloc;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;
    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    // Set to produce an error
    mVramFree = 1;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(heapSize,
                                                spillRatio,
                                                mHeapType,
                                                NULL,
                                                &mHeap)));

    DLOGV("Allocating from RAM");

    // Allocate from ram - should be 1 less due to service structs
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, ramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
    }

    DLOGV("Allocating from VRAM");

    // Allocate from vram
    for (UINT32 i = 0; i < numAlloc; i++) {
        // Should produce a warning but shouldn't fail
        EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(mHeap, vramAllocSize, &handle)));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

        // Try mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapMap(mHeap, handle, &pAlloc, &retSize)));

        // Try un-mapping it - should succeed
        EXPECT_TRUE(STATUS_SUCCEEDED(heapUnmap(mHeap, pAlloc)));

        // Try freeing it - should fail
        EXPECT_TRUE(STATUS_FAILED(heapFree(mHeap, handle)));
    }

    freeAllocations();
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(mHeap)));
}

TEST_P(HybridHeapTest, hybridFillResizeAlloc)
{
    ALLOCATION_HANDLE handle;
    UINT32 memHeapLimit;
    UINT32 vramHeapLimit;
    UINT32 vramAllocSize;
    UINT32 ramAllocSize;
    UINT32 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 spillRatio = 50;
    UINT32 numAlloc = AllocationCount / 2;

    // Split the 50% and allocate half from ram and half from vram
    memHeapLimit = (UINT32)(heapSize * ((DOUBLE)spillRatio / 100));
    vramHeapLimit = heapSize - memHeapLimit;
    vramAllocSize = vramHeapLimit / numAlloc;
    ramAllocSize = memHeapLimit / numAlloc;

    EXPECT_EQ(STATUS_SUCCESS, heapInitialize(heapSize,
                                             spillRatio,
                                             mHeapType,
                                             NULL,
                                             &mHeap));

    // Allocate from ram - account for the service structs
    // Store an allocation handle somewhere in the middle
    for (UINT32 i = 0; i < numAlloc; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(mHeap, ramAllocSize, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[i] = handle;
    }

    // Allocate from vram and account for the service structs.
    for (UINT32 i = 0; i < numAlloc - 1; i++) {
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(mHeap, vramAllocSize, &handle));
        EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));
        mHandles[numAlloc + i] = handle;
    }

    // Set to produce an error if vram allocation is called
    mVramAlloc = 0;

    // Try to allocate and ensure we fail
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(mHeap, ramAllocSize - 200, &handle));
    EXPECT_FALSE(IS_VALID_ALLOCATION_HANDLE(handle));

    // Resize a ram handle from somewhere in the middle
    EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(mHeap, &mHandles[numAlloc / 2], 100));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(mHandles[numAlloc / 2]));

    // Ensure we are able to allocate from direct RAM
    EXPECT_EQ(STATUS_SUCCESS, heapAlloc(mHeap, ramAllocSize - 200, &handle));
    EXPECT_TRUE(IS_VALID_ALLOCATION_HANDLE(handle));

    EXPECT_EQ(STATUS_SUCCESS, heapFree(mHeap, handle));

    freeAllocations();
    EXPECT_EQ(STATUS_SUCCESS, heapRelease(mHeap));
}

INSTANTIATE_TEST_CASE_P(PermutatedHeapType, HybridHeapTest,
                        Values(FLAGS_USE_AIV_HEAP, FLAGS_USE_SYSTEM_HEAP));
