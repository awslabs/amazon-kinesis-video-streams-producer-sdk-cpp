/**
 * System heap definitions
 */

#ifndef __SYSTEM_HEAP_H__
#define __SYSTEM_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_ALLOCATION_TYPE 1

/**
 * Creates the heap
 */
DEFINE_CREATE_HEAP(sysHeapCreate);

/**
 * Allocate a buffer from the heap
 */
DEFINE_HEAP_ALLOC(sysHeapAlloc);

/**
 * Free the previously allocated buffer handle
 */
DEFINE_HEAP_FREE(sysHeapFree);

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(sysHeapGetAllocSize);

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(sysHeapSetAllocSize);

/**
 * Maps the allocation handle to memory. This is needed for in-direct allocation on vRAM
 */
DEFINE_HEAP_MAP(sysHeapMap);

/**
 * Un-maps the previously mapped buffer
 */
DEFINE_HEAP_UNMAP(sysHeapUnmap);

/**
 * Release the entire heap
 */
DEFINE_RELEASE_HEAP(sysHeapRelease);

/**
 * Initialize the heap with a given limit
 */
DEFINE_INIT_HEAP(sysHeapInit);

/**
 * Debug/check heap
 */
DEFINE_HEAP_CHK(sysHeapDebugCheckAllocator);

/**
 * Dealing with the allocation sizes
 */
DEFINE_HEADER_SIZE(sysGetAllocationHeaderSize);
DEFINE_FOOTER_SIZE(sysGetAllocationFooterSize);
DEFINE_ALIGNED_SIZE(sysGetAllocationAlignedSize);
DEFINE_ALLOC_SIZE(sysGetAllocationSize);
DEFINE_HEAP_LIMITS(sysGetHeapLimits);

#ifdef __cplusplus
}
#endif

#endif // __SYSTEM_HEAP_H__
