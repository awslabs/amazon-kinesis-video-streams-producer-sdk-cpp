/**
 * Heap base abstract class
 */

#ifndef __COMMON_HEAP_H__
#define __COMMON_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 'Magic' - aka a sentinel which will be added as a guard band for each allocation in debug mode
 */
#define ALLOCATION_HEADER_MAGIC "__HEADER_MAGIC_GUARD_BAND__"
#define ALLOCATION_FOOTER_MAGIC "__FOOTER_MAGIC_GUARD_BAND__"

// Ensure it's more than the actual header and footer size
// IMPORTANT!!! This must be a multiple of 8 for alignment
#define ALLOCATION_HEADER_MAGIC_SIZE 32
#define ALLOCATION_FOOTER_MAGIC_SIZE 32

/**
 * The internal base structure representing the allocation header for
 * all of the heap implementations.
 *
 * IMPORTANT!!! Ensure this structure is tightly packed without tight packing directives
 */
typedef struct {
    // The aligned allocation size without the service structures - aka header and footer included
    UINT64 size;

    // The type of the heap
    UINT32 type;

    // Union used for flags or VRAM handler
    union {
        UINT32 vramHandle;
        UINT32 flags;
        UINT32 fileHandle;
    };

#ifdef HEAP_DEBUG
    BYTE magic[ALLOCATION_HEADER_MAGIC_SIZE];
#endif
} ALLOCATION_HEADER, *PALLOCATION_HEADER;

typedef struct {
    // The size of the allocation without the service structure - header and footer. This is the same
    // as the size in the header structure needed for a back reference to the beginning of the allocation.
    UINT64 size;
#ifdef HEAP_DEBUG
    BYTE magic[ALLOCATION_FOOTER_MAGIC_SIZE];
#endif
} ALLOCATION_FOOTER, *PALLOCATION_FOOTER;

/**
 * Allocate a buffer from the heap
 */
DEFINE_HEAP_ALLOC(commonHeapAlloc);

/**
 * Free the previously allocated buffer handle
 */
DEFINE_HEAP_FREE(commonHeapFree);

/**
 * Gets the heap size
 */
DEFINE_GET_HEAP_SIZE(commonHeapGetSize);

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(commonHeapGetAllocSize);

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(commonHeapSetAllocSize);

/**
 * Maps the allocation handle to memory. This is needed for in-direct allocation on vRAM
 */
DEFINE_HEAP_MAP(commonHeapMap);

/**
 * Un-maps the previously mapped buffer
 */
DEFINE_HEAP_UNMAP(commonHeapUnmap);

/**
 * Release the entire heap
 */
DEFINE_RELEASE_HEAP(commonHeapRelease);

/**
 * Initialize the heap with a given limit
 */
DEFINE_INIT_HEAP(commonHeapInit);

/**
 * Debug/check heap
 */
DEFINE_HEAP_CHK(commonHeapDebugCheckAllocator);

/**
 * Increment the allocations/count
 */
VOID incrementUsage(PHeap, UINT64);

/**
 * Decrenents the allocations/count
 */
VOID decrementUsage(PHeap, UINT64);

/**
 * Creates the heap object itself
 */
STATUS commonHeapCreate(PHeap*, UINT32);

/**
 * Validate heap
 */
STATUS validateHeap(PHeap);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_HEAP_H__
