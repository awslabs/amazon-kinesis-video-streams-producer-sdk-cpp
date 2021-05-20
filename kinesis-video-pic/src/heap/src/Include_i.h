/**
 * Memory heap public include file
 */
#ifndef __MEM_HEAP_INCLUDE_I_H__
#define __MEM_HEAP_INCLUDE_I_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Including the headers
 */
#include "com/amazonaws/kinesis/video/heap/Include.h"

/**
 * Invalid allocation value
 */
#ifndef INVALID_ALLOCATION_VALUE
#define INVALID_ALLOCATION_VALUE MAX_UINT64
#endif

////////////////////////////////////////////////////////////////////
// Forward declaration of the functions
////////////////////////////////////////////////////////////////////

/**
 * Releases the entire heap.
 * IMPORTANT: Some heaps will leak memory if the allocations are not freed previously
 */
typedef STATUS (*HeapReleaseFunc)(PHeap);

/**
 * Frees allocated memory
 */
typedef STATUS (*HeapFreeFunc)(PHeap, ALLOCATION_HANDLE);

/**
 * Gets the heap size
 */
typedef STATUS (*HeapGetSizeFunc)(PHeap, PUINT64);

/**
 * Gets the allocation size
 */
typedef STATUS (*HeapGetAllocSizeFunc)(PHeap, ALLOCATION_HANDLE, PUINT64);

/**
 * Sets the allocation size
 */
typedef STATUS (*HeapSetAllocSizeFunc)(PHeap, PALLOCATION_HANDLE, UINT64, UINT64);

/**
 * Allocates memory from the heap
 */
typedef STATUS (*HeapAllocFunc)(PHeap, UINT64, PALLOCATION_HANDLE);

/**
 * Maps the allocated handle and retrieves a memory address
 */
typedef STATUS (*HeapMapFunc)(PHeap, ALLOCATION_HANDLE, PVOID*, PUINT64);

/**
 * Un-maps the previously mapped memory
 */
typedef STATUS (*HeapUnmapFunc)(PHeap, PVOID);

/**
 * Debug validates/outputs information about the heap
 */
typedef STATUS (*HeapDebugCheckAllocatorFunc)(PHeap, BOOL);

/**
 * Creates and initializes the heap
 */
typedef STATUS (*HeapInitializeFunc)(PHeap, UINT64);

/**
 * Returns the allocation size
 */
typedef UINT64 (*GetAllocationSizeFunc)(PHeap, ALLOCATION_HANDLE);

/**
 * Returns the allocation header size
 */
typedef UINT64 (*GetAllocationHeaderSizeFunc)();

/**
 * Returns the allocation footer size
 */
typedef UINT64 (*GetAllocationFooterSizeFunc)();

/**
 * Returns the adjusted aligned alloc size
 */
typedef UINT64 (*GetAllocationAlignedSizeFunc)(UINT64);

/**
 * Returns the allocation limits
 */
typedef VOID (*GetHeapLimitsFunc)(PUINT64, PUINT64);

/**
 * Definition macros making it easier for function declarations/definitions
 */
#define DEFINE_CREATE_HEAP(name)         STATUS name(PHeap* ppHeap)
#define DEFINE_INIT_HEAP(name)           STATUS name(PHeap pHeap, UINT64 heapLimit)
#define DEFINE_RELEASE_HEAP(name)        STATUS name(PHeap pHeap)
#define DEFINE_GET_HEAP_SIZE(name)       STATUS name(PHeap pHeap, PUINT64 pHeapSize)
#define DEFINE_HEAP_ALLOC(name)          STATUS name(PHeap pHeap, UINT64 size, PALLOCATION_HANDLE pHandle)
#define DEFINE_HEAP_FREE(name)           STATUS name(PHeap pHeap, ALLOCATION_HANDLE handle)
#define DEFINE_HEAP_GET_ALLOC_SIZE(name) STATUS name(PHeap pHeap, ALLOCATION_HANDLE handle, PUINT64 pAllocSize)
#define DEFINE_HEAP_SET_ALLOC_SIZE(name) STATUS name(PHeap pHeap, PALLOCATION_HANDLE pHandle, UINT64 size, UINT64 newSize)
#define DEFINE_HEAP_MAP(name)            STATUS name(PHeap pHeap, ALLOCATION_HANDLE handle, PVOID* ppAllocation, PUINT64 pSize)
#define DEFINE_HEAP_UNMAP(name)          STATUS name(PHeap pHeap, PVOID pAllocation)
#define DEFINE_HEAP_CHK(name)            STATUS name(PHeap pHeap, BOOL dump)
#define DEFINE_ALLOC_SIZE(name)          UINT64 name(PHeap pHeap, ALLOCATION_HANDLE handle)
#define DEFINE_HEADER_SIZE(name)         UINT64 name()
#define DEFINE_FOOTER_SIZE(name)         UINT64 name()
#define DEFINE_ALIGNED_SIZE(name)        UINT64 name(UINT64 size)
#define DEFINE_HEAP_LIMITS(name)         VOID name(PUINT64 pMinHeapSize, PUINT64 pMaxHeapSize)

typedef struct {
    /**
     * Public Heap struct encapsulation
     */
    Heap heap;

    /**
     * Heap functions - polymorphism
     */
    HeapInitializeFunc heapInitializeFn;
    HeapReleaseFunc heapReleaseFn;
    HeapGetSizeFunc heapGetSizeFn;
    HeapFreeFunc heapFreeFn;
    HeapGetAllocSizeFunc heapGetAllocSizeFn;
    HeapSetAllocSizeFunc heapSetAllocSizeFn;
    HeapAllocFunc heapAllocFn;
    HeapMapFunc heapMapFn;
    HeapUnmapFunc heapUnmapFn;
    HeapDebugCheckAllocatorFunc heapDebugCheckAllocatorFn;
    GetAllocationSizeFunc getAllocationSizeFn;
    GetAllocationHeaderSizeFunc getAllocationHeaderSizeFn;
    GetAllocationFooterSizeFunc getAllocationFooterSizeFn;
    GetAllocationAlignedSizeFunc getAllocationAlignedSizeFn;
    GetHeapLimitsFunc getHeapLimitsFn;
} BaseHeap, *PBaseHeap;

/**
 * Including the internal headers
 */
#include "Common.h"
#include "SystemHeap.h"
#include "AivHeap.h"
#include "HybridHeap.h"
#include "HybridFileHeap.h"

#ifdef __cplusplus
}
#endif

#endif // __MEM_HEAP_INCLUDE_I_H__
