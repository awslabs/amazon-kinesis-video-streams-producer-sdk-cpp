/**
 * Hybrid file heap class
 */

#ifndef __HYBRID_FILE_HEAP_H__
#define __HYBRID_FILE_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_ALLOCATION_TYPE 4

// Default location of the root directory for the files
#define FILE_HEAP_DEFAULT_ROOT_DIRECTORY "."

// Extension for the heap files
#define FILE_HEAP_FILE_EXTENSION ".hfh"

// Starting index for the file heap files
#define FILE_HEAP_STARTING_FILE_INDEX 1

// Define the max root directory length accounting for the max file size of 10, a separator and the file extension
#define FILE_HEAP_MAX_ROOT_DIR_LEN (MAX_PATH_LEN - 10 - SIZEOF(FILE_HEAP_FILE_EXTENSION) - 1)

/**
 * We will encode a sequential number as the handle in upper 32 bits
 */
#define TO_FILE_HANDLE(h)   ((UINT32)((UINT64)(h) >> 32))
#define FROM_FILE_HANDLE(h) (ALLOCATION_HANDLE)(((UINT64)(h) << 32) | ALIGNMENT_BITS)

/**
 * Hybrid heap struct
 */
typedef struct {
    /**
     * Base Heap struct encapsulation
     */
    BaseHeap heap;

    /**
     * Spill ratio - this will indicate the ratio of the memory to use from main memory and file
     */
    DOUBLE spillRatio;

    /**
     * Root directory to store the files
     */
    CHAR rootDirectory[MAX_PATH_LEN + 1];

    /**
     * Will keep the ever increasing number which will be used as a handle. OK to wrap
     */
    UINT32 handleNum;

    /**
     * The direct memory allocation based heap
     */
    PBaseHeap pMemHeap;
} HybridFileHeap, *PHybridFileHeap;

/**
 * Hybrid heap internal functions
 */
STATUS hybridFileCreateHeap(PHeap, UINT32, PCHAR, PHybridFileHeap*);

/**
 * Allocate a buffer from the heap
 */
DEFINE_HEAP_ALLOC(hybridFileHeapAlloc);

/**
 * Free the previously allocated buffer handle
 */
DEFINE_HEAP_FREE(hybridFileHeapFree);

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(hybridFileHeapGetAllocSize);

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(hybridFileHeapSetAllocSize);

/**
 * Maps the allocation handle to memory. This is needed for in-direct allocation in file system
 */
DEFINE_HEAP_MAP(hybridFileHeapMap);

/**
 * Un-maps the previously mapped buffer
 */
DEFINE_HEAP_UNMAP(hybridFileHeapUnmap);

/**
 * Release the entire heap
 */
DEFINE_RELEASE_HEAP(hybridFileHeapRelease);

/**
 * Initialize the heap with a given limit
 */
DEFINE_INIT_HEAP(hybridFileHeapInit);

/**
 * Debug/check heap
 */
DEFINE_HEAP_CHK(hybridFileHeapDebugCheckAllocator);

/**
 * Dealing with the allocation sizes
 */
DEFINE_HEADER_SIZE(hybridFileGetAllocationHeaderSize);
DEFINE_FOOTER_SIZE(hybridFileGetAllocationFooterSize);
DEFINE_ALIGNED_SIZE(hybridFileGetAllocationAlignedSize);
DEFINE_ALLOC_SIZE(hybridFileGetAllocationSize);
DEFINE_HEAP_LIMITS(hybridFileGetHeapLimits);

/**
 * Auxiliary functionality
 */
STATUS removeHeapFile(UINT64, DIR_ENTRY_TYPES, PCHAR, PCHAR);

#ifdef __cplusplus
}
#endif

#endif // __HYBRID_FILE_HEAP_H__
