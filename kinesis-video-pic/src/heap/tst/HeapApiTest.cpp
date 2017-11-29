#include "HeapTestFixture.h"

class HeapApiTest : public HeapTestBase {
};

TEST_F(HeapApiTest, InvalidInputHeapInitialize_HeapPointerNull) {
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_MinHeapSize) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_MaxHeapSize) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_SpillRatio) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 101, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 101, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_HeapTypeFlags) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, HEAP_FLAGS_NONE, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_VRAM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_REOPEN_VRAM_LIBRARY, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY, &pHeap)));
}

TEST_F(HeapApiTest, IdempotentHeapRelease_NullHeapRelease) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(NULL)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(NULL)));
}

TEST_F(HeapApiTest, InvalidHeapGetSize_NullHeap) {
    PHeap pHeap;
    UINT64 size;

    EXPECT_TRUE(STATUS_FAILED(heapGetSize(NULL, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetSize(pHeap, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetSize(NULL, NULL)));
}

TEST_F(HeapApiTest, InvalidHeapDebugCheckAllocator_NullHeapDebugCheckAllocator) {
    EXPECT_TRUE(STATUS_FAILED(heapDebugCheckAllocator(NULL, TRUE)));
    EXPECT_TRUE(STATUS_FAILED(heapDebugCheckAllocator(NULL, FALSE)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_NullHeap) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(NULL, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(NULL, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapGetAllocSize_NullHeapHandle) {
    PHeap pHeap;
    UINT32 size;
    ALLOCATION_HANDLE handle;

    // Create and allocate heap
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));

    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, handle, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, INVALID_HANDLE_VALUE, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, handle, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, INVALID_HANDLE_VALUE, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, INVALID_HANDLE_VALUE, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, INVALID_HANDLE_VALUE, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, handle, NULL)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
    EXPECT_EQ(1000, size);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_NullHandle) {
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 1000, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 1000, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_ZeroAlloc) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 0, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 0, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapFree_NullHeap) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(NULL, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(NULL, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapFree_InvalidHandle) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(pHeap, INVALID_ALLOCATION_HANDLE_VALUE)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(pHeap, INVALID_ALLOCATION_HANDLE_VALUE)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullHeap) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc;
    UINT32 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(NULL, handle, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(NULL, handle, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_InvalidHandle) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc;
    UINT32 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullAllocation) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT32 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, NULL, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, NULL, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullSize) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, &pAlloc, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, &pAlloc, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapUnmap_NullHeap) {
    PHeap pHeap;
    PVOID pAlloc;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(NULL, pAlloc)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(NULL, pAlloc)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapUnmap_InvalidAllocation) {
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(pHeap, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(pHeap, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}
