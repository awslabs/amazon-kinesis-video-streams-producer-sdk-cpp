/**
 * AIV Heap definitions
 */

#ifndef __AIV_HEAP_H__
#define __AIV_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#define ALLOCATION_FLAGS_NONE 0x00
#define ALLOCATION_FLAGS_ALLOC 0x01
#define ALLOCATION_FLAGS_FREE 0x02

#define AIV_ALLOCATION_TYPE 2

/**
 * Allocation header
 */
typedef struct AIV_ALLOCATION_HEADER
{
    ALLOCATION_HEADER header;
    UINT32 allocSize;
    BYTE state;
    AIV_ALLOCATION_HEADER* pNext;
    AIV_ALLOCATION_HEADER* pPrev;
} *PAIV_ALLOCATION_HEADER;

/**
 * Allocation footer
 */
typedef struct
{
    ALLOCATION_FOOTER footer;
} AIV_ALLOCATION_FOOTER, *PAIV_ALLOCATION_FOOTER;

// Macros to convert to and from handle
#define TO_AIV_HANDLE(pAiv, p) (ALLOCATION_HANDLE)((UINT64)((UINT32)((PBYTE)(p) - (PBYTE)((pAiv)->pAllocation))) << 32)
#define FROM_AIV_HANDLE(pAiv, h) ((PVOID)((PBYTE)((pAiv)->pAllocation) + (UINT32)((UINT64)(h) >> 32)))

/**
 * Minimal free allocation size - if we end up with a free block of lesser size we will coalesce this to the allocated block
 */
#define MIN_FREE_ALLOCATION_SIZE 16

/**
 * AIV heap struct
 */
typedef struct
{
    /**
     * Base Heap struct encapsulation
     */
    BaseHeap heap;

    /**
     * The large allocation to be used as a heap
     */
    PVOID pAllocation;

    /**
     * Pointers to free and allocated lists
     */
    PAIV_ALLOCATION_HEADER pFree;
    PAIV_ALLOCATION_HEADER pAlloc;
} AivHeap, *PAivHeap;

/**
 * Creates the heap
 */
DEFINE_CREATE_HEAP(aivHeapCreate);

/**
 * Allocate a buffer from the heap
 */
DEFINE_HEAP_ALLOC(aivHeapAlloc);

/**
 * Free the previously allocated buffer handle
 */
DEFINE_HEAP_FREE(aivHeapFree);

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(aivHeapGetAllocSize);

/**
 * Maps the allocation handle to memory. This is needed for in-direct allocation on vRAM
 */
DEFINE_HEAP_MAP(aivHeapMap);

/**
 * Un-maps the previously mapped buffer
 */
DEFINE_HEAP_UNMAP(aivHeapUnmap);

/**
 * Release the entire heap
 */
DEFINE_RELEASE_HEAP(aivHeapRelease);

/**
 * Initialize the heap with a given limit
 */
DEFINE_INIT_HEAP(aivHeapInit);

/**
 * Debug/check heap
 */
DEFINE_HEAP_CHK(aivHeapDebugCheckAllocator);

/**
 * Dealing with the allocation sizes
 */
DEFINE_HEADER_SIZE(aivGetAllocationHeaderSize);
DEFINE_FOOTER_SIZE(aivGetAllocationFooterSize);
DEFINE_ALLOC_SIZE(aivGetAllocationSize);
DEFINE_HEAP_LIMITS(aivGetHeapLimits);

/**
 * AIV Heap specific functions
 */
PAIV_ALLOCATION_HEADER getFreeBlock(PAivHeap, UINT32);
VOID splitFreeBlock(PAivHeap, PAIV_ALLOCATION_HEADER, UINT32);
VOID addAllocatedBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID removeAllocatedBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID addFreeBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID insertFreeBlockBefore(PAivHeap, PAIV_ALLOCATION_HEADER, PAIV_ALLOCATION_HEADER);
VOID insertFreeBlockLast(PAIV_ALLOCATION_HEADER, PAIV_ALLOCATION_HEADER);
VOID coalesceFreeBlock(PAIV_ALLOCATION_HEADER);
BOOL checkOverlap(PAIV_ALLOCATION_HEADER, PAIV_ALLOCATION_HEADER);

#ifdef __cplusplus
}
#endif

#endif // __AIV_HEAP_H__
