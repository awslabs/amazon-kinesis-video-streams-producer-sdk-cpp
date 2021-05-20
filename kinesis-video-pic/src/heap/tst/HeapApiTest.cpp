#include "HeapTestFixture.h"

class HeapApiTest : public HeapTestBase {
};

TEST_F(HeapApiTest, InvalidInputHeapInitialize_HeapPointerNull) {
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             NULL)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_MinHeapSize) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE - 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_MaxHeapSize) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MAX_HEAP_SIZE + 1,
                                             20,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_SpillRatio) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 101, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 101, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE,
                                             101,
                                             FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                                             NULL,
                                             &pHeap)));
}

TEST_F(HeapApiTest, InvalidInputHeapInitialize_HeapTypeFlags) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, HEAP_FLAGS_NONE, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_VRAM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_REOPEN_VRAM_LIBRARY, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_VRAM_HEAP | FLAGS_REOPEN_VRAM_LIBRARY, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_FILE_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_HYBRID_FILE_HEAP | FLAGS_REOPEN_VRAM_LIBRARY, NULL, &pHeap)));
}

TEST_F(HeapApiTest, IdempotentHeapRelease_NullHeapRelease) {
    PHeap pHeap;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(NULL)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(NULL)));
}

TEST_F(HeapApiTest, InvalidHeapGetSize_NullHeap) {
    PHeap pHeap = (PHeap) 12345;
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
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(NULL, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(NULL, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapGetAllocSize_NullHeapHandle) {
    PHeap pHeap;
    UINT64 size;
    ALLOCATION_HANDLE handle;

    // Create and allocate heap
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));

    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, handle, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, handle, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, INVALID_ALLOCATION_HANDLE_VALUE, &size)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(NULL, INVALID_ALLOCATION_HANDLE_VALUE, NULL)));
    EXPECT_TRUE(STATUS_FAILED(heapGetAllocSize(pHeap, handle, NULL)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapGetAllocSize(pHeap, handle, &size)));
    EXPECT_EQ(1000, size);

    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_NullHandle) {
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 1000, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 1000, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_ZeroAlloc) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 0, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapAlloc(pHeap, 0, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapAlloc_MaxAlloc) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    EXPECT_EQ(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap));
    EXPECT_EQ(STATUS_INVALID_ALLOCATION_SIZE, heapAlloc(pHeap, MAX_ALLOCATION_SIZE, &handle));
    EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));

    EXPECT_EQ(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap));
    EXPECT_EQ(STATUS_INVALID_ALLOCATION_SIZE, heapAlloc(pHeap, MAX_ALLOCATION_SIZE, &handle));
    EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));
}

TEST_F(HeapApiTest, InvalidHeapFree_NullHeap) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(NULL, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(NULL, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapFree_InvalidHandle) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(pHeap, INVALID_ALLOCATION_HANDLE_VALUE)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_FAILED(heapFree(pHeap, INVALID_ALLOCATION_HANDLE_VALUE)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullHeap) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc;
    UINT64 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(NULL, handle, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(NULL, handle, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_InvalidHandle) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc;
    UINT64 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, INVALID_ALLOCATION_HANDLE_VALUE, &pAlloc, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullAllocation) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle;
    UINT64 size;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, NULL, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, NULL, &size)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapMap_NullSize) {
    PHeap pHeap = NULL;
    ALLOCATION_HANDLE handle;
    PVOID pAlloc = NULL;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, &pAlloc, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapAlloc(pHeap, 1000, &handle)));
    EXPECT_TRUE(STATUS_FAILED(heapMap(pHeap, handle, &pAlloc, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapFree(pHeap, handle)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapUnmap_NullHeap) {
    PHeap pHeap;
    PVOID pAlloc = (PVOID) 12345;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(NULL, pAlloc)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(NULL, pAlloc)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapUnmap_InvalidAllocation) {
    PHeap pHeap;

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(pHeap, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));

    EXPECT_TRUE(STATUS_SUCCEEDED(heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_SYSTEM_HEAP, NULL, &pHeap)));
    EXPECT_TRUE(STATUS_FAILED(heapUnmap(pHeap, NULL)));
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidFileHeapCreate_InvalidParams) {
    PHeap pHeap = NULL;
    CHAR filePath[MAX_PATH_LEN + 2];

    // Non-existent path
    EXPECT_NE(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP, (PCHAR) "/doesNotExist", &pHeap));

    // Max path
    MEMSET(filePath, 'a', ARRAY_SIZE(filePath));
    filePath[ARRAY_SIZE(filePath) - 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, heapInitialize(MIN_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP, filePath, &pHeap));

    // Max heap
    EXPECT_NE(STATUS_SUCCESS, heapInitialize(MAX_LARGE_HEAP_SIZE, 20, FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP, NULL, &pHeap));

    // Should be a no-op call
    EXPECT_TRUE(STATUS_SUCCEEDED(heapRelease(pHeap)));
}

TEST_F(HeapApiTest, InvalidHeapResize_InvalidValues) {
    PHeap pHeap;
    ALLOCATION_HANDLE handle, storedHandle, invalidHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    UINT64 heapSize = MIN_HEAP_SIZE * 2 + 100000;
    UINT32 heapTypes[] = {FLAGS_USE_AIV_HEAP,
                           FLAGS_USE_SYSTEM_HEAP,
                           FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                           FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_VRAM_HEAP,
                           FLAGS_USE_AIV_HEAP | FLAGS_USE_HYBRID_FILE_HEAP,
                           FLAGS_USE_SYSTEM_HEAP | FLAGS_USE_HYBRID_FILE_HEAP};

    for (UINT32 heapType = 0; heapType < ARRAY_SIZE(heapTypes); heapType++) {
        EXPECT_EQ(STATUS_SUCCESS, heapInitialize(heapSize, 50, heapTypes[heapType], NULL, &pHeap));
        EXPECT_EQ(STATUS_SUCCESS, heapAlloc(pHeap, 1000, &handle));
        EXPECT_NE(INVALID_ALLOCATION_HANDLE_VALUE, handle);
        storedHandle = handle;

        EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, NULL, 10000));
        EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &invalidHandle, 10000));
        EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &invalidHandle, 0));
        EXPECT_NE(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, 0));
        EXPECT_EQ(storedHandle, handle);

        // Same size
        EXPECT_EQ(STATUS_SUCCESS, heapSetAllocSize(pHeap, &handle, 1000));
        EXPECT_EQ(storedHandle, handle);

        EXPECT_EQ(STATUS_SUCCESS, heapFree(pHeap, handle));
        EXPECT_EQ(STATUS_SUCCESS, heapRelease(pHeap));
    }
}
