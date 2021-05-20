/**
 * AIV Heap definitions
 */

#ifndef __AIV_HEAP_H__
#define __AIV_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ALLOCATION_FLAGS_NONE  0x0000
#define ALLOCATION_FLAGS_ALLOC 0x0001
#define ALLOCATION_FLAGS_FREE  0x0002

#define AIV_ALLOCATION_TYPE 2

/**
 * Minimum free block size in bytes
 */
#define MIN_FREE_ALLOCATION_SIZE 16

/**
 * Allocation header for low fragmentation heap implementation.
 *
 * IMPORTANT!!! Make sure the structure is tightly packed without the tight packing directives
 */
typedef struct AIV_ALLOCATION_HEADER {
    // Base structure
    ALLOCATION_HEADER header;

    // The allocation size specified by the caller without alignment or padding
    UINT64 allocSize;

    // Chaining left and right
    struct AIV_ALLOCATION_HEADER* pNext;
    struct AIV_ALLOCATION_HEADER* pPrev;
} * PAIV_ALLOCATION_HEADER;

// Macros to convert to and from handle
#define AIV_HANDLE_SHIFT_BITS    2
#define TO_AIV_HANDLE(pAiv, p)   (ALLOCATION_HANDLE)((UINT64)(((PBYTE)(p) - (PBYTE)((pAiv)->pAllocation))) << AIV_HANDLE_SHIFT_BITS)
#define FROM_AIV_HANDLE(pAiv, h) ((PVOID)((PBYTE)((pAiv)->pAllocation) + ((UINT64)(h) >> AIV_HANDLE_SHIFT_BITS)))

/**
 * AIV heap struct
 */
typedef struct {
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
 * Validates the allocation by checking the range for now
 */
#define CHK_AIV_ALLOCATION(pAiv, pAlloc)                                                                                                             \
    do {                                                                                                                                             \
        CHK_ERR((PBYTE)(pAlloc) != NULL && (PBYTE)(pAlloc) >= ((PBYTE)((PAivHeap)(pAiv))->pAllocation) &&                                            \
                    (PBYTE)(pAlloc) < ((PBYTE)((PAivHeap)(pAiv))->pAllocation) + ((PHeap)(pAiv))->heapLimit,                                         \
                STATUS_INVALID_HANDLE_ERROR, "Invalid handle value.");                                                                               \
    } while (FALSE)

#ifdef ALIGNED_MEMORY_MODEL
#define HEAP_PACKED_SIZE(size) ROUND_UP((size), 8)
#else
#define HEAP_PACKED_SIZE(size) (size)
#endif

/**
 * Gets the allocation header
 */
#define GET_AIV_ALLOCATION_HEADER(p) ((PAIV_ALLOCATION_HEADER)(p) -1)

/**
 * Retrieve the allocation size
 */
#define GET_AIV_ALLOCATION_SIZE(pAlloc) (((PALLOCATION_HEADER)(pAlloc))->size)

/**
 * Sets the allocation size
 */
#define SET_AIV_ALLOCATION_SIZE(pAlloc, s) (((PALLOCATION_HEADER)(pAlloc))->size = (UINT64)(s))

/**
 * Gets the pointer to the allocation itself
 */
#define GET_AIV_ALLOCATION(pAlloc) ((PBYTE)((PAIV_ALLOCATION_HEADER)(pAlloc) + 1))

/**
 * Gets the allocation footer
 */
#define GET_AIV_ALLOCATION_FOOTER(pAlloc) ((PALLOCATION_FOOTER)(GET_AIV_ALLOCATION(pAlloc) + GET_AIV_ALLOCATION_SIZE(pAlloc)))

/**
 * Gets the allocation footer size
 */
#define GET_AIV_ALLOCATION_FOOTER_SIZE(pAlloc) (GET_AIV_ALLOCATION_FOOTER(pAlloc)->size)

/**
 * Sets the allocation footer size
 */
#define SET_AIV_ALLOCATION_FOOTER_SIZE(pAlloc) (GET_AIV_ALLOCATION_FOOTER(pAlloc)->size = GET_AIV_ALLOCATION_SIZE(pAlloc))

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
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(aivHeapSetAllocSize);

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
DEFINE_ALIGNED_SIZE(aivGetAllocationAlignedSize);
DEFINE_ALLOC_SIZE(aivGetAllocationSize);
DEFINE_HEAP_LIMITS(aivGetHeapLimits);

/**
 * AIV Heap specific functions
 */
PAIV_ALLOCATION_HEADER getFreeBlock(PAivHeap, UINT64);
VOID splitFreeBlock(PAivHeap, PAIV_ALLOCATION_HEADER, UINT64);
VOID splitAllocatedBlock(PAivHeap, PAIV_ALLOCATION_HEADER, UINT64);
VOID addAllocatedBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID removeChainedBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID addFreeBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
VOID coalesceFreeToAllocatedBlock(PAivHeap, PAIV_ALLOCATION_HEADER, PAIV_ALLOCATION_HEADER, UINT64);
BOOL checkOverlap(PAIV_ALLOCATION_HEADER, PAIV_ALLOCATION_HEADER);
PAIV_ALLOCATION_HEADER getLeftBlock(PAivHeap, PAIV_ALLOCATION_HEADER);
PAIV_ALLOCATION_HEADER getRightBlock(PAivHeap, PAIV_ALLOCATION_HEADER);

#ifdef __cplusplus
}
#endif

#endif // __AIV_HEAP_H__
