/**
 * Hybrid heap class
 */

#ifndef __HYBRID_HEAP_H__
#define __HYBRID_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * VRAM function definitions with their symbol names
 */
#define VRAM_INIT_FUNC_SYMBOL_NAME "native_region_init"
typedef UINT32 (*VramInit)(VOID);
#define VRAM_ALLOC_FUNC_SYMBOL_NAME "native_region_alloc"
typedef UINT32 (*VramAlloc)(UINT32);
#define VRAM_FREE_FUNC_SYMBOL_NAME "native_region_free"
typedef UINT32 (*VramFree)(UINT32);
#define VRAM_LOCK_FUNC_SYMBOL_NAME "native_region_lock"
typedef VOID* (*VramLock)(UINT32);
#define VRAM_UNLOCK_FUNC_SYMBOL_NAME "native_region_unlock_handle"
typedef UINT32 (*VramUnlock)(UINT32);
#define VRAM_UNINIT_FUNC_SYMBOL_NAME "native_region_uninit"
typedef UINT32 (*VramUninit)(VOID);
#define VRAM_GETMAX_FUNC_SYMBOL_NAME "native_region_get_max"
typedef UINT32 (*VramGetMax)(VOID);

/**
 * VRAM library name
 */
#define VRAM_LIBRARY_NAME      "libNativeVramAlloc.so"
#define VRAM_LIBRARY_FULL_PATH "/data/data/com.amazon.avod/lib/libNativeVramAlloc.so"

#define VRAM_ALLOCATION_TYPE 3

#define INVALID_VRAM_HANDLE 0

/**
 * VRAM handle conversion macros and defines
 */
#define ALIGNMENT_BITS                 (UINT64) 0x03
#define TO_VRAM_HANDLE(h)              ((UINT32)((UINT64)(h) >> 32))
#define FROM_VRAM_HANDLE(h)            (ALLOCATION_HANDLE)(((UINT64)(h) << 32) | ALIGNMENT_BITS)
#define IS_DIRECT_ALLOCATION_HANDLE(h) (((UINT64)(h) &ALIGNMENT_BITS) == (UINT64) 0x00)

/**
 * Hybrid heap struct
 */
typedef struct {
    /**
     * Base Heap struct encapsulation
     */
    BaseHeap heap;

    /**
     * Extracted v-ram functions
     */
    VramInit vramInit;
    VramAlloc vramAlloc;
    VramFree vramFree;
    VramLock vramLock;
    VramUnlock vramUnlock;
    VramUninit vramUninit;
    VramGetMax vramGetMax;

    /**
     * Whether the vRam has been initialized
     */
    BOOL vramInitialized;

    /**
     * Dynamic library handle
     */
    PVOID libHandle;

    /**
     * Spill ratio - this will indicate the ratio of the memory to use from main memory and VRAM
     */
    DOUBLE spillRatio;

    /**
     * The direct memory allocation based heap
     */
    PBaseHeap pMemHeap;
} HybridHeap, *PHybridHeap;

/**
 * Hybrid heap internal functions
 */
STATUS hybridCreateHeap(PHeap, UINT32, UINT32, PHybridHeap*);

/**
 * Allocate a buffer from the heap
 */
DEFINE_HEAP_ALLOC(hybridHeapAlloc);

/**
 * Free the previously allocated buffer handle
 */
DEFINE_HEAP_FREE(hybridHeapFree);

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(hybridHeapGetAllocSize);

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(hybridHeapSetAllocSize);

/**
 * Maps the allocation handle to memory. This is needed for in-direct allocation on vRAM
 */
DEFINE_HEAP_MAP(hybridHeapMap);

/**
 * Un-maps the previously mapped buffer
 */
DEFINE_HEAP_UNMAP(hybridHeapUnmap);

/**
 * Release the entire heap
 */
DEFINE_RELEASE_HEAP(hybridHeapRelease);

/**
 * Initialize the heap with a given limit
 */
DEFINE_INIT_HEAP(hybridHeapInit);

/**
 * Debug/check heap
 */
DEFINE_HEAP_CHK(hybridHeapDebugCheckAllocator);

/**
 * Dealing with the allocation sizes
 */
DEFINE_HEADER_SIZE(hybridGetAllocationHeaderSize);
DEFINE_FOOTER_SIZE(hybridGetAllocationFooterSize);
DEFINE_ALIGNED_SIZE(hybridGetAllocationAlignedSize);
DEFINE_ALLOC_SIZE(hybridGetAllocationSize);
DEFINE_HEAP_LIMITS(hybridGetHeapLimits);

#ifdef __cplusplus
}
#endif

#endif // __HYBRID_HEAP_H__
